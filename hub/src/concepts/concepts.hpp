// ps5-homebrew-ui - Factories of every UI design in the app.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/concept.hpp"

#include <memory>

namespace hui::concepts
{

// Emuland keeps one design: the hub (src/concepts/emuland.cpp). The kit's
// gallery designs were left out of this app.
std::unique_ptr<app::Concept> make_emuland(app::Context &context);

} // namespace hui::concepts
