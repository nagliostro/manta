#!/usr/bin/env bash
# Emuland - Render the hub app on the PC (no console) and bring the PNGs back.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# usage (from WSL): scripts/hub-preview.sh [output dir] [width height]
# Copies hub/ to ~/emuland/hub (the build is much faster outside OneDrive),
# runs the kit's tour through Mesa and copies the pictures to the output dir
# (default: hub/build/snapshots). EMULAND_LANG=en renders the English UI.

set -euo pipefail
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
work=~/emuland/hub
out=${1:-$here/hub/build/snapshots}
export PATH=/usr/lib/llvm-18/bin:$PATH

mkdir -p "$work" "$out"
rsync -a --delete --exclude build --exclude .deps --exclude dist --exclude .git \
    "$here/hub/" "$work/"
rm -rf "$work/build/snapshots"
bash "$work/tools/host-snapshots.sh" "$work/build/snapshots" all ${2:-} ${3:-}
rm -f "$out"/*.png
cp "$work"/build/snapshots/*.png "$out/"
ls "$out"
