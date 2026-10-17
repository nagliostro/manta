// Emuland - The emulator catalog.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Mirrors catalog/emuladores.json. Descriptions and highlights come from each
// project's README.

#include "emuland/model.hpp"

#include <cctype>

namespace emuland
{

namespace
{

constexpr const char *kPsxs5Requirements[] = {"etaHEN + kstuff", "ShadowMountPlus"};
constexpr const char *kPs5sx2Requirements[] = {"kstuff", "ShadowMountPlus",
                                               "PS5SXHelper.elf (autoload)"};
constexpr const char *kEdenRequirements[] = {"etaHEN + kstuff", "ShadowMountPlus"};

constexpr FolderSpec kPsxs5Folders[] = {
    {"jogos", {"Jogos", "Games"}, "games", {"Uma pasta por jogo", "One folder per game"},
     FolderNeed::none, true, "cue,bin,chd,pbp,iso,img,mdf,ccd,m3u"},
    {"bios",
     {"BIOS", "BIOS"},
     "bios",
     {"scph5500/5501/5502.bin (opcional)", "scph5500/5501/5502.bin (optional)"},
     FolderNeed::optional,
     false,
     "bin",
     {"BIOS integrada", "Built-in BIOS"}},
    {"capas",
     {"Capas", "Covers"},
     "covers",
     {"Baixadas sozinhas; as suas têm prioridade", "Downloaded automatically; yours win"},
     FolderNeed::none,
     false,
     "png,jpg,jpeg"},
};

constexpr FolderSpec kPs5sx2Folders[] = {
    {"jogos", {"Jogos", "Games"}, "games", {".iso, .chd, .cso, .zso", ".iso, .chd, .cso, .zso"},
     FolderNeed::none, true, "iso,chd,cso,zso,cue,bin"},
    {"bios", {"BIOS", "BIOS"}, "bios", {"Obrigatória", "Required"}, FolderNeed::required, false,
     "bin"},
    {"cheats", {"Cheats", "Cheats"}, "cheats", {"Arquivos .pnach", ".pnach files"},
     FolderNeed::none, false, "pnach"},
    {"patches",
     {"Patches", "Patches"},
     "patches",
     {"Widescreen e 60 FPS (.pnach)", "Widescreen and 60 FPS (.pnach)"},
     FolderNeed::none,
     false,
     "pnach"},
    {"texturas", {"Texturas", "Textures"}, "textures", {"Pacotes HD", "HD packs"}},
    {"memcards", {"Memory cards", "Memory cards"}, "memcards", {"Cartões .ps2", ".ps2 cards"},
     FolderNeed::none, false, "ps2,mcd"},
};

constexpr FolderSpec kEdenFolders[] = {
    {"jogos", {"Jogos", "Games"}, "roms", {".nsp, .xci", ".nsp, .xci"}, FolderNeed::none, true,
     "nsp,xci,nsz,xcz"},
    {"keys",
     {"Chaves", "Keys"},
     "keys",
     {"prod.keys obrigatório", "prod.keys required"},
     FolderNeed::required,
     false,
     "keys"},
    {"firmware",
     {"Firmware", "Firmware"},
     "firmware",
     {"Arquivos .nca extraídos", "Extracted .nca files"},
     FolderNeed::required,
     false,
     "nca"},
    {"updates", {"Atualizações e DLC", "Updates and DLC"}, "updates", {"Arquivos .nsp", ".nsp files"},
     FolderNeed::none, false, "nsp,nsz"},
    {"mods",
     {"Mods e cheats", "Mods and cheats"},
     "mods",
     {"mods/<title ID>/<mod>/", "mods/<title ID>/<mod>/"}},
};

constexpr const char *kSwanRequirements[] = {"etaHEN + kstuff", "ShadowMountPlus"};
constexpr const char *kRetroRequirements[] = {"etaHEN + kstuff", "ShadowMountPlus 1.7beta4+"};
constexpr const char *kCemuRequirements[] = {"etaHEN, OnionHEN ou ELF loader (9021)",
                                             "ShadowMountPlus"};
constexpr const char *kXemuRequirements[] = {"etaHEN + kstuff", "ShadowMountPlus"};
constexpr const char *kRpcs3Requirements[] = {"etaHEN + kstuff", "ShadowMountPlus",
                                              "PS3UPDAT.PUP"};

constexpr FolderSpec kSwanFolders[] = {
    {"jogos", {"Jogos", "Games"}, "games", {"Uma pasta por jogo", "One folder per game"},
     FolderNeed::none, true, "cue,bin,chd,pbp,iso,img,m3u"},
    {"bios",
     {"BIOS", "BIOS"},
     "bios",
     {"SCPH1001.BIN recomendado; lê também a do PSXS5", "SCPH1001.BIN recommended; reads PSXS5's too"},
     FolderNeed::optional,
     false,
     "bin",
     {"BIOS integrada", "Built-in BIOS"}},
    {"capas", {"Capas", "Covers"}, "covers", {"Imagens .png e .jpg", ".png and .jpg images"},
     FolderNeed::none, false, "png,jpg,jpeg"},
};

// RetroArch keeps everything inside its own app folder.
constexpr FolderSpec kRetroFolders[] = {
    {"jogos", {"Jogos", "Games"}, "content", {"Um jogo ou pasta por item; também pela WebUI",
     "One game or folder per item; also through the WebUI"}, FolderNeed::none, true},
    {"bios", {"BIOS e sistema", "BIOS and system"}, "system",
     {"Arquivos de BIOS de cada núcleo (.bin)", "Each core's BIOS files (.bin)"},
     FolderNeed::none, false, "bin,rom"},
    {"saves", {"Saves", "Saves"}, "savefiles", {"Saves da memória do jogo", "Game save RAM"}},
};
constexpr const char *kRetroMerge[] = {"system"};

constexpr FolderSpec kCemuFolders[] = {
    {"jogos", {"Jogos de Wii U", "Wii U games"}, "games",
     {".wud, .wux, .wua ou pastas", ".wud, .wux, .wua or folders"}, FolderNeed::none, true},
    {"3ds", {"Jogos de 3DS", "3DS games"}, "azahar/games",
     {"Dumps descriptografados", "Decrypted dumps"}, FolderNeed::none, false, "3ds,cci,cxi,cia"},
    {"3ds-sys", {"Sistema do 3DS", "3DS system"}, "azahar/sysdata",
     {"aes_keys.txt do seu console", "Your console's aes_keys.txt"}},
    {"ds", {"Jogos de DS", "DS games"}, "melonds/games", {"Arquivos .nds", ".nds files"},
     FolderNeed::none, false, "nds"},
};

constexpr FolderSpec kXemuFolders[] = {
    {"bios",
     {"BIOS e MCPX", "BIOS and MCPX"},
     ".",
     {"mcpx_1.0.bin e Complex_4627.bin", "mcpx_1.0.bin and Complex_4627.bin"},
     FolderNeed::required,
     false,
     "bin"},
    {"jogos", {"Jogos", "Games"}, "games", {"Imagens XISO (.iso)", "XISO images (.iso)"},
     FolderNeed::none, true, "iso,xiso"},
    {"patches", {"Patches", "Patches"}, "patches", {"Patches .jmp por jogo", "Per-game .jmp patches"},
     FolderNeed::none, false, "jmp"},
};

constexpr FolderSpec kRpcs3Folders[] = {
    {"jogos", {"Jogos", "Games"}, "games", {"Pastas de jogos de PS3", "PS3 game folders"},
     FolderNeed::none, true},
};

constexpr EmulatorSpec kCatalog[] = {
    {
        "psxs5",
        "PSXS5",
        "PS1",
        {"PlayStation", "PlayStation"},
        "SynoPiia",
        "GPL-3.0-or-later",
        "SynoPiia/PSXS5",
        "PPSA97510",
        "/data/PSXS5",
        "ps1",
        false,
        {"BIOS", "BIOS"},
        {"Emulador de PlayStation 1 com Beetle PSX HW e PCSX-ReARMed, escolhidos jogo a jogo.",
         "PlayStation 1 emulator with Beetle PSX HW and PCSX-ReARMed, picked game by game."},
        {{"Até 16× a resolução do PS1, pelo Vulkan", "Up to 16× the PS1 resolution, through Vulkan"},
         {"Sem polígonos tremendo (PGXP)", "No wobbly polygons (PGXP)"},
         {"RetroAchievements, cheats automáticos e save states",
          "RetroAchievements, automatic cheats and save states"}},
        kPsxs5Requirements,
        kPsxs5Folders,
        "PSXS5-v",
        Maker::sony,
    },
    {
        "ps5sx2",
        "PS5SX2",
        "PS2",
        {"PlayStation 2", "PlayStation 2"},
        "Swordpdf",
        "GPL-3.0-or-later",
        "Swordpdf/PS5SX2",
        "PPSA99203",
        "/data/PCSX2",
        "ps2",
        false,
        {"BIOS", "BIOS"},
        {"O PCSX2 rodando nativo no PS5, com recompiladores na CPU e renderização Vulkan.",
         "PCSX2 running natively on the PS5, with recompilers on the CPU and Vulkan rendering."},
        {{"Até 6× a resolução nativa, saída 4K e FSR", "Up to 6× native resolution, 4K and FSR"},
         {"Ajustes pelo celular com QR code", "Settings from your phone via QR code"},
         {"Pacotes de texturas HD com um botão", "HD texture packs in one button"}},
        kPs5sx2Requirements,
        kPs5sx2Folders,
        "PS5SX2-",
        Maker::sony,
    },
    {
        "prosperoeden",
        "ProsperoEden",
        "Switch",
        {"Nintendo Switch", "Nintendo Switch"},
        "blackbearreloaded",
        "GPL-3.0-or-later",
        "blackbearreloaded/ProsperoEden",
        "PPSA99008",
        "/data/prosperoeden",
        "switch",
        true,
        {"chaves e firmware", "keys and firmware"},
        {"Port do Eden para o PS5, com renderização Vulkan pelo driver Mesa RADV.",
         "Eden ported to the PS5, with Vulkan rendering through the Mesa RADV driver."},
        {{"Resolução de 0,5× a 4×, com FSR", "0.5× to 4× resolution, with FSR"},
         {"Perfis e ajustes por jogo", "Profiles and per-game settings"},
         {"Multiplayer local (experimental)", "Local multiplayer (experimental)"}},
        kEdenRequirements,
        kEdenFolders,
        "ProsperoEden-v",
        Maker::nintendo,
    },
    {
        "swanstation",
        "SwanStationPS5",
        "PS1",
        {"PlayStation", "PlayStation"},
        "AngelXex",
        "GPL-3.0",
        "darkxex/SwanStationPS5",
        "PPSA98510",
        "/data/SwanStationPS5",
        "ps1-swan",
        false,
        {"BIOS", "BIOS"},
        {"Emulador de PlayStation 1 com o núcleo SwanStation (do DuckStation), desenhado pela GPU.",
         "PlayStation 1 emulator on the SwanStation core (from DuckStation), drawn on the GPU."},
        {{"Até 16× a resolução do PS1, com PGXP", "Up to 16× the PS1 resolution, with PGXP"},
         {"Lê a biblioteca e a BIOS do PSXS5", "Reads PSXS5's library and BIOS"},
         {"Roda sem BIOS, com a OpenBIOS integrada", "Runs without a BIOS on its built-in OpenBIOS"}},
        kSwanRequirements,
        kSwanFolders,
        "PPSA98510",
        Maker::sony,
        {},
        "--rom",
    },
    {
        "retroarch",
        "RetroArch",
        "Retro",
        {"N64 · GameCube/Wii · PSP · SNES e mais", "N64 · GameCube/Wii · PSP · SNES and more"},
        "Mihawk",
        "GPL-3.0-or-later",
        "mihawk-99/PS5_RetroArch",
        "PPSA99169",
        "/data/homebrew/PPSA99169",
        "retro",
        false,
        {"BIOS", "BIOS"},
        {"RetroArch nativo com 33 núcleos: Nintendo 64, GameCube/Wii, PSP, SNES, GBA, Mega Drive, "
         "arcade e outros. O pacote tem cerca de 790 MB.",
         "Native RetroArch with 33 cores: Nintendo 64, GameCube/Wii, PSP, SNES, GBA, Mega Drive, "
         "arcade and more. The package is about 790 MB."},
        {{"N64 e GameCube/Wii pela GPU (Mupen64Plus e Dolphin)",
          "N64 and GameCube/Wii on the GPU (Mupen64Plus and Dolphin)"},
         {"Biblioteca e capas pelo celular (WebUI, porta 6769)",
          "Library and covers from your phone (WebUI, port 6769)"},
         {"Até 4 controles, mapeamento por jogo", "Up to 4 controllers, per-game mappings"}},
        kRetroRequirements,
        kRetroFolders,
        "PS5_RetroArch-",
        Maker::multi,
        kRetroMerge,
    },
    {
        "ps5cemu",
        "PS5CEMU-HAR",
        "Wii U",
        {"Wii U · 3DS · DS", "Wii U · 3DS · DS"},
        "premohq",
        "GPL-3.0",
        "premohq/PS5CEMU-HAR",
        "PPSA99360",
        "/data/ps5cemu",
        "wiiu",
        false,
        {"chaves", "keys"},
        {"Wii U e 3DS no PS5, com Cemu e Azahar num só app (e DS pelo melonDS).",
         "Wii U and 3DS on the PS5, with Cemu and Azahar in one app (and DS through melonDS)."},
        {{"Cemu (Wii U) e Azahar (3DS) num único menu", "Cemu (Wii U) and Azahar (3DS) in one menu"},
         {"Mods e pacotes gráficos", "Mods and graphic packs"},
         {"Chaves de disco em /data/ps5cemu/keys.txt", "Disc keys in /data/ps5cemu/keys.txt"}},
        kCemuRequirements,
        kCemuFolders,
        "PS5CEMU-HAR-",
        Maker::nintendo,
        {},
        "--rom",
    },
    {
        "xpsemu",
        "XPSemu",
        "Xbox",
        {"Xbox", "Xbox"},
        "AZiZ",
        "GPL-2.0",
        "ZiZc3/XPSemu",
        "PPSA97358",
        "/data/xemu",
        "xbox",
        false,
        {"BIOS e MCPX", "BIOS and MCPX"},
        {"Emulador do Xbox original, baseado no xemu, com renderização Vulkan. Em alpha.",
         "Original Xbox emulator based on xemu, rendering through Vulkan. In alpha."},
        {{"Até 4 jogadores, um controle por usuário", "Up to 4 players, one controller per user"},
         {"Cria o HD do Xbox se você não tiver um", "Creates the Xbox hard disk if you have none"},
         {"Patches e capas por jogo", "Per-game patches and covers"}},
        kXemuRequirements,
        kXemuFolders,
        "PPSA97358",
        Maker::microsoft,
    },
    {
        "rpcs3",
        "RPCS3",
        "PS3",
        {"PlayStation 3", "PlayStation 3"},
        "lavavex",
        "GPL-3.0",
        "lavavex/rpcs3-PS5",
        "PPSA99300",
        "/data/rpcs3",
        "ps3",
        false,
        {"firmware do PS3", "PS3 firmware"},
        {"RPCS3 como app nativo do PS5, experimental. O autor publica só o código-fonte: é preciso "
         "compilar no PC. Instalado por você, o Emuland o reconhece e abre.",
         "RPCS3 as a native PS5 app, experimental. The author publishes source only: it has to be "
         "built on a PC. Once you install it, Emuland finds and opens it."},
        {{"Need for Speed Carbon a 60 fps num PS5 Pro", "Need for Speed Carbon at 60 fps on a PS5 Pro"},
         {"Vulkan pelo driver RADV, modo Big Picture", "Vulkan through RADV, Big Picture mode"},
         {"Precisa do firmware do PS3 e dos seus jogos", "Needs the PS3 firmware and your games"}},
        kRpcs3Requirements,
        kRpcs3Folders,
        nullptr,
        Maker::sony,
    },
};

} // namespace

std::span<const EmulatorSpec> catalog()
{
    return kCatalog;
}

void tidy_game_name(const std::string &file, std::string &title, std::string &region)
{
    std::string name = file.substr(file.find_last_of('/') + 1);
    // An extension is 2 to 4 letters or digits after the last dot ("Vol. 2" is not one).
    const std::size_t dot = name.find_last_of('.');
    if (dot != std::string::npos && name.size() - dot - 1 >= 2 && name.size() - dot - 1 <= 4)
    {
        bool plain = true;
        for (std::size_t i = dot + 1; i < name.size(); ++i)
            plain = plain && std::isalnum(static_cast<unsigned char>(name[i])) != 0;
        if (plain)
            name.resize(dot);
    }
    static const char *const kRegions[] = {"USA", "Europe", "Japan", "Brazil", "World",
                                           "Korea", "Asia", "Australia"};
    region.clear();
    std::string kept;
    for (std::size_t i = 0; i < name.size(); ++i)
    {
        const char open = name[i];
        if (open == '(' || open == '[')
        {
            const std::size_t end = name.find(open == '(' ? ')' : ']', i);
            if (end != std::string::npos)
            {
                // "(USA, Europe)": the first one names it.
                const std::string tag = name.substr(i + 1, end - i - 1);
                const std::string first = tag.substr(0, tag.find(','));
                for (const char *known : kRegions)
                {
                    if (region.empty() && open == '(' && first == known)
                        region = first;
                }
                i = end;
                continue;
            }
        }
        kept += open == '_' ? ' ' : open;
    }
    title.clear();
    for (std::size_t i = 0; i < kept.size(); ++i)
    {
        if (kept.compare(i, 3, " - ") == 0)
        {
            title += ": ";
            i += 2;
            continue;
        }
        if (kept[i] == ' ' && (title.empty() || title.back() == ' '))
            continue;
        title += kept[i];
    }
    while (!title.empty() && title.back() == ' ')
        title.pop_back();
    if (title.empty())
        title = name;
}

std::string title_key(const std::string &title)
{
    std::string key;
    for (const char c : title)
    {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isalnum(u) != 0)
            key += static_cast<char>(std::tolower(u));
        else if (u >= 0x80)
            key += c; // accented letters stay as they are
    }
    return key;
}

Library &library()
{
    static Library instance = []
    {
        Library made;
        made.emulators.resize(std::size(kCatalog));
        for (std::size_t i = 0; i < std::size(kCatalog); ++i)
            made.emulators[i].folder_counts.assign(kCatalog[i].folders.size(), -1);
        return made;
    }();
    return instance;
}

} // namespace emuland
