// Emuland - Each emulator's icon and background as GPU textures.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "emuland/art.hpp"

#include "core/save_file.hpp"
#include "emuland/image.h"
#include "emuland/model.hpp"
#include "gfx/renderer.hpp"
#include "platform/ps5/system.hpp"

#include <deque>
#include <mutex>
#include <unordered_map>

namespace emuland
{

namespace
{

Art g_art;
hui::gfx::Renderer *g_renderer = nullptr;

struct Decoded
{
    std::string path;
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
};
std::mutex g_cover_mutex;
std::deque<Decoded> g_decoded;                         // posted, not uploaded yet
std::unordered_map<std::string, Cover> g_covers;       // uploaded, by game path

std::uint32_t load(hui::gfx::Renderer &renderer, const std::string &path, int *out_width = nullptr,
                   int *out_height = nullptr)
{
    std::string bytes;
    if (!hui::save::read_file(path, &bytes, 16u << 20))
    {
        hui::sys::log("[EMULAND] art missing %s", path.c_str());
        return 0;
    }
    int width = 0;
    int height = 0;
    unsigned char *pixels = emuland_decode_image(reinterpret_cast<const unsigned char *>(bytes.data()),
                                                 static_cast<int>(bytes.size()), &width, &height);
    if (pixels == nullptr)
    {
        hui::sys::log("[EMULAND] art unreadable %s", path.c_str());
        return 0;
    }
    const std::uint32_t texture = renderer.batch().create_texture(width, height, pixels);
    emuland_free_image(pixels);
    if (out_width != nullptr)
        *out_width = width;
    if (out_height != nullptr)
        *out_height = height;
    return texture;
}

} // namespace

int load_art(hui::gfx::Renderer &renderer, const std::string &assets)
{
    int loaded = 0;
    g_art = {};
    g_renderer = &renderer;
    for (const EmulatorSpec &emulator : catalog())
    {
        const std::string base = assets + "/art/" + emulator.id;
        const std::uint32_t icon = load(renderer, base + "-icon.png");
        const std::uint32_t background = load(renderer, base + "-bg.jpg");
        loaded += (icon != 0 ? 1 : 0) + (background != 0 ? 1 : 0);
        g_art.icons.push_back(icon);
        g_art.backgrounds.push_back(background);
    }
    g_art.mark = load(renderer, assets + "/art/manta-mark.png", &g_art.mark_width,
                      &g_art.mark_height);
    loaded += g_art.mark != 0 ? 1 : 0;
    return loaded;
}

const Art &art()
{
    return g_art;
}

void post_cover(const std::string &game_path, int width, int height,
                std::vector<unsigned char> pixels)
{
    std::lock_guard lock(g_cover_mutex);
    g_decoded.push_back({game_path, width, height, std::move(pixels)});
}

bool has_cover(const std::string &game_path)
{
    std::lock_guard lock(g_cover_mutex);
    return g_covers.count(game_path) != 0;
}

bool take_covers(int max)
{
    if (g_renderer == nullptr)
        return false;
    bool any = false;
    for (int i = 0; i < max; ++i)
    {
        Decoded next;
        {
            std::lock_guard lock(g_cover_mutex);
            if (g_decoded.empty())
                break;
            next = std::move(g_decoded.front());
            g_decoded.pop_front();
        }
        const std::uint32_t texture =
            g_renderer->batch().create_texture(next.width, next.height, next.pixels.data());
        if (texture == 0)
            continue;
        std::lock_guard lock(g_cover_mutex);
        g_covers[next.path] = {texture, next.width, next.height};
        any = true;
    }
    return any;
}

Cover cover_of(const std::string &game_path)
{
    std::lock_guard lock(g_cover_mutex);
    const auto found = g_covers.find(game_path);
    return found != g_covers.end() ? found->second : Cover{};
}

} // namespace emuland
