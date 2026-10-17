#!/usr/bin/env bash
# Builds an Emuland PS5 app inside WSL (outside OneDrive) and copies the
# result back to <app>/dist/. Usage, from Windows:
#   wsl -d Ubuntu -- bash scripts/build-ps5.sh            (build the hub app)
#   wsl -d Ubuntu -- bash scripts/build-ps5.sh deploy IP  (build and send by FTP)
#   APP=app wsl -d Ubuntu -- bash scripts/build-ps5.sh    (the console test app)
set -euo pipefail
here="$(cd "$(dirname "$0")/.." && pwd)"
app="${APP:-hub}"
work="$HOME/emuland/$app"
export PATH=/usr/lib/llvm-18/bin:$PATH
mkdir -p "$work"
rsync -a --delete --exclude .deps --exclude build --exclude dist --exclude .git --exclude .env \
    --exclude runtime/libc.prx "$here/$app/" "$work/"
# The hub ships the uninstall helper payload (helper/uninstall_helper.c) in its
# assets, built with the payload SDK of the hub's build. On a fresh machine the
# SDK is fetched first, or the app would ship without the helper.
if [ "$app" = hub ]; then
    sdk="$work/.deps/native/ps5-payload-sdk"
    [ -d "$sdk" ] || (cd "$work" && bash tools/setup-native-dependencies.sh)
    make -s -C "$here/helper" PS5_PAYLOAD_SDK="$sdk" uninstall_helper.elf
    mkdir -p "$here/hub/assets/helpers" "$work/assets/helpers"
    cp "$here/helper/uninstall_helper.elf" "$here/hub/assets/helpers/"
    cp "$here/helper/uninstall_helper.elf" "$work/assets/helpers/"
fi
cd "$work"
if [ "${1:-}" = deploy ]; then
    make deploy PS5_HOST="${2:?PS5 IP}" FTP_PORT="${3:-2121}"
else
    make
fi
rm -rf "$here/$app/dist"
mkdir -p "$here/$app/dist"
cp -r dist/* "$here/$app/dist/"
echo "Copied to $app/dist/"
