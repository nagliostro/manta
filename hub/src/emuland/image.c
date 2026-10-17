/* Emuland - PNG and JPEG decoding (stb_image) behind a two-function C API.
 * Copyright (C) 2026 Emuland contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Only the two formats the art uses, from memory, no HDR: the console's C
 * library is small, and nothing here needs stdio or the float paths. */

#include "emuland/image.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_FAILURE_STRINGS
#include "../../third_party/stb/stb_image.h"

/* Covers come from the network and from the user's folders: a small file can
 * declare a huge picture and take the app's memory as it decodes. No cover
 * or piece of art is near this (64 MB of pixels at most). */
#define EMULAND_MAX_SIDE 4096

unsigned char *emuland_decode_image(const unsigned char *bytes, int length, int *width,
                                    int *height)
{
    int channels = 0;
    if (!stbi_info_from_memory(bytes, length, width, height, &channels) || *width <= 0 ||
        *height <= 0 || *width > EMULAND_MAX_SIDE || *height > EMULAND_MAX_SIDE)
        return 0;
    return stbi_load_from_memory(bytes, length, width, height, &channels, 4);
}

void emuland_free_image(unsigned char *pixels)
{
    stbi_image_free(pixels);
}
