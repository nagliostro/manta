/* Manta - What the payloads share: files, the home screen, notifications.
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Both payloads run in the ELF loader's process (port 9021), outside any
 * sandbox: manta_helper.elf (sent by Manta) and manta-installer.elf (sent by
 * the user). Plain C11 on the payload SDK's libc. */

#ifndef MANTA_PAYLOAD_H
#define MANTA_PAYLOAD_H

#include <stddef.h>

#define MANTA_TITLE "PPSA98265"
#define MANTA_HOME "/data/homebrew/" MANTA_TITLE
#define MANTA_STATE "/data/emuland"

/* Appends a time-stamped line to the log opened with log_open. */
void log_open(const char *path);
void say(const char *format, ...);

/* A notification in the PS5's corner. */
void notify(const char *text);

/* Manta has a folder "PPSA98265_<n>" in /mnt/sandbox while it runs. */
int manta_running(void);
/* Waits until Manta has been closed for a few seconds (at most limit_s). */
void wait_for_manta(int limit_s);

int is_directory(const char *path);
int is_file(const char *path);
/* Removes a tree without following links. 0 on success. */
int remove_tree(const char *path);
/* Mode 0777 on everything below path: the console starts nothing else. */
int open_to_all(const char *path);
/* Copies one file (through "<to>.part"). 0 on success. */
int copy_file(const char *from, const char *to);
/* Does path/sce_sys/param.json name title_id? */
int holds_title(const char *path, const char *title_id);

/* Puts every top-level entry of staged in target, moving target's entries of
 * the same names to backup first; on failure target is put back as it was.
 * Same drive only (renames). 0 on success. */
int swap_into(const char *staged, const char *target, const char *backup);

/* The home screen reads the icon, the backgrounds and the name from the
 * copies the system made at registration: refresh them from target. */
void refresh_copies(const char *title_id, const char *target);

/* POST body to ShadowMountPlus (127.0.0.1:10101, its docs/api.md). Returns
 * the HTTP status, 0 without an answer; the start of the reply goes to reply. */
int shadowmount_post(const char *route, const char *body, char *reply, size_t reply_size);

#endif
