/* Manta - What the payloads share: files, the home screen, notifications.
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "payload.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

typedef struct
{
    char unused[45];
    char message[3075];
} notify_req_t;
int sceKernelSendNotificationRequest(int32_t device, void *req, size_t size, int32_t blocking);

static FILE *g_log;

void log_open(const char *path)
{
    mkdir(MANTA_STATE, 0777);
    g_log = fopen(path, "a");
}

void say(const char *format, ...)
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

void notify(const char *text)
{
    static notify_req_t req;
    memset(&req, 0, sizeof(req));
    strncpy(req.message, text, sizeof(req.message) - 1);
    sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
}

int manta_running(void)
{
    DIR *dir = opendir("/mnt/sandbox");
    if (dir == NULL)
        return 0;
    int found = 0;
    struct dirent *entry;
    while (!found && (entry = readdir(dir)) != NULL)
        found = strncmp(entry->d_name, MANTA_TITLE "_", sizeof(MANTA_TITLE)) == 0;
    closedir(dir);
    return found;
}

void wait_for_manta(int limit_s)
{
    int waited = 0;
    int quiet = 0;
    /* Closed for three seconds in a row: the system has let its files go,
     * and a quick reopen is not mistaken for gone. */
    while (quiet < 3 && waited < limit_s)
    {
        quiet = manta_running() ? 0 : quiet + 1;
        sleep(1);
        ++waited;
    }
}

int is_directory(const char *path)
{
    struct stat info;
    return lstat(path, &info) == 0 && S_ISDIR(info.st_mode);
}

int is_file(const char *path)
{
    struct stat info;
    return lstat(path, &info) == 0 && S_ISREG(info.st_mode);
}

static int walk(const char *path, int depth, int remove_it)
{
    struct stat info;
    if (lstat(path, &info) != 0)
        return errno == ENOENT ? 0 : -1;
    if (!S_ISDIR(info.st_mode))
    {
        if (remove_it)
            return unlink(path) == 0 ? 0 : -1;
        return chmod(path, 0777) == 0 || !S_ISREG(info.st_mode) ? 0 : -1;
    }
    if (depth > 64)
        return -1;
    if (!remove_it && chmod(path, 0777) != 0)
        return -1;
    DIR *dir = opendir(path);
    if (dir == NULL)
        return -1;
    int result = 0;
    struct dirent *entry;
    while (result == 0 && (entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        char child[1024];
        if ((size_t)snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >= sizeof(child))
            result = -1;
        else
            result = walk(child, depth + 1, remove_it);
    }
    closedir(dir);
    if (result == 0 && remove_it)
        result = rmdir(path) == 0 ? 0 : -1;
    return result;
}

int remove_tree(const char *path)
{
    return walk(path, 0, 1);
}

int open_to_all(const char *path)
{
    return walk(path, 0, 0);
}

int copy_file(const char *from, const char *to)
{
    char part[1024];
    snprintf(part, sizeof(part), "%s.part", to);
    const int in = open(from, O_RDONLY);
    if (in < 0)
        return -1;
    const int out = open(part, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (out < 0)
    {
        close(in);
        return -1;
    }
    static char buffer[256 * 1024];
    int ok = 1;
    for (;;)
    {
        const ssize_t got = read(in, buffer, sizeof(buffer));
        if (got == 0)
            break;
        if (got < 0)
        {
            if (errno == EINTR)
                continue;
            ok = 0;
            break;
        }
        for (ssize_t done = 0; ok && done < got;)
        {
            const ssize_t put = write(out, buffer + done, (size_t)(got - done));
            if (put > 0)
                done += put;
            else if (!(put < 0 && errno == EINTR))
                ok = 0;
        }
        if (!ok)
            break;
    }
    close(in);
    ok = close(out) == 0 && ok;
    ok = ok && rename(part, to) == 0;
    if (!ok)
        unlink(part);
    else
        chmod(to, 0777);
    return ok ? 0 : -1;
}

int holds_title(const char *path, const char *title_id)
{
    char file[1024];
    snprintf(file, sizeof(file), "%s/sce_sys/param.json", path);
    FILE *json = fopen(file, "rb");
    if (json == NULL)
        return 0;
    static char text[65536];
    const size_t got = fread(text, 1, sizeof(text) - 1, json);
    fclose(json);
    text[got] = '\0';
    char quoted[32];
    snprintf(quoted, sizeof(quoted), "\"%s\"", title_id);
    const char *key = strstr(text, "\"titleId\"");
    return key != NULL && strstr(key, quoted) != NULL;
}

int swap_into(const char *staged, const char *target, const char *backup)
{
    char names[256][256];
    int count = 0;
    DIR *dir = opendir(staged);
    if (dir == NULL)
        return -1;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < 256)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            snprintf(names[count++], sizeof(names[0]), "%s", entry->d_name);
    }
    closedir(dir);
    if (count == 0)
        return -1;
    remove_tree(backup);
    if (mkdir(backup, 0777) != 0)
        return -1;
    mkdir(target, 0777);
    int moved[256] = {0}; /* 1: target's entry went to backup */
    int placed = 0;
    int ok = 1;
    char from[1024], to[1024], keep[1024];
    for (; ok && placed < count; ++placed)
    {
        snprintf(to, sizeof(to), "%s/%s", target, names[placed]);
        snprintf(keep, sizeof(keep), "%s/%s", backup, names[placed]);
        snprintf(from, sizeof(from), "%s/%s", staged, names[placed]);
        struct stat info;
        if (lstat(to, &info) == 0)
        {
            if (rename(to, keep) != 0)
            {
                ok = 0;
                break;
            }
            moved[placed] = 1;
        }
        if (rename(from, to) != 0)
        {
            ok = 0;
            if (moved[placed])
                rename(keep, to);
            break;
        }
    }
    if (ok)
        return 0;
    /* Back as it was: the new entries out, the old ones in. */
    for (int i = 0; i < placed; ++i)
    {
        snprintf(to, sizeof(to), "%s/%s", target, names[i]);
        snprintf(keep, sizeof(keep), "%s/%s", backup, names[i]);
        snprintf(from, sizeof(from), "%s/%s", staged, names[i]);
        rename(to, from);
        if (moved[i])
            rename(keep, to);
    }
    return -1;
}

void refresh_copies(const char *title_id, const char *target)
{
    static const char *const names[] = {"icon0.png", "pic0.dds", "pic1.dds", "param.json"};
    char from[1024], to[1024], folder[512];
    for (int place = 0; place < 2; ++place)
    {
        if (place == 0)
            snprintf(folder, sizeof(folder), "/user/appmeta/%s", title_id);
        else
            snprintf(folder, sizeof(folder), "/user/app/%s/sce_sys", title_id);
        if (!is_directory(folder))
            continue;
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        {
            snprintf(from, sizeof(from), "%s/sce_sys/%s", target, names[i]);
            snprintf(to, sizeof(to), "%s/%s", folder, names[i]);
            if (is_file(from))
                copy_file(from, to);
        }
    }
    snprintf(to, sizeof(to), "/user/app/%s/icon0.png", title_id);
    snprintf(from, sizeof(from), "%s/sce_sys/icon0.png", target);
    if (is_file(to) && is_file(from))
        copy_file(from, to);
}

int shadowmount_post(const char *route, const char *body, char *reply, size_t reply_size)
{
    reply[0] = '\0';
    const int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return 0;
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(10101);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    struct timeval timeout = {20, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) != 0)
    {
        close(sock);
        return 0;
    }
    char request[1024];
    const int length = snprintf(request, sizeof(request),
                                "POST /api/v1%s HTTP/1.1\r\n"
                                "Host: 127.0.0.1\r\nContent-Type: application/json\r\n"
                                "Content-Length: %zu\r\nConnection: close\r\n\r\n%s",
                                route, strlen(body), body);
    int status = 0;
    if (length > 0 && (size_t)length < sizeof(request) && send(sock, request, (size_t)length, 0) == length)
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
        snprintf(reply, reply_size, "%s", start ? start + 4 : answer);
    }
    close(sock);
    return status;
}
