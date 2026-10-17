// Emuland - Each emulator's icon and background as GPU textures.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The art is each project's own (sce_sys/icon0.png and the home-screen
// background), reduced copies in assets/art. Loaded once at start-up, before
// the first frame: nothing is read from disk while a frame is drawn.

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hui::gfx
{
class Renderer;
}

namespace emuland
{

struct Art
{
    std::vector<std::uint32_t> icons;       // one per catalog entry, 0 when missing
    std::vector<std::uint32_t> backgrounds; // 16:9, same order
    std::uint32_t mark = 0; // Manta's M, white on clear (assets/art/manta-mark.png)
    int mark_width = 0;
    int mark_height = 0;
};

// assets is the folder holding art/ (e.g. /app0/assets). Returns how many
// images loaded.
int load_art(hui::gfx::Renderer &renderer, const std::string &assets);
const Art &art();

// From the covers thread: a decoded cover (RGBA8) for take_covers() to upload.
void post_cover(const std::string &game_path, int width, int height,
                std::vector<unsigned char> pixels);
// Has this game's cover been uploaded already? (Any thread.)
bool has_cover(const std::string &game_path);

} // namespace emuland
