// Emuland - The installer, run on a PC against the real GitHub releases.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// usage: installer_test <scratch dir>   (tools/installer-test.sh builds and runs it)
//
// The same installer code as the console (src/platform/ps5/installer), with
// the PC's libcurl and every folder moved under the scratch dir. It checks:
//   1. the release check finds a ZIP for every emulator;
//   2. a fresh install of PSXS5 lands in homebrew/PPSA97510 with mode 0777,
//      a record of its tag, and the console's sce_sys copies refreshed;
//   3. an update over an older copy keeps the user's own files, moves the
//      replaced ones to backup/, and refuses to run while the title "runs";
//   6. an uninstall removes the app and its record, and the data only on wipe.

#include "emuland/model.hpp"
#include "platform/ps5/installer/installer.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <sys/stat.h>
#include <thread>

extern "C" void emuland_net_setup(CURL *easy)
{
    curl_easy_setopt(easy, CURLOPT_NOSIGNAL, 1L); // the PC's own certificate store
}

namespace emuland
{
bool installer::send_uninstall_helper()
{
    return false; // no console here
}

void local_time(int *hour, int *minute)
{
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    *hour = local.tm_hour;
    *minute = local.tm_min;
}
} // namespace emuland

namespace
{

int failures = 0;

void expect(bool ok, const char *what)
{
    std::printf("%s %s\n", ok ? "  ok  " : "  FAIL", what);
    if (!ok)
        ++failures;
}

bool exists(const std::string &path)
{
    struct stat info{};
    return stat(path.c_str(), &info) == 0;
}

unsigned mode_of(const std::string &path)
{
    struct stat info{};
    return stat(path.c_str(), &info) == 0 ? (info.st_mode & 0777) : 0;
}

std::string slurp(const std::string &path)
{
    std::string text;
    if (std::FILE *file = std::fopen(path.c_str(), "rb"))
    {
        char buffer[4096];
        std::size_t count = 0;
        while ((count = std::fread(buffer, 1, sizeof(buffer), file)) > 0)
            text.append(buffer, count);
        std::fclose(file);
    }
    return text;
}

void spit(const std::string &path, const std::string &text)
{
    if (std::FILE *file = std::fopen(path.c_str(), "wb"))
    {
        std::fwrite(text.data(), 1, text.size(), file);
        std::fclose(file);
    }
}

emuland::JobStatus wait_job(int finished_before)
{
    emuland::JobStatus status;
    const char *names[] = {"idle",       "checking",   "downloading", "verifying", "extracting",
                           "installing", "removing", "importing", "done",        "failed",    "cancelled"};
    int last = -1;
    for (;;)
    {
        status = emuland::job_status();
        if (static_cast<int>(status.stage) != last)
        {
            last = static_cast<int>(status.stage);
            std::printf("    stage %s\n", names[last]);
        }
        if (status.finished > finished_before && status.queued == 0)
            return status;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "usage: %s <scratch dir>\n", argv[0]);
        return 2;
    }
    curl_global_init(CURL_GLOBAL_DEFAULT);
    const std::string root = argv[1];
    emuland::installer::Paths paths;
    paths.homebrew = root + "/data/homebrew";
    paths.state = root + "/data/emuland";
    paths.registered = root + "/user";
    paths.sandboxes = root + "/mnt/sandbox";
    paths.data = root + "/data";
    paths.shadowmount.clear(); // no ShadowMountPlus on a PC
    for (const std::string &dir : {root + "/data", paths.homebrew, paths.registered,
                                   paths.registered + "/app", root + "/mnt", paths.sandboxes})
        mkdir(dir.c_str(), 0777);
    emuland::installer::set_paths(paths);

    std::puts("1. check");
    emuland::start_check();
    emuland::JobStatus status = wait_job(0);
    expect(status.stage == emuland::Stage::done, "check finished without error");
    for (std::size_t i = 0; i < emuland::catalog().size(); ++i)
    {
        emuland::Release release;
        const bool found = emuland::installer::cached_release(i, release);
        std::printf("    %s: %s %s %llu bytes %s\n", emuland::catalog()[i].name, release.tag.c_str(),
                    release.asset.c_str(), static_cast<unsigned long long>(release.size),
                    release.digest.c_str());
        if (emuland::source_only(emuland::catalog()[i]))
            expect(!found, "source only: nothing looked up");
        else
            expect(found && !release.url.empty(), "release with a ZIP");
    }

    const std::string psxs5 = paths.homebrew + "/PPSA97510";
    std::puts("2. fresh install of PSXS5");
    // The console's copy of sce_sys, as ShadowMountPlus leaves it.
    mkdir((paths.registered + "/app/PPSA97510").c_str(), 0777);
    mkdir((paths.registered + "/app/PPSA97510/sce_sys").c_str(), 0777);
    spit(paths.registered + "/app/PPSA97510/sce_sys/param.json", "old");
    emuland::start_install(0);
    status = wait_job(status.finished);
    if (status.stage != emuland::Stage::done)
        std::printf("    error %d: %s\n", static_cast<int>(status.error), status.detail.c_str());
    expect(status.stage == emuland::Stage::done, "install finished");
    expect(exists(psxs5 + "/eboot.bin"), "eboot.bin in place");
    expect(exists(psxs5 + "/sce_sys/param.json"), "param.json in place");
    expect(mode_of(psxs5 + "/eboot.bin") == 0777, "eboot.bin is 0777");
    expect(!emuland::installer::installed_tag("PPSA97510").empty(), "record of the tag");
    expect(slurp(paths.registered + "/app/PPSA97510/sce_sys/param.json") ==
               slurp(psxs5 + "/sce_sys/param.json"),
           "registered sce_sys refreshed");
    expect(!exists(paths.homebrew + "/.emuland-new-PPSA97510") &&
               !exists(paths.state + "/staging/PPSA97510"),
           "no staging left");

    std::puts("3. update over an older copy");
    spit(psxs5 + "/eboot.bin", "old eboot");
    spit(psxs5 + "/user-notes.txt", "mine");
    spit(paths.state + "/installed/PPSA97510.txt", "v0.0.1\n");
    mkdir((paths.sandboxes + "/PPSA97510_000").c_str(), 0777);
    emuland::start_install(0);
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::failed && status.error == emuland::JobError::running,
           "refused while the title runs");
    rmdir((paths.sandboxes + "/PPSA97510_000").c_str());
    emuland::start_install(0);
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::done, "update finished");
    expect(slurp(psxs5 + "/eboot.bin") != "old eboot", "eboot.bin replaced");
    expect(slurp(psxs5 + "/user-notes.txt") == "mine", "user's own file kept");
    expect(slurp(paths.state + "/backup/PPSA97510/eboot.bin") == "old eboot",
           "previous eboot.bin in backup");
    expect(emuland::installer::installed_tag("PPSA97510") != "v0.0.1", "record updated");

    std::puts("4. PS5SX2 and ProsperoEden (a nested folder; loose files beside the app)");
    emuland::start_install(1);
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::done, "PS5SX2 installed");
    expect(exists(paths.homebrew + "/PPSA99203/eboot.bin"), "PS5SX2 eboot.bin in place");
    emuland::start_install(2);
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::done, "ProsperoEden installed");
    expect(exists(paths.homebrew + "/PPSA99008/eboot.bin"), "ProsperoEden eboot.bin in place");
    expect(!exists(paths.homebrew + "/PPSA99008/LICENSE"), "nothing from outside the app folder");

    std::puts("5. the other catalog entries with a release (RetroArch's 790 MB left out)");
    for (std::size_t i = 3; i < emuland::catalog().size(); ++i)
    {
        const emuland::EmulatorSpec &spec = emuland::catalog()[i];
        if (emuland::source_only(spec) || std::string(spec.id) == "retroarch")
            continue;
        emuland::start_install(static_cast<int>(i));
        status = wait_job(status.finished);
        if (status.stage != emuland::Stage::done)
            std::printf("    %s: error %d %s\n", spec.name, static_cast<int>(status.error),
                        status.detail.c_str());
        expect(status.stage == emuland::Stage::done &&
                   exists(paths.homebrew + "/" + spec.title_id + "/eboot.bin"),
               spec.name);
    }
    emuland::start_install(7); // RPCS3: source only
    status = wait_job(status.finished);
    expect(status.error == emuland::JobError::no_release, "source-only entry refused");

    std::puts("6. uninstall PSXS5 keeping its data, PS5SX2 with it");
    const std::string psxs5_data = root + "/data/PSXS5";
    mkdir(psxs5_data.c_str(), 0777);
    spit(psxs5_data + "/game.bin", "mine");
    emuland::start_uninstall(0, false);
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::done && status.uninstall, "PSXS5 uninstalled");
    expect(!exists(psxs5), "PSXS5 app folder gone");
    expect(emuland::installer::installed_tag("PPSA97510").empty(), "PSXS5 record gone");
    expect(!exists(paths.state + "/backup/PPSA97510"), "PSXS5 backup gone");
    expect(slurp(psxs5_data + "/game.bin") == "mine", "PSXS5 data kept");
    const std::string pcsx2_data = root + "/data/PCSX2";
    mkdir(pcsx2_data.c_str(), 0777);
    spit(pcsx2_data + "/game.iso", "mine");
    emuland::start_uninstall(1, true);
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::done, "PS5SX2 uninstalled");
    expect(!exists(paths.homebrew + "/PPSA99203"), "PS5SX2 app folder gone");
    expect(!exists(pcsx2_data), "PS5SX2 data deleted");
    expect(exists(paths.homebrew + "/PPSA99008/eboot.bin"), "other emulators untouched");
    // RetroArch keeps its data inside the app folder: those folders stay.
    const std::string retro = paths.homebrew + "/PPSA99169";
    for (const char *dir : {"", "/sce_sys", "/content", "/system", "/savefiles", "/cores"})
        mkdir((retro + dir).c_str(), 0777);
    spit(retro + "/eboot.bin", "app");
    spit(retro + "/content/game.n64", "mine");
    spit(retro + "/system/bios.bin", "mine");
    emuland::start_uninstall(4, false);
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::done, "RetroArch uninstalled");
    expect(!exists(retro + "/eboot.bin") && !exists(retro + "/sce_sys") && !exists(retro + "/cores"),
           "RetroArch app files gone");
    expect(slurp(retro + "/content/game.n64") == "mine" && slurp(retro + "/system/bios.bin") == "mine" &&
               exists(retro + "/savefiles"),
           "RetroArch games, BIOS and saves kept");
    emuland::start_uninstall(4, true);
    status = wait_job(status.finished);
    expect(!exists(retro), "RetroArch wiped");
    // A mount record pointing at a folder that is not the app (a whole drive,
    // /data): refused, nothing deleted.
    const std::string eden_record = paths.registered + "/app/PPSA99008";
    const std::string precious = root + "/data/precious";
    mkdir(eden_record.c_str(), 0777);
    mkdir(precious.c_str(), 0777);
    spit(precious + "/save.dat", "mine");
    spit(eden_record + "/mount.lnk", precious + "\n");
    emuland::start_uninstall(2, false);
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::failed, "uninstall through a foreign mount.lnk refused");
    expect(slurp(precious + "/save.dat") == "mine", "the folder it pointed to untouched");
    expect(exists(paths.homebrew + "/PPSA99008/eboot.bin"), "the real app untouched too");
    std::remove((eden_record + "/mount.lnk").c_str());

    std::puts("7. GitHub's rate limit");
    expect(exists(paths.state + "/check.txt"), "time of the check kept");
    emuland::start_check(); // at start, an hour after: GitHub left alone
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::done && status.checked_hour >= 0,
           "recent check: skipped, its time shown");
    spit(paths.state + "/check.txt",
         std::to_string(std::time(nullptr)) + "|" + std::to_string(std::time(nullptr) + 1800) + "\n");
    emuland::installer::set_paths(paths); // read it again
    emuland::start_check(true);
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::failed &&
               status.error == emuland::JobError::rate_limited && status.retry_hour >= 0,
           "asked for under the limit: refused without calling GitHub");
    emuland::Release cached;
    expect(emuland::installer::cached_release(1, cached) && !cached.url.empty(),
           "known releases kept");

    std::puts("8. import from a USB drive");
    const std::string drive = root + "/mnt/usb0";
    const std::string pcsx2 = paths.data + "/PCSX2";
    paths.usb = {drive}; // not there yet
    paths.usb_mounts_only = false; // plain folders stand for the drives here
    emuland::installer::set_paths(paths);
    emuland::start_import(1, -1);
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::failed && status.error == emuland::JobError::no_usb,
           "no drive: refused");
    for (const std::string &dir : {drive, drive + "/Emuland", drive + "/Emuland/ps2",
                                   drive + "/Emuland/ps2/jogos", drive + "/Emuland/ps2/jogos/Okami",
                                   drive + "/Emuland/ps2/bios"})
        mkdir(dir.c_str(), 0777);
    spit(drive + "/Emuland/ps2/jogos/Okami/okami.iso", std::string(3u << 20, 'o'));
    spit(drive + "/Emuland/ps2/jogos/ico.chd", "ico");
    spit(drive + "/Emuland/ps2/jogos/._ico.chd", "macos");
    spit(drive + "/Emuland/ps2/bios/scph39001.bin", "bios");
    paths.usb = {root + "/mnt/usb1", drive};
    emuland::installer::set_paths(paths);
    emuland::start_import(1, 1); // BIOS only
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::done && status.import && status.imported == 1,
           "one folder: its file copied");
    expect(slurp(pcsx2 + "/bios/scph39001.bin") == "bios" && !exists(pcsx2 + "/games/ico.chd"),
           "only the BIOS folder");
    emuland::start_import(1, -1);
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::done && status.imported == 2, "every folder: the rest");
    expect(slurp(pcsx2 + "/games/Okami/okami.iso").size() == (3u << 20) &&
               slurp(pcsx2 + "/games/ico.chd") == "ico",
           "games copied with their folders");
    expect(!exists(pcsx2 + "/games/._ico.chd"), "macOS leftovers skipped");
    expect(mode_of(pcsx2 + "/games/ico.chd") == 0777, "copies are 0777");
    expect(exists(drive + "/Emuland/ps2/jogos/ico.chd"), "drive left as it was");
    emuland::start_import(1, -1);
    status = wait_job(status.finished);
    expect(status.stage == emuland::Stage::done && status.imported == 0, "again: nothing new");

    std::puts("9. prepare a USB drive");
    std::string where;
    const int made = emuland::prepare_usb(emuland::Lang::pt, where);
    std::printf("    %d folders on %s\n", made, where.c_str());
    expect(made > 0 && where == drive, "folders made on the drive");
    const std::string readme = slurp(drive + "/Emuland/LEIA-ME.txt");
    expect(exists(drive + "/Emuland/switch/jogos") &&
               readme.find("Emuland/switch/") != std::string::npos,
           "installed emulators' folders and the LEIA-ME");
    expect(!exists(drive + "/Emuland/ps1") && readme.find("Emuland/ps1/") == std::string::npos,
           "none for an emulator not installed (PSXS5 was uninstalled)");
    expect(slurp(drive + "/Emuland/ps2/jogos/ico.chd") == "ico", "files on the drive untouched");
    expect(emuland::prepare_usb(emuland::Lang::pt, where) == 0, "again: nothing to make");
    // The drive above kept the Emuland/ it had from before the new name; a
    // drive without either gets Manta/.
    const std::string fresh = root + "/mnt/usb6";
    mkdir(fresh.c_str(), 0777);
    paths.usb = {fresh};
    emuland::installer::set_paths(paths);
    expect(emuland::prepare_usb(emuland::Lang::pt, where) > 0 &&
               exists(fresh + "/Manta/switch/jogos") && !exists(fresh + "/Emuland") &&
               slurp(fresh + "/Manta/LEIA-ME.txt").find("Manta/switch/") != std::string::npos,
           "a new drive gets Manta/");
    paths.usb = {drive};
    paths.usb_mounts_only = true; // as on the console: a bare folder is no drive
    emuland::installer::set_paths(paths);
    expect(emuland::prepare_usb(emuland::Lang::pt, where) == -1,
           "an empty mount point (no drive plugged in) is left alone");
    paths.usb = {root + "/mnt/usb5"};
    emuland::installer::set_paths(paths);
    expect(emuland::prepare_usb(emuland::Lang::pt, where) == -1, "no drive: -1");

    std::printf("%s (%d failure%s)\n", failures == 0 ? "PASSED" : "FAILED", failures,
                failures == 1 ? "" : "s");
    std::fflush(stdout);
    // The installer's thread waits forever (as on the console, where the app
    // never returns from main): leave without running static destructors.
    std::_Exit(failures == 0 ? 0 : 1);
}
