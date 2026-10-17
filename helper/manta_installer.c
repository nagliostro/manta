/* Manta - One-step installer payload (manta-installer.elf).
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * The release's ZIP is built into this ELF. Sent to the console's ELF loader
 * (port 9021: etaHEN, LegacyJB, kstuff's loader, or a PC with send-elf.py /
 * NetCat GUI), it installs or updates Manta with no FTP and no network:
 *
 *   1. waits for Manta to close if it is open;
 *   2. unpacks PPSA98265/ from the ZIP into /data/emuland/installer-staging,
 *      refusing any path outside it, and opens every file to all (0777);
 *   3. swaps it into /data/homebrew/PPSA98265 (the previous files go to
 *      /data/emuland/backup/PPSA98265-installer; a failure puts them back);
 *   4. refreshes the home screen's copies of sce_sys, asks ShadowMountPlus to
 *      scan, and posts a notification.
 *
 * Built by helper/Makefile with MANTA_ZIP (the ZIP's path) and MANTA_VERSION.
 * Log: /data/emuland/manta-installer.log. */

#include "payload.h"

#include "miniz.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef MANTA_ZIP
#error "build with -DMANTA_ZIP='\"path/to/Manta.zip\"'"
#endif
#ifndef MANTA_VERSION
#define MANTA_VERSION "?"
#endif

__asm__(".section .rodata\n"
        ".balign 16\n"
        ".global manta_zip_start\n"
        "manta_zip_start:\n"
        ".incbin \"" MANTA_ZIP "\"\n"
        ".global manta_zip_end\n"
        "manta_zip_end:\n"
        ".previous\n");
extern const unsigned char manta_zip_start[];
extern const unsigned char manta_zip_end[];

#define PREFIX MANTA_TITLE "/"
#define STAGING MANTA_STATE "/installer-staging"
#define STAGED STAGING "/" MANTA_TITLE
#define BACKUP MANTA_STATE "/backup/" MANTA_TITLE "-installer"

/* Below the app's folder, nothing else: no "..", no absolute path, no
 * backslash, no control characters. */
static int safe_relative(const char *path)
{
    if (path[0] == '\0' || path[0] == '/')
        return 0;
    for (const char *c = path; *c != '\0'; ++c)
    {
        if ((unsigned char)*c < 32 || *c == '\\')
            return 0;
    }
    const char *part = path;
    while (*part != '\0')
    {
        const char *slash = strchr(part, '/');
        const size_t length = slash ? (size_t)(slash - part) : strlen(part);
        if ((length == 1 && part[0] == '.') || (length == 2 && part[0] == '.' && part[1] == '.'))
            return 0;
        if (slash == NULL)
            break;
        part = slash + 1;
    }
    return 1;
}

static int make_parents(const char *file)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s", file);
    for (char *at = path + 1; *at != '\0'; ++at)
    {
        if (*at != '/')
            continue;
        *at = '\0';
        mkdir(path, 0777);
        *at = '/';
    }
    return 0;
}

static size_t write_out(void *opaque, mz_uint64 offset, const void *data, size_t size)
{
    (void)offset;
    const int descriptor = *(int *)opaque;
    size_t done = 0;
    while (done < size)
    {
        const ssize_t put = write(descriptor, (const char *)data + done, size - done);
        if (put <= 0)
            return 0;
        done += (size_t)put;
    }
    return size;
}

static int unpack(void)
{
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    const size_t size = (size_t)(manta_zip_end - manta_zip_start);
    if (!mz_zip_reader_init_mem(&zip, manta_zip_start, size, 0))
    {
        say("the built-in ZIP is unreadable (%zu bytes)", size);
        return -1;
    }
    remove_tree(STAGING);
    mkdir(MANTA_STATE, 0777);
    mkdir(STAGING, 0777);
    mkdir(STAGED, 0777);
    int ok = 1;
    const mz_uint count = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; ok && i < count; ++i)
    {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, i, &stat))
        {
            ok = 0;
            break;
        }
        const char *name = stat.m_filename;
        if (strncmp(name, PREFIX, strlen(PREFIX)) != 0)
            continue; /* not part of the app */
        const char *relative = name + strlen(PREFIX);
        if (relative[0] == '\0')
            continue;
        char clean[512];
        snprintf(clean, sizeof(clean), "%s", relative);
        const size_t length = strlen(clean);
        if (length > 0 && clean[length - 1] == '/')
            clean[length - 1] = '\0';
        if (!safe_relative(clean))
        {
            say("refused path %s", name);
            ok = 0;
            break;
        }
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", STAGED, clean);
        make_parents(path);
        if (stat.m_is_directory)
        {
            mkdir(path, 0777);
            continue;
        }
        int descriptor = open(path, O_WRONLY | O_CREAT | O_EXCL, 0777);
        ok = descriptor >= 0 &&
             mz_zip_reader_extract_to_callback(&zip, i, write_out, &descriptor, 0) != 0;
        if (descriptor >= 0)
            ok = close(descriptor) == 0 && ok;
        if (!ok)
            say("could not unpack %s", name);
    }
    mz_zip_reader_end(&zip);
    if (!ok)
        return -1;
    char eboot[512];
    snprintf(eboot, sizeof(eboot), "%s/eboot.bin", STAGED);
    if (!holds_title(STAGED, MANTA_TITLE) || !is_file(eboot))
    {
        say("the built-in ZIP holds no Manta app");
        return -1;
    }
    return open_to_all(STAGED);
}

int main(void)
{
    log_open(MANTA_STATE "/manta-installer.log");
    say("Manta %s installer started", MANTA_VERSION);
    if (manta_running())
    {
        notify("Manta: feche o Manta para concluir a instalação");
        say("waiting for Manta to close");
        wait_for_manta(60 * 60);
    }
    notify("Manta: instalando a versão " MANTA_VERSION "...");
    if (unpack() != 0)
    {
        remove_tree(STAGING);
        notify("Manta: a instalação falhou (veja /data/emuland/manta-installer.log)");
        return 1;
    }
    mkdir("/data/homebrew", 0777);
    if (swap_into(STAGED, MANTA_HOME, BACKUP) != 0)
    {
        say("swap into %s failed", MANTA_HOME);
        remove_tree(STAGING);
        notify("Manta: a instalação falhou; nada foi mudado");
        return 1;
    }
    remove_tree(STAGING);
    chmod(MANTA_HOME, 0777);
    refresh_copies(MANTA_TITLE, MANTA_HOME);
    char reply[256];
    const int status = shadowmount_post("/scan", "{}", reply, sizeof(reply));
    say("installed in %s; ShadowMountPlus scan: HTTP %d %s", MANTA_HOME, status, reply);
    notify("Manta " MANTA_VERSION " instalado. Ele aparece na tela inicial em instantes.");
    return 0;
}
