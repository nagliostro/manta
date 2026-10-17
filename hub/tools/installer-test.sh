#!/usr/bin/env bash
# Emuland - Build and run the installer test on a PC (real GitHub downloads).
# SPDX-License-Identifier: GPL-3.0-or-later
#
# usage: tools/installer-test.sh [scratch dir]   (needs libcurl4-openssl-dev)

set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
scratch=${1:-/tmp/emuland-installer-test}
out="$root/build/installer-test"
cxx=$(command -v "${HOST_CXX:-clang++}")
cc=$(command -v "${HOST_CC:-clang}")
mkdir -p "$out"
rm -rf "$scratch"
mkdir -p "$scratch"

objects=()
for source in "$root"/src/third_party/miniz/*.c "$root"/src/platform/ps5/installer/self_update_sha256.c; do
    object="$out/$(basename "$source").o"
    "$cc" -std=c11 -O2 -w -I"$root/src" -c "$source" -o "$object"
    objects+=("$object")
done
"$cxx" -std=c++20 -O1 -g -Wall -Wextra -fsanitize=address,undefined -I"$root/src" \
    "$root/host/installer_test.cpp" "$root/host/platform_host.cpp" "$root/src/emuland/model.cpp" \
    "$root"/src/platform/ps5/installer/{installer,archive,files,json}.cpp \
    "${objects[@]}" -lcurl -pthread -fsanitize=address,undefined -o "$out/installer_test"
"$out/installer_test" "$scratch"
