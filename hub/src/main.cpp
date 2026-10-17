// Emuland - Application entry point (PS5).
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Adapted from ps5-homebrew-ui's src/main.cpp (GPL-3.0-or-later). Order:
//
//   1. ask the HEN for /data (before any thread exists, as the console test
//      app did; docs/TESTE-PS5.md),
//   2. read what is installed under /data/homebrew,
//   3. open the display, fonts, art, controller and sound,
//   4. run the shell every frame: input, update, sounds, draw, present.
//
// An elevated app has no /app0 (docs/ADOPTING.md of the kit), so the assets
// are found in the install folder when /app0 is gone.

#include "app/shell.hpp"
#include "audio/cues.hpp"
#include "audio/mixer.hpp"
#include "core/frame_stats.hpp"
#include "core/input.hpp"
#include "core/save_file.hpp"
#include "core/settings.hpp"
#include "core/version.hpp"
#include "demo/catalog.hpp"
#include "emuland/art.hpp"
#include "emuland/model.hpp"
#include "platform/ps5/installer/installer.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/audio_out.hpp"
#include "platform/ps5/display_egl.hpp"
#include "platform/ps5/pad.hpp"
#include "platform/ps5/system.hpp"
#include "platform/ps5/unlock.hpp"
#include "ui/fonts.hpp"

#include <GL/glcorearb.h>
#include <curl/curl.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <unistd.h>

extern "C" void hui_heap_stats(std::size_t *live_bytes, std::size_t *peak_bytes,
                               std::size_t *blocks, std::size_t *failures);

namespace
{

constexpr const char *kTitleId = "PPSA98265";
// Settings live under /data once the HEN has freed the app (it can no longer
// see /download0 then), in the title's own storage otherwise.
constexpr const char *kDataRoot = "/data/emuland";
constexpr const char *kSandboxRoot = "/download0/emuland";

std::string app_root()
{
    if (access("/app0/sce_sys/param.json", F_OK) == 0)
        return "/app0";
    return std::string("/data/homebrew/") + kTitleId;
}

bool open_display(hui::ps5::Display &display, int *resolution)
{
    using hui::Settings;
    const Settings::Resolution &mode = Settings::kResolutions[*resolution];
    if (display.open(mode.width, mode.height))
    {
        hui::sys::log("[EMULAND] display mode %s %dx%d", mode.label, display.width(),
                      display.height());
        return true;
    }
    if (*resolution == 0)
        return false;
    hui::sys::log("[EMULAND] display mode %s failed, using 1080p", mode.label);
    *resolution = 0;
    return display.open(Settings::kResolutions[0].width, Settings::kResolutions[0].height);
}

bool load_font(hui::gfx::Renderer &renderer, const std::string &assets, const char *name,
               hui::gfx::Font *font, hui::ui::FontRef *ref)
{
    std::string data;
    const std::string path = assets + "/fonts/" + name;
    if (!hui::save::read_file(path, &data) || !font->load(data))
    {
        hui::sys::log("[EMULAND] font %s failed: %s", name, font->error().c_str());
        return false;
    }
    ref->font = font;
    ref->texture = renderer.batch().create_font_texture(*font);
    return true;
}

void log_heap(std::uint64_t frames)
{
    std::size_t live = 0;
    std::size_t peak = 0;
    std::size_t blocks = 0;
    std::size_t failures = 0;
    hui_heap_stats(&live, &peak, &blocks, &failures);
    hui::sys::log("[EMULAND] heap frames=%llu live=%zu peak=%zu blocks=%zu failures=%zu",
                  static_cast<unsigned long long>(frames), live, peak, blocks, failures);
}

} // namespace

int main()
{
    using namespace hui;
    sys::log("[EMULAND] entry");

    // 1. /data, first thing (the HEN changes this process's credentials).
    const emuland::UnlockResult unlock = emuland::unlock_data();
    const bool data = emuland::data_listable();
    sys::log("[EMULAND] /data: %s listable=%d", emuland::describe(unlock), data ? 1 : 0);
    const std::string data_root = data ? kDataRoot : kSandboxRoot;
    save::ensure_directory(data_root);
    if (data)
    {
        // The log moves to /data, where FTP can read it after the app closed.
        // Fully buffered: unbuffered writes to /data take tens of ms each.
        std::rename("/data/emuland/hub.log", "/data/emuland/hub.prev.log");
        if (std::freopen("/data/emuland/hub.log", "w", stdout) != nullptr)
            std::setvbuf(stdout, nullptr, _IOFBF, 1 << 16);
        sys::log("[EMULAND] log moved to /data/emuland/hub.log (unlock %s)",
                 emuland::describe(unlock));
    }

    // 2. What is installed, and a look at GitHub for newer releases (on the
    // installer's own thread; the screen shows the cached answer meanwhile).
    curl_global_init(CURL_GLOBAL_DEFAULT);
    // The online catalog's last copy joins the built-in one before anything
    // reads the catalog (it is fixed at the first read).
    if (data)
        emuland::installer::load_online_catalog();
    emuland::scan(emuland::library());
    if (data)
    {
        emuland::start_check();
        emuland::start_covers(emuland::load_cover_choice());
    }
    const std::string root = app_root();
    const std::string assets = root + "/assets";
    sys::log("[EMULAND] assets %s", assets.c_str());

    // 3. Display, fonts, art.
    const Settings saved = app::Shell::load_settings(data_root);
    int resolution = ps5::Display::supports_display_modes() ? saved.resolution : 0;
    ps5::Display display;
    if (!open_display(display, &resolution))
    {
        sys::log("[EMULAND] fatal: display open failed");
        std::fflush(stdout);
        sys::park();
    }
    gfx::Renderer renderer;
    gfx::Font regular;
    gfx::Font semibold;
    gfx::Font bold;
    gfx::Font mono;
    gfx::Font pixel;
    gfx::Font hand;
    ui::Fonts fonts;
    if (!renderer.init() ||
        !load_font(renderer, assets, "inter-regular.huifont", &regular, &fonts.regular) ||
        !load_font(renderer, assets, "inter-semibold.huifont", &semibold, &fonts.semibold) ||
        !load_font(renderer, assets, "inter-bold.huifont", &bold, &fonts.display) ||
        !load_font(renderer, assets, "dejavu-sans-mono.huifont", &mono, &fonts.mono) ||
        !load_font(renderer, assets, "press-start-2p.huifont", &pixel, &fonts.pixel) ||
        !load_font(renderer, assets, "patrick-hand.huifont", &hand, &fonts.hand))
    {
        sys::log("[EMULAND] fatal: renderer or fonts failed");
        std::fflush(stdout);
        sys::park();
    }
    sys::log("[EMULAND] art images=%d", emuland::load_art(renderer, assets));
    demo::Catalog catalog; // the kit's Context wants one; no design draws it

    ps5::Pad pad;
    pad.open();
    InputTracker tracker;
    audio::Mixer mixer;
    ps5::AudioOut audio_out;
    audio_out.start(mixer);
    audio::SoundBank sounds;
    const auto bank = sounds.load(assets + "/audio/sfx");
    sys::log("[EMULAND] sounds files=%d rejected=%d", bank.files, bank.rejected);

    app::Shell shell(fonts, catalog, data_root, renderer.glass_texture());
    const std::string version = read_content_version(root + "/sce_sys/param.json");
    shell.set_version(version);
    sys::log("[EMULAND] version %s", version.empty() ? "unknown" : version.c_str());
    std::fflush(stdout);

    // 4. The frame loop.
    std::int64_t previous = sys::monotonic_us();
    std::int64_t last_frame_start = previous;
    std::uint64_t frames = 0;
    FrameStats stats;
    int slow_frames = 0; // over 20 ms: a skipped refresh at 60 Hz
    float slowest = 0.0f;
    PadSample samples[64];
    for (;;)
    {
        const std::int64_t now = sys::monotonic_us();
        float dt = frames == 0 ? 1.0f / 60.0f : static_cast<float>(now - last_frame_start) / 1e6f;
        last_frame_start = now;
        if (dt > 0.05f)
            dt = 0.05f; // a hitch must not teleport the animations
        const std::size_t count = pad.read(samples);
        const InputFrame input = tracker.update(std::span<const PadSample>(samples, count),
                                                static_cast<std::uint64_t>(now));

        shell.update(input, dt);
        if (shell.take_settings_changed())
        {
            const Settings &settings = shell.settings();
            mixer.set_bus_gain(audio::Bus::sfx, Settings::gain(settings.sfx_volume));
            mixer.set_bus_gain(audio::Bus::ui, Settings::gain(settings.ui_volume));
            InputSettings input_settings = tracker.settings();
            input_settings.swap_confirm = settings.swap_confirm;
            tracker.set_settings(input_settings);
        }
        const app::Feedback &feedback = shell.feedback();
        for (const audio::CueEvent &event : feedback.cues)
            sounds.play(mixer, event.set == audio::SoundSet::count ? shell.sound_set() : event.set,
                        event);
        if (shell.settings().haptics && feedback.rumble_strength > 0.0f)
            pad.rumble(feedback.rumble_strength, feedback.rumble_seconds);
        pad.tick(dt);
        // Navy light bar, the app's accent (docs/design.md).
        pad.set_light_bar(0x1e, 0x3a, 0x5f);

        shell.compose(renderer);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        renderer.present(0, display.width(), display.height());
        if (!display.swap())
        {
            sys::log("[EMULAND] fatal: swap failed frame=%llu error=%s",
                     static_cast<unsigned long long>(frames),
                     ps5::egl_error_name(display.last_error()));
            std::fflush(stdout);
            sys::park();
        }
        ++frames;
        const std::int64_t presented = sys::monotonic_us();
        const float frame_ms = static_cast<float>(presented - previous) / 1000.0f;
        previous = presented;
        if (frames == 1)
        {
            sys::log("[EMULAND] first frame shapes=%zu draws=%zu", renderer.last_instances(),
                     renderer.last_draw_calls());
            sys::log("[EMULAND] ready splash_hidden=%d", sys::hide_splash_screen() ? 1 : 0);
            log_heap(frames);
            std::fflush(stdout);
        }
        else
        {
            stats.add(static_cast<double>(frame_ms));
            if (frame_ms > 20.0f)
                ++slow_frames;
            if (frame_ms > slowest)
                slowest = frame_ms;
        }
        // Every 10 s. The lines stay in the buffer (a write to /data costs a
        // hitch) and reach the disk when an emulator is opened or the app quits.
        if (stats.count() == 600)
        {
            char summary[160];
            stats.format(summary, sizeof(summary));
            sys::log("[EMULAND] %s slow=%d slowest=%.1fms draws=%zu shapes=%zu", summary,
                     slow_frames, static_cast<double>(slowest), renderer.last_draw_calls(),
                     renderer.last_instances());
            stats.reset();
            slow_frames = 0;
            slowest = 0.0f;
        }
    }
}
