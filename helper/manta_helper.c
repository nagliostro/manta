/* Manta - Helper payload: what Manta can only finish once it has closed.
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Manta sends this ELF to the loader on 127.0.0.1:9021 (installer::send_helper)
 * and leaves it requests in /data/emuland. The payload waits until Manta has
 * closed, then:
 *
 *   1. Manta's own update (self-update-request.txt: staged folder, target,
 *      tag): Manta downloaded, verified and unpacked the new version; an app
 *      can't replace its running files, so the swap happens here, as the
 *      ps5-native-app-boilerplate's self-update helper does. The previous
 *      files go to /data/emuland/backup/PPSA98265-self.
 *   2. Uninstalls (uninstall-request.txt, title IDs): ShadowMountPlus refuses
 *      its uninstall API (409 "Device busy") while any game runs, Manta
 *      included, and the system won't load libSceAppInstUtil into an app; so
 *      the icons are taken off the home screen from here.
 *
 * Log: /data/emuland/manta-helper.log. Two copies may wait at once: the first
 * to take a request file (rename) does the work. By hand, for a leftover icon:
 *
 *   echo PPSA99203 > /data/emuland/uninstall-request.txt   (over FTP)
 *   python scripts/send-elf.py <PS5-IP> helper/manta_helper.elf */

#include "payload.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define UPDATE_REQUEST MANTA_STATE "/self-update-request.txt"
#define UNINSTALL_REQUEST MANTA_STATE "/uninstall-request.txt"
#define STAGED MANTA_STATE "/self-update/" MANTA_TITLE
#define BACKUP MANTA_STATE "/backup/" MANTA_TITLE "-self"
#define WAIT_LIMIT_S (12 * 60 * 60) /* Manta left open longer: give up */
#define RETRY_LIMIT_S 90            /* ShadowMountPlus still busy after Manta closed */

/* A title ID is four capitals and five digits (PPSA99203). */
static int valid_title(const char *text)
{
    if (strlen(text) != 9)
        return 0;
    for (int i = 0; i < 4; ++i)
        if (text[i] < 'A' || text[i] > 'Z')
            return 0;
    for (int i = 4; i < 9; ++i)
        if (text[i] < '0' || text[i] > '9')
            return 0;
    return 1;
}

static void read_line(FILE *file, char *line, size_t size)
{
    line[0] = '\0';
    if (fgets(line, (int)size, file) != NULL)
        line[strcspn(line, "\r\n")] = '\0';
}

static void apply_update(void)
{
    if (rename(UPDATE_REQUEST, UPDATE_REQUEST ".taken") != 0)
        return; /* none, or another copy took it */
    char staged[512], target[512], tag[64];
    FILE *request = fopen(UPDATE_REQUEST ".taken", "r");
    if (request == NULL)
        return;
    read_line(request, staged, sizeof(staged));
    read_line(request, target, sizeof(target));
    read_line(request, tag, sizeof(tag));
    fclose(request);
    remove(UPDATE_REQUEST ".taken");
    say("update %s: %s -> %s", tag, staged, target);
    /* Only the folder Manta stages into, only into Manta's own folder. */
    if (strcmp(staged, STAGED) != 0 || target[0] != '/' || strstr(target, "..") != NULL ||
        !holds_title(staged, MANTA_TITLE) || !holds_title(target, MANTA_TITLE))
    {
        say("refused: not Manta's staged update or folder");
        notify("Manta: a atualização não pôde ser aplicada");
        remove_tree(staged);
        return;
    }
    open_to_all(staged);
    if (swap_into(staged, target, BACKUP) != 0)
    {
        say("swap failed; the previous version stays");
        notify("Manta: a atualização falhou, a versão anterior continua");
        remove_tree(staged);
        return;
    }
    remove_tree(staged);
    refresh_copies(MANTA_TITLE, target);
    char text[160];
    snprintf(text, sizeof(text), "Manta atualizado para %s. Abra de novo.", tag);
    notify(text);
    say("updated to %s", tag);
}

static void apply_uninstalls(void)
{
    if (rename(UNINSTALL_REQUEST, UNINSTALL_REQUEST ".taken") != 0)
        return;
    char titles[16][16];
    int count = 0;
    FILE *request = fopen(UNINSTALL_REQUEST ".taken", "r");
    if (request != NULL)
    {
        char line[64];
        while (count < 16 && fgets(line, sizeof(line), request) != NULL)
        {
            line[strcspn(line, "\r\n ")] = '\0';
            if (valid_title(line))
                snprintf(titles[count++], sizeof(titles[0]), "%s", line);
        }
        fclose(request);
    }
    remove(UNINSTALL_REQUEST ".taken");
    int removed = 0;
    for (int i = 0; i < count; ++i)
    {
        char body[64];
        char reply[512];
        snprintf(body, sizeof(body), "{\"title_id\":\"%s\"}", titles[i]);
        int status = 0;
        for (int tried = 0; tried < RETRY_LIMIT_S; tried += 3)
        {
            status = shadowmount_post("/games/uninstall", body, reply, sizeof(reply));
            say("%s: HTTP %d %s", titles[i], status, reply);
            if (status != 409 && status != 0)
                break;
            sleep(3); /* busy or not listening yet */
        }
        removed += status == 200;
    }
    if (removed > 0)
        notify(removed == 1 ? "Manta: emulador removido da tela inicial"
                            : "Manta: emuladores removidos da tela inicial");
    say("uninstalls done: %d of %d", removed, count);
}

int main(void)
{
    log_open(MANTA_STATE "/manta-helper.log");
    say("started, waiting for Manta to close");
    wait_for_manta(WAIT_LIMIT_S);
    say("Manta closed");
    apply_update();
    apply_uninstalls();
    return 0;
}
