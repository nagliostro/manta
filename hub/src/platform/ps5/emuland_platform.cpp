// Emuland - PS5 side of the library: installed versions, folders, launching.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Everything here was first proven by the console test app (app/, see
// docs/TESTE-PS5.md): contentVersion from param.json, opendir/readdir under
// /data once the HEN has freed the app, and sceSystemServiceLaunchApp, which
// returns 0x80940010 and still opens the title (sceLncUtilLaunchApp, called
// from an app, closes it).

#include "emuland/model.hpp"
#include "platform/ps5/console_curl.h"
#include "platform/ps5/installer/installer.hpp"
#include "platform/ps5/system.hpp"
#include "platform/ps5/unlock.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <arpa/inet.h>
#include <cerrno>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <strings.h>
#include <sys/stat.h>
#include <vector>

namespace
{

struct LaunchParam
{
    std::uint32_t size;
    std::int32_t user_id;
    std::uint32_t app_opt;
    std::uint64_t crash_report;
    std::uint64_t check_flag;
};
static_assert(sizeof(LaunchParam) == 32);

struct KernelTimeval
{
    std::int64_t tv_sec;
    std::int64_t tv_usec;
};

// What sceKernelConvertUtcToLocaltime fills (OrbisTimesec): 16 bytes. Declared
// as 8 (and dst as an int) it wrote past them on the stack, and the clock
// read 03:28 at 19:48 (2026-10-16).
struct KernelTimesec
{
    std::int64_t rule_start; // when the zone's current rule began
    std::uint32_t west_seconds;
    std::uint32_t dst_seconds;
};
static_assert(sizeof(KernelTimesec) == 16);

} // namespace

extern "C"
{
    int sceSystemServiceLaunchApp(const char *title_id, const char **arguments, LaunchParam *param);
    int sceUserServiceGetForegroundUser(int *user);
    int sceUserServiceGetInitialUser(int *user);
    int sceSystemServiceParamGetInt(int parameter, int *value);
    int sceKernelGettimeofday(KernelTimeval *time);
    // The console's own time zone and daylight saving (libSceRtc is not linked).
    int sceKernelConvertUtcToLocaltime(std::int64_t utc, std::int64_t *local, KernelTimesec *zone,
                                       std::uint64_t *dst);
}

extern "C" void emuland_net_setup(CURL *easy)
{
    console_curl_setup(easy); // no signals, the console's certificates, non-blocking sockets
}

namespace emuland
{

namespace
{

constexpr int kParamLanguage = 1;
constexpr int kLanguagePortuguesePt = 7;
constexpr int kLanguagePortugueseBr = 17;
constexpr const char *kStateDir = "/data/emuland";
constexpr const char *kLanguageFile = "/data/emuland/idioma.txt";

std::string read_small(const std::string &path)
{
    std::FILE *file = std::fopen(path.c_str(), "rb");
    if (file == nullptr)
        return {};
    char buffer[8192];
    const std::size_t count = std::fread(buffer, 1, sizeof(buffer) - 1, file);
    std::fclose(file);
    return std::string(buffer, count);
}

// contentVersion of an installed title, empty when it is not installed.
std::string installed_version(const char *title_id)
{
    const std::string json =
        read_small(std::string("/data/homebrew/") + title_id + "/sce_sys/param.json");
    if (json.empty())
        return {};
    const std::size_t key = json.find("\"contentVersion\"");
    const std::size_t open = key == std::string::npos ? key : json.find('"', json.find(':', key));
    const std::size_t close = open == std::string::npos ? open : json.find('"', open + 1);
    if (close == std::string::npos)
        return "?";
    return json.substr(open + 1, close - open - 1);
}

bool matches(const char *name, const char *extensions)
{
    if (extensions == nullptr)
        return true;
    const char *dot = std::strrchr(name, '.');
    if (dot == nullptr || dot[1] == '\0')
        return false;
    const std::size_t length = std::strlen(dot + 1);
    for (const char *at = extensions; *at != '\0';)
    {
        const char *end = std::strchr(at, ',');
        const std::size_t size = end != nullptr ? static_cast<std::size_t>(end - at) : std::strlen(at);
        if (size == length && strncasecmp(at, dot + 1, size) == 0)
            return true;
        if (end == nullptr)
            break;
        at = end + 1;
    }
    return false;
}

bool is_directory(const std::string &path)
{
    struct stat info{};
    return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

// What a folder holds, -1 when it does not exist (see FolderSpec::extensions).
// Depth-limited: a link back to a parent folder (copied over FTP) must not
// recurse until the stack runs out.
int count_entries(const std::string &path, const char *extensions, int depth = 0)
{
    DIR *directory = depth < 6 ? opendir(path.c_str()) : nullptr;
    if (directory == nullptr)
        return -1;
    std::vector<std::string> stems;
    int count = 0;
    while (const dirent *entry = readdir(directory))
    {
        if (entry->d_name[0] == '.')
            continue;
        const std::string child = path + "/" + entry->d_name;
        if (extensions != nullptr && is_directory(child))
        {
            count += count_entries(child, extensions, depth + 1) > 0 ? 1 : 0;
            continue;
        }
        if (!matches(entry->d_name, extensions))
            continue;
        std::string stem = entry->d_name;
        if (extensions != nullptr)
            stem = stem.substr(0, stem.rfind('.'));
        bool seen = false;
        for (const std::string &other : stems)
            seen = seen || strcasecmp(other.c_str(), stem.c_str()) == 0;
        if (!seen)
        {
            stems.push_back(stem);
            ++count;
        }
    }
    closedir(directory);
    return count;
}

std::uint64_t size_of(const std::string &path, int depth = 0)
{
    struct stat info{};
    if (stat(path.c_str(), &info) != 0)
        return 0;
    if (!S_ISDIR(info.st_mode))
        return static_cast<std::uint64_t>(info.st_size);
    std::uint64_t total = 0;
    DIR *directory = depth < 4 ? opendir(path.c_str()) : nullptr;
    if (directory == nullptr)
        return 0;
    while (const dirent *entry = readdir(directory))
    {
        if (entry->d_name[0] != '.')
            total += size_of(path + "/" + entry->d_name, depth + 1);
    }
    closedir(directory);
    return total;
}

// Which file stands for a game when several share its title: the playlist
// of a multi-disc game, then the cue sheet over its bin tracks.
int file_rank(const std::string &format)
{
    static const char *const kOrder[] = {"M3U", "CUE", "CHD", "ISO", "PBP", "CSO", "ZSO",
                                         "XCI", "NSP", "XISO"};
    for (int i = 0; i < static_cast<int>(std::size(kOrder)); ++i)
    {
        if (format == kOrder[i])
            return i;
    }
    return format == "BIN" ? 99 : 50;
}

// The games in an emulator's games folder: each matching file, or each
// subfolder holding one (one folder per game). Discs and tracks of one game
// (same title and region) show once.
void list_games(int emulator, const EmulatorSpec &spec, const FolderSpec &folder,
                std::vector<Game> &games)
{
    const std::string path = std::string(spec.data) + "/" + folder.path;
    DIR *directory = opendir(path.c_str());
    if (directory == nullptr)
        return;
    const std::size_t first = games.size();
    while (const dirent *entry = readdir(directory))
    {
        if (entry->d_name[0] == '.')
            continue;
        const std::string name = entry->d_name;
        Game game;
        game.path = path + "/" + name;
        game.emulator = emulator;
        if (is_directory(game.path))
        {
            if (folder.extensions != nullptr && count_entries(game.path, folder.extensions) <= 0)
                continue;
            game.stem = name;
        }
        else
        {
            if (!matches(name.c_str(), folder.extensions))
                continue;
            const std::size_t dot = name.find_last_of('.');
            game.stem = dot == std::string::npos ? name : name.substr(0, dot);
            if (dot != std::string::npos)
            {
                for (std::size_t i = dot + 1; i < name.size(); ++i)
                    game.format += static_cast<char>(
                        std::toupper(static_cast<unsigned char>(name[i])));
            }
        }
        tidy_game_name(game.stem, game.title, game.region);
        bool merged = false;
        for (std::size_t i = first; i < games.size() && !merged; ++i)
        {
            Game &other = games[i];
            if (other.region != game.region || title_key(other.title) != title_key(game.title))
                continue;
            merged = true;
            const int mine = file_rank(game.format);
            const int theirs = file_rank(other.format);
            if (mine < theirs || (mine == theirs && game.stem < other.stem))
                other = game;
        }
        if (!merged)
            games.push_back(game);
    }
    closedir(directory);
    for (std::size_t i = first; i < games.size(); ++i)
        games[i].size = size_of(games[i].path);
}

} // namespace

void scan(Library &library)
{
    library.data_access = data_listable();
    const auto entries = catalog();
    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        const EmulatorSpec &spec = entries[i];
        EmulatorState &state = library.emulators[i];
        state.installed = installed_version(spec.title_id);
        state.installed_tag = installer::installed_tag(spec.title_id);
        state.registered = state.installed.empty() || registered(spec);
        Release release;
        if (installer::cached_release(i, release))
        {
            state.release = release;
            state.latest = release.tag;
        }
        for (std::size_t f = 0; f < spec.folders.size(); ++f)
            state.folder_counts[f] = count_entries(
                std::string(spec.data) + "/" + spec.folders[f].path, spec.folders[f].extensions);
        hui::sys::log("[EMULAND] %s installed=%s", spec.name,
                      state.installed.empty() ? "no" : state.installed.c_str());
    }
    // The library: the games of what is installed, by title.
    library.games.clear();
    for (std::size_t i = 0; i < entries.size(); ++i)
    {
        if (library.emulators[i].installed.empty())
            continue;
        for (std::size_t f = 0; f < entries[i].folders.size(); ++f)
        {
            if (!entries[i].folders[f].games)
                continue;
            const std::size_t before = library.games.size();
            list_games(static_cast<int>(i), entries[i], entries[i].folders[f], library.games);
            if (library.emulators[i].folder_counts[f] >= 0)
                library.emulators[i].folder_counts[f] =
                    static_cast<int>(library.games.size() - before);
        }
    }
    std::stable_sort(library.games.begin(), library.games.end(),
                     [](const Game &a, const Game &b) {
                         return title_key(a.title) < title_key(b.title);
                     });
    hui::sys::log("[EMULAND] games=%zu", library.games.size());
}

// ShadowMountPlus registers an app in /user/app/<TITLEID> (with mount.lnk),
// as the system does for an installed package. Seen on 2026-10-16: opening
// before that gives 0x80940031. Without /data access nothing can be told,
// so nothing is held back.
// The system won't load libSceAppInstUtil into an app (0x80020063, and a
// linked import took Emuland down): the helper runs in the ELF loader's
// process instead. Its ELF ships in the app's assets.
bool installer::send_uninstall_helper()
{
    const char *const places[] = {"/app0/assets/helpers/uninstall_helper.elf",
                                  "/data/homebrew/PPSA98265/assets/helpers/uninstall_helper.elf"};
    std::FILE *file = nullptr;
    for (const char *place : places)
    {
        if ((file = std::fopen(place, "rb")) != nullptr)
            break;
    }
    if (file == nullptr)
    {
        hui::sys::log("[EMULAND] uninstall helper not found");
        return false;
    }
    std::vector<char> elf;
    char buffer[16384];
    std::size_t count = 0;
    while ((count = std::fread(buffer, 1, sizeof(buffer), file)) > 0)
        elf.insert(elf.end(), buffer, buffer + count);
    std::fclose(file);

    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return false;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(9021);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bool sent = connect(sock, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0;
    std::size_t done = 0;
    while (sent && done < elf.size())
    {
        const ssize_t moved = send(sock, elf.data() + done, elf.size() - done, 0);
        if (moved > 0)
            done += static_cast<std::size_t>(moved);
        else if (moved < 0 && errno == EINTR)
            continue;
        else
            sent = false;
    }
    close(sock);
    hui::sys::log("[EMULAND] uninstall helper %zu bytes to 127.0.0.1:9021 sent=%d", elf.size(),
                  sent ? 1 : 0);
    return sent;
}

bool registered(const EmulatorSpec &emulator)
{
    if (!data_listable())
        return true;
    struct stat info{};
    const std::string app = std::string("/user/app/") + emulator.title_id;
    return stat(app.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

int launch(const EmulatorSpec &emulator, const std::string &game)
{
    int user = -1;
    if (sceUserServiceGetForegroundUser(&user) != 0)
        sceUserServiceGetInitialUser(&user);
    LaunchParam param{};
    param.size = sizeof(param);
    param.user_id = user;
    // The option twice: SwanStationPS5 reads its arguments from argv[1] on,
    // PS5CEMU-HAR from argv[0], and whether the system puts a program name
    // first is not documented. Twice, each finds one pair.
    const char *arguments[] = {emulator.rom_option, game.c_str(), emulator.rom_option,
                               game.c_str(), nullptr};
    const bool with_game = emulator.rom_option != nullptr && !game.empty();
    hui::sys::log("[EMULAND] launching %s user=0x%08x%s%s", emulator.title_id,
                  static_cast<unsigned>(user), with_game ? " game=" : "",
                  with_game ? game.c_str() : "");
    std::fflush(stdout); // the log must be on disk if this call takes the app down
    const int result =
        sceSystemServiceLaunchApp(emulator.title_id, with_game ? arguments : nullptr, &param);
    hui::sys::log("[EMULAND] sceSystemServiceLaunchApp(%s)=0x%08x", emulator.title_id,
                  static_cast<unsigned>(result));
    return result;
}

int create_missing_folders(const EmulatorSpec &emulator, EmulatorState &state)
{
    mkdir(emulator.data, 0777);
    int made = 0;
    for (std::size_t f = 0; f < emulator.folders.size(); ++f)
    {
        if (state.folder_counts[f] >= 0)
            continue;
        const std::string path = std::string(emulator.data) + "/" + emulator.folders[f].path;
        if (mkdir(path.c_str(), 0777) == 0)
        {
            chmod(path.c_str(), 0777);
            state.folder_counts[f] = 0;
            ++made;
        }
        else
        {
            hui::sys::log("[EMULAND] mkdir %s failed", path.c_str());
        }
    }
    return made;
}

Lang system_language()
{
    int language = 0;
    if (sceSystemServiceParamGetInt(kParamLanguage, &language) != 0)
        return Lang::pt;
    return language == kLanguagePortuguesePt || language == kLanguagePortugueseBr ? Lang::pt
                                                                                  : Lang::en;
}

void local_time(int *hour, int *minute)
{
    KernelTimeval now{};
    std::int64_t local = 0;
    KernelTimesec zone{};
    std::uint64_t dst = 0;
    if (sceKernelGettimeofday(&now) != 0 ||
        sceKernelConvertUtcToLocaltime(now.tv_sec, &local, &zone, &dst) != 0)
        return;
    const std::int64_t seconds_of_day = ((local % 86400) + 86400) % 86400;
    *hour = static_cast<int>(seconds_of_day / 3600);
    *minute = static_cast<int>(seconds_of_day / 60 % 60);
}

int load_language_choice()
{
    const std::string saved = read_small(kLanguageFile);
    return saved.empty() || saved[0] < '0' || saved[0] > '2' ? 0 : saved[0] - '0';
}

void save_language_choice(int choice)
{
    mkdir(kStateDir, 0777);
    std::FILE *file = std::fopen(kLanguageFile, "wb");
    if (file == nullptr)
        return;
    std::fputc('0' + choice, file);
    std::fclose(file);
}

void quit()
{
    std::fflush(stdout);
    hui::sys::quit();
}

} // namespace emuland
