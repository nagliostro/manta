/* Emuland - Uninstall helper payload.
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Takes uninstalled emulators off the PS5's home screen once Emuland closes.
 * Emuland can't do it itself (2026-10-16):
 *   - ShadowMountPlus refuses its uninstall API (409 "Device busy") while any
 *     game runs, and Emuland is one;
 *   - the system won't load libSceAppInstUtil into an app (0x80020063), and a
 *     payload that links or dlopens it never reaches main / never returns.
 * So this payload waits in the background until Emuland is gone, then asks
 * ShadowMountPlus (POST /api/v1/games/uninstall, its docs/api.md), which makes
 * the system call from its own process.
 *
 * Emuland deletes the files, writes the title IDs (one per line) to
 * REQUEST_PATH and sends this ELF to the loader on 127.0.0.1:9021. Run by hand
 * for a leftover icon:
 *
 *   echo PPSA99203 > /data/emuland/uninstall-request.txt   (over FTP)
 *   python scripts/send-elf.py <console IP> helper/uninstall_helper.elf
 *
 * Log: LOG_PATH. Two copies may wait at once (two uninstalls): the first to
 * read the request file takes it, and a repeated ask only gets an error. */

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define EMULAND_TITLE "PPSA98265"
#define REQUEST_PATH "/data/emuland/uninstall-request.txt"
#define LOG_PATH "/data/emuland/uninstall-helper.log"
#define SANDBOXES "/mnt/sandbox"
#define API_PORT 10101
#define WAIT_LIMIT_S (12 * 60 * 60) /* Emuland left open longer: give up */
#define RETRY_LIMIT_S 90            /* ShadowMountPlus still busy after Emuland closed */

typedef struct
{
    char unused[45];
    char message[3075];
} notify_req_t;
int sceKernelSendNotificationRequest(int32_t device, void *req, size_t size, int32_t blocking);

static FILE *g_log;

static void say(const char *format, ...)
{
    if (g_log == NULL)
        return;
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    fprintf(g_log, "[%02d:%02d:%02d] ", local.tm_hour, local.tm_min, local.tm_sec);
    va_list args;
    va_start(args, format);
    vfprintf(g_log, format, args);
    va_end(args);
    fputc('\n', g_log);
    fflush(g_log);
}

static void notify(const char *text)
{
    static notify_req_t req;
    memset(&req, 0, sizeof(req));
    strncpy(req.message, text, sizeof(req.message) - 1);
    sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
}

/* Emuland has a folder "PPSA98265_<n>" in the sandboxes folder while it runs. */
static int emuland_running(void)
{
    DIR *dir = opendir(SANDBOXES);
    if (dir == NULL)
        return 0;
    int found = 0;
    struct dirent *entry;
    while (!found && (entry = readdir(dir)) != NULL)
        found = strncmp(entry->d_name, EMULAND_TITLE "_", sizeof(EMULAND_TITLE)) == 0;
    closedir(dir);
    return found;
}

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

/* POST {"title_id":...} to ShadowMountPlus; returns the HTTP status (0: no
 * answer) and keeps the start of the body for the log. */
static int ask_uninstall(const char *title, char *body, size_t body_size)
{
    body[0] = '\0';
    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return 0;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(API_PORT);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    struct timeval timeout = {20, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) != 0)
    {
        close(sock);
        return 0;
    }
    char json[64];
    snprintf(json, sizeof(json), "{\"title_id\":\"%s\"}", title);
    char request[512];
    const int length = snprintf(request, sizeof(request),
                                "POST /api/v1/games/uninstall HTTP/1.1\r\n"
                                "Host: 127.0.0.1\r\nContent-Type: application/json\r\n"
                                "Content-Length: %zu\r\nConnection: close\r\n\r\n%s",
                                strlen(json), json);
    int status = 0;
    if (send(sock, request, (size_t)length, 0) == length)
    {
        char answer[1024];
        size_t got = 0;
        ssize_t count;
        while (got < sizeof(answer) - 1 &&
               (count = recv(sock, answer + got, sizeof(answer) - 1 - got, 0)) > 0)
            got += (size_t)count;
        answer[got] = '\0';
        sscanf(answer, "HTTP/%*s %d", &status);
        const char *start = strstr(answer, "\r\n\r\n");
        snprintf(body, body_size, "%s", start ? start + 4 : answer);
    }
    close(sock);
    return status;
}

int main(void)
{
    mkdir("/data/emuland", 0777);
    g_log = fopen(LOG_PATH, "a");
    say("started, waiting for Emuland to close");

    int waited = 0;
    while (emuland_running() && waited < WAIT_LIMIT_S)
    {
        sleep(2);
        waited += 2;
    }
    say("Emuland closed after %d s", waited);
    sleep(3); /* ShadowMountPlus releases Emuland's runtime first */

    char titles[16][16];
    int count = 0;
    FILE *request = fopen(REQUEST_PATH, "r");
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
    remove(REQUEST_PATH);

    int removed = 0;
    for (int i = 0; i < count; ++i)
    {
        char body[512];
        int status = 0;
        for (int tried = 0; tried < RETRY_LIMIT_S; tried += 3)
        {
            status = ask_uninstall(titles[i], body, sizeof(body));
            say("%s: HTTP %d %s", titles[i], status, body);
            if (status != 409 && status != 0)
                break;
            sleep(3); /* busy or not listening yet */
        }
        removed += status == 200;
    }
    if (removed > 0)
        notify(removed == 1 ? "Manta: emulador removido da tela inicial"
                            : "Manta: emuladores removidos da tela inicial");
    say("done: %d of %d", removed, count);
    return 0;
}
