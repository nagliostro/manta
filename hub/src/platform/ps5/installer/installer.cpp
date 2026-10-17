// Emuland - Installs and updates emulators from their GitHub releases.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// One worker thread (1 MiB of stack: console threads get a small one by
// default) takes jobs in order. An install:
//
//   1. the newest release (GitHub API, the catalog's file name prefix; only
//      emulators that publish pre-releases alone take them);
//   2. the ZIP into <state>/downloads, hashed while it arrives, compared with
//      the SHA-256 GitHub publishes for the file;
//   3. the app folder out of it (the template's archive.cpp: the one folder
//      with sce_sys/param.json and eboot.bin, nothing outside it, no links),
//      unpacked into <state>/staging, away from the folder apps are scanned in;
//   4. into place with the template's swap: only the entries the release
//      replaces move, the previous ones go to <state>/backup/<TID>;
//   5. mode 0777 on every file (the console refuses to start anything else),
//      the console's copies of sce_sys refreshed, and a record of the tag.
//
// Network: libcurl's multi interface, waiting only when a pass moved nothing
// (docs/CURL.md of the template: the console's poll does not always wake).
// Swap, permissions, registered copies and the mount record are adapted from
// the template's examples/self-update-helper/updater.cpp (GPL-3.0-or-later).

#include "platform/ps5/installer/installer.hpp"

#include "platform/ps5/installer/archive.hpp"
#include "platform/ps5/installer/files.hpp"
#include "emuland/json.hpp"
#include "emuland/online_catalog.hpp"
#include "platform/ps5/installer/self_update_sha256.h"
#include "platform/ps5/system.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <dirent.h>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace emuland
{

namespace
{

using installer::Paths;
using self_update::Kind;

constexpr int kCheck = -2;     // asked for: always goes to GitHub
constexpr int kAutoCheck = -3; // at start: only when the last one is old
constexpr int kPrepareUsb = -4; // the USB drive's folders (writing to it can take a while)
// GitHub answers 60 unauthenticated API calls an hour per IP, one per
// emulator in a check (a 403 on 2026-10-16 after a few restarts): the check
// at start skips GitHub when the last full one is this recent.
constexpr std::int64_t kCheckEverySeconds = 6 * 60 * 60;
// An uninstall job: kUninstall + index * 2 + (1 when the data goes too).
constexpr int kUninstall = 1000;
// An import from a USB drive: kImport + index * 32 + folder (kAllFolders: each).
constexpr int kImport = 3000;
constexpr int kAllFolders = 31;
constexpr std::size_t kApiLimit = 2u << 20;
constexpr long kConnectMs = 20000;

Paths g_paths;
std::mutex g_mutex;
std::condition_variable g_wake;
std::deque<int> g_jobs;
JobStatus g_status;
std::vector<Release> g_releases; // newest seen, per catalog entry
bool g_releases_loaded = false;
std::int64_t g_checked_at = 0;    // UTC seconds of the last full check
std::int64_t g_limited_until = 0; // GitHub's rate limit: no API calls before this
bool g_thread_started = false;
std::atomic<bool> g_cancel{false};
std::atomic<std::uint64_t> g_progress{0};
std::atomic<int> g_prepare_lang{0}; // the LEIA-ME's language: 0 pt, 1 en

// The log is fully buffered (main.cpp): flush each line, so an install that
// dies half-way still leaves its last step in hub.log.
template <typename... Args> void note(const char *format, Args... args)
{
    hui::sys::log(format, args...);
    std::fflush(stdout);
}

// True while an install waits behind the check: the check makes way for it.
bool install_waiting()
{
    std::lock_guard lock(g_mutex);
    for (const int job : g_jobs)
    {
        if (job >= 0)
            return true;
    }
    return false;
}

// ---------------------------------------------------------------- files

std::string releases_file()
{
    return g_paths.state + "/releases.txt";
}

// checked_at|limited_until, UTC seconds.
std::string check_file()
{
    return g_paths.state + "/check.txt";
}

std::string self_update_request()
{
    return g_paths.state + "/self-update-request.txt";
}

std::string record_file(const char *title_id)
{
    return g_paths.state + "/installed/" + title_id + ".txt";
}

bool write_text(const std::string &path, const std::string &text)
{
    const std::string temporary = path + ".tmp";
    std::FILE *file = std::fopen(temporary.c_str(), "wb");
    if (file == nullptr)
        return false;
    const bool ok = std::fwrite(text.data(), 1, text.size(), file) == text.size();
    if (std::fclose(file) != 0 || !ok)
        return false;
    return std::rename(temporary.c_str(), path.c_str()) == 0;
}

std::string read_text(const std::string &path)
{
    std::string body;
    (void)self_update::read_small(path, 1u << 16, body);
    return body;
}

std::vector<std::string> split(const std::string &text, char separator)
{
    std::vector<std::string> parts;
    std::string current;
    for (const char c : text)
    {
        if (c == separator)
        {
            parts.push_back(current);
            current.clear();
        }
        else if (c != '\r')
        {
            current += c;
        }
    }
    if (!current.empty())
        parts.push_back(current);
    return parts;
}

// One line per catalog entry: title|tag|asset|url|digest|size|prerelease.
void load_releases_locked()
{
    if (g_releases_loaded)
        return;
    g_releases_loaded = true;
    g_releases.assign(catalog().size(), Release{});
    for (const std::string &line : split(read_text(releases_file()), '\n'))
    {
        const std::vector<std::string> f = split(line, '|');
        if (f.size() < 7)
            continue;
        for (std::size_t i = 0; i < catalog().size(); ++i)
        {
            if (f[0] != catalog()[i].title_id)
                continue;
            Release &r = g_releases[i];
            r.tag = f[1];
            r.asset = f[2];
            r.url = f[3];
            r.digest = f[4];
            r.size = std::strtoull(f[5].c_str(), nullptr, 10);
            r.prerelease = f[6] == "1";
        }
    }
    const std::vector<std::string> check = split(read_text(check_file()), '|');
    g_checked_at = check.size() > 0 ? std::strtoll(check[0].c_str(), nullptr, 10) : 0;
    g_limited_until = check.size() > 1 ? std::strtoll(check[1].c_str(), nullptr, 10) : 0;
}

void save_check_locked()
{
    mkdir(g_paths.state.c_str(), 0777);
    if (!write_text(check_file(), std::to_string(g_checked_at) + "|" +
                                      std::to_string(g_limited_until) + "\n"))
        note("[EMULAND] could not save %s", check_file().c_str());
}

std::int64_t now_utc()
{
    return static_cast<std::int64_t>(std::time(nullptr));
}

// The console's local clock at a UTC moment, from its clock now (the zone
// comes from the system, see local_time). Rounds up: "after HH:MM".
void clock_at(std::int64_t at, int *hour, int *minute)
{
    int now_hour = -1;
    int now_minute = 0;
    local_time(&now_hour, &now_minute);
    if (now_hour < 0)
        return;
    const std::int64_t delta = at - now_utc();
    const std::int64_t minutes = delta >= 0 ? (delta + 59) / 60 : -((-delta) / 60);
    const std::int64_t total = ((now_hour * 60 + now_minute + minutes) % 1440 + 1440) % 1440;
    *hour = static_cast<int>(total / 60);
    *minute = static_cast<int>(total % 60);
}

void save_releases_locked()
{
    std::string text;
    for (std::size_t i = 0; i < g_releases.size(); ++i)
    {
        const Release &r = g_releases[i];
        if (r.tag.empty())
            continue;
        text += std::string(catalog()[i].title_id) + "|" + r.tag + "|" + r.asset + "|" + r.url +
                "|" + r.digest + "|" + std::to_string(r.size) + "|" + (r.prerelease ? "1" : "0") +
                "\n";
    }
    mkdir(g_paths.state.c_str(), 0777);
    if (!write_text(releases_file(), text))
        note("[EMULAND] could not save %s", releases_file().c_str());
}

bool list_entries(const std::string &folder, std::vector<std::string> &names)
{
    DIR *directory = opendir(folder.c_str());
    if (directory == nullptr)
        return false;
    while (const dirent *entry = readdir(directory))
    {
        const std::string name = entry->d_name;
        if (name != "." && name != "..")
            names.push_back(name);
    }
    closedir(directory);
    return true;
}

// A running title has a folder "<TITLEID>_<n>" in the sandboxes folder.
bool title_running(const char *title_id)
{
    std::vector<std::string> names;
    if (!list_entries(g_paths.sandboxes, names))
        return false;
    const std::string prefix = std::string(title_id) + "_";
    for (const std::string &name : names)
    {
        if (name.compare(0, prefix.size(), prefix) == 0)
            return true;
    }
    return false;
}

bool same_filesystem(const std::string &first, const std::string &second)
{
    struct stat a{};
    struct stat b{};
    return stat(first.c_str(), &a) == 0 && stat(second.c_str(), &b) == 0 && a.st_dev == b.st_dev;
}

std::string parent_of(const std::string &path)
{
    const std::size_t slash = path.rfind('/');
    return slash == std::string::npos || slash == 0 ? std::string("/") : path.substr(0, slash);
}

// ShadowMountPlus records where it mounted an app from in
// <registered>/app/<TITLEID>/mount.lnk (a folder) or mount_img.lnk (an image).
// 1: path is that folder; 0: no record; -1: an image, whose files can't be
// replaced one by one.
int mounted_source(const char *title_id, std::string &path)
{
    const std::string app = g_paths.registered + "/app/" + title_id;
    std::string body;
    if (!self_update::read_small(app + "/mount.lnk", 1024, body))
        return self_update::kind(app + "/mount_img.lnk") == Kind::file ? -1 : 0;
    while (!body.empty() && (body.back() == '\n' || body.back() == '\r' || body.back() == ' ' ||
                             body.back() == '\0' || body.back() == '/'))
        body.pop_back();
    if (body.size() < 2 || body[0] != '/' || body.find("/..") != std::string::npos ||
        body.find("/./") != std::string::npos)
        return 0;
    path = body;
    return 1;
}

// Is path this title's app folder (its param.json names the title)? A
// mount.lnk is someone else's record: before Emuland replaces or deletes what
// it points to, the folder must prove to be the app, or an uninstall could
// take a whole drive or /data with it.
bool holds_title(const std::string &path, const char *title_id)
{
    std::string json;
    if (!self_update::read_small(path + "/sce_sys/param.json", 64u << 10, json))
        return false;
    const std::size_t key = json.find("\"titleId\"");
    return key != std::string::npos &&
           json.find(std::string("\"") + title_id + "\"", key) != std::string::npos;
}

// The console starts an app only when everyone can read and run its files
// (mode 0777, as an FTP copy arrives). The extraction already writes them so:
// this only mends what is missing. No chown: on 2026-10-16 the app died in
// this walk (once taking the console down), and chown was its only new call.
bool open_permissions(const std::string &path, unsigned depth = 0)
{
    struct stat info{};
    if (depth > 32 || lstat(path.c_str(), &info) != 0)
        return false;
    const Kind what = S_ISDIR(info.st_mode) ? Kind::directory
                      : S_ISREG(info.st_mode) ? Kind::file
                                              : Kind::other;
    if (what == Kind::other)
        return false;
    if ((info.st_mode & 0777) != 0777 && chmod(path.c_str(), 0777) != 0)
        return false;
    if (what == Kind::file)
        return true;
    std::vector<std::string> names;
    if (!list_entries(path, names))
        return false;
    for (const std::string &name : names)
    {
        if (!open_permissions(path + "/" + name, depth + 1))
            return false;
    }
    return true;
}

// ShadowMountPlus copies an app's sce_sys once, when it first registers the
// title: after an update those copies are brought up to date. Best effort.
void refresh_registered(const char *title_id, const std::string &target)
{
    const std::string source = target + "/sce_sys";
    std::vector<std::string> names;
    if (!self_update::list_files(source, 64, names))
        return;
    const std::string app = g_paths.registered + "/app/" + title_id;
    for (const std::string &folder :
         {g_paths.registered + "/appmeta/" + title_id, app + "/sce_sys"})
    {
        if (self_update::kind(folder) == Kind::directory)
        {
            for (const std::string &name : names)
                self_update::copy_file(source + "/" + name, folder + "/" + name);
        }
    }
    if (self_update::kind(app + "/icon0.png") == Kind::file)
        self_update::copy_file(source + "/icon0.png", app + "/icon0.png");
}

// Puts every top-level entry of staged in target's place, moving target's own
// entries of the same names to backup first. On failure target is as it was.
bool swap_entries(const std::string &target, const std::string &staged, const std::string &backup)
{
    std::vector<std::string> present;
    std::vector<std::string> fresh;
    if (!list_entries(target, present) || !list_entries(staged, fresh) || fresh.empty() ||
        mkdir(backup.c_str(), 0777) != 0)
        return false;
    std::vector<std::string> old;
    for (const std::string &name : present)
    {
        if (std::find(fresh.begin(), fresh.end(), name) != fresh.end())
            old.push_back(name);
    }
    std::size_t moved = 0;
    std::size_t placed = 0;
    bool ok = true;
    for (; ok && moved < old.size(); ++moved)
        ok = std::rename((target + "/" + old[moved]).c_str(), (backup + "/" + old[moved]).c_str()) ==
             0;
    if (!ok)
        --moved;
    for (; ok && placed < fresh.size(); ++placed)
        ok = std::rename((staged + "/" + fresh[placed]).c_str(),
                         (target + "/" + fresh[placed]).c_str()) == 0;
    if (!ok && placed > 0)
        --placed;
    if (ok)
        return true;
    for (std::size_t i = 0; i < placed; ++i)
        (void)std::rename((target + "/" + fresh[i]).c_str(), (staged + "/" + fresh[i]).c_str());
    for (std::size_t i = 0; i < moved; ++i)
        (void)std::rename((backup + "/" + old[i]).c_str(), (target + "/" + old[i]).c_str());
    (void)rmdir(backup.c_str());
    return false;
}

// The user's files in a folder the release also ships (RetroArch's system/,
// where BIOS files go) move into the staged copy before the swap, so the
// update replaces the release's own files and nothing of theirs. Same drive:
// every move is a rename.
void merge_user_files(const std::string &from, const std::string &into, unsigned depth = 0)
{
    std::vector<std::string> names;
    if (depth > 16 || !list_entries(from, names))
        return;
    for (const std::string &name : names)
    {
        const std::string source = from + "/" + name;
        const std::string destination = into + "/" + name;
        const Kind here = self_update::kind(source);
        const Kind there = self_update::kind(destination);
        if (here == Kind::directory && there == Kind::directory)
            merge_user_files(source, destination, depth + 1);
        else if ((here == Kind::directory || here == Kind::file) && there == Kind::absent)
        {
            mkdir(into.c_str(), 0777);
            if (std::rename(source.c_str(), destination.c_str()) != 0)
                note("[EMULAND] keep %s failed", source.c_str());
        }
    }
}

// ---------------------------------------------------------------- network

// What a GitHub API answer said besides its body.
struct Reply
{
    long status = 0;
    long long remaining = -1;  // x-ratelimit-remaining, -1 when absent
    long long reset = 0;       // x-ratelimit-reset, UTC seconds
    long long retry_after = -1; // retry-after, seconds
};

std::size_t on_header(char *data, std::size_t size, std::size_t count, void *user)
{
    Reply &reply = *static_cast<Reply *>(user);
    const std::size_t bytes = size * count;
    const std::string line(data, bytes);
    const std::size_t colon = line.find(':');
    if (colon == std::string::npos)
        return bytes;
    std::string name = line.substr(0, colon);
    for (char &c : name)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    const long long value = std::strtoll(line.c_str() + colon + 1, nullptr, 10);
    if (name == "x-ratelimit-remaining")
        reply.remaining = value;
    else if (name == "x-ratelimit-reset")
        reply.reset = value;
    else if (name == "retry-after")
        reply.retry_after = value;
    return bytes;
}

struct Sink
{
    int descriptor = -1;        // the download, written with write() (see write_all)
    std::uint64_t written = 0; // bytes on disk so far
    int write_error = 0;       // errno of a failed write
    std::string *body = nullptr;
    std::size_t limit = 0;
    self_update_sha256 *hash = nullptr;
    bool overflow = false;
};

std::size_t on_body(char *data, std::size_t size, std::size_t count, void *user)
{
    Sink &sink = *static_cast<Sink *>(user);
    const std::size_t bytes = size * count;
    if (sink.body != nullptr)
    {
        if (sink.body->size() + bytes > sink.limit)
        {
            sink.overflow = true;
            return 0;
        }
        sink.body->append(data, bytes);
    }
    if (sink.descriptor >= 0)
    {
        // The console's stdio gave up on a download near its end (curl 42 on
        // 2026-10-14): write() again until every byte is on disk.
        std::size_t done = 0;
        while (done < bytes)
        {
            const ssize_t count = write(sink.descriptor, data + done, bytes - done);
            if (count > 0)
            {
                done += static_cast<std::size_t>(count);
                continue;
            }
            if (count < 0 && (errno == EINTR || errno == EAGAIN))
                continue;
            sink.write_error = count < 0 ? errno : ENOSPC;
            return 0;
        }
        sink.written += bytes;
    }
    if (sink.hash != nullptr)
        self_update_sha256_add(sink.hash, data, bytes);
    g_progress.fetch_add(bytes);
    return bytes;
}

// 0 when the answer arrived whole with HTTP 200; otherwise sets detail.
// With post: a JSON request to a local service over plain HTTP (the answer's
// body arrives whatever the status, for its error fields). With reply: the
// status and rate-limit headers, whatever the status.
int fetch(const std::string &url, Sink &sink, bool api, std::string &detail,
          const std::string *post = nullptr, Reply *reply = nullptr)
{
    CURL *easy = curl_easy_init();
    if (easy == nullptr)
    {
        detail = "curl_easy_init failed";
        return -1;
    }
    emuland_net_setup(easy);
    char error[CURL_ERROR_SIZE] = {};
    curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, kUserAgent);
    if (post != nullptr)
        headers = curl_slist_append(headers, "Content-Type: application/json");
    if (api)
    {
        headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
        headers = curl_slist_append(headers, "X-GitHub-Api-Version: 2022-11-28");
    }
    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 8L);
    curl_easy_setopt(easy, CURLOPT_PROTOCOLS_STR, post != nullptr ? "http" : "https");
    curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS_STR, "https");
    if (post != nullptr)
    {
        curl_easy_setopt(easy, CURLOPT_POSTFIELDS, post->c_str());
        curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, 20000L);
    }
    curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(easy, CURLOPT_BUFFERSIZE, 256L * 1024L);
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, kConnectMs);
    curl_easy_setopt(easy, CURLOPT_LOW_SPEED_LIMIT, 512L); // bytes/s ...
    curl_easy_setopt(easy, CURLOPT_LOW_SPEED_TIME, 60L);   // ... for a minute: give up
    curl_easy_setopt(easy, CURLOPT_FAILONERROR, post != nullptr || reply != nullptr ? 0L : 1L);
    if (reply != nullptr)
    {
        curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, on_header);
        curl_easy_setopt(easy, CURLOPT_HEADERDATA, reply);
    }
    curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, error);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, on_body);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &sink);

    CURLM *multi = curl_multi_init();
    curl_multi_add_handle(multi, easy);
    CURLcode result = CURLE_OK;
    bool done = false;
    while (!done)
    {
        if (g_cancel.load())
        {
            result = CURLE_ABORTED_BY_CALLBACK;
            break;
        }
        const std::uint64_t before = g_progress.load();
        int running = 0;
        curl_multi_perform(multi, &running);
        int queued = 0;
        while (CURLMsg *message = curl_multi_info_read(multi, &queued))
        {
            if (message->msg == CURLMSG_DONE)
            {
                done = true;
                result = message->data.result;
            }
        }
        if (!done && g_progress.load() == before)
            curl_multi_poll(multi, nullptr, 0, 10, nullptr);
    }
    long status = 0;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
    if (reply != nullptr)
        reply->status = status;
    curl_multi_remove_handle(multi, easy);
    curl_multi_cleanup(multi);
    curl_easy_cleanup(easy);
    curl_slist_free_all(headers);
    if (result == CURLE_OK && status == 200 && !sink.overflow)
        return 0;
    char text[512];
    std::snprintf(text, sizeof(text), "curl %d (%s) HTTP %ld %s", static_cast<int>(result),
                  curl_easy_strerror(result), status, error);
    detail = text;
    return result == CURLE_ABORTED_BY_CALLBACK ? 1 : -1;
}

bool starts_with(const std::string &text, const char *prefix)
{
    return text.compare(0, std::strlen(prefix), prefix) == 0;
}

bool ends_with(const std::string &text, const char *suffix)
{
    const std::size_t size = std::strlen(suffix);
    return text.size() >= size && text.compare(text.size() - size, size, suffix) == 0;
}

// The newest release of one emulator that carries its ZIP.
JobError find_release(std::size_t index, Release &out, std::string &detail)
{
    const EmulatorSpec &spec = catalog()[index];
    if (source_only(spec))
    {
        detail = std::string(spec.repo) + " publishes source only";
        return JobError::no_release;
    }
    {
        std::lock_guard lock(g_mutex);
        if (now_utc() < g_limited_until)
        {
            detail = "GitHub rate limit, until " + std::to_string(g_limited_until);
            return JobError::rate_limited;
        }
    }
    const std::string url =
        std::string("https://api.github.com/repos/") + spec.repo + "/releases?per_page=10";
    std::string body;
    Sink sink;
    sink.body = &body;
    sink.limit = kApiLimit;
    Reply reply;
    const int fetched = fetch(url, sink, true, detail, nullptr, &reply);
    if (fetched != 0)
    {
        // 403 with no calls left, or 429 / retry-after (GitHub's secondary
        // limit): wait for the reset instead of spending more.
        const bool limited = reply.status == 429 ||
                             (reply.status == 403 && (reply.remaining == 0 || reply.retry_after >= 0));
        if (!limited)
            return JobError::network;
        const std::int64_t now = now_utc();
        std::lock_guard lock(g_mutex);
        g_limited_until = reply.retry_after >= 0 ? now + reply.retry_after
                          : reply.reset > now    ? static_cast<std::int64_t>(reply.reset)
                                                 : now + 3600;
        save_check_locked();
        detail += " (rate limit, " + std::to_string(g_limited_until - now) + " s)";
        return JobError::rate_limited;
    }
    json::Value releases;
    if (!json::parse(body, releases) || releases.type != json::Value::Type::array)
    {
        detail = "GitHub answer unreadable";
        return JobError::network;
    }
    for (const json::Value &release : releases.items)
    {
        if (release["draft"].truthy())
            continue;
        const bool pre = release["prerelease"].truthy();
        if (pre && !spec.prerelease)
            continue;
        for (const json::Value &asset : release["assets"].items)
        {
            const std::string &name = asset["name"].str();
            // The name becomes a file name in downloads/: no folders in it.
            if (!starts_with(name, spec.asset_prefix) || !ends_with(name, ".zip") ||
                name.find_first_of("/\\") != std::string::npos || name.find("..") != std::string::npos)
                continue;
            out.tag = release["tag_name"].str();
            out.asset = name;
            out.url = asset["browser_download_url"].str();
            out.digest = asset["digest"].str();
            out.size = asset["size"].u64();
            out.prerelease = pre;
            return out.url.empty() ? JobError::no_release : JobError::none;
        }
    }
    detail = "no release with " + std::string(spec.asset_prefix) + "*.zip";
    return JobError::no_release;
}

// ---------------------------------------------------------------- jobs

void set_stage(Stage stage, std::uint64_t total)
{
    std::lock_guard lock(g_mutex);
    g_status.stage = stage;
    g_status.total = total;
    g_progress.store(0);
}

// Is the emulator on the console (installed by Emuland or by hand)?
bool on_console(const char *title_id)
{
    struct stat info{};
    return stat(record_file(title_id).c_str(), &info) == 0 ||
           stat((g_paths.homebrew + "/" + title_id).c_str(), &info) == 0;
}

// catalog/online.json from Manta's repository (raw.githubusercontent.com,
// which GitHub's API limit does not count): kept when valid, applied at the
// next start. Best effort: a failure leaves the last copy in place.
void fetch_online_catalog()
{
    if (g_paths.online_catalog.empty())
        return;
    std::string body;
    Sink sink;
    sink.body = &body;
    sink.limit = 256u << 10;
    std::string detail;
    if (fetch(g_paths.online_catalog, sink, false, detail) != 0)
    {
        note("[EMULAND] online catalog: %s", detail.c_str());
        return;
    }
    OnlineCatalog parsed;
    std::string error;
    if (!parse_online_catalog(body, parsed, error))
    {
        note("[EMULAND] online catalog refused: %s", error.c_str());
        return;
    }
    const std::string file = g_paths.state + "/catalog.json";
    if (read_text(file) != body)
    {
        mkdir(g_paths.state.c_str(), 0777);
        if (!write_text(file, body))
            note("[EMULAND] could not save %s", file.c_str());
    }
    const int fresh = new_online_entries(parsed);
    note("[EMULAND] online catalog: %zu emulators, %zu changes, %d new", parsed.emulators.size(),
         parsed.changes.size(), fresh);
    std::lock_guard lock(g_mutex);
    g_status.catalog_new = fresh;
}

// forced: asked for on the Updates screen; otherwise the check at start, which
// keeps the last answer while it is recent or GitHub's limit holds.
JobError run_check(bool forced, std::string &detail)
{
    const std::int64_t now = now_utc();
    {
        std::lock_guard lock(g_mutex);
        if (!forced && now < g_limited_until)
        {
            note("[EMULAND] check skipped: GitHub rate limit for %lld s more",
                 static_cast<long long>(g_limited_until - now));
            return JobError::none;
        }
        if (!forced && g_checked_at > 0 && now >= g_checked_at &&
            now - g_checked_at < kCheckEverySeconds)
        {
            note("[EMULAND] check skipped: last one %lld min ago",
                 static_cast<long long>((now - g_checked_at) / 60));
            return JobError::none;
        }
    }
    fetch_online_catalog();
    // Installed ones first: if GitHub's limit cuts the check short, their
    // updates are what was worth knowing (the rest look again on install).
    std::vector<std::size_t> order;
    for (int pass = 0; pass < 2; ++pass)
    {
        for (std::size_t i = 0; i < catalog().size(); ++i)
        {
            if (on_console(catalog()[i].title_id) == (pass == 0))
                order.push_back(i);
        }
    }
    JobError worst = JobError::none;
    for (const std::size_t i : order)
    {
        if (g_cancel.load())
            break;
        if (install_waiting())
        {
            note("[EMULAND] check paused for an install");
            std::lock_guard lock(g_mutex);
            g_jobs.push_back(forced ? kCheck : kAutoCheck);
            return worst;
        }
        if (source_only(catalog()[i]))
            continue; // nothing published to compare with
        Release release;
        std::string reason;
        const JobError error = find_release(i, release, reason);
        note("[EMULAND] check %s: %s %s", catalog()[i].name,
                      error == JobError::none ? release.tag.c_str() : "failed", reason.c_str());
        if (error == JobError::none)
        {
            std::lock_guard lock(g_mutex);
            g_releases[i] = release;
        }
        else if (catalog()[i].hidden && error != JobError::rate_limited)
        {
            // Manta's repository without a release yet: not a failed check.
        }
        else if (worst == JobError::none || error == JobError::rate_limited)
        {
            worst = error;
            detail = reason;
        }
        if (error == JobError::rate_limited)
            break; // the rest would only be refused too
    }
    std::lock_guard lock(g_mutex);
    save_releases_locked();
    // A repo without the expected file still counts as looked at.
    if (!g_cancel.load() && worst != JobError::network && worst != JobError::rate_limited)
    {
        g_checked_at = now;
        save_check_locked();
    }
    return worst;
}

// Manta's own update, unpacked and ready: the helper payload puts it in
// place once Manta has closed (an app can't replace its running files), as
// the template's self-update does. The request names what goes where.
JobError hand_over_self_update(const std::string &staged, const std::string &target,
                               const Release &release, std::string &detail)
{
    if (!write_text(self_update_request(), staged + "\n" + target + "\n" + release.tag + "\n"))
    {
        detail = "cannot write " + self_update_request();
        self_update::remove_tree(staged);
        return JobError::install;
    }
    if (!installer::send_helper())
    {
        std::remove(self_update_request().c_str());
        self_update::remove_tree(staged);
        detail = "the ELF loader on 127.0.0.1:9021 did not take the helper";
        return JobError::no_loader;
    }
    note("[EMULAND] Manta %s staged in %s, applies when Manta closes", release.tag.c_str(),
         staged.c_str());
    std::lock_guard lock(g_mutex);
    g_status.self_staged = true;
    g_status.needs_close = true;
    return JobError::none;
}

JobError run_install(std::size_t index, std::string &detail)
{
    const EmulatorSpec &spec = catalog()[index];
    Release release;
    {
        std::lock_guard lock(g_mutex);
        release = g_releases[index];
    }
    set_stage(Stage::checking, 0);
    if (release.url.empty())
    {
        const JobError error = find_release(index, release, detail);
        if (error != JobError::none)
            return error;
        std::lock_guard lock(g_mutex);
        g_releases[index] = release;
        save_releases_locked();
    }
    // Manta itself is always running: its files change once it has closed.
    if (!spec.hidden && title_running(spec.title_id))
    {
        detail = std::string(spec.name) + " is running";
        return JobError::running;
    }
    std::string target = g_paths.homebrew + "/" + spec.title_id;
    std::string mounted;
    const int mount = mounted_source(spec.title_id, mounted);
    if (mount < 0)
    {
        detail = "installed as a disk image (mount_img.lnk)";
        return JobError::install;
    }
    if (mount > 0 && self_update::kind(mounted) != Kind::absent && !holds_title(mounted, spec.title_id))
    {
        detail = "mount.lnk points to " + mounted + ", which is not " + spec.title_id;
        return JobError::install;
    }
    if (mount > 0)
        target = mounted;
    note("[EMULAND] install %s %s -> %s", spec.name, release.tag.c_str(), target.c_str());

    // 2. Download, hashed on the way in.
    mkdir(g_paths.state.c_str(), 0777);
    const std::string downloads = g_paths.state + "/downloads";
    mkdir(downloads.c_str(), 0777);
    const std::string archive = downloads + "/" + release.asset;
    const std::string partial = archive + ".part";
    std::remove(partial.c_str());
    set_stage(Stage::downloading, release.size);
    const int descriptor = open(partial.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (descriptor < 0)
    {
        detail = "cannot write " + partial + ": " + std::strerror(errno);
        return JobError::download;
    }
    self_update_sha256 hash;
    self_update_sha256_start(&hash);
    Sink sink;
    sink.descriptor = descriptor;
    sink.hash = &hash;
    const int fetched = fetch(release.url, sink, false, detail);
    const bool synced = fsync(descriptor) == 0;
    const bool closed = close(descriptor) == 0;
    if (sink.write_error != 0)
        detail += " | write failed after " + std::to_string(sink.written) + " of " +
                  std::to_string(release.size) + " bytes: " + std::strerror(sink.write_error);
    if (fetched != 0 || !closed || !synced)
    {
        if (fetched == 0)
            detail = "close or sync failed: " + std::string(std::strerror(errno));
        std::remove(partial.c_str());
        return fetched > 0 ? JobError::none : JobError::download;
    }
    note("[EMULAND] downloaded %s: %llu bytes", release.asset.c_str(),
                  static_cast<unsigned long long>(sink.written));

    // 3. The file GitHub published?
    set_stage(Stage::verifying, 0);
    unsigned char digest[32];
    self_update_sha256_finish(&hash, digest);
    char hex[65];
    self_update_sha256_hex(digest, hex);
    const std::string expected =
        starts_with(release.digest, "sha256:") ? release.digest.substr(7) : std::string();
    if (!expected.empty() && !self_update_sha256_matches(digest, expected.c_str()))
    {
        detail = std::string("sha256 ") + hex + " != " + expected;
        std::remove(partial.c_str());
        return JobError::digest;
    }
    if (expected.empty())
        note("[EMULAND] %s: GitHub published no digest, sha256 %s", release.asset.c_str(),
                      hex);
    std::rename(partial.c_str(), archive.c_str());

    // 4. The app folder out of the ZIP, next to its destination.
    self_update::ArchiveInfo info;
    if (!self_update::inspect_archive(archive, spec.title_id, info, detail))
    {
        std::remove(archive.c_str());
        return JobError::archive;
    }
    const std::string parent = parent_of(target);
    mkdir(parent.c_str(), 0777);
    // Staged outside homebrew, which ShadowMountPlus scans: a half-made app
    // there has a param.json it may pick up. A rename needs one filesystem, so
    // next to the destination otherwise.
    const std::string staging = g_paths.state + (spec.hidden ? "/self-update" : "/staging");
    mkdir(staging.c_str(), 0777);
    if (spec.hidden && !same_filesystem(staging, parent))
    {
        detail = "Manta runs from " + target + ", another drive than " + g_paths.state;
        std::remove(archive.c_str());
        return JobError::other_drive;
    }
    const std::string staged = same_filesystem(staging, parent)
                                   ? staging + "/" + spec.title_id
                                   : parent + "/.emuland-new-" + spec.title_id;
    self_update::remove_tree(staged);
    set_stage(Stage::extracting, info.unpacked);
    std::atomic<std::uint64_t> &written = g_progress;
    if (!self_update::extract_archive(archive, spec.title_id, staged, g_cancel, written, detail))
    {
        self_update::remove_tree(staged);
        std::remove(archive.c_str());
        return g_cancel.load() ? JobError::none : JobError::archive;
    }
    std::remove(archive.c_str());
    note("[EMULAND] extracted to %s", staged.c_str());

    // 5. Into place. From here on the job can't be cancelled (see cancel_job).
    set_stage(Stage::installing, 0);
    if (!open_permissions(staged))
    {
        detail = "chmod failed in " + staged;
        self_update::remove_tree(staged);
        return JobError::install;
    }
    if (spec.hidden)
        return hand_over_self_update(staged, target, release, detail);
    note("[EMULAND] permissions open, moving into %s", target.c_str());
    if (self_update::kind(target) == Kind::directory)
    {
        // The previous version is kept when it can be moved by a rename.
        const std::string backups = g_paths.state + "/backup";
        mkdir(backups.c_str(), 0777);
        std::string backup = backups + "/" + spec.title_id;
        if (!same_filesystem(backups, parent))
            backup = parent + "/.emuland-old-" + spec.title_id;
        self_update::remove_tree(backup);
        for (const char *folder : spec.merge)
            merge_user_files(target + "/" + folder, staged + "/" + folder);
        if (!swap_entries(target, staged, backup))
        {
            detail = "swap failed: " + std::string(std::strerror(errno));
            self_update::remove_tree(staged);
            return JobError::install;
        }
        self_update::remove_tree(staged);
        if (backup.find("/.emuland-old-") != std::string::npos)
            self_update::remove_tree(backup); // never leave an app copy where apps are scanned
    }
    else if (std::rename(staged.c_str(), target.c_str()) != 0)
    {
        detail = "rename failed: " + std::string(std::strerror(errno));
        self_update::remove_tree(staged);
        return JobError::install;
    }
    (void)chmod(target.c_str(), 0777);
    note("[EMULAND] in place, refreshing the console's copy");
    refresh_registered(spec.title_id, target);

    mkdir((g_paths.state + "/installed").c_str(), 0777);
    write_text(record_file(spec.title_id),
               release.tag + "\n" + release.asset + "\n" + release.digest + "\n");
    note("[EMULAND] installed %s %s", spec.name, release.tag.c_str());
    return JobError::none;
}

// A catalog path ("/data/PSXS5") on this machine (the PC test moves /data).
std::string data_path(const char *path)
{
    const std::string text = path;
    return starts_with(text, "/data/") ? g_paths.data + text.substr(5) : text;
}

// Asks ShadowMountPlus (docs/api.md of drakmor/ShadowMountPlus) to do
// something; true when it answered status 0. The answer goes to the log.
bool ask_shadowmount(const char *route, const std::string &request, json::Value &answer)
{
    if (g_paths.shadowmount.empty())
        return false;
    std::string body;
    Sink sink;
    sink.body = &body;
    sink.limit = 64u << 10;
    std::string detail;
    const int fetched = fetch(g_paths.shadowmount + route, sink, false, detail, &request);
    note("[EMULAND] shadowmount %s %s -> %s %s", route, request.c_str(),
         fetched == 0 ? "ok" : detail.c_str(), body.substr(0, 400).c_str());
    return json::parse(body, answer) && answer["status"].type == json::Value::Type::number &&
           answer["status"].number == 0.0;
}

// An uninstall: off the console's home screen through ShadowMountPlus (which
// asks the system's AppInstUtil), then the app folder, Emuland's record and
// backup, and with wipe the emulator's own data folder.
JobError run_uninstall(std::size_t index, bool wipe, std::string &detail)
{
    const EmulatorSpec &spec = catalog()[index];
    set_stage(Stage::removing, 0);
    if (title_running(spec.title_id))
    {
        detail = std::string(spec.name) + " is running";
        return JobError::running;
    }
    std::string target = g_paths.homebrew + "/" + spec.title_id;
    std::string mounted;
    const int mount = mounted_source(spec.title_id, mounted);
    if (mount < 0)
    {
        detail = "installed as a disk image (mount_img.lnk): remove it from ShadowMountPlus";
        return JobError::uninstall;
    }
    // Only ever the app's own folder: never what a broken record points to.
    if (mount > 0 && self_update::kind(mounted) == Kind::directory &&
        !holds_title(mounted, spec.title_id))
    {
        detail = mounted + " does not hold " + spec.title_id + ": nothing removed";
        return JobError::uninstall;
    }
    if (mount > 0)
        target = mounted;
    note("[EMULAND] uninstall %s from %s wipe=%d", spec.name, target.c_str(), wipe ? 1 : 0);

    json::Value answer;
    const bool unregistered = ask_shadowmount(
        "/games/uninstall", std::string("{\"title_id\":\"") + spec.title_id + "\"}", answer);

    // The emulator's data inside its own app folder (RetroArch): without wipe
    // only the app's files go, the data folders stay for a reinstall.
    const std::string data = data_path(spec.data);
    const bool data_inside = starts_with(data, (target + "/").c_str()) || data == target;
    bool removed = true;
    if (data_inside && !wipe)
    {
        const std::string base = data == target ? std::string() : data.substr(target.size() + 1) + "/";
        std::vector<std::string> keep;
        for (const FolderSpec &folder : spec.folders)
        {
            const std::string inside = base + folder.path;
            keep.push_back(inside.substr(0, inside.find('/')));
        }
        std::vector<std::string> names;
        list_entries(target, names);
        for (const std::string &name : names)
        {
            if (std::find(keep.begin(), keep.end(), name) == keep.end())
                removed = self_update::remove_tree(target + "/" + name) && removed;
        }
    }
    else
    {
        removed = self_update::remove_tree(target);
    }
    if (!removed)
    {
        detail = "could not remove " + target + ": " + std::strerror(errno);
        return JobError::uninstall;
    }
    std::remove(record_file(spec.title_id).c_str());
    self_update::remove_tree(g_paths.state + "/backup/" + spec.title_id);
    // Only a folder of its own: never /data itself or homebrew.
    if (wipe && !data_inside && starts_with(data, (g_paths.data + "/").c_str()) &&
        data != g_paths.homebrew && data.find("/..") == std::string::npos)
        note("[EMULAND] data %s removed=%d", data.c_str(),
             self_update::remove_tree(data) ? 1 : 0);

    // Refused while a game runs (Emuland is one, 409 "Device busy"), and its
    // later scan only drops the mount link, leaving the icon (2026-10-16). The
    // helper payload waits for Emuland to close and asks again then.
    if (!unregistered && !g_paths.shadowmount.empty())
    {
        if (std::FILE *file = std::fopen((g_paths.state + "/uninstall-request.txt").c_str(), "a"))
        {
            std::fprintf(file, "%s\n", spec.title_id);
            std::fclose(file);
        }
        const bool sent = installer::send_helper();
        note("[EMULAND] uninstall helper sent=%d", sent ? 1 : 0);
        std::lock_guard lock(g_mutex);
        g_status.needs_close = true;
    }
    note("[EMULAND] uninstalled %s", spec.name);
    return JobError::none;
}

// ---------------------------------------------------------------- import

// Is a drive mounted there? /mnt/usb0 to usb7 exist, empty, with nothing
// plugged in (seen 2026-10-16): only another device under the folder counts,
// or a folder written to it would land in the system's /mnt. Pendrives and
// external HDDs alike (exFAT or FAT32; the PS5 reads no NTFS) mount there.
bool drive_mounted(const std::string &path)
{
    struct stat info{};
    struct stat parent{};
    if (stat(path.c_str(), &info) != 0 || !S_ISDIR(info.st_mode))
        return false;
    if (!g_paths.usb_mounts_only)
        return true;
    return stat(parent_of(path).c_str(), &parent) == 0 && parent.st_dev != info.st_dev;
}

struct Copy
{
    std::string from;
    std::string to;
    std::uint64_t size = 0;
};

// Files under from/ that into/ lacks (or holds with another size), to copy.
// Hidden entries are left out: macOS puts "._name" and ".DS_Store" on every
// drive it touches, Windows "System Volume Information" at the root only.
void plan_copies(const std::string &from, const std::string &into, std::vector<Copy> &plan,
                 std::uint64_t &total, int &already, unsigned depth = 0)
{
    std::vector<std::string> names;
    if (depth > 8 || !list_entries(from, names))
        return;
    std::sort(names.begin(), names.end());
    for (const std::string &name : names)
    {
        if (name.empty() || name[0] == '.')
            continue;
        const std::string source = from + "/" + name;
        const std::string target = into + "/" + name;
        struct stat info{};
        if (lstat(source.c_str(), &info) != 0)
            continue;
        if (S_ISDIR(info.st_mode))
        {
            plan_copies(source, target, plan, total, already, depth + 1);
        }
        else if (S_ISREG(info.st_mode))
        {
            struct stat there{};
            if (stat(target.c_str(), &there) == 0 && S_ISREG(there.st_mode) &&
                there.st_size == info.st_size)
            {
                ++already; // copied before: a second Import picks up where it left
                continue;
            }
            plan.push_back({source, target, static_cast<std::uint64_t>(info.st_size)});
            total += static_cast<std::uint64_t>(info.st_size);
        }
    }
}

// Each missing folder of path, opened to everyone like the rest of /data.
bool make_folders(const std::string &path)
{
    for (std::size_t slash = path.find('/', 1); ; slash = path.find('/', slash + 1))
    {
        const std::string part = path.substr(0, slash);
        if (mkdir(part.c_str(), 0777) == 0)
            chmod(part.c_str(), 0777);
        else if (errno != EEXIST)
            return false;
        if (slash == std::string::npos)
            return true;
    }
}

// Copies one file through "<to>.emuland-part", so a pulled drive or a
// cancel never leaves a half file under the real name. 0, or the errno.
int copy_one(const Copy &copy, std::vector<char> &buffer)
{
    const std::string part = copy.to + ".emuland-part";
    const int in = open(copy.from.c_str(), O_RDONLY);
    if (in < 0)
        return errno;
    const int out = open(part.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (out < 0)
    {
        const int error = errno;
        close(in);
        return error;
    }
    int error = 0;
    while (error == 0)
    {
        if (g_cancel.load())
        {
            error = ECANCELED;
            break;
        }
        const ssize_t got = read(in, buffer.data(), buffer.size());
        if (got == 0)
            break;
        if (got < 0)
        {
            if (errno != EINTR)
                error = errno;
            continue;
        }
        std::size_t done = 0;
        while (done < static_cast<std::size_t>(got))
        {
            const ssize_t put = write(out, buffer.data() + done, static_cast<std::size_t>(got) - done);
            if (put > 0)
                done += static_cast<std::size_t>(put);
            else if (put < 0 && errno == EINTR)
                continue;
            else
            {
                error = put < 0 ? errno : ENOSPC;
                break;
            }
        }
        g_progress.fetch_add(done);
    }
    close(in);
    if (close(out) != 0 && error == 0)
        error = errno;
    if (error == 0 && std::rename(part.c_str(), copy.to.c_str()) != 0)
        error = errno;
    if (error != 0)
        unlink(part.c_str());
    else
        chmod(copy.to.c_str(), 0777);
    return error;
}

// Copies Emuland/<usb>/<folder key>/ from the first USB drive that has it
// into the emulator's data folders. Nothing on the drive is changed, nothing
// on the console is deleted; a file already there with the same size is kept.
JobError run_import(std::size_t index, int folder, std::string &detail)
{
    const EmulatorSpec &spec = catalog()[index];
    set_stage(Stage::importing, 0);
    std::string source;
    for (const std::string &drive : g_paths.usb)
    {
        if (!drive_mounted(drive))
            continue;
        // Manta/, or Emuland/ from before the new name (2026-10-16).
        for (const char *root : {"/Manta/", "/Emuland/"})
        {
            const std::string candidate = drive + root + spec.usb;
            if (source.empty() && self_update::kind(candidate) == Kind::directory)
                source = candidate;
        }
        if (!source.empty())
            break;
    }
    if (source.empty())
    {
        detail = std::string("no Manta/") + spec.usb + "/ on a USB drive";
        return JobError::no_usb;
    }
    std::vector<Copy> plan;
    std::uint64_t total = 0;
    int already = 0;
    int found = 0;
    for (std::size_t f = 0; f < spec.folders.size(); ++f)
    {
        if (folder != kAllFolders && static_cast<std::size_t>(folder) != f)
            continue;
        const std::string from = source + "/" + spec.folders[f].key;
        if (self_update::kind(from) != Kind::directory)
            continue;
        ++found;
        plan_copies(from, data_path(spec.data) + "/" + spec.folders[f].path, plan, total, already);
    }
    note("[EMULAND] import %s from %s folder=%d: %zu files, %llu bytes, %d already there",
         spec.name, source.c_str(), folder, plan.size(), static_cast<unsigned long long>(total),
         already);
    if (found == 0)
    {
        detail = source + (folder == kAllFolders
                               ? std::string("/ has none of the emulator's folders")
                               : "/" + std::string(spec.folders[static_cast<std::size_t>(folder)].key) +
                                     "/ is missing");
        return JobError::no_usb;
    }
    set_stage(Stage::importing, total);
    std::vector<char> buffer(1u << 20);
    int copied = 0;
    for (const Copy &copy : plan)
    {
        if (g_cancel.load())
            break;
        int error = make_folders(parent_of(copy.to)) ? 0 : errno;
        if (error == 0)
            error = copy_one(copy, buffer);
        if (error == ECANCELED)
            break;
        if (error != 0)
        {
            detail = copy.from + ": " + std::strerror(error);
            std::lock_guard lock(g_mutex);
            g_status.imported = copied;
            return error == ENOSPC ? JobError::no_space : JobError::copy;
        }
        ++copied;
    }
    note("[EMULAND] imported %d of %zu files", copied, plan.size());
    std::lock_guard lock(g_mutex);
    g_status.imported = copied;
    return JobError::none;
}

JobError run_prepare(std::string &detail)
{
    set_stage(Stage::preparing, 0);
    std::string where;
    const int made = prepare_usb(g_prepare_lang.load() == 0 ? Lang::pt : Lang::en, where);
    if (made == -1)
    {
        detail = "no USB drive";
        return JobError::no_usb;
    }
    if (made == -3)
    {
        detail = "no emulator installed";
        return JobError::none_installed;
    }
    if (made < 0)
    {
        detail = "could not write to " + where;
        return JobError::usb_write;
    }
    std::lock_guard lock(g_mutex);
    g_status.prepared = made;
    return JobError::none;
}

void *worker(void *)
{
    installer::clean_leftovers();
    for (;;)
    {
        int job = 0;
        {
            std::unique_lock lock(g_mutex);
            g_wake.wait(lock, [] { return !g_jobs.empty(); });
            job = g_jobs.front();
            g_jobs.pop_front();
            g_status.uninstall = job >= kUninstall && job < kImport;
            g_status.import = job >= kImport;
            g_status.imported = 0;
            g_status.prepare = job == kPrepareUsb;
            g_status.prepared = 0;
            g_status.needs_close = false;
            g_status.emulator = job >= kImport      ? (job - kImport) / 32
                                : job >= kUninstall ? (job - kUninstall) / 2
                                : job >= 0          ? job
                                                    : -1;
            g_status.queued = static_cast<int>(g_jobs.size());
            g_status.error = JobError::none;
            g_status.detail.clear();
            g_status.stage = Stage::checking;
        }
        g_cancel.store(false);
        g_progress.store(0);
        std::string detail;
        const bool check = job == kCheck || job == kAutoCheck;
        const JobError error =
            check                ? run_check(job == kCheck, detail)
            : job == kPrepareUsb ? run_prepare(detail)
            : job >= kImport     ? run_import(static_cast<std::size_t>((job - kImport) / 32),
                                              (job - kImport) % 32, detail)
            : job >= kUninstall ? run_uninstall(static_cast<std::size_t>((job - kUninstall) / 2),
                                                 (job - kUninstall) % 2 == 1, detail)
                                 : run_install(static_cast<std::size_t>(job), detail);
        if (!detail.empty())
            note("[EMULAND] job %d: %s", job, detail.c_str());
        std::fflush(stdout);
        std::lock_guard lock(g_mutex);
        g_status.error = error;
        g_status.detail = detail;
        g_status.stage = g_cancel.load()         ? Stage::cancelled
                         : error != JobError::none ? Stage::failed
                                                   : Stage::done;
        if (g_status.stage == Stage::cancelled)
            g_jobs.clear();
        if (check && error == JobError::none && g_checked_at > 0)
            clock_at(g_checked_at, &g_status.checked_hour, &g_status.checked_minute);
        if (error == JobError::rate_limited)
            clock_at(g_limited_until, &g_status.retry_hour, &g_status.retry_minute);
        ++g_status.finished;
    }
    return nullptr;
}

void enqueue(int job)
{
    std::lock_guard lock(g_mutex);
    load_releases_locked();
    if (std::find(g_jobs.begin(), g_jobs.end(), job) == g_jobs.end())
        g_jobs.push_back(job);
    g_status.queued = static_cast<int>(g_jobs.size());
    if (!g_thread_started)
    {
        pthread_attr_t attributes;
        pthread_attr_init(&attributes);
        pthread_attr_setstacksize(&attributes, 1u << 20);
        pthread_t thread;
        if (pthread_create(&thread, &attributes, worker, nullptr) == 0)
        {
            pthread_detach(thread);
            g_thread_started = true;
        }
        pthread_attr_destroy(&attributes);
    }
    g_wake.notify_one();
}

} // namespace

namespace installer
{

void set_paths(const Paths &paths)
{
    std::lock_guard lock(g_mutex);
    g_paths = paths;
    g_releases_loaded = false;
}

int load_online_catalog()
{
    std::string body;
    if (!self_update::read_small(g_paths.state + "/catalog.json", 256u << 10, body))
        return 0;
    OnlineCatalog parsed;
    std::string error;
    if (!parse_online_catalog(body, parsed, error))
    {
        note("[EMULAND] cached online catalog refused: %s", error.c_str());
        return 0;
    }
    const int added = apply_online_catalog(parsed);
    note("[EMULAND] online catalog: %d emulators added, %zu changes", added, parsed.changes.size());
    return added;
}

void clean_leftovers()
{
    const std::string staging = g_paths.state + "/staging";
    if (self_update::kind(staging) == Kind::directory)
        note("[EMULAND] leftover staging removed=%d", self_update::remove_tree(staging) ? 1 : 0);
    std::vector<std::string> names;
    if (!list_entries(g_paths.homebrew, names))
        return;
    for (const std::string &name : names)
    {
        if (starts_with(name, ".emuland-new-") || starts_with(name, ".emuland-old-"))
        {
            const bool removed = self_update::remove_tree(g_paths.homebrew + "/" + name);
            note("[EMULAND] leftover %s removed=%d", name.c_str(), removed ? 1 : 0);
        }
    }
}

std::string installed_tag(const char *title_id)
{
    const std::vector<std::string> lines = split(read_text(record_file(title_id)), '\n');
    return lines.empty() ? std::string() : lines[0];
}

bool cached_release(std::size_t index, Release &out)
{
    std::lock_guard lock(g_mutex);
    load_releases_locked();
    if (index >= g_releases.size() || g_releases[index].tag.empty())
        return false;
    out = g_releases[index];
    return true;
}

} // namespace installer

void start_check(bool forced)
{
    enqueue(forced ? kCheck : kAutoCheck);
}

void start_install(int index)
{
    if (index >= 0)
    {
        enqueue(index);
        return;
    }
    for (std::size_t i = 0; i < catalog().size(); ++i)
    {
        if (has_update(i))
            enqueue(static_cast<int>(i));
    }
}

void start_uninstall(int index, bool wipe_data)
{
    if (index >= 0 && static_cast<std::size_t>(index) < catalog().size())
        enqueue(kUninstall + index * 2 + (wipe_data ? 1 : 0));
}

void start_import(int index, int folder)
{
    if (index < 0 || static_cast<std::size_t>(index) >= catalog().size())
        return;
    const int which = folder < 0 || folder >= kAllFolders ? kAllFolders : folder;
    enqueue(kImport + index * 32 + which);
}

void start_prepare_usb(Lang lang)
{
    g_prepare_lang.store(lang == Lang::pt ? 0 : 1);
    enqueue(kPrepareUsb);
}

int prepare_usb(Lang lang, std::string &where)
{
    // The drive already prepared, else the first one plugged in. A drive
    // prepared before the new name keeps its Emuland/ folder: its files stay
    // where the user put them.
    std::string drive;
    std::string root = "/Manta";
    for (const std::string &candidate : g_paths.usb)
    {
        if (!drive_mounted(candidate))
            continue;
        const bool manta = self_update::kind(candidate + "/Manta") == Kind::directory;
        const bool legacy = self_update::kind(candidate + "/Emuland") == Kind::directory;
        if (manta || legacy)
        {
            drive = candidate;
            root = manta ? "/Manta" : "/Emuland";
            break;
        }
        if (drive.empty())
            drive = candidate;
    }
    if (drive.empty())
        return -1;
    where = drive;
    const std::string shown = root.substr(1); // "Manta"
    // Only what is on the console: a folder for an emulator not installed
    // would only raise the question of where it comes from.
    std::vector<const EmulatorSpec *> installed;
    for (const EmulatorSpec &spec : catalog())
    {
        if (!source_only(spec) && on_console(spec.title_id))
            installed.push_back(&spec);
    }
    if (installed.empty())
        return -3;
    const bool pt = lang == Lang::pt;
    std::string readme = pt ? "Manta - pastas do pendrive\r\n\r\n"
                              "Copie seus arquivos para estas pastas e use Importar, na aba Pastas "
                              "do Manta.\r\nO Manta só copia: nada aqui é apagado, e o que já "
                              "está no console com o mesmo tamanho é pulado.\r\n"
                            : "Manta - USB drive folders\r\n\r\n"
                              "Copy your files into these folders and use Import, on Manta's "
                              "Folders tab.\r\nManta only copies: nothing here is deleted, and "
                              "what the console already has with the same size is skipped.\r\n";
    int made = 0;
    const auto make = [&made](const std::string &path) {
        if (mkdir(path.c_str(), 0777) == 0)
            ++made;
        else if (errno != EEXIST)
            return false;
        return true;
    };
    if (!make(drive + root))
    {
        note("[EMULAND] prepare usb: mkdir %s%s: %s", drive.c_str(), root.c_str(),
             std::strerror(errno));
        return -2;
    }
    for (const EmulatorSpec *installed_spec : installed)
    {
        const EmulatorSpec &spec = *installed_spec;
        const std::string base = drive + root + "/" + spec.usb;
        if (!make(base))
            return -2;
        readme += std::string("\r\n") + spec.name + " (" + spec.system.get(lang) + ") -> " + shown + "/" +
                  spec.usb + "/\r\n";
        for (const FolderSpec &folder : spec.folders)
        {
            if (!make(base + "/" + folder.key))
                return -2;
            readme += std::string("  ") + folder.key + "/  " + folder.name.get(lang) + ": " +
                      folder.note.get(lang) + "\r\n";
        }
    }
    // UTF-8, which Notepad, TextEdit and the rest read.
    if (!write_text(drive + root + "/" + (pt ? "LEIA-ME.txt" : "README.txt"), readme))
        note("[EMULAND] prepare usb: readme not written");
    note("[EMULAND] prepare usb %s: %d folders made", drive.c_str(), made);
    return made;
}

void cancel_job()
{
    std::lock_guard lock(g_mutex);
    if (g_status.stage == Stage::installing || g_status.stage == Stage::removing)
        return; // the folder is being swapped or removed: let it finish
    g_cancel.store(true);
}

JobStatus job_status()
{
    std::lock_guard lock(g_mutex);
    JobStatus status = g_status;
    status.done = g_progress.load();
    status.queued = static_cast<int>(g_jobs.size());
    return status;
}

} // namespace emuland
