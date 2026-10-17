/* Emuland - PNG and JPEG decoding (stb_image) behind a two-function C API.
 * Copyright (C) 2026 Emuland contributors
 * SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/* RGBA8, top row first; NULL when the bytes are not a PNG or JPEG. */
unsigned char *emuland_decode_image(const unsigned char *bytes, int length, int *width,
                                    int *height);
void emuland_free_image(unsigned char *pixels);

#ifdef __cplusplus
}
#endif
