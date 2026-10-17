// Emuland - PC preview side of the library: sample data, no file system.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The three states the screens must handle, as in the HTML prototype: PSXS5
// up to date, PS5SX2 with an update, ProsperoEden not installed.
// EMULAND_LANG=en renders the English interface.

#include "core/save_file.hpp"
#include "emuland/art.hpp"
#include "emuland/image.h"
#include "emuland/model.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace emuland
{

void scan(Library &library)
{
    // Filled once: later scans (after an install) keep what the simulation did.
    static bool filled = false;
    if (filled)
        return;
    filled = true;
    library.data_access = true;
    auto &e = library.emulators;
    e[0].installed = "01.000.000";
    e[0].installed_tag = "v2.2.0";
    e[0].folder_counts = {12, 0, 12};
    e[1].installed = "02.000.000";
    e[1].installed_tag.clear(); // copied by hand: its tag is not known
    e[1].folder_counts = {14, 5, 0, 6, 0, 2};
    e[2].installed.clear();
    e[2].folder_counts = {-1, -1, -1, -1, -1};
    for (std::size_t i = 3; i < e.size(); ++i)
        std::fill(e[i].folder_counts.begin(), e[i].folder_counts.end(), -1);
    // SwanStationPS5 too, with an empty games folder: it opens PSXS5's games,
    // so choosing it shows the "open in which emulator" dialog.
    e[3].installed = "01.002.008";
    e[3].installed_tag = "1.2.8";
    std::fill(e[3].folder_counts.begin(), e[3].folder_counts.end(), 0);
    // The others in the catalog: not installed, with their real newest releases
    // (the PS3 port publishes source only).
    const char *tags[] = {"v2.2.0",    "vk-285-171",    "v1.002.000", "1.2.8",
                          "v1.0.0-beta.1", "v3.5.0", "Alpha-2.1",  ""};
    const std::uint64_t sizes[] = {18360114, 18523969, 39615644, 16318258,
                                   788929239, 46964967, 21413061, 0};
    for (std::size_t i = 0; i < e.size() && i < 8; ++i)
    {
        e[i].latest = tags[i];
        e[i].release.tag = tags[i];
        e[i].release.size = sizes[i];
        e[i].release.prerelease = i == 2;
    }
    // The games folders of the two installed ones, as their files are named.
    const std::pair<const char *, std::uint64_t> ps1[] = {
        {"Castlevania - Symphony of the Night (USA).chd", 498000000},
        {"Metal Gear Solid (USA) (Disc 1).chd", 512000000},
        {"Gran Turismo 2 (USA) (Simulation Mode).chd", 421000000},
        {"Resident Evil 2 (USA) (Disc 1).chd", 389000000},
        {"Spyro the Dragon (USA).chd", 302000000},
        {"Tony Hawk's Pro Skater 2 (USA).chd", 276000000},
        {"Crash Bandicoot (USA).chd", 251000000},
        {"Silent Hill (USA).chd", 365000000},
        {"Final Fantasy VII (USA).m3u", 1980000000},
        {"Chrono Cross (USA).m3u", 1210000000},
        {"Tekken 3 (USA).chd", 330000000},
        {"Ape Escape (USA).chd", 402000000},
    };
    const std::pair<const char *, std::uint64_t> ps2[] = {
        {"God of War II (USA).iso", 7950000000},
        {"Shadow of the Colossus (USA).chd", 2310000000},
        {"Grand Theft Auto - San Andreas (USA).iso", 4380000000},
        {"Gran Turismo 4 (USA).iso", 7690000000},
        {"Metal Gear Solid 3 - Snake Eater (USA).chd", 3720000000},
        {"Final Fantasy X (USA).iso", 3940000000},
        {"Kingdom Hearts II (USA).chd", 3110000000},
        {"Ico (Europe).iso", 690000000},
        {"Okami (USA).chd", 2840000000},
        {"Persona 4 (USA).iso", 3770000000},
        {"Burnout 3 - Takedown (USA).iso", 3510000000},
        {"Silent Hill 2 (USA) [SLUS-20228].iso", 1840000000},
        {"Ratchet & Clank (USA).chd", 2520000000},
        {"Jak and Daxter - The Precursor Legacy (USA).iso", 1690000000},
    };
    const auto add = [&](int emulator, const char *folder, const char *file, std::uint64_t size)
    {
        Game game;
        tidy_game_name(file, game.title, game.region);
        const char *dot = std::strrchr(file, '.');
        for (const char *c = dot + 1; *c != 0; ++c)
            game.format += static_cast<char>(*c - ('a' <= *c && *c <= 'z' ? 32 : 0));
        game.path = std::string(folder) + "/" + file;
        game.stem = std::string(file, dot);
        game.emulator = emulator;
        game.size = size;
        library.games.push_back(game);
    };
    if (std::getenv("EMULAND_NO_GAMES") != nullptr)
        return; // the Games screen's empty state
    for (const auto &[file, size] : ps1)
        add(0, "/data/PSXS5/games", file, size);
    for (const auto &[file, size] : ps2)
        add(1, "/data/PCSX2/games", file, size);
    std::sort(library.games.begin(), library.games.end(),
              [](const Game &a, const Game &b) { return title_key(a.title) < title_key(b.title); });
    start_covers(true);
}

// The installer, simulated frame by frame (job_status is asked once a frame):
// about two seconds of download, then the short steps, then the new version.
namespace
{
JobStatus g_job;
int g_ticks = 0;
std::vector<int> g_queue;
bool g_wipe = false;

void begin_next()
{
    if (g_queue.empty())
        return;
    g_job.emulator = g_queue.front();
    g_job.uninstall = false;
    g_job.import = false;
    g_queue.erase(g_queue.begin());
    g_job.stage = Stage::downloading;
    g_job.total = library().emulators[static_cast<std::size_t>(g_job.emulator)].release.size;
    g_job.done = 0;
    g_ticks = 0;
}
} // namespace

void start_check(bool)
{
    g_job.stage = Stage::checking;
    g_job.emulator = -1;
    g_job.uninstall = false;
    g_job.import = false;
    g_ticks = 0;
}

void start_install(int index)
{
    if (index >= 0)
        g_queue.push_back(index);
    for (std::size_t i = 0; index < 0 && i < catalog().size(); ++i)
    {
        if (has_update(i))
            g_queue.push_back(static_cast<int>(i));
    }
    if (g_job.stage == Stage::idle || g_job.stage == Stage::done || g_job.stage == Stage::failed ||
        g_job.stage == Stage::cancelled)
        begin_next();
}

void start_uninstall(int index, bool wipe_data)
{
    g_job.stage = Stage::removing;
    g_job.emulator = index;
    g_job.uninstall = true;
    g_job.import = false;
    g_job.needs_close = false;
    g_wipe = wipe_data;
    g_ticks = 0;
}

void start_import(int index, int)
{
    g_job.stage = Stage::importing;
    g_job.emulator = index;
    g_job.uninstall = false;
    g_job.import = true;
    g_job.prepare = false;
    g_job.total = 4700ull << 20;
    g_job.done = 0;
    g_ticks = 0;
}

void start_prepare_usb(Lang)
{
    g_job.stage = Stage::preparing;
    g_job.emulator = -1;
    g_job.uninstall = false;
    g_job.import = false;
    g_job.prepare = true;
    g_ticks = 0;
}

int prepare_usb(Lang, std::string &where)
{
    where = "/mnt/usb0";
    return 27;
}

void cancel_job()
{
    g_queue.clear();
    g_job.stage = Stage::cancelled;
    ++g_job.finished;
}

JobStatus job_status()
{
    ++g_ticks;
    switch (g_job.stage)
    {
    case Stage::checking:
        if (g_ticks > 40)
        {
            g_job.stage = Stage::done;
            g_job.checked_hour = 21;
            g_job.checked_minute = 40;
            ++g_job.finished;
        }
        break;
    case Stage::downloading:
        g_job.done = std::min(g_job.total, g_job.total * static_cast<std::uint64_t>(g_ticks) / 120);
        if (g_ticks > 120)
        {
            g_job.stage = Stage::extracting;
            g_ticks = 0;
        }
        break;
    case Stage::verifying:
    case Stage::extracting:
        g_job.done = g_job.total * static_cast<std::uint64_t>(g_ticks) / 30;
        if (g_ticks > 30)
        {
            g_job.stage = Stage::installing;
            g_ticks = 0;
        }
        break;
    case Stage::installing:
        if (g_ticks > 15)
        {
            EmulatorState &state = library().emulators[static_cast<std::size_t>(g_job.emulator)];
            state.installed = "01.000.000";
            state.installed_tag = state.latest;
            for (int &count : state.folder_counts)
                count = std::max(count, 0);
            g_job.stage = g_queue.empty() ? Stage::done : Stage::downloading;
            ++g_job.finished;
            begin_next();
        }
        break;
    case Stage::importing:
        g_job.done = g_job.total * static_cast<std::uint64_t>(std::min(g_ticks, 150)) / 150;
        if (g_ticks > 150)
        {
            EmulatorState &state = library().emulators[static_cast<std::size_t>(g_job.emulator)];
            for (int &count : state.folder_counts)
                count = std::max(count, 0) + 1;
            g_job.imported = 3;
            g_job.stage = Stage::done;
            ++g_job.finished;
        }
        break;
    case Stage::preparing:
        if (g_ticks > 20)
        {
            g_job.prepared = 27;
            g_job.stage = Stage::done;
            ++g_job.finished;
        }
        break;
    case Stage::removing:
        if (g_ticks > 45)
        {
            EmulatorState &state = library().emulators[static_cast<std::size_t>(g_job.emulator)];
            state.installed.clear();
            state.installed_tag.clear();
            for (int &count : state.folder_counts)
                count = g_wipe ? -1 : count;
            g_job.stage = Stage::done;
            ++g_job.finished;
        }
        break;
    default:
        break;
    }
    JobStatus status = g_job;
    status.queued = static_cast<int>(g_queue.size());
    return status;
}

int launch(const EmulatorSpec &, const std::string &)
{
    return 0;
}

bool registered(const EmulatorSpec &)
{
    return true;
}

int create_missing_folders(const EmulatorSpec &emulator, EmulatorState &state)
{
    int made = 0;
    for (std::size_t f = 0; f < emulator.folders.size(); ++f)
    {
        if (state.folder_counts[f] < 0)
        {
            state.folder_counts[f] = 0;
            ++made;
        }
    }
    return made;
}

Lang system_language()
{
    const char *lang = std::getenv("EMULAND_LANG");
    return lang != nullptr && std::strcmp(lang, "en") == 0 ? Lang::en : Lang::pt;
}

void local_time(int *hour, int *minute)
{
    *hour = 21;
    *minute = 40;
}

int load_language_choice()
{
    return 0;
}

void save_language_choice(int)
{
}

void quit()
{
}

// The preview has no covers: every game shows its text poster.
namespace
{
bool g_cover_choice = true;
}

// EMULAND_COVERS=<folder> shows <folder>/<file name>.png as each game's cover.
void start_covers(bool)
{
    const char *folder = std::getenv("EMULAND_COVERS");
    if (folder == nullptr)
        return;
    for (const Game &game : library().games)
    {
        std::string bytes;
        if (has_cover(game.path) ||
            !hui::save::read_file(std::string(folder) + "/" + game.stem + ".png", &bytes, 16u << 20))
            continue;
        int width = 0;
        int height = 0;
        unsigned char *pixels =
            emuland_decode_image(reinterpret_cast<const unsigned char *>(bytes.data()),
                                 static_cast<int>(bytes.size()), &width, &height);
        if (pixels == nullptr)
            continue;
        post_cover(game.path, width, height,
                   std::vector<unsigned char>(pixels, pixels + width * height * 4));
        emuland_free_image(pixels);
    }
}

bool load_cover_choice()
{
    return g_cover_choice;
}

void save_cover_choice(bool download)
{
    g_cover_choice = download;
}

} // namespace emuland
