<p align="center">
  <img src="docs/brand/manta-icon-512.png" width="128" alt="Manta icon">
</p>

<h1 align="center">Manta</h1>

<p align="center">
  <b>English</b> · <a href="README.pt-BR.md">Português</a> · <a href="README.es.md">Español</a>
</p>

<p align="center">
  <b>Every PS5 emulator in one place.</b><br>
  Install, update, organize and play — no FTP, no PC, no hassle.
</p>

<p align="center">
  <a href="../../releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/nagliostro/manta?label=release"></a>
  <img alt="Platform" src="https://img.shields.io/badge/platform-PS5%20homebrew-1E3A5F">
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/badge/license-GPL--3.0-5B6878"></a>
</p>

<p align="center">
  <img src="docs/screenshots/inicio.png" width="900" alt="Manta's home screen">
</p>

---

## Why Manta exists

Every native PS5 emulator lives in a different repository, with its own way to install, its own
folders and its own release pace. Using three emulators meant downloading three ZIPs on a PC,
copying them over FTP, fixing permissions, remembering where each BIOS goes and checking GitHub
every now and then for a new version.

Manta brings all of that into **one app, on the console itself**:

- you install Manta once;
- from it, you install and update each emulator with one button;
- you copy games and BIOS from a USB drive without knowing any path;
- and you open your games from a single library, with covers, just like the PS5 home screen.

## Features

| | |
|---|---|
| **Emulator catalog** | 8 native PS5 emulators, grouped by maker, with each one's icon, art, author, license and requirements. |
| **Install and update** | Downloads the latest release straight from GitHub, checks the published **SHA-256**, extracts it and puts it in place. The previous version is kept until the new one is ready. "Update all" in one go. |
| **Uninstall** | Removes just the app ("Keep my files") or everything, including games, BIOS and saves ("Delete everything"). The icon leaves the home screen on its own when Manta closes. |
| **Game library** | Every game from every installed emulator in one cover grid, filtered by system. Discs of the same game show up once. |
| **Automatic covers** | Uses the covers you already have and, if you want, downloads the missing ones from [libretro-thumbnails](https://github.com/libretro-thumbnails/libretro-thumbnails). |
| **Play directly** | On the emulators that support it (SwanStationPS5, PS5CEMU-HAR), ✕ opens the game directly, skipping the emulator's menu. |
| **"Open in which emulator?"** | When more than one installed emulator runs the game (e.g. PSXS5 and SwanStationPS5 for PS1), Manta asks which one to use and remembers your last choice. |
| **Folders** | Shows every folder of every emulator (games, BIOS, cheats, saves…), how many files each holds and what is missing. Creates the ones that don't exist. |
| **Import from USB** | Copies from `Manta/<system>/<folder>/` on a USB drive or external disk to the right place on the console. It only copies: nothing is deleted, and what is already there is skipped. |
| **Prepare USB drive** | Creates the folder tree of the installed emulators on the drive, with a `README.txt` explaining what goes in each one. |
| **Updates itself** | A new Manta version shows up in Updates, just like the emulators. Manta downloads it, checks it and swaps it in as soon as you close the app. |
| **Online catalog** | New emulators reach Manta without a new app version: the catalog comes from this repository's [`catalog/online.json`](catalog/online.json). |
| **Bilingual** | English and Portuguese, following the console's language (or pick one in Settings). |
| **Made for the TV** | Streaming-style interface, 60 fps, 4K, smooth sounds and motion, a reduce-motion option, AA contrast. |

### Emulators in the catalog

| Emulator | System | Author | Install through Manta |
|---|---|---|---|
| [PSXS5](https://github.com/SynoPiia/PSXS5) | PlayStation | SynoPiia | ✅ |
| [SwanStationPS5](https://github.com/darkxex/SwanStationPS5) | PlayStation | AngelXex | ✅ (opens games directly) |
| [PS5SX2](https://github.com/Swordpdf/PS5SX2) | PlayStation 2 | Swordpdf | ✅ |
| [RPCS3 PS5](https://github.com/lavavex/rpcs3-PS5) | PlayStation 3 | lavavex | ⚠️ source code only; detected if installed by hand |
| [ProsperoEden](https://github.com/blackbearreloaded/ProsperoEden) | Nintendo Switch | blackbearreloaded | ✅ |
| [PS5CEMU-HAR](https://github.com/premohq/PS5CEMU-HAR) | Wii U · 3DS · DS | premohq | ✅ (opens games directly) |
| [XPSemu](https://github.com/ZiZc3/XPSemu) | Xbox | AZiZ | ✅ |
| [RetroArch PS5](https://github.com/mihawk-99/PS5_RetroArch) | N64, GameCube/Wii, PSP, SNES, GBA, Mega Drive, arcade… | Mihawk | ✅ |

Manta **does not include** any emulator, BIOS or game: it downloads the emulators from each
author's official repository. BIOS files and games are yours.

## Screenshots

<p align="center">
  <img src="docs/screenshots/jogos.png" width="440" alt="Game library">
  <img src="docs/screenshots/escolher-emulador.png" width="440" alt="Open in which emulator?">
  <img src="docs/screenshots/detalhes.png" width="440" alt="An emulator's details">
  <img src="docs/screenshots/pastas.png" width="440" alt="Folders and USB import">
  <img src="docs/screenshots/atualizacoes.png" width="440" alt="Updates">
  <img src="docs/screenshots/instalando.png" width="440" alt="Installing an emulator">
</p>

## Requirements

- **A jailbroken PS5** with a HEN that unlocks `/data` (etaHEN, LegacyJB or OnionHEN, with
  **kstuff**).
- **[ShadowMountPlus](https://github.com/drakmor/ShadowMountPlus)**, which puts the apps in
  `/data/homebrew/` (Manta and the emulators it installs) on the home screen.
- **An FTP server** on the console (etaHEN's or ftpsrv, port 2121), only to install Manta the
  first time.
- **Internet on the PS5** to install and update emulators and download covers.
- Optional: an **ELF loader** on port 9021 (included in etaHEN and LegacyJB), used to remove an
  uninstalled emulator's icon from the home screen.

Tested on: firmware 13.60, LegacyJB + kstuff + ShadowMountPlus.

## Installation

The [**Releases**](../../releases/latest) page offers two ways to install. Pick one.

### Option 1 — `.elf` installer (recommended, no FTP)

1. Download **`Manta-vX.Y.Z-installer.elf`**.
2. Send the file to the PS5's **ELF loader** (port **9021**), the same way you send any other
   payload:
   - from a PC: `python scripts/send-elf.py <PS5-IP> Manta-vX.Y.Z-installer.elf`
     or an app such as NetCat GUI;
   - or through your HEN's payload menu, if it loads `.elf` files from a USB drive.
3. The PS5 shows a notification that Manta was installed. **Manta** appears on the home screen a
   moment later.

The installer carries the app inside it, so it needs no internet. If Manta is open, it waits for
you to close it.

### Option 2 — ZIP over FTP

1. Download **`Manta-vX.Y.Z-PPSA98265.zip`** and extract it. You get a folder named
   **`PPSA98265`**.
2. With FTP on, copy **the whole folder** to `/data/homebrew/` on the console, so you end up with
   `/data/homebrew/PPSA98265/eboot.bin`, `.../sce_sys/`, `.../assets/`.
   - **FTP client** (FileZilla, WinSCP…): connect to `PS5-IP`, port `2121`, anonymous user,
     upload the folder and give it and everything inside permission `777`.
   - **Script** (Python 3): `python scripts/deploy-ps5.py <PS5-IP> --folder path/to/PPSA98265`
     uploads everything with the right permissions.
3. Wait a few seconds: ShadowMountPlus registers the app and **Manta** appears on the home screen.

> The PS5's IP is under **Settings › Network › Connection Status**. To check a download, the
> release's `SHA256SUMS.txt` lists every file's hash (`certutil -hashfile <file> SHA256` on
> Windows, `sha256sum` on Linux/macOS).

On first start, Manta asks the HEN for access to `/data` and looks for the latest emulator
versions.

### Updating Manta

**From Manta itself:** when a new version is out, it shows up in **Updates** (the "Manta" row).
Press **Update**. Manta downloads and checks the new version, then asks you to close the app. As
soon as you do, the swap happens and the PS5 shows a notification. Just open it again. This uses
the ELF loader (port 9021).

**Without the ELF loader:** send the new `.elf` installer, or copy the new ZIP over the folder.
Your emulators, games and settings are left untouched.

### Uninstalling Manta

Delete `/data/homebrew/PPSA98265` (and, if you want, `/data/emuland`, which holds the cover cache
and the settings). The installed emulators keep working.

## How to use

### Controls

| Button | Action |
|---|---|
| ✕ | Confirm · Play · Install |
| ○ | Back |
| □ | In Games: open the Folders of the game's emulator |
| L1 / R1 | Switch tabs |
| OPTIONS | Settings |

### Step by step

1. **Install an emulator** — On **Home**, pick the emulator and press ✕ to see its details. Press
   **Install**. Manta downloads, checks and installs it. At the end, **close Manta** when it asks:
   ShadowMountPlus only registers new apps while Manta is closed. Open Manta again.
2. **Prepare a USB drive** (exFAT or FAT32) — In **Folders**, press **Prepare USB drive**. Manta
   creates `Manta/<system>/jogos`, `Manta/<system>/bios` and so on, plus a `README.txt`.
3. **Copy your files on the PC** — Games and BIOS go in the right folders on the drive. The folder
   names are the same in every language (`jogos` = games). Example:

   ```
   Manta/
   ├── ps1/
   │   ├── jogos/   Crash Bandicoot (USA).chd
   │   └── bios/    scph5501.bin
   └── ps2/
       ├── jogos/   Shadow of the Colossus (USA).iso
       └── bios/    (your PS2 BIOS)
   ```

4. **Import** — Plug the drive into the PS5. In **Folders**, pick the emulator and press
   **Import all from USB** (or ✕ on a folder to import just that one).
5. **Play** — In **Games**, pick the game and press ✕. If more than one emulator runs it, Manta
   asks which one to use.
6. **Keep everything up to date** — The **Updates** tab shows what has a new version. **Update
   all** handles everything at once. Manta checks for updates when it opens (at most every 6
   hours, to respect GitHub's limit).

### Where each emulator keeps its files

| Emulator | Folder on the console | Folder on the USB drive |
|---|---|---|
| PSXS5 | `/data/PSXS5` (`games`, `bios`, `covers`) | `Manta/ps1/` |
| SwanStationPS5 | `/data/SwanStationPS5` (`games`, `bios`, `covers`) | `Manta/ps1-swan/` |
| PS5SX2 | `/data/PCSX2` (`games`, `bios`, `cheats`, `patches`, `textures`, `memcards`) | `Manta/ps2/` |
| RPCS3 | `/data/rpcs3` (`games`) | `Manta/ps3/` |
| ProsperoEden | `/data/prosperoeden` (`roms`, `keys`, `firmware`, `updates`, `mods`) | `Manta/switch/` |
| PS5CEMU-HAR | `/data/ps5cemu` | `Manta/wiiu/` |
| XPSemu | `/data/xemu` | `Manta/xbox/` |
| RetroArch | inside the app: `content`, `system`, `savefiles` | `Manta/retroarch/` |

The **Folders** screen always shows the exact path and what is missing.

## FAQ

**The emulator says "BIOS not found", but the game runs.** SwanStationPS5 ships a free BIOS
(OpenBIOS) that runs most games. An original BIOS improves compatibility: put `scph5501.bin`
(USA), `scph5502.bin` (Europe) or `scph5500.bin` (Japan) in the `bios` folder.

**I installed an emulator and it doesn't show up on the home screen.** Close Manta:
ShadowMountPlus only registers new apps while no game or app is open.

**"GitHub limit: try again after HH:MM".** Without an account, GitHub accepts 60 requests per hour
per IP. Manta keeps the last answer and tells you when you can try again.

**Does Manta delete anything without asking?** No. Importing only copies, updating keeps the
previous version until the new one is in place, and uninstalling asks whether you want to keep
your files.

**Where is the log?** In `/data/emuland/hub.log` (the previous run's in `hub.prev.log`). The `.elf`
installer writes `manta-installer.log`, and the version swap and uninstalls write
`manta-helper.log`, all in the same folder. Attach it when you open an issue.

**I want to suggest an emulator for the catalog.** Open an issue or a pull request against
[`catalog/online.json`](catalog/online.json) ([format](catalog/README.md)). Once accepted, it shows
up for everyone without a new Manta version.

## Security

- Downloads only over **HTTPS**, only from the official repositories listed in the catalog,
  checked against the **SHA-256** GitHub publishes for each file.
- The ZIP is inspected before extraction: nothing outside the app folder, no `..` paths, no
  symbolic links, size and memory limits.
- Manta only removes an app's folder after confirming it really belongs to that app.
- Images (covers) with absurd dimensions are rejected before decoding.
- The online catalog goes through strict validation: an emulator may only use a data folder of
  its own in `/data/<name>`, and a document that breaks any rule is ignored entirely.
- Manta's own version swap only accepts the folder Manta prepared and only writes to Manta's
  folder. If anything fails midway, the previous version comes back.

Found a vulnerability? Open an issue (or, if it is sensitive, contact the maintainer through their
profile before disclosing it).

## Building from source

Needs **Windows with WSL (Ubuntu)** or Linux, with `clang-18`, `lld-18`, `ninja-build`, `rsync`,
`python3`, `make` and `zip`.

```bash
# Windows
scripts\build-ps5.cmd                         # builds hub/dist/PPSA98265 and hub/dist/PPSA98265.zip
python scripts/deploy-ps5.py <PS5-IP>         # uploads the build over FTP
python scripts/deploy-ps5.py <PS5-IP> --log   # fetches /data/emuland/hub.log into logs/

# Linux / WSL
bash scripts/build-ps5.sh
```

The build also produces `hub/dist/manta-installer.elf` (the one-step installer, with the ZIP
inside).

**Publishing a version:** change `MANTA_VERSION` in `hub/src/emuland/model.hpp` and
`contentVersion` in `hub/sce_sys/param.json` (a test checks that they match), write the
`CHANGELOG.md` entry (and, if you want, `docs/RELEASE-vX.Y.Z.md`), commit and push the tag:

```bash
git tag v1.4.0 && git push origin main v1.4.0
```

GitHub Actions ([`release.yml`](.github/workflows/release.yml)) builds, runs the tests and
publishes the release with the ZIP, the `.elf` installer and `SHA256SUMS.txt`. Installed copies of
Manta see the new version in Updates.

On first run, the build downloads the PS5 payload SDK, ps5-opengl and libcurl (PacBrew).

**PC preview** (no console): `wsl -- bash scripts/hub-preview.sh <folder>` renders every screen
to PNG. **Tests:** inside `hub/`, `make test-unit` and `bash tools/installer-test.sh`.

### Layout

```
hub/        the app (C++20, OpenGL 4.6 through the ps5-homebrew-ui kit)
  src/concepts/emuland.cpp      every screen
  src/emuland/                  catalog, game library, art
  src/platform/ps5/             installer, covers, launcher, /data access
  host/                         PC build for preview and tests
helper/     payloads: manta_helper (version swap, icons of uninstalled apps) and the .elf installer
catalog/    online.json (online catalog) and emuladores.json (mirror of the built-in one)
.github/    automatic build and publishing of each version
scripts/    build, FTP upload and preview
docs/       design (source of truth for the interface), plan, visual identity
```

## Credits

- Interface built on [ps5-homebrew-ui](https://github.com/blackbearreloaded/ps5-homebrew-ui)
  and [ps5-native-app-boilerplate](https://github.com/blackbearreloaded/ps5-native-app-boilerplate),
  by BlackBearReloaded, with the [PS5 Payload SDK](https://github.com/ps5-payload-dev/sdk).
- Covers from [libretro-thumbnails](https://github.com/libretro-thumbnails/libretro-thumbnails).
- [Inter](https://github.com/rsms/inter) font; images with [stb](https://github.com/nothings/stb);
  ZIP with [miniz](https://github.com/richgel999/miniz); networking with libcurl + OpenSSL.
- Every author of the emulators in the catalog, without whom Manta would have nothing to bring
  together.

Full notices in [hub/THIRD_PARTY_NOTICES.md](hub/THIRD_PARTY_NOTICES.md).

## License

[GPL-3.0-or-later](LICENSE). PlayStation and PS5 are trademarks of Sony Interactive Entertainment;
this project is not affiliated with Sony. Only use games and BIOS files you own.
