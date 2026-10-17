// Emuland - The art and covers without a GPU, for the unit tests.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// src/emuland/art.cpp uploads textures through OpenGL, which the tests keep
// out: here every icon, background and cover is simply missing.

#include "emuland/art.hpp"
#include "emuland/model.hpp"

namespace emuland
{

const Art &art()
{
    static const Art empty = []
    {
        Art made;
        made.icons.assign(catalog().size(), 0);
        made.backgrounds.assign(catalog().size(), 0);
        return made;
    }();
    return empty;
}

void post_cover(const std::string &, int, int, std::vector<unsigned char>)
{
}

bool has_cover(const std::string &)
{
    return false;
}

bool take_covers(int)
{
    return false;
}

Cover cover_of(const std::string &)
{
    return {};
}

} // namespace emuland
