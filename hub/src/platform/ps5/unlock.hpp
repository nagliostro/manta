/*
 * Emuland - asking the console's HEN for access to /data.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

namespace emuland
{
enum class UnlockResult
{
    ok,
    already,
    no_answer,
    cant_request,
    skipped_after_crash,
};

[[nodiscard]] bool data_listable() noexcept;
// Call first thing in main(), before any thread exists.
[[nodiscard]] UnlockResult unlock_data() noexcept;
[[nodiscard]] const char *describe(UnlockResult result) noexcept;
} // namespace emuland
