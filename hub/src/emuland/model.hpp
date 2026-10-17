// Emuland - The emulator catalog, what is installed, and the platform calls.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The catalog is static data (the same entries as catalog/emuladores.json);
// the library is what the console has: installed versions and how many files
// each data folder holds. The platform functions are implemented once for the
// PS5 (src/platform/ps5/emuland_platform.cpp) and once for the PC preview
// (host/emuland_host.cpp), so the design never touches the file system.

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace emuland
{

// Manta's version, as the About screen and the release tag show it. Keep it
// in step with contentVersion in sce_sys/param.json (1.3.0 is 01.003.000).
#define MANTA_VERSION "1.4.0"
// Manta's own GitHub repository: its releases update it, and its
// catalog/online.json adds emulators without a new version.
#define MANTA_REPO "nagliostro/manta"
inline constexpr const char *kVersion = MANTA_VERSION;
inline constexpr const char *kRepo = MANTA_REPO;
inline constexpr const char *kTitleId = "PPSA98265";
inline constexpr const char *kUserAgent = "User-Agent: Manta/" MANTA_VERSION " (PS5 homebrew)";

enum class Lang
{
    pt,
    en,
};

// A string in both interface languages.
struct Text
{
    const char *pt;
    const char *en;

    const char *get(Lang lang) const
    {
        return lang == Lang::pt ? pt : en;
    }
};

enum class FolderNeed
{
    none,     // nice to have
    optional, // used when present (PS1 BIOS)
    required, // the emulator does not start without it
};

// Who made the system: one home shelf each.
enum class Maker
{
    sony,
    nintendo,
    microsoft,
    multi, // one app, many systems (RetroArch)
};

struct FolderSpec
{
    const char *key;  // short id, also the folder's name on the USB drive
    Text name;        // "Jogos", "BIOS"
    const char *path; // relative to the emulator's data folder
    Text note;        // formats or what goes inside
    FolderNeed need = FolderNeed::none;
    bool games = false; // the folder that holds the games
    // What counts, as a comma list of extensions (nullptr: every entry). Files
    // that share a name count once (CTR.bin + CTR.cue is one game), and a
    // subfolder counts once when it holds a match (one folder per game).
    const char *extensions = nullptr;
    // Shown instead of "Empty" when the emulator copes without the files
    // (PSXS5 has a built-in BIOS).
    Text when_empty{nullptr, nullptr};
};

struct EmulatorSpec
{
    const char *id;         // file names of its art: assets/art/<id>-icon.png
    const char *name;       // "PS5SX2"
    const char *short_name; // "PS2"
    Text system;            // "PlayStation 2"
    const char *author;
    const char *license;
    const char *repo;     // owner/name on GitHub
    const char *title_id; // installed under /data/homebrew/<title_id>
    const char *data;     // where the emulator keeps games, BIOS, saves
    const char *usb;      // Emuland/<usb>/ on a USB drive
    bool prerelease;      // its releases are pre-releases only
    Text needs;           // what its required folders hold ("BIOS")
    Text description;
    Text highlights[3];
    std::span<const char *const> requirements;
    std::span<const FolderSpec> folders;
    // The release file to install: <asset_prefix>....zip (catalog/emuladores.json).
    // nullptr: the project publishes source only, there is nothing to install.
    const char *asset_prefix = nullptr;
    Maker maker = Maker::sony;
    // Folders inside the app that hold the user's files (RetroArch keeps BIOS in
    // system/): an update keeps every file of theirs the new release lacks.
    std::span<const char *const> merge = {};
    // The option that opens a game straight away, followed by its path
    // ("--rom", as SwanStationPS5 and PS5CEMU-HAR read it). nullptr: the
    // emulator opens on its own menu, where the user picks the game.
    const char *rom_option = nullptr;
    // Manta's own entry: it updates like an emulator (Updates screen) but is
    // not shown on the shelves, in Games or in Folders, and is never removed.
    bool hidden = false;
};

inline bool source_only(const EmulatorSpec &emulator)
{
    return emulator.asset_prefix == nullptr;
}

// The built-in entries, then those of the online catalog, then Manta itself
// (hidden). Fixed at the first call: entries are added before it.
std::span<const EmulatorSpec> catalog();

// The online catalog (catalog/online.json, cached in /data/emuland), applied
// at start before catalog() is first asked. A new entry is appended; one that
// names a built-in id may change only where its releases come from.
// The strings are copied and live as long as the app.
struct CatalogOverride
{
    std::string id;
    std::string repo;
    std::string asset_prefix;
    std::string rom_option;
    int prerelease = -1; // -1: unchanged
};
bool add_catalog_entry(const EmulatorSpec &entry);
bool override_catalog_entry(const CatalogOverride &change);
// Keeps a copy of text for as long as the app runs (for an online entry's fields).
const char *keep_text(const std::string &text);

// The newest release Emuland would install, from GitHub.
struct Release
{
    std::string tag;    // "v2.2.0", "vk-285-171", "v1.002.000"
    std::string asset;  // file name
    std::string url;    // browser_download_url
    std::string digest; // "sha256:..." as GitHub publishes it
    std::uint64_t size = 0;
    bool prerelease = false;
};

// What the console has. count -1: the folder does not exist yet.
struct EmulatorState
{
    std::string installed;     // contentVersion, empty when not installed
    std::string installed_tag; // the release Emuland installed, empty if it did not
    bool registered = true;    // the console knows the app (it opens only then)
    std::string latest;        // newest release tag, empty while unknown
    Release release;
    std::vector<int> folder_counts;
};

// A game in an emulator's games folder.
struct Game
{
    std::string title;  // "Castlevania: Symphony of the Night", from the file name
    std::string region; // "USA", "Europe", "Japan"...; empty when the name has none
    std::string format; // "CHD", "ISO"; empty for a folder
    std::string path;   // the file or folder on the console
    std::string stem;   // its name without the extension, as cover files are named
    int emulator = -1;  // catalog index
    std::uint64_t size = 0;
};

struct Library
{
    std::vector<EmulatorState> emulators;
    std::vector<Game> games; // every installed emulator's, by title
    bool data_access = false; // /data could be read
};

// The title people read in a No-Intro or Redump name: no extension, no
// "(USA)" or "[SLUS-20946]" tags, " - " as ": ". region gets the first
// region tag.
void tidy_game_name(const std::string &file, std::string &title, std::string &region);
// The title for comparing: letters and digits only, lower case ("Ratchet &
// Clank" and libretro's "Ratchet _ Clank" are the same game).
std::string title_key(const std::string &title);

Library &library();

inline bool installed(std::size_t index)
{
    return !library().emulators[index].installed.empty();
}

// "v1.002.000" and "01.002.000" name the same version; tags of other shapes
// ("v2.2.0", "vk-285-171") can't be compared with contentVersion.
inline bool same_version(const std::string &tag, const std::string &content_version)
{
    const std::string bare = !tag.empty() && (tag[0] == 'v' || tag[0] == 'V') ? tag.substr(1) : tag;
    return bare == content_version || ("0" + bare) == content_version;
}

// An update is offered when the release differs from what Emuland installed,
// or, for a copy installed by hand, when the tag can't be matched to its
// contentVersion (installing the newest one is then the way to know).
inline bool has_update(std::size_t index)
{
    const EmulatorState &state = library().emulators[index];
    if (state.installed.empty() || state.latest.empty())
        return false;
    if (!state.installed_tag.empty())
        return state.installed_tag != state.latest;
    return !same_version(state.latest, state.installed);
}

// ---- background work (one job at a time, on its own thread) ----

enum class Stage
{
    idle,
    checking,    // asking GitHub for the newest releases
    downloading, // the release file
    verifying,   // SHA-256 against GitHub's digest
    extracting,  // the app folder out of the ZIP
    installing,  // into /data/homebrew, previous version kept
    removing,    // an uninstall: off the home screen, then the files
    importing,   // files from a USB drive into the data folders
    preparing,   // the folders on a USB drive (quiet: the button shows it)
    done,
    failed,
    cancelled,
};

enum class JobError
{
    none,
    network,    // GitHub could not be reached
    no_release, // no release with the expected file
    running,    // the emulator is open: it can't be replaced now
    download,   // the transfer failed or was cut
    digest,     // the file is not the one GitHub published
    archive,    // the ZIP has no usable app folder
    install,    // the files could not be put in place
    uninstall,  // the app's folder could not be removed
    rate_limited, // GitHub's hourly limit for calls without an account
    no_usb,       // no USB drive with Emuland/<usb>/ (or the folder) on it
    no_space,     // the console's storage filled up during an import
    copy,         // a file could not be read from the drive or written
    usb_write,    // the USB drive refused a new folder
    none_installed, // prepare: no emulator on the console to make folders for
    no_loader,      // Manta's update: the ELF loader (port 9021) did not take the helper
    other_drive,    // Manta's update: it runs from another drive than /data
};

struct JobStatus
{
    Stage stage = Stage::idle;
    int emulator = -1; // catalog index of the one being installed
    bool uninstall = false;   // the job removes it instead
    bool needs_close = false; // ShadowMountPlus finishes once Emuland closes
    bool import = false;      // the job copies from a USB drive
    int imported = 0;         // import: files copied
    bool prepare = false;     // the job made the USB drive's folders
    int prepared = 0;         // prepare: folders made
    int queued = 0;    // installs still waiting after this one
    std::uint64_t done = 0;
    std::uint64_t total = 0;
    JobError error = JobError::none;
    std::string detail;     // the technical reason, for the log and the dialog
    int finished = 0;       // counts finished jobs: a change means "scan again"
    int checked_hour = -1;  // local time of the last successful check
    int checked_minute = 0;
    int retry_hour = -1;    // rate_limited: local time GitHub answers again
    int retry_minute = 0;
    bool self_staged = false; // Manta's own update is ready: it applies when Manta closes
    int catalog_new = 0;      // emulators the online catalog adds at the next start
};

// Asks GitHub for every emulator's newest release. Not forced (the check at
// start): keeps the last answer while it is under 6 hours old.
void start_check(bool forced = false);
// Installs or updates one emulator (index), or every one with an update (-1).
void start_install(int index);
// Removes one emulator; with wipe_data also its games, BIOS, saves and settings.
void start_uninstall(int index, bool wipe_data);
// Copies Emuland/<usb>/ from a USB drive into the emulator's folders: one
// folder (its index) or each of them (-1).
void start_import(int index, int folder);
// Makes Emuland/<usb>/<folder>/ for every installed emulator on a USB drive
// or external HDD (the one with Emuland/ already, else the first), with a
// LEIA-ME.txt (README.txt) saying what goes where. Returns how many folders it
// made (0: all there), -1 with no drive, -2 when it can't write, -3 with no
// emulator installed.
// where: the drive's mount point.
int prepare_usb(Lang lang, std::string &where);
// The same on the installer's thread: the result comes in job_status().
void start_prepare_usb(Lang lang);
void cancel_job();
JobStatus job_status();

// ---- platform ----

// Fills the library from the console (or sample data on the PC).
void scan(Library &library);
// Opens the emulator, on game (a path) when it takes one (rom_option);
// returns the system's result code (0x80940010 also opens it, see
// docs/TESTE-PS5.md).
int launch(const EmulatorSpec &emulator, const std::string &game = {});
// Has the console registered the app? ShadowMountPlus does it only while no
// game runs, Emuland included (the PS5's "installed" notice comes after
// Emuland closes); before that the app won't open.
bool registered(const EmulatorSpec &emulator);
// Creates the data folders that do not exist; returns how many it made.
int create_missing_folders(const EmulatorSpec &emulator, EmulatorState &state);
// The console's language when the setting says "automatic".
Lang system_language();
// Local time, for the clock in the top bar.
void local_time(int *hour, int *minute);
// Saved choice of language: 0 automatic, 1 Portuguese, 2 English.
int load_language_choice();
void save_language_choice(int choice);
// Closes the app (the console) or does nothing (the PC preview).
void quit();

// ---- covers ----

// Finds a cover for each game without one, on its own thread: the emulator's
// covers folder (the user's, which win), Emuland's cache, then, with
// download, libretro-thumbnails (No-Intro and Redump names, as the files).
// Call again after a scan: games already shown are skipped.
void start_covers(bool download);
// On the main thread: turns up to max decoded covers into textures. True
// when one arrived.
bool take_covers(int max);
// A game's cover texture by its path; texture 0 while it has none.
struct Cover
{
    std::uint32_t texture = 0;
    int width = 0;
    int height = 0;
};
Cover cover_of(const std::string &game_path);
// The "Download covers" setting (on unless turned off).
bool load_cover_choice();
void save_cover_choice(bool download);

} // namespace emuland
