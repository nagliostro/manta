// Emuland - Game covers, found or downloaded on their own thread.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// For each game without a cover, in this order:
//   1. the emulator's covers folder (PSXS5 and SwanStation): the user's own,
//      and the ones PSXS5 downloads itself;
//   2. Emuland's cache, /data/emuland/covers/<usb>/<file name>.png;
//   3. with the setting on, libretro-thumbnails (thumbnails.libretro.com), the
//      box art RetroArch shows. Its files carry No-Intro and Redump names, as
//      game files do; its folder list (one page per system, kept 30 days)
//      finds the closest name: same title, then the same region.
// Each cover is decoded and reduced here, so the frame only uploads it
// (take_covers in art.cpp). Nothing in the emulators' folders changes.

#include "emuland/art.hpp"
#include "emuland/image.h"
#include "emuland/model.hpp"
#include "platform/ps5/console_curl.h"
#include "platform/ps5/system.hpp"

#include <algorithm>
#include <cctype>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <map>
#include <mutex>
#include <pthread.h>
#include <string>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

extern "C" void emuland_net_setup(CURL *easy); // emuland_platform.cpp

namespace emuland
{

namespace
{

constexpr const char *kCacheDir = "/data/emuland/covers";
constexpr const char *kChoiceFile = "/data/emuland/capas.txt";
constexpr const char *kServer = "https://thumbnails.libretro.com/";
constexpr int kMaxWidth = 256;              // the card is 224 wide at 1080p
constexpr std::time_t kIndexAge = 30 * 86400; // the folder list is fetched again after this
constexpr std::size_t kMaxImage = 8u << 20;
constexpr std::size_t kMaxIndex = 16u << 20;

// A game that needs a cover, copied from the library for this thread.
struct Want
{
    std::string path;
    std::string stem;
    std::string title;
    std::string region;
    int emulator = -1;
};

std::mutex g_mutex;
std::condition_variable g_wake;
std::vector<Want> g_pending;
bool g_has_pending = false;
bool g_download = true;
bool g_started = false;

// libretro-thumbnails' name for each emulator's games (none: Switch, and
// RetroArch's content folder, which mixes systems).
const char *libretro_system(const EmulatorSpec &spec)
{
    static const std::pair<const char *, const char *> kSystems[] = {
        {"psxs5", "Sony - PlayStation"},  {"swanstation", "Sony - PlayStation"},
        {"ps5sx2", "Sony - PlayStation 2"}, {"rpcs3", "Sony - PlayStation 3"},
        {"ps5cemu", "Nintendo - Wii U"},  {"xpsemu", "Microsoft - Xbox"},
    };
    for (const auto &[id, system] : kSystems)
    {
        if (std::strcmp(spec.id, id) == 0)
            return system;
    }
    return nullptr;
}

std::string stem_of(const std::string &name)
{
    const std::size_t dot = name.find_last_of('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

bool is_image(const std::string &name)
{
    const std::size_t dot = name.find_last_of('.');
    if (dot == std::string::npos)
        return false;
    const char *ext = name.c_str() + dot + 1;
    return strcasecmp(ext, "png") == 0 || strcasecmp(ext, "jpg") == 0 ||
           strcasecmp(ext, "jpeg") == 0;
}

bool exists(const std::string &path)
{
    struct stat info{};
    return stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

std::vector<std::string> list_names(const std::string &path)
{
    std::vector<std::string> names;
    DIR *directory = opendir(path.c_str());
    if (directory == nullptr)
        return names;
    while (const dirent *entry = readdir(directory))
    {
        if (entry->d_name[0] != '.')
            names.emplace_back(entry->d_name);
    }
    closedir(directory);
    return names;
}

void make_dirs(const std::string &path)
{
    for (std::size_t at = 1; at <= path.size(); ++at)
    {
        if (at == path.size() || path[at] == '/')
        {
            const std::string part = path.substr(0, at);
            if (mkdir(part.c_str(), 0777) == 0)
                chmod(part.c_str(), 0777);
        }
    }
}

bool read_file(const std::string &path, std::string &bytes, std::size_t limit)
{
    std::FILE *file = std::fopen(path.c_str(), "rb");
    if (file == nullptr)
        return false;
    bytes.clear();
    char buffer[65536];
    std::size_t count = 0;
    while ((count = std::fread(buffer, 1, sizeof(buffer), file)) > 0 && bytes.size() < limit)
        bytes.append(buffer, count);
    std::fclose(file);
    return bytes.size() < limit;
}

bool write_file(const std::string &path, const std::string &bytes)
{
    const std::string part = path + ".emuland-part";
    std::FILE *file = std::fopen(part.c_str(), "wb");
    if (file == nullptr)
        return false;
    const bool written = std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
    const bool closed = std::fclose(file) == 0;
    if (!written || !closed || std::rename(part.c_str(), path.c_str()) != 0)
    {
        std::remove(part.c_str());
        return false;
    }
    chmod(path.c_str(), 0777);
    return true;
}

// How close a name is to the game: -1 another game, higher is closer.
int score(const std::string &name, const Want &want)
{
    if (name == want.stem)
        return 1000;
    std::string title;
    std::string region;
    tidy_game_name(name, title, region);
    if (title_key(title) != title_key(want.title))
        return -1;
    int points = 100;
    if (!want.region.empty() && region == want.region)
        points += 20;
    else if (want.region.empty() && region == "USA")
        points += 10;
    for (const char c : name)
        points -= c == '(' || c == '[' ? 1 : 0; // fewer tags: the plain release
    return points;
}

std::string best(const std::vector<std::string> &names, const Want &want)
{
    std::string found;
    int found_score = -1;
    for (const std::string &name : names)
    {
        const int points = score(name, want);
        if (points > found_score)
        {
            found_score = points;
            found = name;
        }
    }
    return found;
}

// An image in a folder of covers named after the games.
std::string find_in(const std::string &dir, const Want &want,
                    std::map<std::string, std::vector<std::string>> &listings)
{
    for (const char *ext : {".png", ".jpg", ".jpeg"})
    {
        if (exists(dir + "/" + want.stem + ext))
            return dir + "/" + want.stem + ext;
    }
    auto listed = listings.find(dir);
    if (listed == listings.end())
    {
        std::vector<std::string> stems;
        for (const std::string &name : list_names(dir))
        {
            if (is_image(name))
                stems.push_back(name);
        }
        listed = listings.emplace(dir, std::move(stems)).first;
    }
    std::string match;
    int match_score = -1;
    for (const std::string &name : listed->second)
    {
        const int points = score(stem_of(name), want);
        if (points > match_score)
        {
            match_score = points;
            match = name;
        }
    }
    return match.empty() ? std::string() : dir + "/" + match;
}

std::size_t on_body(char *data, std::size_t size, std::size_t count, void *user)
{
    auto &sink = *static_cast<std::pair<std::string *, std::size_t> *>(user);
    const std::size_t bytes = size * count;
    if (sink.first->size() + bytes > sink.second)
        return 0;
    sink.first->append(data, bytes);
    return bytes;
}

// 200 with the whole body: 0. A 404: 1 (the name is not there). Anything
// else (no network, a server error): -1.
int get(const std::string &url, std::string &body, std::size_t limit)
{
    CURL *easy = curl_easy_init();
    if (easy == nullptr)
        return -1;
    emuland_net_setup(easy);
    body.clear();
    std::pair<std::string *, std::size_t> sink{&body, limit};
    curl_slist *headers = curl_slist_append(nullptr, kUserAgent);
    curl_easy_setopt(easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_MAXREDIRS, 4L);
    curl_easy_setopt(easy, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(easy, CURLOPT_REDIR_PROTOCOLS_STR, "https");
    curl_easy_setopt(easy, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT_MS, 10000L);
    curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, 60000L);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, on_body);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &sink);
    const CURLcode result = curl_easy_perform(easy);
    long status = 0;
    curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
    curl_easy_cleanup(easy);
    curl_slist_free_all(headers);
    if (result == CURLE_OK && status == 200)
        return 0;
    if (result == CURLE_OK && status == 404)
        return 1;
    hui::sys::log("[EMULAND] covers: %s curl %d HTTP %ld", url.c_str(), static_cast<int>(result),
                  status);
    return -1;
}

std::string encode(const std::string &text)
{
    static const char kHex[] = "0123456789ABCDEF";
    std::string out;
    for (const char c : text)
    {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isalnum(u) != 0 || c == '-' || c == '_' || c == '.' || c == '~')
        {
            out += c;
            continue;
        }
        out += '%';
        out += kHex[u >> 4];
        out += kHex[u & 15];
    }
    return out;
}

std::string decode(const std::string &text)
{
    std::string out;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '%' && i + 2 < text.size())
        {
            out += static_cast<char>(std::strtol(text.substr(i + 1, 2).c_str(), nullptr, 16));
            i += 2;
        }
        else
        {
            out += text[i] == '+' ? ' ' : text[i];
        }
    }
    return out;
}

// The names libretro-thumbnails has for a system (without ".png"): from the
// cache while it is under 30 days old, else from its folder page.
bool load_index(const char *system, const std::string &cache, bool download,
                std::vector<std::string> &names, bool *offline)
{
    const std::string file = cache + "/index.txt";
    struct stat info{};
    std::string text;
    const bool fresh = stat(file.c_str(), &info) == 0 &&
                       std::time(nullptr) - info.st_mtime < kIndexAge;
    const bool cached = (fresh || !download) && read_file(file, text, kMaxIndex) && !text.empty();
    std::string page;
    const int got = cached || !download
                        ? 1
                        : get(std::string(kServer) + encode(system) + "/Named_Boxarts/", page,
                              kMaxIndex);
    if (!cached && got != 0)
    {
        *offline = got < 0;
        if (!read_file(file, text, kMaxIndex)) // an old list beats none
            return false;
    }
    else if (!cached)
    {
        text.clear();
        for (std::size_t at = page.find("href=\""); at != std::string::npos;
             at = page.find("href=\"", at))
        {
            at += 6;
            const std::size_t end = page.find('"', at);
            if (end == std::string::npos)
                break;
            const std::string href = page.substr(at, end - at);
            if (href.size() > 4 && href.compare(href.size() - 4, 4, ".png") == 0 &&
                href.find('/') == std::string::npos && href[0] != '?')
                text += decode(href.substr(0, href.size() - 4)) + "\n";
        }
        make_dirs(cache);
        write_file(file, text);
    }
    names.clear();
    std::size_t start = 0;
    for (std::size_t end = text.find('\n'); end != std::string::npos;
         start = end + 1, end = text.find('\n', start))
    {
        if (end > start)
            names.push_back(text.substr(start, end - start));
    }
    return !names.empty();
}

// Decoded, reduced to kMaxWidth and handed to the frame.
bool post(const std::string &file, const Want &want)
{
    std::string bytes;
    if (!read_file(file, bytes, kMaxImage))
        return false;
    int width = 0;
    int height = 0;
    unsigned char *pixels =
        emuland_decode_image(reinterpret_cast<const unsigned char *>(bytes.data()),
                             static_cast<int>(bytes.size()), &width, &height);
    if (pixels == nullptr || width <= 0 || height <= 0)
    {
        hui::sys::log("[EMULAND] covers: unreadable %s", file.c_str());
        if (pixels != nullptr)
            emuland_free_image(pixels);
        return false;
    }
    // A box filter: each output pixel averages the source pixels it covers.
    const int out_w = width > kMaxWidth ? kMaxWidth : width;
    const int out_h = width > kMaxWidth ? (height * kMaxWidth + width / 2) / width : height;
    std::vector<unsigned char> out(static_cast<std::size_t>(out_w) * out_h * 4);
    for (int y = 0; y < out_h; ++y)
    {
        const int y0 = y * height / out_h;
        const int y1 = std::max(y0 + 1, (y + 1) * height / out_h);
        for (int x = 0; x < out_w; ++x)
        {
            const int x0 = x * width / out_w;
            const int x1 = std::max(x0 + 1, (x + 1) * width / out_w);
            unsigned sum[4] = {};
            for (int sy = y0; sy < y1; ++sy)
            {
                const unsigned char *row = pixels + (static_cast<std::size_t>(sy) * width + x0) * 4;
                for (int sx = x0; sx < x1; ++sx, row += 4)
                {
                    for (int c = 0; c < 4; ++c)
                        sum[c] += row[c];
                }
            }
            const unsigned n = static_cast<unsigned>((y1 - y0) * (x1 - x0));
            unsigned char *to = &out[(static_cast<std::size_t>(y) * out_w + x) * 4];
            for (int c = 0; c < 4; ++c)
                to[c] = static_cast<unsigned char>(sum[c] / n);
        }
    }
    emuland_free_image(pixels);
    post_cover(want.path, out_w, out_h, std::move(out));
    return true;
}

void *worker(void *)
{
    std::map<std::string, std::vector<std::string>> listings;
    std::map<std::string, std::vector<std::string>> indexes;
    for (;;)
    {
        std::vector<Want> wants;
        bool download = false;
        {
            std::unique_lock lock(g_mutex);
            g_wake.wait(lock, [] { return g_has_pending; });
            wants = std::move(g_pending);
            g_pending.clear();
            g_has_pending = false;
            download = g_download;
        }
        listings.clear(); // folders may have changed (an import, a download)
        for (auto it = indexes.begin(); it != indexes.end();)
            it = it->second.empty() ? indexes.erase(it) : std::next(it); // offline last time
        int found = 0;
        int downloaded = 0;
        bool offline = false;
        for (const Want &want : wants)
        {
            {
                std::lock_guard lock(g_mutex);
                if (g_has_pending)
                    break; // a newer list holds the rest
            }
            if (has_cover(want.path))
                continue;
            const EmulatorSpec &spec = catalog()[static_cast<std::size_t>(want.emulator)];
            const std::string cache = std::string(kCacheDir) + "/" + spec.usb;
            std::string file;
            for (const FolderSpec &folder : spec.folders)
            {
                if (file.empty() && std::strcmp(folder.key, "capas") == 0)
                    file = find_in(std::string(spec.data) + "/" + folder.path, want, listings);
            }
            if (file.empty())
                file = find_in(cache, want, listings);
            const char *system = libretro_system(spec);
            if (file.empty() && download && !offline && system != nullptr)
            {
                auto index = indexes.find(system);
                if (index == indexes.end())
                {
                    std::vector<std::string> names;
                    load_index(system, cache, true, names, &offline);
                    index = indexes.emplace(system, std::move(names)).first;
                }
                const std::string name = best(index->second, want);
                if (!name.empty())
                {
                    std::string image;
                    const int got = get(std::string(kServer) + encode(system) + "/Named_Boxarts/" +
                                            encode(name) + ".png",
                                        image, kMaxImage);
                    offline = got < 0;
                    const std::string target = cache + "/" + want.stem + ".png";
                    make_dirs(cache);
                    if (got == 0 && write_file(target, image))
                    {
                        file = target;
                        ++downloaded;
                    }
                }
            }
            if (!file.empty() && post(file, want))
                ++found;
        }
        hui::sys::log("[EMULAND] covers: %zu wanted, %d shown, %d downloaded%s", wants.size(),
                      found, downloaded, offline ? ", offline" : "");
    }
    return nullptr;
}

} // namespace

void start_covers(bool download)
{
    std::vector<Want> wants;
    for (const Game &game : library().games)
    {
        if (!has_cover(game.path))
            wants.push_back({game.path, game.stem, game.title, game.region, game.emulator});
    }
    std::lock_guard lock(g_mutex);
    g_pending = std::move(wants);
    g_has_pending = true;
    g_download = download;
    if (!g_started)
    {
        pthread_attr_t attributes;
        pthread_attr_init(&attributes);
        pthread_attr_setstacksize(&attributes, 1u << 20);
        pthread_t thread;
        if (pthread_create(&thread, &attributes, worker, nullptr) == 0)
        {
            pthread_detach(thread);
            g_started = true;
        }
        pthread_attr_destroy(&attributes);
    }
    g_wake.notify_one();
}

bool load_cover_choice()
{
    std::FILE *file = std::fopen(kChoiceFile, "rb");
    if (file == nullptr)
        return true;
    const int c = std::fgetc(file);
    std::fclose(file);
    return c != '0';
}

void save_cover_choice(bool download)
{
    make_dirs("/data/emuland");
    std::FILE *file = std::fopen(kChoiceFile, "wb");
    if (file == nullptr)
        return;
    std::fputc(download ? '1' : '0', file);
    std::fclose(file);
}

} // namespace emuland
