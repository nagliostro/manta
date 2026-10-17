/*
 * Emuland - asking the console's HEN for access to /data.
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Adapted from PSXS5's src/platform/ps5_unlock.c (GPL-3.0-or-later). A
 * sandboxed title cannot list or create folders under /data. LegacyJB and
 * OnionHEN watch each sandbox for {"PID":<pid>} in /download0/etahen_jailbreak;
 * etaHEN and LegacyJB also answer a HijackerCommand on 127.0.0.1:9028. The
 * file goes first, the TCP command when nothing answered it. A marker file in
 * /download0 makes a launch that died while asking skip the request once.
 */

#include "platform/ps5/unlock.hpp"

#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

extern "C" int sceKernelUsleep(unsigned microseconds);

namespace emuland
{
namespace
{
constexpr const char *requests[] = {"/download0/etahen_jailbreak", "/download0/onionhen_jailbreak"};
constexpr char crash_marker[] = "/download0/emuland_unlock_attempt";

bool publish(const char *path, int pid) noexcept
{
    char staged[64];
    std::snprintf(staged, sizeof(staged), "%s.tmp", path);
    unlink(staged);
    unlink(path);
    const int descriptor = open(staged, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (descriptor < 0)
        return false;
    fchmod(descriptor, 0666);
    char body[32];
    const int length = std::snprintf(body, sizeof(body), "{\"PID\":%d}\n", pid);
    const bool written =
        write(descriptor, body, static_cast<size_t>(length)) == length && fsync(descriptor) == 0;
    close(descriptor);
    if (!written || rename(staged, path) != 0)
    {
        unlink(staged);
        return false;
    }
    return true;
}

// etaHEN's legacy command server; same layout as its HijackerCommand.
struct HijackerCommand
{
    int magic; // 0xDEADBEEF
    int cmd;   // JAILBREAK_CMD = 5
    int pid;
    int ret; // -1337 until answered
    char msg1[0x500];
    char msg2[0x500];
};

bool request_over_tcp(int pid) noexcept
{
    const int connection = socket(AF_INET, SOCK_STREAM, 0);
    if (connection < 0)
        return false;
    timeval timeout{2, 0};
    setsockopt(connection, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(connection, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    sockaddr_in address{};
    address.sin_len = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = htons(9028);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bool answered = false;
    if (connect(connection, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0)
    {
        static HijackerCommand command;
        std::memset(&command, 0, sizeof(command));
        command.magic = static_cast<int>(0xDEADBEEFU);
        command.cmd = 5;
        command.pid = pid;
        command.ret = -1337;
        if (send(connection, &command, sizeof(command), 0) == static_cast<ssize_t>(sizeof(command)))
        {
            size_t received = 0;
            while (received < sizeof(command))
            {
                const ssize_t count =
                    recv(connection, reinterpret_cast<char *>(&command) + received,
                         sizeof(command) - received, 0);
                if (count <= 0)
                    break;
                received += static_cast<size_t>(count);
            }
            answered = received >= 16 && (command.ret == 0 || command.ret == -1337);
        }
    }
    close(connection);
    return answered;
}

bool wait_listable(int polls) noexcept
{
    for (int poll = 0; poll < polls; ++poll)
    {
        sceKernelUsleep(16667);
        if (data_listable())
            return true;
    }
    return false;
}
} // namespace

bool data_listable() noexcept
{
    DIR *directory = opendir("/data");
    if (directory == nullptr)
        return false;
    closedir(directory);
    return true;
}

UnlockResult unlock_data() noexcept
{
    if (data_listable())
        return UnlockResult::already;
    if (access(crash_marker, F_OK) == 0)
    {
        // The previous launch died while asking: skip this one launch, ask again next time.
        unlink(crash_marker);
        return UnlockResult::skipped_after_crash;
    }
    const int marker = open(crash_marker, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (marker >= 0)
    {
        fsync(marker);
        close(marker);
    }

    seteuid(geteuid()); // own credential before the HEN changes it
    UnlockResult result = UnlockResult::no_answer;
    const int pid = getpid();
    for (int round = 0; round < 3 && result != UnlockResult::ok; ++round)
    {
        int published = 0;
        for (const char *request : requests)
            published += publish(request, pid) ? 1 : 0;
        if (published == 0)
        {
            result = UnlockResult::cant_request;
            break;
        }
        if (wait_listable(90)) // about 1.5 s for a service to act
            result = UnlockResult::ok;
        for (const char *request : requests)
            unlink(request);
        if (result != UnlockResult::ok && round == 0 && request_over_tcp(pid) && wait_listable(60))
            result = UnlockResult::ok;
    }
    unlink(crash_marker);
    return result;
}

const char *describe(UnlockResult result) noexcept
{
    switch (result)
    {
    case UnlockResult::ok:
        return "LIBERADO";
    case UnlockResult::already:
        return "JA LIBERADO";
    case UnlockResult::no_answer:
        return "NENHUM SERVICO RESPONDEU";
    case UnlockResult::cant_request:
        return "NAO CONSEGUIU PEDIR";
    case UnlockResult::skipped_after_crash:
        return "PULADO (O PEDIDO ANTERIOR FECHOU O APP)";
    }
    return "?";
}
} // namespace emuland
