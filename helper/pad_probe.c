/* Emuland - Controller probe payload (test 5).
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Question it answers: can a payload running in the background read the
 * DualSense while an emulator owns the screen, and can it bring Emuland back?
 * Send it once to the ELF loader (port 9021). For ten minutes it:
 *
 *   - opens the controller from its own process (plain scePadOpen, then with
 *     scePadSetProcessPrivilege, then scePadGetHandle), and logs which worked;
 *   - logs every change of the buttons it reads, the "intercepted" bit and
 *     the connected flag, and which app is in front;
 *   - when L3 + R3 are held for one second, calls sceLncUtilLaunchApp on
 *     Emuland (PPSA98265), the call PS5SX2's disc daemon uses from a payload
 *     to bring an app to the front.
 *
 * Log: /data/emuland/padprobe.log (and klog), notifications at start and end.
 * The system libraries are dlopen'd, as PS5SX2's daemon does, so the ELF
 * loader's process needs no extra NEEDED entries. */

#include <dlfcn.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <ps5/klog.h>

#define EMULAND_TITLE "PPSA98265"
#define LOG_PATH "/data/emuland/padprobe.log"
#define RUN_SECONDS 600

#define BTN_L3 0x00000002u
#define BTN_R3 0x00000004u
#define BTN_INTERCEPTED 0x80000000u

typedef struct
{
    char unused[45];
    char message[3075];
} notify_req_t;
int sceKernelSendNotificationRequest(int32_t device, void *req, size_t size, int32_t blocking);
int sceKernelUsleep(unsigned int microseconds);

typedef struct
{
    uint32_t size;
    int user_id;
    uint32_t app_opt;
    uint64_t crash_report;
    uint64_t check_flag;
} lnc_app_param_t;

/* The controller state as scePadReadState fills it (120 bytes; the fields
 * used here were confirmed by the console test app, app/src/pad.hpp). */
typedef struct
{
    uint8_t raw[0x78];
} pad_data_t;

static FILE *g_log;

static void say(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void say(const char *fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    klog_printf("[emuland-probe] %s\n", buf);
    if (g_log)
    {
        fprintf(g_log, "%ld %s\n", (long)time(NULL), buf);
        fflush(g_log);
    }
}

static void notify(const char *text)
{
    static notify_req_t req;
    memset(&req, 0, sizeof(req));
    snprintf(req.message, sizeof(req.message), "%s", text);
    sceKernelSendNotificationRequest(0, &req, sizeof(req), 0);
}

typedef int (*fn_void)(void);
typedef int (*fn_int)(int);
typedef int (*fn_open)(int, int, int, const void *);
typedef int (*fn_handle)(int, int, int);
typedef int (*fn_read)(int, void *);
typedef int (*fn_user)(int *);
typedef int (*fn_init)(void *);
typedef uint32_t (*fn_bigapp)(void);
typedef int (*fn_title)(uint32_t, char *);
typedef int (*fn_launch)(const char *, const char **, lnc_app_param_t *);

static fn_bigapp p_bigapp;
static fn_title p_title;
static fn_launch p_launch;
static fn_user p_fguser;

static const char *front_title(char *out)
{
    strcpy(out, "?");
    if (p_bigapp && p_title)
    {
        const uint32_t id = p_bigapp();
        if (id != 0 && id != 0xffffffffu && p_title(id, out) == 0)
            return out;
        strcpy(out, "none");
    }
    return out;
}

static void bring_emuland(void)
{
    if (!p_launch)
    {
        say("launch: sceLncUtilLaunchApp not found");
        return;
    }
    int user = -1;
    if (p_fguser)
        p_fguser(&user);
    lnc_app_param_t prm;
    memset(&prm, 0, sizeof(prm));
    prm.size = sizeof(prm);
    prm.user_id = user;
    const int rc = p_launch(EMULAND_TITLE, NULL, &prm);
    say("launch: sceLncUtilLaunchApp(" EMULAND_TITLE ", user 0x%08x) = 0x%08x", (unsigned)user,
        (unsigned)rc);
}

int main(void)
{
    mkdir("/data/emuland", 0777);
    g_log = fopen(LOG_PATH, "w");
    say("probe start pid=%d", (int)getpid());
    notify("Emuland: teste 5 por 10 min. Abra um emulador pelo Emuland e saia pelo menu dele.");

    void *sys = dlopen("libSceSystemService.sprx", RTLD_NOW | RTLD_GLOBAL);
    if (sys)
    {
        p_bigapp = (fn_bigapp)dlsym(sys, "sceLncUtilGetAppIdOfRunningBigApp");
        p_title = (fn_title)dlsym(sys, "sceLncUtilGetAppTitleId");
        p_launch = (fn_launch)dlsym(sys, "sceLncUtilLaunchApp");
    }
    void *usr = dlopen("libSceUserService.sprx", RTLD_NOW | RTLD_GLOBAL);
    int user = -1;
    if (usr)
    {
        fn_init init = (fn_init)dlsym(usr, "sceUserServiceInitialize");
        fn_user initial = (fn_user)dlsym(usr, "sceUserServiceGetInitialUser");
        p_fguser = (fn_user)dlsym(usr, "sceUserServiceGetForegroundUser");
        if (init)
            say("sceUserServiceInitialize = 0x%08x", (unsigned)init(NULL));
        if (p_fguser)
            say("foreground user rc=0x%08x", (unsigned)p_fguser(&user));
        if (user < 0 && initial)
            say("initial user rc=0x%08x", (unsigned)initial(&user));
    }
    say("system %p user %p launch %p user=0x%08x", sys, usr, (void *)p_launch, (unsigned)user);

    void *pad_lib = dlopen("libScePad.sprx", RTLD_NOW | RTLD_GLOBAL);
    if (!pad_lib)
    {
        say("dlopen libScePad.sprx failed: %s", dlerror());
        notify("Emuland: teste do controle falhou (libScePad)");
        return 1;
    }
    fn_void pad_init = (fn_void)dlsym(pad_lib, "scePadInit");
    fn_open pad_open = (fn_open)dlsym(pad_lib, "scePadOpen");
    fn_handle pad_get = (fn_handle)dlsym(pad_lib, "scePadGetHandle");
    fn_read pad_read = (fn_read)dlsym(pad_lib, "scePadReadState");
    fn_int pad_priv = (fn_int)dlsym(pad_lib, "scePadSetProcessPrivilege");
    say("pad: init %p open %p get %p read %p privilege %p", (void *)pad_init, (void *)pad_open,
        (void *)pad_get, (void *)pad_read, (void *)pad_priv);
    if (pad_init)
        say("scePadInit = 0x%08x", (unsigned)pad_init());

    int handle = pad_open ? pad_open(user, 0, 0, NULL) : -1;
    say("scePadOpen = 0x%08x", (unsigned)handle);
    if (handle < 0 && pad_priv)
    {
        say("scePadSetProcessPrivilege(1) = 0x%08x", (unsigned)pad_priv(1));
        handle = pad_open ? pad_open(user, 0, 0, NULL) : -1;
        say("scePadOpen after privilege = 0x%08x", (unsigned)handle);
    }
    if (handle < 0 && pad_get)
    {
        handle = pad_get(user, 0, 0);
        say("scePadGetHandle = 0x%08x", (unsigned)handle);
    }
    typedef int (*fn_open_ext)(int, int, int, const void *);
    fn_open_ext pad_open_ext = (fn_open_ext)dlsym(pad_lib, "scePadOpenExt");
    if (handle < 0 && pad_open_ext)
    {
        handle = pad_open_ext(user, 0, 0, NULL);
        say("scePadOpenExt = 0x%08x", (unsigned)handle);
    }
    if (handle < 0 && pad_open)
    {
        handle = pad_open(0xff, 0, 0, NULL); /* the system user */
        say("scePadOpen(system user) = 0x%08x", (unsigned)handle);
    }
    if (handle < 0 || !pad_read)
        say("no controller handle: watching which app is in front instead");

    uint32_t last = 0xffffffffu;
    int last_connected = -1;
    int last_rc = 1;
    int combo_ticks = 0;
    int reads_ok = 0;
    int reads_fail = 0;
    char front[32];
    char last_front[32] = "";
    const time_t end = time(NULL) + RUN_SECONDS;
    /* Without the controller: when an emulator that was in front goes away
     * (closed from its own menu), bring Emuland back. */
    char watched[32] = "";
    while (handle < 0 && time(NULL) < end)
    {
        front_title(front);
        if (strcmp(front, last_front) != 0)
        {
            say("front: %s -> %s", last_front, front);
            const int was_emulator = !strncmp(last_front, "PPSA97510", 9) ||
                                     !strncmp(last_front, "PPSA99203", 9) ||
                                     !strncmp(last_front, "PPSA99008", 9);
            if (was_emulator)
                strcpy(watched, last_front);
            if (watched[0] && strncmp(front, "PPSA", 4) != 0)
            {
                say("emulator %s left the front: bringing Emuland back", watched);
                sceKernelUsleep(1500000); /* let the shell finish closing it */
                bring_emuland();
                watched[0] = 0;
            }
            else if (!strncmp(front, EMULAND_TITLE, 9))
            {
                watched[0] = 0;
            }
            strcpy(last_front, front);
        }
        sceKernelUsleep(500000);
    }
    while (handle >= 0 && time(NULL) < end)
    {
        pad_data_t d;
        memset(&d, 0, sizeof(d));
        const int rc = pad_read(handle, &d);
        if (rc == 0)
            ++reads_ok;
        else
            ++reads_fail;
        uint32_t buttons;
        memcpy(&buttons, d.raw, sizeof(buttons));
        const int connected = d.raw[0x4c];
        front_title(front);
        if (rc != last_rc || buttons != last || connected != last_connected ||
            strcmp(front, last_front) != 0)
        {
            say("read rc=0x%08x buttons=0x%08x intercepted=%d connected=%d front=%s ok=%d fail=%d",
                (unsigned)rc, (unsigned)buttons, (buttons & BTN_INTERCEPTED) ? 1 : 0, connected,
                front, reads_ok, reads_fail);
            last = buttons;
            last_connected = connected;
            last_rc = rc;
            strcpy(last_front, front);
        }
        if ((buttons & (BTN_L3 | BTN_R3)) == (BTN_L3 | BTN_R3))
        {
            if (++combo_ticks == 20) /* 20 x 50 ms */
            {
                say("combo L3+R3 held 1 s with %s in front", front);
                bring_emuland();
            }
        }
        else
        {
            combo_ticks = 0;
        }
        sceKernelUsleep(50000);
    }
    say("probe end ok=%d fail=%d", reads_ok, reads_fail);
    notify("Emuland: teste do controle terminou");
    if (g_log)
        fclose(g_log);
    return 0;
}
