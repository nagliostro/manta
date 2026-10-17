// Emuland - The hub: a streaming-style home for the console's emulators.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The layout of prototype/index.html, approved in phase 1 (docs/design.md):
//
//   - a billboard for the focused emulator over its own art, which cross-fades
//     when the focus moves, as Netflix and Prime Video do on TVs;
//   - shelves of 16:9 cards below it; going down hides the billboard's text
//     and buttons and lifts the focused shelf;
//   - tabs in the top bar (Home, Updates, Folders, Settings), L1 / R1 move
//     between them as in the PS5 store;
//   - one accent-coloured ring that glides to whatever has the focus.
//
// What the console cannot do yet (install, update, import from USB) answers
// with a dialog or a toast that says so, instead of pretending.

#include "concepts/concepts.hpp"

#include "core/tween.hpp"
#include "emuland/art.hpp"
#include "emuland/model.hpp"
#include "ui/glyphs.hpp"
#include "ui/motion.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace hui::concepts
{

namespace
{

using emuland::EmulatorSpec;
using emuland::FolderNeed;
using emuland::Lang;
using emuland::Text;
using gfx::Color;
using gfx::Rect;

// ---- palette (docs/design.md) ----
const Color kBg = Color::rgb(0x0b0e14);
const Color kSurface = Color::rgb(0x151a23);
const Color kSurface2 = Color::rgb(0x1c2330);
const Color kText = Color::rgb(0xf2f4f7);
const Color kText2 = Color::rgb(0x9aa4b2);
const Color kAccent = Color::rgb(0xa9c1da);
const Color kOk = Color::rgb(0x7fb69a);
const Color kWarn = Color::rgb(0xd9b26f);
const Color kLine = Color::rgb(0xffffff, 0.08f);
const Color kClear = Color::rgb(0x000000, 0.0f);
const Color kInk = Color::rgb(0x0b0e14);

constexpr float kMargin = 96.0f;
constexpr float kRight = 1824.0f;
constexpr float kTileW = 384.0f;
constexpr float kTileH = 216.0f;
constexpr float kTileGap = 28.0f;
constexpr float kShelfTop = 680.0f;
constexpr float kShelfPitch = 300.0f;
constexpr float kButtonH = 72.0f;
// Games: a grid of box-shaped covers (DVD cases are about 0.72 wide to 1 high).
constexpr int kCoverColumns = 7;
constexpr float kCoverW = 224.0f;
constexpr float kCoverH = 312.0f;
constexpr float kCoverGap = 26.0f;
constexpr float kCoverPitch = kCoverH + 36.0f;
constexpr float kGridTop = 468.0f;

// ---- strings ----
struct Strings
{
    Text home{"Início", "Home"};
    Text games_tab{"Jogos", "Games"};
    Text all_filter{"Todos", "All"};
    Text library_eyebrow{"BIBLIOTECA", "LIBRARY"};
    Text all_games{"Todos os jogos", "All games"};
    Text no_games_title{"Nenhum jogo ainda", "No games yet"};
    Text no_games_text{"Copie seus jogos para a pasta jogos de cada sistema no pendrive ou HD "
                       "(Manta/ps2/jogos, por exemplo) e use Importar em Pastas. Eles aparecem "
                       "aqui, prontos para jogar.",
                       "Copy your games to each system's jogos folder on the USB drive or HDD "
                       "(Manta/ps2/jogos, for example) and use Import in Folders. They show "
                       "up here, ready to play."};
    Text no_emulator_games{"Instale um emulador no Início para montar sua biblioteca.",
                           "Install an emulator on Home to build your library."};
    Text open_folders{"Abrir Pastas", "Open Folders"};
    Text opening_in{"Abrindo no", "Opening in"};
    Text pick_in_menu{"Escolha o jogo no menu do", "Pick the game in the menu of"};
    Text choose_emulator{"Abrir em qual emulador?", "Open in which emulator?"};
    Text choose_emulator_text{"roda em mais de um emulador instalado.",
                              "runs in more than one installed emulator."};
    Text updates_tab{"Atualizações", "Updates"};
    Text folders_tab{"Pastas", "Folders"};
    Text settings_tab{"Ajustes", "Settings"};
    Text row_library{"Seus emuladores", "Your emulators"};
    Text row_news{"Novidades", "What's new"};
    Text row_folders{"Pastas importantes", "Key folders"};
    Text row_sony{"Sony · PlayStation", "Sony · PlayStation"};
    Text row_nintendo{"Nintendo", "Nintendo"};
    Text row_microsoft{"Microsoft · Xbox", "Microsoft · Xbox"};
    Text row_multi{"Vários sistemas", "Many systems"};
    Text source_only{"Só código-fonte", "Source only"};
    Text none_installed{"Nenhum emulador instalado ainda. Escolha um no Início.",
                        "No emulator installed yet. Pick one on Home."};
    Text play{"Jogar", "Play"};
    Text install{"Instalar", "Install"};
    Text update{"Atualizar", "Update"};
    Text folders{"Pastas", "Folders"};
    Text details{"Detalhes", "Details"};
    Text select{"Selecionar", "Select"};
    Text back{"Voltar", "Back"};
    Text menu{"Menu", "Menu"};
    Text up_to_date{"Atualizado", "Up to date"};
    Text update_available{"Atualização disponível", "Update available"};
    Text not_installed{"Não instalado", "Not installed"};
    Text installed_chip{"Instalado", "Installed"};
    Text badge_update{"Atualização", "Update"};
    Text badge_new{"Novo", "New"};
    Text no_games{"Nenhum jogo", "No games"};
    Text more_title{"Mais emuladores", "More emulators"};
    Text more_text{"O catálogo completo está nas fileiras abaixo, por fabricante: os emuladores "
                   "nativos para PS5 que publicam releases no GitHub.",
                   "The whole catalog is on the rows below, by maker: the native PS5 emulators "
                   "that publish releases on GitHub."};
    Text more_none{"Instale com um botão, direto do release oficial de cada autor.",
                   "Install with one button, straight from each author's official release."};
    Text see_catalog{"Ver catálogo", "See the catalog"};
    Text got_it{"Entendi", "Got it"};
    Text more{"Mais", "More"};
    Text highlights{"DESTAQUES", "HIGHLIGHTS"};
    Text requirements{"REQUISITOS", "REQUIREMENTS"};
    Text requirements_note{"Verificado ao abrir o Manta", "Checked when Manta opens"};
    Text version{"VERSÃO", "VERSION"};
    Text installed_label{"Instalada", "Installed"};
    Text available_label{"Disponível", "Available"};
    Text channel{"Canal", "Channel"};
    Text stable{"Estável", "Stable"};
    Text prerelease{"Pré-release", "Pre-release"};
    Text author{"Autor", "Author"};
    Text license{"Licença", "License"};
    Text title_id{"Title ID", "Title ID"};
    Text repository{"Repositório", "Repository"};
    Text unknown{"Ainda não verificada", "Not checked yet"};
    Text usb_layout{"NO PENDRIVE OU HD EXTERNO", "ON THE USB DRIVE OR HDD"};
    Text usb_text{"Copie seus arquivos para estas pastas (exFAT ou FAT32) e use Importar.",
                  "Copy your files to these folders (exFAT or FAT32), then use Import."};
    Text import_all{"Importar tudo do pendrive", "Import all from USB"};
    Text create_missing{"Criar pastas que faltam", "Create missing folders"};
    Text import_label{"Importar do pendrive", "Import from USB"};
    Text not_created{"Não criada", "Not created"};
    Text empty{"Vazia", "Empty"};
    Text nothing_to_create{"Todas as pastas já existem", "All folders already exist"};
    Text updates_eyebrow{"ATUALIZAÇÕES", "UPDATES"};
    Text all_up_to_date{"Tudo atualizado", "All up to date"};
    Text check_now{"Procurar agora", "Check now"};
    Text last_check{"Última verificação:", "Last checked:"};
    Text settings_title{"Ajustes", "Settings"};
    Text language{"Idioma", "Language"};
    Text automatic{"Automático", "Automatic"};
    Text reduce_motion{"Reduzir movimento", "Reduce motion"};
    Text sounds{"Sons da interface", "Interface sounds"};
    Text about{"Sobre o Manta", "About Manta"};
    Text exit{"Sair do Manta", "Quit Manta"};
    Text covers_setting{"Baixar capas dos jogos", "Download game covers"};
    Text resolution{"Resolução", "Resolution"};
    Text next_start{"Vale na próxima vez que abrir o Manta",
                    "Applies the next time Manta opens"};
    Text on{"Sim", "On"};
    Text off{"Não", "Off"};
    Text about_text{"O Manta instala, atualiza e abre emuladores nativos para PS5 publicados no "
                    "GitHub. Cada emulador vem do release oficial do seu autor. Nenhuma BIOS, "
                    "chave, firmware ou jogo é baixado ou distribuído.",
                    "Manta installs, updates and opens native PS5 emulators published on "
                    "GitHub. Each emulator comes from its author's official release. No BIOS, "
                    "keys, firmware or games are downloaded or distributed."};
    Text art_credit{"Ícones e artes de cada emulador: dos próprios projetos. Capas dos jogos: "
                    "libretro-thumbnails.",
                    "Each emulator's icon and artwork come from its own project."};
    Text checking{"Procurando atualizações…", "Checking for updates…"};
    Text never_checked{"Ainda não verificado no GitHub", "Not checked on GitHub yet"};
    Text offline{"Sem conexão com o GitHub", "No connection to GitHub"};
    Text installing_title{"Instalando", "Installing"};
    Text updating_title{"Atualizando", "Updating"};
    Text step_download{"Baixando", "Downloading"};
    Text step_verify{"Conferindo SHA-256", "Checking SHA-256"};
    Text step_extract{"Extraindo", "Extracting"};
    Text step_install{"Instalando em /data/homebrew", "Installing to /data/homebrew"};
    Text step_release{"Procurando o release", "Finding the release"};
    Text queued{"na fila", "queued"};
    Text cancel{"Cancelar", "Cancel"};
    Text cancelled{"Cancelado. Nada foi alterado.", "Cancelled. Nothing was changed."};
    Text installed_toast{"instalado", "installed"};
    Text found_updates{"atualizações disponíveis", "updates available"};
    Text found_update{"atualização disponível", "update available"};
    Text all_current{"Tudo atualizado", "Everything is up to date"};
    Text error_network{"Sem conexão com o GitHub", "No connection to GitHub"};
    Text error_network_text{"Confira a internet do console e tente de novo.",
                            "Check the console's internet connection and try again."};
    Text error_release{"Nenhum arquivo para instalar", "Nothing to install"};
    Text error_release_text{"O release mais recente não tem o arquivo esperado.",
                            "The newest release does not have the expected file."};
    Text error_running{"Feche o emulador primeiro", "Close the emulator first"};
    Text error_running_text{"Ele está aberto em segundo plano. Feche pelo botão PS (Fechar app) "
                            "e tente de novo.",
                            "It is open in the background. Close it from the PS button (Close "
                            "app) and try again."};
    Text error_download{"O download falhou", "The download failed"};
    Text error_download_text{"A conexão caiu ou o servidor recusou. Nada foi alterado.",
                             "The connection dropped or the server refused. Nothing was changed."};
    Text error_digest{"Arquivo diferente do publicado", "Not the published file"};
    Text error_digest_text{"O arquivo baixado não confere com o SHA-256 do GitHub e foi apagado.",
                           "The downloaded file does not match GitHub's SHA-256 and was deleted."};
    Text error_archive{"Pacote inválido", "Unusable package"};
    Text error_archive_text{"O ZIP não tem a pasta do emulador. Nada foi alterado.",
                            "The ZIP has no emulator folder. Nothing was changed."};
    Text error_install{"Não foi possível instalar", "Could not install"};
    Text error_install_text{"Os arquivos não puderam ser colocados no lugar. A versão anterior "
                            "continua instalada.",
                            "The files could not be put in place. The previous version is still "
                            "installed."};
    Text no_data_title{"Sem acesso a /data", "No access to /data"};
    Text no_data_text{"O Manta pediu acesso ao etaHEN/LegacyJB e não teve resposta. Sem ele não "
                      "dá para ver o que está instalado nem abrir emuladores. Confira se o HEN "
                      "está ativo e abra o Manta de novo.",
                      "Manta asked etaHEN/LegacyJB for access and got no answer. Without it, it "
                      "cannot see what is installed or open emulators. Check that the HEN is "
                      "running and open Manta again."};
    Text launch_failed{"O console não abriu o emulador", "The console did not open the emulator"};
    Text registering_chip{"Registro pendente", "Registration pending"};
    Text register_title{"Feche o Manta para concluir", "Close Manta to finish"};
    Text register_text{"O PS5 só registra um app novo com o Manta fechado. Feche agora: em alguns "
                       "segundos aparece o aviso de instalado, e o emulador fica na tela inicial "
                       "e no Manta.",
                       "The PS5 registers a new app only while Manta is closed. Close it now: "
                       "in a few seconds the installed notice shows, and the emulator is on the "
                       "home screen and in Manta."};
    Text close_now{"Fechar o Manta", "Close Manta"};
    Text uninstall{"Desinstalar", "Uninstall"};
    Text uninstall_title{"Desinstalar", "Uninstall"};
    Text uninstall_text{"O emulador sai do PS5. E os seus jogos, BIOS, saves e configurações? "
                        "Mantidos, eles voltam a funcionar se você instalar de novo. Apagados, "
                        "não há como recuperar.",
                        "The emulator leaves the PS5. What about your games, BIOS, saves and "
                        "settings? Kept, they work again if you reinstall. Deleted, they can't "
                        "be recovered."};
    Text keep_files{"Manter meus arquivos", "Keep my files"};
    Text delete_all{"Apagar tudo", "Delete everything"};
    Text uninstalling_title{"Desinstalando", "Uninstalling"};
    Text uninstalling_text{"Tirando o emulador da tela inicial e apagando a pasta do app.",
                           "Taking the emulator off the home screen and deleting the app folder."};
    Text uninstalled_toast{"desinstalado", "uninstalled"};
    Text unregister_title{"Feche o Manta para concluir", "Close Manta to finish"};
    Text unregister_text{"Os arquivos já foram apagados. O PS5 só tira o ícone da tela inicial com "
                         "o Manta fechado: ao fechar, ele sai em alguns segundos, com um aviso.",
                         "The files are already deleted. The PS5 takes the icon off the home "
                         "screen only while Manta is closed: once it closes, the icon goes in a "
                         "few seconds, with a notice."};
    Text error_uninstall{"Não foi possível desinstalar", "Could not uninstall"};
    Text error_uninstall_text{"A pasta do emulador não pôde ser apagada por inteiro.",
                              "The emulator's folder could not be fully deleted."};
    Text later{"Depois", "Later"};
    Text prepare_usb{"Preparar pendrive", "Prepare USB drive"};
    Text preparing_usb{"Preparando", "Preparing"};
    Text usb_ready{"pastas criadas no pendrive", "folders made on the USB drive"};
    Text usb_already{"O pendrive já tem todas as pastas", "The USB drive has every folder"};
    Text error_usb_write{"Não foi possível gravar no pendrive", "Could not write to the USB drive"};
    Text error_usb_write_text{"Confira se ele está em exFAT ou FAT32 e não está protegido contra "
                              "gravação.",
                              "Check that it is exFAT or FAT32 and not write-protected."};
    Text importing_title{"Importando para", "Importing to"};
    Text importing_text{"Os arquivos continuam no pendrive. O que já está no console fica como está.",
                        "The files stay on the USB drive. What is already on the console stays."};
    Text imported_one{"arquivo importado", "file imported"};
    Text imported_many{"arquivos importados", "files imported"};
    Text nothing_new{"Nada novo no pendrive", "Nothing new on the USB drive"};
    Text error_no_usb{"Nenhum pendrive ou HD encontrado", "No USB drive or HDD found"};
    Text error_no_usb_text{"Conecte um pendrive ou HD externo em exFAT ou FAT32 (o PS5 não lê "
                           "NTFS), com as pastas no layout mostrado em Pastas, e tente de novo.",
                           "Connect a USB drive or external HDD in exFAT or FAT32 (the PS5 "
                           "reads no NTFS), with the folders laid out as shown in Folders, and "
                           "try again."};
    Text prepare_needs_install{"Instale um emulador antes de preparar o pendrive",
                        "Install an emulator before preparing the drive"};
    Text error_no_space{"Sem espaço no console", "No space on the console"};
    Text error_no_space_text{"Libere espaço no armazenamento do PS5. Os arquivos já copiados "
                             "ficaram no console.",
                             "Free up space on the PS5's storage. The files already copied stay "
                             "on the console."};
    Text error_copy{"Não foi possível copiar", "Could not copy"};
    Text error_copy_text{"Um arquivo não pôde ser lido do pendrive ou gravado no console. Os já "
                         "copiados ficaram.",
                         "A file could not be read from the USB drive or written to the "
                         "console. The ones already copied stay."};
    Text error_limit{"Limite do GitHub atingido", "GitHub limit reached"};
    Text error_limit_text{"Sem conta, o GitHub aceita poucas consultas por hora. As versões já "
                          "conhecidas continuam valendo.",
                          "Without an account, GitHub takes few requests an hour. The versions "
                          "already known still apply."};
    Text limit_until{"Tente de novo depois das", "Try again after"};
    Text limit_lead{"Limite do GitHub: tente depois das", "GitHub limit: try again after"};
    Text registered_toast{"pronto para abrir", "ready to open"};
};
const Strings kS;

enum class Screen
{
    home,
    games,
    details,
    folders,
    updates,
    settings,
};

// What pressing Cross on an item does.
enum class Act
{
    none,
    tab,           // a: Screen
    play,          // a: emulator
    install,       // a: emulator (outside Details: opens Details first)
    uninstall,     // a: emulator
    open_folders,  // a: emulator
    open_details,  // a: emulator
    more,
    card,          // a: emulator; plays when installed, installs otherwise
    open_folder,   // a: emulator, b: folder
    pick_emulator, // a: emulator (Folders tabs)
    play_game,     // a: game (library().games), b: its row in the grid
    game_filter,   // a: emulator, -1 all
    import_folder, // b: folder
    import_all,
    prepare_usb,
    create_missing,
    check_updates,
    setting, // a: Setting
    dialog,  // a: button index
};

enum class Setting
{
    language,
    reduce_motion,
    sounds,
    resolution,
    about,
    exit,
    covers,
};

enum class Look
{
    pill,      // a top-bar tab
    button,    // a pill button
    primary,   // the filled pill button
    tile,      // a 16:9 card
    row,       // a list row or a settings row
    folder_tab, // an emulator pill on the Folders and Games screens
    cover,      // a game's box in the grid
};

struct Item
{
    Act act = Act::none;
    int a = 0;
    int b = 0;
    Rect rect;
    Look look = Look::button;
    int row = -1; // home shelves: which shelf and which card
    int col = -1;
    std::string label;
    bool nav = false; // in the top bar

    bool same(const Item &other) const
    {
        return act == other.act && a == other.a && b == other.b && row == other.row &&
               col == other.col;
    }
};

enum class TileKind
{
    emulator,
    news,
    more,
    folder,
};

struct Tile
{
    TileKind kind = TileKind::emulator;
    int emulator = 0;
    int folder = 0;
};

struct Shelf
{
    const Text *title = nullptr;
    std::vector<Tile> tiles;
};

// DialogButton::action besides 0 (close).
constexpr int kDialogQuit = 1;          // closes Emuland
constexpr int kDialogUninstallKeep = 2; // uninstalls dialog_emulator_, keeps the data
constexpr int kDialogUninstallWipe = 3; // ... and deletes the data too
constexpr int kDialogPlayIn = 100;      // + catalog index: opens dialog_game_ in it

struct DialogButton
{
    std::string label;
    int action = 0; // 0 close
};

struct Dialog
{
    bool open = false;
    std::string title;
    std::string body;
    bool credits = false;
    std::vector<DialogButton> buttons;
};

const char *plural(Lang lang, int n, const char *pt_one, const char *pt_many, const char *en_one,
                   const char *en_many)
{
    if (lang == Lang::pt)
        return n == 1 ? pt_one : pt_many;
    return n == 1 ? en_one : en_many;
}

// The version as people read it: contentVersion 01.002.000 is v1.2.0.
std::string pretty_version(const std::string &raw)
{
    int major = 0;
    int minor = 0;
    int patch = 0;
    const char *text_in = raw.c_str();
    if (*text_in == 'v' || *text_in == 'V')
        ++text_in;
    char rest = 0;
    if (std::sscanf(text_in, "%d.%d.%d%c", &major, &minor, &patch, &rest) != 3)
        return raw;
    char text[32];
    std::snprintf(text, sizeof(text), "v%d.%d.%d", major, minor, patch);
    return text;
}

// Each picture is taken after its step's wait and before its step's input.
constexpr app::TourStep kTour[] = {
    {1.4f, 0, Direction::none, "inicio"},
    {0.3f, 0, Direction::right},
    {0.9f, 0, Direction::none, "mais"},
    {0.3f, action_bit(Action::confirm)},
    {0.9f, 0, Direction::none, "sony"},
    {0.3f, 0, Direction::right},
    {0.3f, 0, Direction::right},
    {0.3f, 0, Direction::right},
    {0.9f, 0, Direction::none, "ps3"},
    {0.4f, 0, Direction::down},
    {0.9f, 0, Direction::none, "nintendo"},
    {0.4f, 0, Direction::down},
    {0.4f, 0, Direction::down},
    {0.9f, 0, Direction::none, "retroarch"},
    {0.4f, 0, Direction::down},
    {0.9f, 0, Direction::none, "fileira-pastas"},
    {0.4f, action_bit(Action::back)},
    {0.4f, 0, Direction::left},
    {0.4f, 0, Direction::down},
    {0.9f, 0, Direction::none, "novidades"},
    {0.3f, 0, Direction::up},
    {0.3f, 0, Direction::up},
    {0.9f, 0, Direction::none, "botoes"},
    {0.3f, action_bit(Action::north)},
    {0.9f, 0, Direction::none, "detalhes"},
    {0.3f, 0, Direction::right}, // Uninstall is the last button
    {0.3f, 0, Direction::right},
    {0.3f, 0, Direction::right},
    {0.3f, action_bit(Action::confirm)},
    {0.9f, 0, Direction::none, "desinstalar"},
    {0.3f, action_bit(Action::back)},
    {0.3f, action_bit(Action::back)},
    {0.5f, action_bit(Action::page_next)},
    {0.9f, 0, Direction::none, "jogos"},
    {0.3f, 0, Direction::right},
    {0.3f, 0, Direction::right},
    {0.4f, 0, Direction::down},
    {0.9f, 0, Direction::none, "jogos-grade"},
    {0.3f, action_bit(Action::confirm)}, // a PS1 game: PSXS5 or SwanStationPS5
    {0.9f, 0, Direction::none, "escolher-emulador"},
    {0.3f, action_bit(Action::back)},
    {0.4f, 0, Direction::up},
    {0.4f, 0, Direction::up},
    {0.3f, 0, Direction::right},
    {0.3f, 0, Direction::right},
    {0.3f, action_bit(Action::confirm)},
    {0.9f, 0, Direction::none, "jogos-ps2"},
    {0.3f, 0, Direction::down},
    {0.3f, action_bit(Action::confirm)},
    {1.2f, 0, Direction::none, "abrindo-jogo"},
    {2.2f, 0, Direction::none},
    {0.5f, action_bit(Action::page_next)},
    {0.9f, 0, Direction::none, "atualizacoes"},
    {0.3f, action_bit(Action::confirm)},
    {1.2f, 0, Direction::none, "instalando"},
    {4.0f, 0, Direction::none, "instalado"},
    {0.2f, action_bit(Action::page_next)},
    {0.9f, 0, Direction::none, "pastas"},
    {0.3f, 0, Direction::down},
    {0.9f, 0, Direction::none, "pastas-lista"},
    {0.3f, action_bit(Action::confirm)},
    {1.2f, 0, Direction::none, "importando"},
    {2.4f, 0, Direction::none, "importado"},
    {0.3f, 0, Direction::right},
    {0.3f, 0, Direction::down},
    {0.3f, 0, Direction::down},
    {0.3f, action_bit(Action::confirm)},
    {0.45f, 0, Direction::none, "preparando"},
    {1.4f, 0, Direction::none, "preparado"},
    {0.3f, action_bit(Action::page_next)},
    {0.9f, 0, Direction::none, "ajustes"},
    {0.3f, 0, Direction::down},
    {0.3f, 0, Direction::down},
    {0.3f, 0, Direction::right},
    {0.3f, action_bit(Action::confirm)},
    {0.9f, 0, Direction::none, "sobre"},
    {0.3f, action_bit(Action::back)},
    {0.5f, action_bit(Action::back)},
    {0.5f, 0, Direction::down},
    {0.4f, 0, Direction::left},
    {0.4f, action_bit(Action::confirm)},
    {0.7f, 0, Direction::none, "abrindo"},
    {2.4f, 0, Direction::none},
};

constexpr const char *kTechniques[] = {
    "Billboard over each emulator's own art, cross-faded on every focus change",
    "Shelves that lift as the focus goes down; the billboard text steps aside",
    "One accent ring that glides between cards, buttons, rows and tabs",
};

class Emuland final : public app::Concept
{
  public:
    explicit Emuland(app::Context &context) : context_(context)
    {
        language_choice_ = emuland::load_language_choice();
        cover_choice_ = emuland::load_cover_choice();
        apply_language();
        // Start on the first emulator with something to say: an update, else
        // the first installed one.
        const int count = emulator_count();
        int start = 0;
        for (int i = count - 1; i >= 0; --i)
        {
            if (emuland::installed(static_cast<std::size_t>(i)))
                start = i;
        }
        for (int i = count - 1; i >= 0; --i)
        {
            if (emuland::has_update(static_cast<std::size_t>(i)))
                start = i;
        }
        selected_ = start;
        shown_ = start;
        background_ = start;
        previous_background_ = start;
        row_ = 0;
        columns_[0] = start;
        build_layout();
        focus_ = find_tile(0, start);
        ring_.snap(ring_rect(focus_));
        if (!emuland::library().data_access)
            open_dialog(L(kS.no_data_title), L(kS.no_data_text), {{L(kS.got_it), 0}});
    }

    const app::ConceptInfo &info() const override
    {
        static const app::ConceptInfo kInfo{
            "emuland",
            "Manta",
            "The console's emulators, like a streaming service",
            "src/concepts/emuland.cpp",
            audio::SoundSet::glass,
            Color::rgb(0x1e3a5f),
            kTechniques,
        };
        return kInfo;
    }

    void enter() override
    {
        age_ = 0.0f;
    }

    void update(const InputFrame &input, float dt, app::Feedback &feedback) override
    {
        age_ += dt;
        clock_ += dt;
        clock_poll_ -= dt;
        if (clock_poll_ <= 0.0f)
        {
            emuland::local_time(&hour_, &minute_);
            clock_poll_ = 5.0f;
        }
        feedback_ = &feedback;
        mute_ = !sounds_on_;
        emuland::take_covers(2); // two a frame: a screenful arrives in a moment, no hitch
        poll_job();
        finish_prepare();
        poll_registration(dt);

        if (launching_ >= 0)
            update_launch(dt);
        else if (job_busy())
            update_job(input);
        else if (dialog_.open)
            update_dialog(input);
        else
            update_screen(input);

        animate(dt); // lays the screen out once, after the springs moved
        feedback_ = nullptr;
    }

    void draw(app::Frame &frame) const override
    {
        frame.backdrop.mode = gfx::BackdropMode::gradient;
        frame.backdrop.colors[0] = kBg;
        frame.backdrop.colors[1] = kBg;
        gfx::DrawList &list = frame.scene;
        const float in = tween::stagger(age_, 0, 0.05f, 0.5f);
        list.push_opacity(in);
        draw_backdrop(list);
        draw_nav(list);
        const float fade = screen_fade_.running ? tween::smoothstep(screen_fade_.progress()) : 1.0f;
        list.push_opacity(fade);
        list.push_transform(1.0f, 0, 0, 0, 18.0f * (1.0f - fade));
        switch (screen_)
        {
        case Screen::home:
            draw_home(list);
            break;
        case Screen::games:
            draw_games(list);
            break;
        case Screen::details:
            draw_details(list);
            break;
        case Screen::folders:
            draw_folders(list);
            break;
        case Screen::updates:
            draw_updates(list);
            break;
        case Screen::settings:
            draw_settings(list);
            break;
        }
        list.pop_transform();
        list.pop_opacity();
        if (!dialog_.open && launching_ < 0 && !job_busy())
            draw_ring(list);
        draw_hints(list);
        list.pop_opacity();

        if (dialog_show_.value > 0.01f)
            draw_dialog(frame.overlay);
        if (job_show_.value > 0.01f)
            draw_job(frame.overlay);
        draw_toast(frame.overlay);
        if (launch_show_.value > 0.01f)
            draw_launch(frame.overlay);
    }

    std::span<const app::TourStep> tour() const override
    {
        return kTour;
    }

  private:
    // ---------------------------------------------------------------- data

    static int emulator_count()
    {
        return static_cast<int>(emuland::catalog().size());
    }
    static const EmulatorSpec &spec(int index)
    {
        return emuland::catalog()[static_cast<std::size_t>(index)];
    }
    static const emuland::EmulatorState &state(int index)
    {
        return emuland::library().emulators[static_cast<std::size_t>(index)];
    }
    static bool is_installed(int index)
    {
        return emuland::installed(static_cast<std::size_t>(index));
    }
    static bool update_for(int index)
    {
        return emuland::has_update(static_cast<std::size_t>(index));
    }
    static int update_count()
    {
        int n = 0;
        for (int i = 0; i < emulator_count(); ++i)
            n += update_for(i) ? 1 : 0;
        return n;
    }
    static std::uint32_t icon_of(int index)
    {
        const auto &icons = emuland::art().icons;
        return static_cast<std::size_t>(index) < icons.size()
                   ? icons[static_cast<std::size_t>(index)]
                   : 0;
    }
    static std::uint32_t background_of(int index)
    {
        const auto &backgrounds = emuland::art().backgrounds;
        return static_cast<std::size_t>(index) < backgrounds.size()
                   ? backgrounds[static_cast<std::size_t>(index)]
                   : 0;
    }

    const char *L(const Text &text) const
    {
        return text.get(lang_);
    }

    void apply_language()
    {
        lang_ = language_choice_ == 1   ? Lang::pt
                : language_choice_ == 2 ? Lang::en
                                        : emuland::system_language();
    }

    // What is installed, as its release tag when Emuland installed it.
    std::string installed_text(int index) const
    {
        const emuland::EmulatorState &s = state(index);
        return s.installed_tag.empty() ? pretty_version(s.installed) : pretty_version(s.installed_tag);
    }

    // The text as it fits in width, ending in an ellipsis when cut.
    std::string fit(const std::string &text, float size, float width) const
    {
        return fit(context_.fonts.regular, text, size, width);
    }
    std::string fit(const ui::FontRef &font, const std::string &text, float size,
                    float width) const
    {
        if (font.measure(text, size) <= width)
            return text;
        std::string cut = text;
        while (!cut.empty() && font.measure(cut + "…", size) > width)
        {
            cut.pop_back();
            while (!cut.empty() && (static_cast<unsigned char>(cut.back()) & 0xc0) == 0x80)
                cut.pop_back(); // whole UTF-8 characters only
        }
        while (!cut.empty() && cut.back() == ' ')
            cut.pop_back();
        return cut + "…";
    }

    std::string games_text(int n) const
    {
        char text[48];
        std::snprintf(text, sizeof(text), "%d %s", n,
                      plural(lang_, n, "jogo", "jogos", "game", "games"));
        return text;
    }
    std::string files_text(int n) const
    {
        char text[48];
        std::snprintf(text, sizeof(text), "%d %s", n,
                      plural(lang_, n, "arquivo", "arquivos", "file", "files"));
        return text;
    }
    std::string updates_text(int n) const
    {
        char text[48];
        std::snprintf(text, sizeof(text), "%d %s", n,
                      plural(lang_, n, "atualização", "atualizações", "update", "updates"));
        return text;
    }

    int game_count(int index) const
    {
        const EmulatorSpec &emulator = spec(index);
        for (std::size_t f = 0; f < emulator.folders.size(); ++f)
        {
            if (emulator.folders[f].games)
                return std::max(0, state(index).folder_counts[f]);
        }
        return 0;
    }

    // ok, missing or optional: are the required (or optional) files there?
    enum class Needs
    {
        ok,
        missing,
        optional,
        builtin, // empty, but the emulator has its own (PSXS5's BIOS)
    };
    Needs needs_state(int index) const
    {
        const EmulatorSpec &emulator = spec(index);
        bool all = true;
        bool required = false;
        bool builtin = false;
        for (std::size_t f = 0; f < emulator.folders.size(); ++f)
        {
            if (emulator.folders[f].need == FolderNeed::none)
                continue;
            if (state(index).folder_counts[f] <= 0)
            {
                all = false;
                required = required || emulator.folders[f].need == FolderNeed::required;
                builtin = builtin || emulator.folders[f].when_empty.pt != nullptr;
            }
        }
        if (all)
            return Needs::ok;
        if (required)
            return Needs::missing;
        return builtin ? Needs::builtin : Needs::optional;
    }

    // ---------------------------------------------------------------- layout

    // Netflix-style rows: what you have, what changed, then the whole catalog
    // by maker, then the folders of what you have.
    std::vector<Shelf> shelves() const
    {
        std::vector<Shelf> result;
        Shelf library{&kS.row_library, {}};
        for (int i = 0; i < emulator_count(); ++i)
        {
            if (is_installed(i))
                library.tiles.push_back({TileKind::emulator, i, 0});
        }
        if (!library.tiles.empty())
        {
            library.tiles.push_back({TileKind::more, emulator_count(), 0});
            result.push_back(library);
        }
        Shelf news{&kS.row_news, {}};
        for (int i = 0; i < emulator_count(); ++i)
        {
            if (update_for(i))
                news.tiles.push_back({TileKind::news, i, 0});
        }
        if (!news.tiles.empty())
            result.push_back(news);
        const std::pair<emuland::Maker, const Text *> makers[] = {
            {emuland::Maker::sony, &kS.row_sony},
            {emuland::Maker::nintendo, &kS.row_nintendo},
            {emuland::Maker::microsoft, &kS.row_microsoft},
            {emuland::Maker::multi, &kS.row_multi},
        };
        for (const auto &[maker, title] : makers)
        {
            Shelf shelf{title, {}};
            for (int i = 0; i < emulator_count(); ++i)
            {
                if (spec(i).maker == maker)
                    shelf.tiles.push_back({TileKind::emulator, i, 0});
            }
            if (!shelf.tiles.empty())
                result.push_back(shelf);
        }
        Shelf folders{&kS.row_folders, {}};
        for (int i = 0; i < emulator_count(); ++i)
        {
            if (!is_installed(i))
                continue;
            const EmulatorSpec &emulator = spec(i);
            for (std::size_t f = 0; f < emulator.folders.size(); ++f)
            {
                if (emulator.folders[f].games || emulator.folders[f].need != FolderNeed::none)
                    folders.tiles.push_back({TileKind::folder, i, static_cast<int>(f)});
            }
        }
        if (!folders.tiles.empty())
            result.push_back(folders);
        return result;
    }

    bool installable(int index) const
    {
        return !emuland::source_only(spec(index));
    }

    float button_width(const std::string &label, bool with_icon) const
    {
        return 80.0f + context_.fonts.semibold.measure(label, 26) + (with_icon ? 40.0f : 0.0f);
    }

    void add(Item item)
    {
        items_.push_back(std::move(item));
    }

    void add_buttons(float x, float y, const std::vector<Item> &buttons)
    {
        for (Item button : buttons)
        {
            const bool icon = button.act == Act::play;
            const float w = button_width(button.label, icon);
            button.rect = {x, y, w, kButtonH};
            x += w + 20.0f;
            add(button);
        }
    }

    void build_layout()
    {
        items_.clear();
        shelves_ = shelves();
        // Top bar tabs.
        const ui::Fonts &fonts = context_.fonts;
        float x = kMargin + brand_width() + 40.0f;
        const Screen tabs[] = {Screen::home, Screen::games, Screen::updates, Screen::folders,
                               Screen::settings};
        const Text *names[] = {&kS.home, &kS.games_tab, &kS.updates_tab, &kS.folders_tab,
                               &kS.settings_tab};
        for (int i = 0; i < 5; ++i)
        {
            Item pill;
            pill.act = Act::tab;
            pill.a = static_cast<int>(tabs[i]);
            pill.label = L(*names[i]);
            pill.look = Look::pill;
            pill.nav = true;
            float w = 56.0f + fonts.semibold.measure(pill.label, 24);
            if (tabs[i] == Screen::updates && update_count() > 0)
                w += 40.0f;
            pill.rect = {x, 48, w, 56};
            x += w + 12.0f;
            add(pill);
        }
        switch (screen_)
        {
        case Screen::home:
            layout_home();
            break;
        case Screen::games:
            layout_games();
            break;
        case Screen::details:
            layout_details();
            break;
        case Screen::folders:
            layout_folders();
            break;
        case Screen::updates:
            layout_updates();
            break;
        case Screen::settings:
            layout_settings();
            break;
        }
        // Keep the focus on the same thing after a rebuild; fall back to the
        // first item of the screen.
        if (!find(focus_))
            focus_ = first_content();
    }

    std::vector<Item> hero_buttons(int index) const
    {
        std::vector<Item> buttons;
        if (index >= emulator_count())
        {
            buttons.push_back({Act::more, 0, 0, {}, Look::primary, -1, -1, L(kS.see_catalog)});
            return buttons;
        }
        if (is_installed(index))
        {
            buttons.push_back({Act::play, index, 0, {}, Look::primary, -1, -1, L(kS.play)});
            if (update_for(index))
                buttons.push_back({Act::install, index, 0, {}, Look::button, -1, -1, L(kS.update)});
        }
        else if (installable(index))
        {
            buttons.push_back({Act::install, index, 0, {}, Look::primary, -1, -1, L(kS.install)});
        }
        buttons.push_back(
            {Act::open_folders, index, 0, {}, Look::button, -1, -1, L(kS.folders)});
        buttons.push_back(
            {Act::open_details, index, 0, {}, Look::button, -1, -1, L(kS.details)});
        return buttons;
    }

    Rect tile_rect(int row, int col) const
    {
        const float scroll = scroll_[static_cast<std::size_t>(row)].offset();
        return {kMargin + static_cast<float>(col) * (kTileW + kTileGap) - scroll,
                kShelfTop + 52.0f + static_cast<float>(row) * kShelfPitch + rows_y_.value, kTileW,
                kTileH};
    }

    void layout_home()
    {
        add_buttons(kMargin, 560, hero_buttons(selected_));
        for (int r = 0; r < static_cast<int>(shelves_.size()); ++r)
        {
            const Shelf &shelf = shelves_[static_cast<std::size_t>(r)];
            for (int c = 0; c < static_cast<int>(shelf.tiles.size()); ++c)
            {
                const Tile &tile = shelf.tiles[static_cast<std::size_t>(c)];
                Item item;
                item.look = Look::tile;
                item.row = r;
                item.col = c;
                item.a = tile.emulator;
                item.b = tile.folder;
                item.act = tile.kind == TileKind::emulator ? Act::card
                           : tile.kind == TileKind::news   ? Act::install
                           : tile.kind == TileKind::more   ? Act::more
                                                           : Act::open_folder;
                item.rect = tile_rect(r, c);
                add(item);
            }
        }
    }

    static const std::vector<emuland::Game> &games()
    {
        return emuland::library().games;
    }
    static const emuland::Game &game(int index)
    {
        return games()[static_cast<std::size_t>(index)];
    }
    int games_of(int emulator) const
    {
        int n = 0;
        for (const emuland::Game &each : games())
            n += emulator < 0 || each.emulator == emulator ? 1 : 0;
        return n;
    }
    // The games the filter shows, in the library's order (by title).
    std::vector<int> filtered_games() const
    {
        std::vector<int> shown;
        for (int i = 0; i < static_cast<int>(games().size()); ++i)
        {
            if (game_filter_ < 0 || game(i).emulator == game_filter_)
                shown.push_back(i);
        }
        return shown;
    }

    // Games: the grid first (the focus lands on the first cover), then the
    // system filters above it, as the PS5's own Game Library.
    void layout_games()
    {
        const std::vector<int> shown = filtered_games();
        for (std::size_t i = 0; i < shown.size(); ++i)
        {
            const int row = static_cast<int>(i) / kCoverColumns;
            const int col = static_cast<int>(i) % kCoverColumns;
            Item card;
            card.act = Act::play_game;
            card.a = shown[i];
            card.b = row;
            card.look = Look::cover;
            card.rect = {kMargin + static_cast<float>(col) * (kCoverW + kCoverGap),
                         kGridTop + static_cast<float>(row) * kCoverPitch + games_y_.value, kCoverW,
                         kCoverH};
            add(card);
        }
        if (games().empty())
        {
            const std::vector<int> have = installed_list();
            if (!have.empty())
            {
                Item open{Act::open_folders, have.front(), 0, {}, Look::primary, -1, -1,
                          L(kS.open_folders)};
                open.rect = {kMargin, 560, button_width(open.label, false), kButtonH};
                add(open);
            }
            return;
        }
        const ui::Fonts &fonts = context_.fonts;
        float x = kMargin;
        for (int i = -1; i < emulator_count(); ++i)
        {
            if (i >= 0 && games_of(i) == 0)
                continue;
            Item pill;
            pill.act = Act::game_filter;
            pill.a = i;
            pill.look = Look::folder_tab;
            pill.label = i < 0 ? L(kS.all_filter) : spec(i).short_name;
            char count[16];
            std::snprintf(count, sizeof(count), "%d", games_of(i));
            const float lead = i < 0 ? 28.0f : 10.0f + 42.0f + 12.0f;
            const float w = lead + fonts.semibold.measure(pill.label, 22) + 12.0f +
                            fonts.regular.measure(count, 22) + 26.0f;
            pill.rect = {x, 368, w, 60};
            x += w + 14.0f;
            add(pill);
        }
    }

    void layout_details()
    {
        std::vector<Item> buttons;
        const int index = selected_;
        if (is_installed(index))
        {
            buttons.push_back({Act::play, index, 0, {}, Look::primary, -1, -1, L(kS.play)});
            if (update_for(index))
                buttons.push_back({Act::install, index, 0, {}, Look::button, -1, -1, L(kS.update)});
        }
        else if (installable(index))
        {
            buttons.push_back({Act::install, index, 0, {}, Look::primary, -1, -1, L(kS.install)});
        }
        buttons.push_back(
            {Act::open_folders, index, 0, {}, Look::button, -1, -1, L(kS.folders)});
        if (is_installed(index))
            buttons.push_back(
                {Act::uninstall, index, 0, {}, Look::button, -1, -1, L(kS.uninstall)});
        add_buttons(kMargin, 380, buttons);
    }

    void layout_folders()
    {
        const ui::Fonts &fonts = context_.fonts;
        float x = kMargin;
        for (int i = 0; i < emulator_count(); ++i)
        {
            if (!is_installed(i) && i != folder_emulator_)
                continue;
            Item tab;
            tab.act = Act::pick_emulator;
            tab.a = i;
            tab.look = Look::folder_tab;
            tab.label = spec(i).short_name;
            const float w = 10.0f + 42.0f + 12.0f + fonts.semibold.measure(tab.label, 22) + 26.0f;
            tab.rect = {x, 262, w, 60};
            x += w + 14.0f;
            add(tab);
        }
        const EmulatorSpec &emulator = spec(folder_emulator_);
        for (std::size_t f = 0; f < emulator.folders.size(); ++f)
        {
            Item row;
            row.act = Act::import_folder;
            row.a = folder_emulator_;
            row.b = static_cast<int>(f);
            row.look = Look::row;
            row.rect = {kMargin, 358.0f + static_cast<float>(f) * 86.0f, 1180, 76};
            add(row);
        }
        const float panel_bottom = folder_panel_rect().y + folder_panel_rect().h;
        add({Act::import_all, folder_emulator_, 0, {1324, panel_bottom + 20, 500, kButtonH},
             Look::primary, -1, -1, L(kS.import_all)});
        add({Act::create_missing, folder_emulator_, 0, {1324, panel_bottom + 112, 500, kButtonH},
             Look::button, -1, -1, L(kS.create_missing)});
        add({Act::prepare_usb, folder_emulator_, 0, {1324, panel_bottom + 204, 500, kButtonH},
             Look::button, -1, -1, L(kS.prepare_usb)});
    }

    std::vector<int> installed_list() const
    {
        std::vector<int> list;
        for (int i = 0; i < emulator_count(); ++i)
        {
            if (is_installed(i))
                list.push_back(i);
        }
        return list;
    }

    // Rows close up when many emulators are installed, so all of them fit.
    static float update_pitch(std::size_t rows)
    {
        return rows <= 5 ? 110.0f : 540.0f / static_cast<float>(rows);
    }

    Rect folder_panel_rect() const
    {
        const float lines = static_cast<float>(spec(folder_emulator_).folders.size());
        return {1324, 358, 500, 96.0f + (lines + 1.0f) * 33.0f + 92.0f};
    }

    void layout_updates()
    {
        float x = kMargin;
        const int n = update_count();
        if (n > 0)
        {
            char label[64];
            std::snprintf(label, sizeof(label), "%s (%d)",
                          lang_ == Lang::pt ? "Atualizar todos" : "Update all", n);
            Item all{Act::install, -1, 0, {}, Look::primary, -1, -1, label};
            all.rect = {x, 362, button_width(all.label, false), kButtonH};
            x += all.rect.w + 20.0f;
            add(all);
        }
        Item check{Act::check_updates, 0, 0, {}, Look::button, -1, -1, L(kS.check_now)};
        check.rect = {x, 362, button_width(check.label, false), kButtonH};
        add(check);
        const std::vector<int> rows = installed_list();
        for (std::size_t r = 0; r < rows.size(); ++r)
        {
            const int i = rows[r];
            if (!update_for(i))
                continue;
            const std::string label = L(kS.update);
            const float w = button_width(label, false);
            const float y = 472.0f + static_cast<float>(r) * update_pitch(rows.size());
            add({Act::install, i, 0, {kMargin + 1300 - 12 - w, y + 12, w, kButtonH}, Look::primary,
                 -1, -1, label});
        }
    }

    void layout_settings()
    {
        const Setting order[] = {Setting::language,   Setting::covers, Setting::reduce_motion,
                                 Setting::sounds,     Setting::resolution, Setting::about,
                                 Setting::exit};
        for (int i = 0; i < 7; ++i)
        {
            const float col = static_cast<float>(i % 2);
            const float row = static_cast<float>(i / 2);
            add({Act::setting, static_cast<int>(order[i]), 0,
                 {kMargin + col * (848 + 32), 380 + row * 112, 848, 96}, Look::row, -1, -1, ""});
        }
    }

    // ---------------------------------------------------------------- focus

    const Item *find(const Item &key) const
    {
        for (const Item &item : items_)
        {
            if (item.same(key) && item.nav == key.nav)
                return &item;
        }
        return nullptr;
    }

    Item first_content() const
    {
        for (const Item &item : items_)
        {
            if (!item.nav)
                return item;
        }
        return items_.front();
    }

    Item find_tile(int row, int col) const
    {
        const Item *best = nullptr;
        for (const Item &item : items_)
        {
            if (item.row != row)
                continue;
            if (best == nullptr || item.col <= col)
                best = &item;
        }
        return best != nullptr ? *best : first_content();
    }

    const Item *nav_pill() const
    {
        const Screen tab = screen_ == Screen::details ? Screen::home : screen_;
        for (const Item &item : items_)
        {
            if (item.nav && item.a == static_cast<int>(tab))
                return &item;
        }
        return nullptr;
    }

    const Item *primary_button() const
    {
        for (const Item &item : items_)
        {
            if (!item.nav && item.row < 0)
                return &item;
        }
        return nullptr;
    }

    // The nearest item in a direction, as in the prototype (and the PS5's own
    // menus): distance along the direction plus twice the sideways offset.
    const Item *spatial(Direction direction, bool nav_only, bool content_only) const
    {
        const Rect a = focus_.rect;
        const float ax = a.cx();
        const float ay = a.cy();
        const Item *best = nullptr;
        float best_score = 1e9f;
        for (const Item &item : items_)
        {
            if (item.same(focus_) && item.nav == focus_.nav)
                continue;
            if ((nav_only && !item.nav) || (content_only && item.nav))
                continue;
            const Rect r = item.rect;
            const float cx = r.cx();
            const float cy = r.cy();
            float main = 0.0f;
            float cross = 0.0f;
            const bool overlap_y = r.y + r.h > a.y && r.y < a.y + a.h;
            const bool overlap_x = r.x + r.w > a.x && r.x < a.x + a.w;
            switch (direction)
            {
            case Direction::right:
                if (cx <= ax + 1)
                    continue;
                main = std::max(0.0f, r.x - (a.x + a.w));
                cross = overlap_y ? 0.0f : std::fabs(cy - ay);
                break;
            case Direction::left:
                if (cx >= ax - 1)
                    continue;
                main = std::max(0.0f, a.x - (r.x + r.w));
                cross = overlap_y ? 0.0f : std::fabs(cy - ay);
                break;
            case Direction::down:
                if (cy <= ay + 1)
                    continue;
                main = std::max(0.0f, r.y - (a.y + a.h));
                cross = overlap_x ? std::fabs(cx - ax) * 0.3f : std::fabs(cx - ax);
                break;
            case Direction::up:
                if (cy >= ay - 1)
                    continue;
                main = std::max(0.0f, a.y - (r.y + r.h));
                cross = overlap_x ? std::fabs(cx - ax) * 0.3f : std::fabs(cx - ax);
                break;
            case Direction::none:
                continue;
            }
            const float score = main + cross * 2.0f;
            if (score < best_score)
            {
                best_score = score;
                best = &item;
            }
        }
        return best;
    }

    void move_to(const Item *item)
    {
        if (item == nullptr)
        {
            refuse();
            return;
        }
        focus_ = *item;
        if (!item->nav)
            saved_focus_[static_cast<int>(screen_)] = *item;
        if (screen_ == Screen::games && !item->nav)
            games_row_ = item->act == Act::play_game ? item->b : 0;
        if (screen_ == Screen::home)
        {
            if (item->row >= 0)
            {
                row_ = item->row;
                columns_[item->row] = item->col;
            }
            else if (!item->nav)
            {
                row_ = -1;
            }
            if (item->row >= 0 && item->a != selected_)
                select(item->a);
        }
        play(audio::Cue::focus, 1.0f, ui::pan_for_x(item->rect.cx()));
    }

    void select(int index)
    {
        selected_ = index;
        if (index != shown_)
        {
            shown_ = index;
            hero_.start(context_.settings.reduced_motion ? 0.12f : 0.4f);
        }
    }

    // The art behind the screen follows what the screen is about, always
    // with a cross-fade (also when Folders switches emulator).
    void follow_art()
    {
        int wanted = background_;
        if (screen_ == Screen::folders)
            wanted = folder_emulator_;
        else if (screen_ == Screen::games)
        {
            if (focus_.act == Act::play_game)
                wanted = game(focus_.a).emulator;
            else if (game_filter_ >= 0)
                wanted = game_filter_;
        }
        else if (selected_ < emulator_count())
            wanted = selected_;
        if (wanted == background_)
            return;
        previous_background_ = background_;
        background_ = wanted;
        background_fade_.start(context_.settings.reduced_motion ? 0.15f : 0.5f);
    }

    void navigate(Direction direction)
    {
        if (focus_.nav)
        {
            if (direction == Direction::left || direction == Direction::right)
                return move_to(spatial(direction, true, false));
            if (direction == Direction::down)
            {
                if (screen_ == Screen::home)
                    return move_to(primary_button());
                const Item &saved = saved_focus_[static_cast<int>(screen_)];
                const Item *back = find(saved);
                return move_to(back != nullptr ? back : spatial(direction, false, true));
            }
            return refuse();
        }
        if (screen_ == Screen::home)
        {
            const int shelves = static_cast<int>(shelves_.size());
            if (focus_.row >= 0)
            {
                const int r = focus_.row;
                const int c = focus_.col;
                const int count = static_cast<int>(shelves_[static_cast<std::size_t>(r)].tiles.size());
                if (direction == Direction::left)
                    return move_to(c > 0 ? find_tile_ptr(r, c - 1) : nullptr);
                if (direction == Direction::right)
                    return move_to(c + 1 < count ? find_tile_ptr(r, c + 1) : nullptr);
                if (direction == Direction::down)
                    return move_to(r + 1 < shelves ? find_tile_ptr(r + 1, columns_[r + 1]) : nullptr);
                if (direction == Direction::up)
                    return move_to(r > 0 ? find_tile_ptr(r - 1, columns_[r - 1]) : primary_button());
                return;
            }
            // Billboard buttons.
            if (direction == Direction::down)
                return move_to(find_tile_ptr(0, columns_[0]));
            if (direction == Direction::up)
                return move_to(nav_pill());
            const Item *next = spatial(direction, false, true);
            if (next != nullptr && next->row >= 0)
                next = nullptr;
            return move_to(next);
        }
        if (screen_ == Screen::games)
            return navigate_games(direction);
        const Item *next = spatial(direction, false, true);
        if (next == nullptr && direction == Direction::up)
            next = nav_pill();
        move_to(next);
    }

    const Item *find_act(Act act, int a) const
    {
        for (const Item &item : items_)
        {
            if (!item.nav && item.act == act && item.a == a)
                return &item;
        }
        return nullptr;
    }

    // The grid by position, as the PS5's Game Library: the column is kept
    // going up and down, a short last row takes its last cover, and up from
    // the first row goes to the filter in use.
    void navigate_games(Direction direction)
    {
        if (focus_.act != Act::play_game)
        {
            if (direction == Direction::down)
            {
                const std::vector<int> shown = filtered_games();
                return move_to(shown.empty() ? spatial(direction, false, true)
                                             : find_act(Act::play_game, shown.front()));
            }
            const Item *next = spatial(direction, false, true);
            if (direction == Direction::up)
                next = nav_pill();
            return move_to(next);
        }
        const std::vector<int> shown = filtered_games();
        const int count = static_cast<int>(shown.size());
        const int pos = static_cast<int>(std::find(shown.begin(), shown.end(), focus_.a) -
                                         shown.begin());
        const int col = pos % kCoverColumns;
        int next = -1;
        switch (direction)
        {
        case Direction::left:
            next = col > 0 ? pos - 1 : -1;
            break;
        case Direction::right:
            next = col + 1 < kCoverColumns && pos + 1 < count ? pos + 1 : -1;
            break;
        case Direction::up:
            if (pos < kCoverColumns)
                return move_to(find_act(Act::game_filter, game_filter_));
            next = pos - kCoverColumns;
            break;
        case Direction::down:
            if (pos + kCoverColumns < count)
                next = pos + kCoverColumns;
            else if ((pos / kCoverColumns + 1) * kCoverColumns < count)
                next = count - 1;
            break;
        case Direction::none:
            return;
        }
        move_to(next >= 0 ? find_act(Act::play_game, shown[static_cast<std::size_t>(next)])
                          : nullptr);
    }

    const Item *find_tile_ptr(int row, int col) const
    {
        const Item *best = nullptr;
        for (const Item &item : items_)
        {
            if (item.row != row)
                continue;
            if (best == nullptr || item.col <= col)
                best = &item;
        }
        return best;
    }

    void refuse()
    {
        if (repeat_)
            return; // a held direction stops quietly at the end
        play(audio::Cue::error, 1.0f, 0.0f, 0.5f);
        nudge_.trigger();
    }

    void play(audio::Cue cue, float pitch = 1.0f, float pan = 0.0f, float gain = 1.0f)
    {
        if (feedback_ != nullptr && !mute_)
            feedback_->play(cue, pitch, pan, gain);
    }

    // ---------------------------------------------------------------- input

    void update_screen(const InputFrame &input)
    {
        repeat_ = input.nav_repeat;
        if (input.nav != Direction::none)
            navigate(input.nav);
        if (input.is_pressed(Action::confirm))
            activate(focus_);
        else if (input.is_pressed(Action::back))
            go_back();
        else if (input.is_pressed(Action::page_next) || input.is_pressed(Action::page_prev))
            cycle_tab(input.is_pressed(Action::page_next) ? 1 : -1);
        else if (input.is_pressed(Action::menu))
            show_tab(Screen::settings, false);
        else if (screen_ == Screen::games && focus_.act == Act::play_game &&
                 input.is_pressed(Action::west))
            open_folders(game(focus_.a).emulator);
        else if (screen_ == Screen::home && selected_ < emulator_count())
        {
            if (input.is_pressed(Action::north))
                open_details(selected_);
            else if (input.is_pressed(Action::west))
                open_folders(selected_);
        }
    }

    void activate(const Item &item)
    {
        switch (item.act)
        {
        case Act::tab:
            show_tab(static_cast<Screen>(item.a), true);
            break;
        case Act::card:
            if (is_installed(item.a))
                play_or_wait(item.a);
            else
                open_details(item.a); // what it is, then Install there
            break;
        case Act::play:
            launching_game_ = -1;
            play_or_wait(item.a);
            break;
        case Act::play_game:
            play_game(item.a);
            break;
        case Act::game_filter:
            if (game_filter_ != item.a)
            {
                game_filter_ = item.a;
                games_row_ = 0;
                folder_switch_.start(context_.settings.reduced_motion ? 0.1f : 0.32f);
                play(audio::Cue::tab);
            }
            break;
        case Act::install:
            // A first install starts from Details, which says what it is.
            if (screen_ != Screen::details && !is_installed(item.a))
                open_details(item.a);
            else
                begin_install(item.a);
            break;
        case Act::uninstall:
            dialog_emulator_ = item.a;
            open_dialog(std::string(L(kS.uninstall_title)) + " " + spec(item.a).name + "?",
                        L(kS.uninstall_text),
                        {{L(kS.keep_files), kDialogUninstallKeep},
                         {L(kS.delete_all), kDialogUninstallWipe},
                         {L(kS.cancel), 0}});
            break;
        case Act::open_folders:
            open_folders(item.a);
            break;
        case Act::open_details:
            open_details(item.a);
            break;
        case Act::more:
        {
            // The catalog is on the shelves below: go to the first maker's row.
            for (int r = 0; r < static_cast<int>(shelves_.size()); ++r)
            {
                if (shelves_[static_cast<std::size_t>(r)].title == &kS.row_sony)
                {
                    move_to(find_tile_ptr(r, 0));
                    break;
                }
            }
            break;
        }
        case Act::open_folder:
            folder_emulator_ = item.a;
        {
            Item focus;
            focus.act = Act::import_folder;
            focus.a = item.a;
            focus.b = item.b;
            focus.look = Look::row;
            saved_focus_[static_cast<int>(Screen::folders)] = focus;
            go(Screen::folders);
        }
            break;
        case Act::pick_emulator:
            if (folder_emulator_ != item.a)
            {
                folder_emulator_ = item.a;
                folder_switch_.start(context_.settings.reduced_motion ? 0.1f : 0.32f);
                play(audio::Cue::tab);
            }
            break;
        case Act::import_folder:
            begin_import(item.a, item.b);
            break;
        case Act::import_all:
            begin_import(item.a, -1);
            break;
        case Act::prepare_usb:
            if (!preparing_)
            {
                preparing_ = true;
                prepare_ready_ = false;
                prepare_since_ = clock_;
                emuland::start_prepare_usb(lang_);
                play(audio::Cue::select);
            }
            break;
        case Act::create_missing:
        {
            const int made = emuland::create_missing_folders(
                spec(item.a), emuland::library().emulators[static_cast<std::size_t>(item.a)]);
            char text[80];
            if (made > 0)
                std::snprintf(text, sizeof(text), "%d %s", made,
                              plural(lang_, made, "pasta criada", "pastas criadas",
                                     "folder created", "folders created"));
            else
                std::snprintf(text, sizeof(text), "%s", L(kS.nothing_to_create));
            toast(text);
            play(made > 0 ? audio::Cue::saved : audio::Cue::select);
            break;
        }
        case Act::check_updates:
            manual_check_ = true;
            emuland::start_check(true);
            toast(L(kS.checking));
            break;
        case Act::setting:
            change_setting(static_cast<Setting>(item.a));
            break;
        case Act::dialog:
        case Act::none:
            break;
        }
    }

    void change_setting(Setting setting)
    {
        switch (setting)
        {
        case Setting::language:
            language_choice_ = (language_choice_ + 1) % 3;
            emuland::save_language_choice(language_choice_);
            apply_language();
            play(audio::Cue::toggle);
            break;
        case Setting::reduce_motion:
            context_.settings.reduced_motion = !context_.settings.reduced_motion;
            context_.settings_changed = true;
            play(audio::Cue::toggle);
            break;
        case Setting::sounds:
            sounds_on_ = !sounds_on_;
            mute_ = !sounds_on_;
            play(audio::Cue::toggle);
            break;
        case Setting::resolution:
            context_.settings.resolution =
                (context_.settings.resolution + 1) % Settings::kResolutionCount;
            context_.settings_changed = true;
            play(audio::Cue::toggle);
            toast(L(kS.next_start));
            break;
        case Setting::about:
            open_dialog("Manta", L(kS.about_text), {{L(kS.got_it), 0}}, true);
            break;
        case Setting::exit:
            emuland::quit();
            break;
        case Setting::covers:
            cover_choice_ = !cover_choice_;
            emuland::save_cover_choice(cover_choice_);
            if (cover_choice_)
                emuland::start_covers(true); // the missing ones, now
            play(audio::Cue::toggle);
            break;
        }
    }

    void cycle_tab(int step)
    {
        const Screen order[] = {Screen::home, Screen::games, Screen::updates, Screen::folders,
                                Screen::settings};
        const Screen current = screen_ == Screen::details ? Screen::home : screen_;
        int index = 0;
        for (int i = 0; i < 5; ++i)
        {
            if (order[i] == current)
                index = i;
        }
        show_tab(order[(index + step + 5) % 5], false);
    }

    void show_tab(Screen screen, bool keep_nav)
    {
        came_from_home_ = false;
        if (screen != screen_)
        {
            screen_ = screen;
            screen_fade_.start(context_.settings.reduced_motion ? 0.1f : 0.28f);
            play(audio::Cue::tab);
        }
        if (screen_ == Screen::folders && folder_emulator_ >= emulator_count())
            folder_emulator_ = 0;
        build_layout();
        if (keep_nav)
        {
            if (const Item *pill = nav_pill())
                focus_ = *pill;
        }
        else
        {
            restore_focus();
        }
        if (screen_ == Screen::home)
            select(selected_);
        ring_.snap(ring_rect(focus_));
    }

    void go(Screen screen)
    {
        back_to_ = screen_;
        screen_ = screen;
        screen_fade_.start(context_.settings.reduced_motion ? 0.1f : 0.28f);
        play(audio::Cue::open);
        build_layout();
        restore_focus();
        ring_.snap(ring_rect(focus_));
    }

    void restore_focus()
    {
        const Item *saved = find(saved_focus_[static_cast<int>(screen_)]);
        if (screen_ == Screen::home && saved == nullptr)
            saved = find_tile_ptr(0, columns_[0]);
        focus_ = saved != nullptr ? *saved : first_content();
        if (screen_ == Screen::home && focus_.row >= 0)
            row_ = focus_.row;
    }

    void go_back()
    {
        if (screen_ == Screen::details ||
            (screen_ == Screen::folders &&
             (back_to_ == Screen::home || back_to_ == Screen::games) && came_from_home_))
        {
            came_from_home_ = false;
            screen_ = screen_ == Screen::folders ? back_to_ : Screen::home;
            screen_fade_.start(context_.settings.reduced_motion ? 0.1f : 0.28f);
            play(audio::Cue::back);
            build_layout();
            restore_focus();
            ring_.snap(ring_rect(focus_));
            return;
        }
        if (screen_ != Screen::home)
        {
            show_tab(Screen::home, false);
            play(audio::Cue::back);
            return;
        }
        if (focus_.row > 0)
        {
            move_to(find_tile_ptr(0, columns_[0]));
            return;
        }
        refuse();
    }

    void open_details(int index)
    {
        select(index);
        came_from_home_ = true;
        saved_focus_[static_cast<int>(Screen::details)] = Item{};
        go(Screen::details);
    }

    void open_folders(int index)
    {
        folder_emulator_ = index;
        came_from_home_ = screen_ == Screen::home || screen_ == Screen::details ||
                          screen_ == Screen::games;
        go(Screen::folders);
    }

    // ---------------------------------------------------------------- installer

    static bool working(emuland::Stage stage)
    {
        return stage == emuland::Stage::checking || stage == emuland::Stage::downloading ||
               stage == emuland::Stage::verifying || stage == emuland::Stage::extracting ||
               stage == emuland::Stage::installing || stage == emuland::Stage::removing ||
               stage == emuland::Stage::importing;
    }

    // An install in progress owns the screen (a check runs quietly behind it).
    bool job_busy() const
    {
        return working(job_.stage) && (job_.emulator >= 0 || installing_);
    }

    void begin_install(int index)
    {
        installing_ = true;
        install_label_ = index;
        emuland::start_install(index);
        job_ = emuland::job_status();
        job_show_.target = 1.0f;
        play(audio::Cue::open);
    }

    // The button keeps its animation at least this long, so a fast drive
    // still shows that something happened before the toast.
    void finish_prepare()
    {
        if (!preparing_ || !prepare_ready_ || clock_ - prepare_since_ < 1.2f)
            return;
        preparing_ = false;
        const emuland::JobStatus &result = prepare_result_;
        if (result.stage == emuland::Stage::failed &&
            result.error == emuland::JobError::none_installed)
        {
            toast(L(kS.prepare_needs_install));
            play(audio::Cue::error);
            return;
        }
        if (result.stage == emuland::Stage::failed)
        {
            const bool missing = result.error == emuland::JobError::no_usb;
            open_dialog(L(missing ? kS.error_no_usb : kS.error_usb_write),
                        L(missing ? kS.error_no_usb_text : kS.error_usb_write_text),
                        {{L(kS.got_it), 0}});
            play(audio::Cue::error);
            return;
        }
        char text[96];
        if (result.prepared > 0)
            std::snprintf(text, sizeof(text), "%d %s", result.prepared, L(kS.usb_ready));
        else
            std::snprintf(text, sizeof(text), "%s", L(kS.usb_already));
        toast(text);
        play(result.prepared > 0 ? audio::Cue::saved : audio::Cue::select);
    }

    void begin_import(int index, int folder)
    {
        installing_ = true;
        install_label_ = index;
        emuland::start_import(index, folder);
        job_ = emuland::job_status();
        job_show_.target = 1.0f;
        play(audio::Cue::open);
    }

    void begin_uninstall(int index, bool wipe_data)
    {
        installing_ = true;
        install_label_ = index;
        emuland::start_uninstall(index, wipe_data);
        job_ = emuland::job_status();
        job_show_.target = 1.0f;
        play(audio::Cue::open);
    }

    void update_job(const InputFrame &input)
    {
        // Once the folder is being moved into place the job runs to its end.
        if (input.is_pressed(Action::back) && job_.stage != emuland::Stage::installing &&
            job_.stage != emuland::Stage::removing)
        {
            emuland::cancel_job();
            play(audio::Cue::back);
        }
    }

    const char *error_title(emuland::JobError error) const
    {
        switch (error)
        {
        case emuland::JobError::network:
            return L(kS.error_network);
        case emuland::JobError::no_release:
            return L(kS.error_release);
        case emuland::JobError::running:
            return L(kS.error_running);
        case emuland::JobError::download:
            return L(kS.error_download);
        case emuland::JobError::digest:
            return L(kS.error_digest);
        case emuland::JobError::archive:
            return L(kS.error_archive);
        case emuland::JobError::uninstall:
            return L(kS.error_uninstall);
        case emuland::JobError::rate_limited:
            return L(kS.error_limit);
        case emuland::JobError::no_usb:
            return L(kS.error_no_usb);
        case emuland::JobError::no_space:
            return L(kS.error_no_space);
        case emuland::JobError::copy:
            return L(kS.error_copy);
        default:
            return L(kS.error_install);
        }
    }

    const char *error_text(emuland::JobError error) const
    {
        switch (error)
        {
        case emuland::JobError::network:
            return L(kS.error_network_text);
        case emuland::JobError::no_release:
            return L(kS.error_release_text);
        case emuland::JobError::running:
            return L(kS.error_running_text);
        case emuland::JobError::download:
            return L(kS.error_download_text);
        case emuland::JobError::digest:
            return L(kS.error_digest_text);
        case emuland::JobError::archive:
            return L(kS.error_archive_text);
        case emuland::JobError::uninstall:
            return L(kS.error_uninstall_text);
        case emuland::JobError::rate_limited:
            return L(kS.error_limit_text);
        case emuland::JobError::no_usb:
            return L(kS.error_no_usb_text);
        case emuland::JobError::no_space:
            return L(kS.error_no_space_text);
        case emuland::JobError::copy:
            return L(kS.error_copy_text);
        default:
            return L(kS.error_install_text);
        }
    }

    // Each finished job: read the console again, then say what happened.
    void poll_job()
    {
        const int before = job_.finished;
        const int was = job_.emulator;
        job_ = emuland::job_status();
        if (working(job_.stage) && job_.emulator >= 0)
            install_label_ = job_.emulator;
        if (job_.finished == before)
            return;
        if (job_.prepare)
        {
            prepare_result_ = job_;
            prepare_ready_ = true;
            return;
        }
        emuland::scan(emuland::library());
        emuland::start_covers(cover_choice_); // new games (an import, an install)
        build_layout();
        const bool install = was >= 0 || installing_;
        if (job_.stage == emuland::Stage::cancelled)
        {
            toast(L(kS.cancelled));
        }
        else if (job_.stage == emuland::Stage::failed && (install || manual_check_))
        {
            std::string body = error_text(job_.error);
            if (job_.error == emuland::JobError::rate_limited && job_.retry_hour >= 0)
            {
                char text[96];
                std::snprintf(text, sizeof(text), " %s %02d:%02d.", L(kS.limit_until),
                              job_.retry_hour, job_.retry_minute);
                body += text;
            }
            open_dialog(error_title(job_.error),
                        body + (job_.detail.empty() ? "" : "\n\n" + job_.detail),
                        {{L(kS.got_it), 0}});
            play(audio::Cue::error);
        }
        else if (job_.stage == emuland::Stage::done && job_.import)
        {
            char text[96];
            if (job_.imported > 0)
                std::snprintf(text, sizeof(text), "%d %s", job_.imported,
                              L(job_.imported == 1 ? kS.imported_one : kS.imported_many));
            else
                std::snprintf(text, sizeof(text), "%s", L(kS.nothing_new));
            toast(text);
            play(job_.imported > 0 ? audio::Cue::complete : audio::Cue::select);
        }
        else if (job_.stage == emuland::Stage::done && job_.uninstall && was >= 0)
        {
            toast(std::string(spec(was).name) + " " + L(kS.uninstalled_toast));
            play(audio::Cue::complete);
            if (job_.needs_close)
                open_dialog(L(kS.unregister_title), L(kS.unregister_text),
                            {{L(kS.close_now), kDialogQuit}, {L(kS.later), 0}});
        }
        else if (job_.stage == emuland::Stage::done && install && was >= 0)
        {
            char text[128];
            std::snprintf(text, sizeof(text), "%s %s %s", spec(was).name,
                          state(was).installed_tag.c_str(), L(kS.installed_toast));
            toast(text);
            play(audio::Cue::complete);
            if (!state(was).registered)
                ask_to_close(was);
        }
        else if (job_.stage == emuland::Stage::done && !install)
        {
            const int n = update_count();
            if (n > 0 || manual_check_)
            {
                char text[96];
                if (n > 0)
                    std::snprintf(text, sizeof(text), "%d %s", n,
                                  L(n == 1 ? kS.found_update : kS.found_updates));
                else
                    std::snprintf(text, sizeof(text), "%s", L(kS.all_current));
                toast(text);
            }
        }
        if (!working(job_.stage) || job_.queued == 0)
        {
            if (job_.stage != emuland::Stage::downloading)
                installing_ = working(job_.stage) && job_.emulator >= 0;
        }
        if (!install)
            manual_check_ = false;
    }

    // ---------------------------------------------------------------- dialog, toast, launch

    void open_dialog(std::string title, std::string body, std::vector<DialogButton> buttons,
                     bool credits = false)
    {
        dialog_.open = true;
        dialog_.title = std::move(title);
        dialog_.body = std::move(body);
        dialog_.buttons = std::move(buttons);
        dialog_.credits = credits;
        dialog_focus_ = 0;
        play(audio::Cue::modal_open);
    }

    void update_dialog(const InputFrame &input)
    {
        const int count = static_cast<int>(dialog_.buttons.size());
        if (input.nav == Direction::left || input.nav == Direction::right)
        {
            const int next = dialog_focus_ + (input.nav == Direction::right ? 1 : -1);
            if (next >= 0 && next < count)
            {
                dialog_focus_ = next;
                play(audio::Cue::focus);
            }
        }
        const int action = input.is_pressed(Action::confirm) && dialog_focus_ < count
                               ? dialog_.buttons[static_cast<std::size_t>(dialog_focus_)].action
                               : 0;
        if (action == kDialogQuit)
        {
            emuland::quit();
            return;
        }
        if (action == kDialogUninstallKeep || action == kDialogUninstallWipe)
        {
            dialog_.open = false;
            begin_uninstall(dialog_emulator_, action == kDialogUninstallWipe);
            return;
        }
        if (action >= kDialogPlayIn)
        {
            dialog_.open = false;
            const int player = action - kDialogPlayIn;
            chosen_player_[static_cast<std::size_t>(game(dialog_game_).emulator)] = player;
            launching_game_ = dialog_game_;
            play_or_wait(player);
            return;
        }
        if (input.is_pressed(Action::confirm) || input.is_pressed(Action::back))
        {
            dialog_.open = false;
            play(audio::Cue::modal_close);
        }
    }

    void toast(std::string text)
    {
        toast_text_ = std::move(text);
        toast_time_ = 2.8f;
        play(audio::Cue::notify);
    }

    // The emulators that can open a game: the one whose folder holds it, then
    // every other installed one for the same system that takes a path
    // (rom_option). One without it opens on its own menu, which lists only its
    // own folder, so it could not find the game.
    std::vector<int> players_of(int game_index) const
    {
        const int owner = game(game_index).emulator;
        std::vector<int> players{owner};
        for (int i = 0; i < emulator_count(); ++i)
        {
            if (i != owner && is_installed(i) && spec(i).rom_option != nullptr &&
                std::strcmp(spec(i).short_name, spec(owner).short_name) == 0)
                players.push_back(i);
        }
        return players;
    }

    // With two emulators for the game, ask which (as Android's "Open with"),
    // focusing the one picked last time for games of that folder.
    void play_game(int game_index)
    {
        const std::vector<int> players = players_of(game_index);
        if (players.size() == 1)
        {
            launching_game_ = game_index;
            play_or_wait(players.front());
            return;
        }
        dialog_game_ = game_index;
        std::vector<DialogButton> buttons;
        for (const int player : players)
            buttons.push_back({spec(player).name, kDialogPlayIn + player});
        buttons.push_back({L(kS.cancel), 0});
        open_dialog(L(kS.choose_emulator),
                    "“" + game(game_index).title + "” " + L(kS.choose_emulator_text),
                    std::move(buttons));
        const int last = chosen_player_[static_cast<std::size_t>(game(game_index).emulator)];
        for (std::size_t i = 0; i < players.size(); ++i)
            dialog_focus_ = players[i] == last ? static_cast<int>(i) : dialog_focus_;
    }

    // A fresh install opens only once the console has registered it.
    void play_or_wait(int index)
    {
        if (state(index).registered)
        {
            start_launch(index);
            return;
        }
        ask_to_close(index);
    }

    // ShadowMountPlus registers new apps only while no game runs, and Emuland
    // is one (its API defers even a requested scan: "game_active").
    void ask_to_close(int index)
    {
        open_dialog(std::string(spec(index).name) + ": " + L(kS.register_title),
                    L(kS.register_text), {{L(kS.close_now), kDialogQuit}, {L(kS.later), 0}});
    }

    // While an install waits for the console to register it, look again
    // every two seconds (one stat per emulator) and say when it can open.
    void poll_registration(float dt)
    {
        registration_poll_ -= dt;
        if (registration_poll_ > 0.0f)
            return;
        registration_poll_ = 2.0f;
        bool changed = false;
        for (int i = 0; i < emulator_count(); ++i)
        {
            emuland::EmulatorState &each = emuland::library().emulators[static_cast<std::size_t>(i)];
            if (!is_installed(i) || each.registered || !emuland::registered(spec(i)))
                continue;
            each.registered = true;
            changed = true;
            toast(std::string(spec(i).name) + " " + L(kS.registered_toast));
            play(audio::Cue::complete);
        }
        if (changed)
            build_layout();
    }

    void start_launch(int index)
    {
        launching_ = index;
        launch_time_ = 0.0f;
        launch_called_ = false;
        select(index);
        play(audio::Cue::launch);
        if (feedback_ != nullptr)
            feedback_->rumble(0.6f, 0.15f);
    }

    void update_launch(float dt)
    {
        launch_time_ += dt;
        // The overlay shows first, then the system is asked to open the
        // emulator; the overlay leaves after a moment either way.
        if (!launch_called_ && launch_time_ > 0.5f)
        {
            launch_called_ = true;
            launch_result_ = emuland::launch(
                spec(launching_), launching_game_ >= 0 ? game(launching_game_).path : std::string());
        }
        if (launch_time_ > 2.2f)
        {
            const bool known_good =
                launch_result_ == 0 || static_cast<unsigned>(launch_result_) == 0x80940010u;
            if (static_cast<unsigned>(launch_result_) == 0x80940031u)
            {
                // Not registered yet (seen right after an install).
                emuland::library().emulators[static_cast<std::size_t>(launching_)].registered =
                    false;
                build_layout();
                ask_to_close(launching_);
            }
            else if (!known_good)
            {
                char text[96];
                std::snprintf(text, sizeof(text), "%s (0x%08x)", L(kS.launch_failed),
                              static_cast<unsigned>(launch_result_));
                toast(text);
            }
            launching_ = -1;
            launching_game_ = -1;
        }
    }

    // ---------------------------------------------------------------- animation

    Rect ring_rect(const Item &item) const
    {
        Rect r = item.rect;
        const float scale = focus_scale(item.look);
        r = {r.cx() - r.w * scale * 0.5f, r.cy() - r.h * scale * 0.5f, r.w * scale, r.h * scale};
        return r;
    }

    float focus_scale(Look look) const
    {
        if (context_.settings.reduced_motion)
            return 1.0f;
        switch (look)
        {
        case Look::tile:
            return 1.08f;
        case Look::row:
            return 1.015f;
        case Look::cover:
            return 1.07f;
        default:
            return 1.06f;
        }
    }

    void animate(float dt)
    {
        // Shelves: the focused one sits under the billboard text; on the
        // first shelf (or the buttons) everything is in its resting place.
        const float lift = row_ >= 1 ? -130.0f - static_cast<float>(row_) * kShelfPitch : 0.0f;
        rows_y_.target = screen_ == Screen::home ? lift : 0.0f;
        rows_y_.update(dt, context_.settings.reduced_motion ? 40.0f : 11.0f);
        compact_.target = screen_ == Screen::home && row_ >= 1 ? 1.0f : 0.0f;
        compact_.update(dt, 14.0f);
        games_y_.target = screen_ == Screen::games ? -static_cast<float>(games_row_) * kCoverPitch
                                                    : 0.0f;
        games_y_.update(dt, context_.settings.reduced_motion ? 40.0f : 12.0f);
        for (int r = 0; r < static_cast<int>(shelves_.size()) && r < kMaxShelves; ++r)
        {
            const float start = static_cast<float>(columns_[r]) * (kTileW + kTileGap);
            scroll_[static_cast<std::size_t>(r)].reveal(start, start + kTileW,
                                                        gfx::kVirtualWidth - kMargin * 2.0f,
                                                        kTileW * 0.5f);
            scroll_[static_cast<std::size_t>(r)].update(dt, 12.0f);
        }
        build_layout(); // positions follow the springs
        follow_art();
        folder_switch_.update(dt);
        hero_.update(dt);
        background_fade_.update(dt);
        screen_fade_.update(dt);
        if (const Item *focused = find(focus_))
            focus_ = *focused;
        ring_.target(ring_rect(focus_));
        ring_.update(dt, 20.0f);
        nudge_.update(dt, 9.0f);
        dialog_show_.target = dialog_.open ? 1.0f : 0.0f;
        dialog_show_.update(dt, context_.settings.reduced_motion ? 40.0f : 16.0f);
        job_show_.target = job_busy() ? 1.0f : 0.0f;
        job_show_.update(dt, context_.settings.reduced_motion ? 40.0f : 14.0f);
        launch_show_.target = launching_ >= 0 ? 1.0f : 0.0f;
        launch_show_.update(dt, 12.0f);
        toast_time_ = std::max(0.0f, toast_time_ - dt);
    }

    // ---------------------------------------------------------------- drawing helpers

    enum class Icon
    {
        check,
        alert,
        arrow,
        dash,
        game,
        folder,
    };

    // Line icons on a 24-unit grid, as in the prototype's SVGs.
    static void icon(gfx::DrawList &list, Icon kind, float x, float y, float size, Color color)
    {
        const float k = size / 24.0f;
        const auto px = [&](float u) { return x + u * k; };
        const auto py = [&](float v) { return y + v * k; };
        const float stroke = 2.3f * k;
        switch (kind)
        {
        case Icon::check:
            list.line(px(5), py(12.5f), px(9.5f), py(17), stroke, color);
            list.line(px(9.5f), py(17), px(19), py(7.5f), stroke, color);
            break;
        case Icon::alert:
            list.ring(px(12), py(12), 9 * k, stroke, color);
            list.line(px(12), py(7.5f), px(12), py(13), stroke, color);
            list.circle(px(12), py(16.6f), 1.4f * k, color);
            break;
        case Icon::arrow:
            list.line(px(12), py(4), px(12), py(16), stroke, color);
            list.line(px(6.5f), py(11), px(12), py(16.5f), stroke, color);
            list.line(px(17.5f), py(11), px(12), py(16.5f), stroke, color);
            list.line(px(5), py(20), px(19), py(20), stroke, color);
            break;
        case Icon::dash:
            list.ring(px(12), py(12), 9 * k, stroke, color);
            list.line(px(8), py(12), px(16), py(12), stroke, color);
            break;
        case Icon::game:
            list.bordered_rect({px(3), py(7), 18 * k, 11 * k}, 5 * k, kClear, 2 * k, color);
            list.line(px(8), py(10.5f), px(8), py(14.5f), 2 * k, color);
            list.line(px(6), py(12.5f), px(10), py(12.5f), 2 * k, color);
            list.circle(px(15.5f), py(12), 1.2f * k, color);
            list.circle(px(17.5f), py(14), 1.2f * k, color);
            break;
        case Icon::folder:
            list.bordered_rect({px(3), py(8), 18 * k, 11 * k}, 1.8f * k, kClear, 1.6f * k, color);
            list.bordered_rect({px(3), py(6), 7 * k, 4 * k}, 1.2f * k, kClear, 1.6f * k, color);
            break;
        }
    }

    // A status chip; returns its width.
    float chip(gfx::DrawList &list, float x, float y, Icon kind, Color tone,
               const std::string &label, float height = 46.0f, float text_size = 22.0f) const
    {
        const float tw = context_.fonts.regular.measure(label, text_size);
        const float w = 20.0f + 22.0f + 10.0f + tw + 20.0f;
        list.bordered_rect({x, y, w, height}, height * 0.5f, Color::rgb(0x151a23, 0.78f), 1.0f,
                           kLine);
        icon(list, kind, x + 18, y + (height - 22.0f) * 0.5f, 22, tone);
        ui::text(list, context_.fonts.regular, label, x + 52, y + height * 0.5f + text_size * 0.36f,
                 text_size, kText);
        return w;
    }

    struct ChipSpec
    {
        Icon icon;
        Color tone;
        std::string label;
    };

    std::vector<ChipSpec> status_chips(int index) const
    {
        std::vector<ChipSpec> chips;
        if (!is_installed(index))
        {
            chips.push_back({Icon::dash, kText2,
                             installable(index) ? L(kS.not_installed) : L(kS.source_only)});
            const std::uint64_t size = state(index).release.size;
            if (size > 0)
                chips.push_back({Icon::arrow, kText2, megabytes(size) + " MB"});
            return chips;
        }
        else if (!state(index).registered)
            chips.push_back({Icon::dash, kWarn, L(kS.registering_chip)});
        else if (update_for(index))
            chips.push_back({Icon::arrow, kAccent, L(kS.update_available)});
        else if (state(index).latest.empty())
            chips.push_back({Icon::check, kOk, L(kS.installed_chip)});
        else
            chips.push_back({Icon::check, kOk, L(kS.up_to_date)});
        const char *needs = L(spec(index).needs);
        std::string capital = needs;
        if (!capital.empty() && capital[0] >= 'a' && capital[0] <= 'z')
            capital[0] = static_cast<char>(capital[0] - 'a' + 'A');
        switch (needs_state(index))
        {
        case Needs::ok:
            chips.push_back({Icon::check, kOk, capital + " ok"});
            break;
        case Needs::missing:
            chips.push_back({Icon::alert, kWarn,
                             lang_ == Lang::pt ? std::string("Falta ") + needs
                                               : capital + " missing"});
            break;
        case Needs::optional:
            chips.push_back({Icon::dash, kText2,
                             capital + (lang_ == Lang::pt ? " opcional" : " optional")});
            break;
        case Needs::builtin:
            for (const emuland::FolderSpec &folder : spec(index).folders)
            {
                if (folder.when_empty.pt != nullptr)
                {
                    chips.push_back({Icon::check, kText2, L(folder.when_empty)});
                    break;
                }
            }
            break;
        }
        const int games = game_count(index);
        chips.push_back({Icon::game, kText2, games > 0 ? games_text(games) : L(kS.no_games)});
        return chips;
    }

    void folder_status(int emulator, int folder, Icon *kind, Color *tone, std::string *label) const
    {
        const emuland::FolderSpec &f = spec(emulator).folders[static_cast<std::size_t>(folder)];
        const int count = state(emulator).folder_counts[static_cast<std::size_t>(folder)];
        const bool required = f.need == FolderNeed::required;
        if (count <= 0 && f.when_empty.pt != nullptr)
        {
            *kind = Icon::check;
            *tone = kText2;
            *label = L(f.when_empty);
        }
        else if (count < 0)
        {
            *kind = required ? Icon::alert : Icon::dash;
            *tone = required ? kWarn : kText2;
            *label = L(kS.not_created);
        }
        else if (count == 0)
        {
            *kind = required ? Icon::alert : Icon::dash;
            *tone = required ? kWarn : kText2;
            *label = L(kS.empty);
        }
        else
        {
            *kind = Icon::check;
            *tone = kOk;
            *label = f.games ? games_text(count) : files_text(count);
        }
    }

    bool focused(const Item &item) const
    {
        return !dialog_.open && launching_ < 0 && !job_busy() && item.same(focus_) &&
               item.nav == focus_.nav;
    }

    void draw_button(gfx::DrawList &list, const Item &item) const
    {
        const bool focus = focused(item);
        const Rect r = item.rect;
        const float scale = focus ? focus_scale(item.look) : 1.0f;
        list.push_transform(scale, r.cx(), r.cy(), 0, 0);
        if (item.look == Look::primary)
            list.rounded_rect(r, r.h * 0.5f, kText);
        else
            list.bordered_rect(r, r.h * 0.5f, Color::rgb(0x1c2330, 0.82f), 1.0f, kLine);
        const Color ink = item.look == Look::primary ? kInk : kText;
        if (item.act == Act::prepare_usb && preparing_)
        {
            // "Preparando" with dots that count up, and a light running
            // around the pill. Placed by its widest form, so it doesn't jump.
            const std::string label = L(kS.preparing_usb);
            const float full = context_.fonts.semibold.measure(label + "...", 26);
            const bool still = context_.settings.reduced_motion;
            const int dots = still ? 3 : static_cast<int>(clock_ * 2.5f) % 4;
            ui::text(list, context_.fonts.semibold, label + std::string(dots, '.'),
                     r.cx() - full * 0.5f, r.cy() + 9.5f, 26, ink);
            if (!still)
                draw_comet(list, r.inset(-4.5f)); // along the focus ring
            list.pop_transform();
            return;
        }
        const float tw = context_.fonts.semibold.measure(item.label, 26);
        float x = r.cx() - tw * 0.5f;
        if (item.act == Act::play)
        {
            x += 20.0f;
            list.triangle({x - 40, r.cy() - 12, 22, 24}, ink, 0.0f, 1.5708f);
        }
        ui::text(list, context_.fonts.semibold, item.label, x, r.cy() + 9.5f, 26, ink);
        list.pop_transform();
    }

    // A short light that runs along a pill's outline, clockwise from the top.
    void draw_comet(gfx::DrawList &list, const Rect &r) const
    {
        constexpr float kPi = 3.14159265f;
        const float radius = r.h * 0.5f;
        const float straight = r.w - r.h;
        const float perimeter = 2.0f * straight + 2.0f * kPi * radius;
        const auto point = [&](float s, float *x, float *y) {
            s = std::fmod(s + perimeter, perimeter);
            if (s < straight)
            {
                *x = r.x + radius + s;
                *y = r.y;
                return;
            }
            s -= straight;
            if (s < kPi * radius)
            {
                const float a = -kPi * 0.5f + s / radius;
                *x = r.x + r.w - radius + std::cos(a) * radius;
                *y = r.cy() + std::sin(a) * radius;
                return;
            }
            s -= kPi * radius;
            if (s < straight)
            {
                *x = r.x + r.w - radius - s;
                *y = r.y + r.h;
                return;
            }
            s -= straight;
            const float a = kPi * 0.5f + s / radius;
            *x = r.x + radius + std::cos(a) * radius;
            *y = r.cy() + std::sin(a) * radius;
        };
        const float head = std::fmod((clock_ - prepare_since_) * 620.0f, perimeter);
        // Dots close enough to read as one stroke that thins into its tail.
        constexpr int kDots = 110;
        for (int i = kDots - 1; i >= 0; --i)
        {
            float x = 0.0f;
            float y = 0.0f;
            point(head - static_cast<float>(i) * 2.0f, &x, &y);
            const float t = 1.0f - static_cast<float>(i) / static_cast<float>(kDots);
            list.circle(x, y, 1.2f + 2.4f * t, Color::rgb(0xffffff, 0.15f + 0.85f * t * t));
        }
    }

    void draw_ring(gfx::DrawList &list) const
    {
        if (focus_.nav)
            return; // the pill itself turns white
        const Rect r = ring_.value();
        float radius = r.h * 0.5f;
        if (focus_.look == Look::tile)
            radius = 22.0f;
        else if (focus_.look == Look::cover)
            radius = 18.0f;
        else if (focus_.look == Look::row)
            radius = 26.0f;
        const float nudge = ui::shake(nudge_.value, clock_, 10.0f, 9.0f);
        const Rect ring{r.x + nudge, r.y, r.w, r.h};
        if (focus_.look != Look::cover) // a cover lights up from behind (draw_cover)
            list.glow(ring, radius, 30, kAccent.with_alpha(0.28f));
        list.bordered_rect(ring.inset(-6), radius + 6, kClear, 3.0f, kAccent);
    }

    // ---------------------------------------------------------------- backdrop and top bar

    void draw_art(gfx::DrawList &list, int index, float alpha) const
    {
        if (alpha <= 0.01f)
            return;
        const std::uint32_t texture = background_of(index);
        if (texture == 0)
            return;
        list.image(texture, {0, 0, gfx::kVirtualWidth, gfx::kVirtualHeight}, gfx::kFullUv,
                   Color::rgb(0xffffff, alpha));
    }

    void draw_backdrop(gfx::DrawList &list) const
    {
        const int art = background_;
        const bool more = screen_ == Screen::home && selected_ >= emulator_count();
        list.rounded_rect({0, 0, gfx::kVirtualWidth, gfx::kVirtualHeight}, 0, kBg);
        if (more)
        {
            list.glow({1100, 160, 520, 300}, 150, 420, kAccent.with_alpha(0.10f));
        }
        else
        {
            const float t = background_fade_.running
                                ? tween::smoothstep(background_fade_.progress())
                                : 1.0f;
            if (t < 1.0f)
                draw_art(list, previous_background_, 1.0f);
            draw_art(list, art, t);
        }
        // Scrims: the art reads on the right, the text on the left and the
        // shelves at the bottom stay on near-black.
        const float W = gfx::kVirtualWidth;
        const float H = gfx::kVirtualHeight;
        const Color b = kBg;
        list.gradient_rect_h({0, 0, 500, H}, 0, b, b.with_alpha(0.94f));
        list.gradient_rect_h({500, 0, 499, H}, 0, b.with_alpha(0.94f), b.with_alpha(0.55f));
        list.gradient_rect_h({999, 0, 499, H}, 0, b.with_alpha(0.55f), b.with_alpha(0.05f));
        list.rounded_rect({1498, 0, W - 1498, H}, 0, b.with_alpha(0.05f));
        list.gradient_rect({0, 626, W, 130}, 0, b.with_alpha(0.0f), b.with_alpha(0.9f));
        list.gradient_rect({0, 756, W, H - 756}, 0, b.with_alpha(0.9f), b);
        list.gradient_rect({0, 0, W, 216}, 0, b.with_alpha(0.85f), b.with_alpha(0.0f));
        if (screen_ == Screen::details)
            list.rounded_rect({0, 0, W, H}, 0, b.with_alpha(0.45f));
        else if (screen_ == Screen::games)
            list.rounded_rect({0, 0, W, H}, 0, b.with_alpha(0.62f));
        else if (screen_ != Screen::home)
            list.rounded_rect({0, 0, W, H}, 0, b.with_alpha(0.8f));
    }

    // The name in the top bar: the M of the logo, then "ANTA" spaced as the
    // rest of the bar's capitals (docs/design.md, Identidade).
    static constexpr float kMarkHeight = 30.0f;
    float mark_width() const
    {
        const emuland::Art &a = emuland::art();
        return a.mark != 0 ? kMarkHeight * static_cast<float>(a.mark_width) /
                                 static_cast<float>(a.mark_height)
                           : 0.0f;
    }
    float brand_width() const
    {
        const ui::FontRef &font = context_.fonts.semibold;
        if (mark_width() <= 0.0f)
            return font.measure("MANTA", 24, 7.7f);
        return mark_width() + 6.0f + font.measure("ANTA", 24, 7.7f);
    }
    void draw_brand(gfx::DrawList &list, float x, float baseline) const
    {
        const ui::FontRef &font = context_.fonts.semibold;
        const float w = mark_width();
        if (w <= 0.0f)
        {
            ui::text(list, font, "MANTA", x, baseline, 24, kText, gfx::Align::left, 7.7f);
            return;
        }
        // The M's feet sit on the baseline; its wing tips rise above the caps.
        list.image(emuland::art().mark, {x, baseline + 1.0f - kMarkHeight, w, kMarkHeight},
                   gfx::kFullUv, kText);
        ui::text(list, font, "ANTA", x + w + 6.0f, baseline, 24, kText, gfx::Align::left, 7.7f);
    }

    void draw_nav(gfx::DrawList &list) const
    {
        const ui::Fonts &fonts = context_.fonts;
        draw_brand(list, kMargin, 84);
        const Screen tab = screen_ == Screen::details ? Screen::home : screen_;
        const int updates = update_count();
        for (const Item &item : items_)
        {
            if (!item.nav)
                continue;
            const bool focus = focused(item);
            const bool on = item.a == static_cast<int>(tab);
            const Rect r = item.rect;
            list.push_transform(focus && !context_.settings.reduced_motion ? 1.06f : 1.0f, r.cx(),
                                r.cy(), 0, 0);
            if (focus)
                list.rounded_rect(r, 28, kText);
            else if (on)
                list.rounded_rect(r, 28, Color::rgb(0xffffff, 0.10f));
            const Color ink = focus ? kInk : on ? kText : kText2;
            ui::text(list, fonts.semibold, item.label, r.x + 28, r.cy() + 8.5f, 24, ink);
            if (item.a == static_cast<int>(Screen::updates) && updates > 0)
            {
                const float cx = r.x + r.w - 28 - 15;
                list.circle(cx, r.cy(), 15, kAccent);
                char count[8];
                std::snprintf(count, sizeof(count), "%d", updates);
                ui::text(list, fonts.semibold, count, cx, r.cy() + 6.5f, 18, kInk,
                         gfx::Align::center);
            }
            list.pop_transform();
        }
        char clock[16];
        std::snprintf(clock, sizeof(clock), "%02d:%02d", hour_, minute_);
        ui::text(list, fonts.semibold, clock, kRight, 84, 24, kText, gfx::Align::right);
    }

    // ---------------------------------------------------------------- home

    void draw_hero_content(gfx::DrawList &list, int index, float alpha, float slide) const
    {
        if (alpha <= 0.01f)
            return;
        const ui::Fonts &fonts = context_.fonts;
        const float x = kMargin + slide;
        list.push_opacity(alpha);
        if (index >= emulator_count())
        {
            list.bordered_rect({x, 168, 120, 120}, 28, kClear, 2, Color::rgb(0xffffff, 0.18f));
            list.line(x + 40, 228, x + 80, 228, 3, kText2);
            list.line(x + 60, 208, x + 60, 248, 3, kText2);
            ui::text(list, fonts.semibold, lang_ == Lang::pt ? "MAIS" : "MORE", x + 152, 205, 24,
                     kText2, gfx::Align::left, 3.4f);
            ui::text(list, fonts.display, L(kS.more_title), x + 148, 283, 88, kText);
            ui::text(list, fonts.regular, L(kS.more_none), x, 352, 26, kText2);
            list.push_opacity(1.0f - compact_.value);
            ui::paragraph(list, fonts.regular, L(kS.more_text), x, 420, 28, 860, 40,
                          kText.with_alpha(0.86f), 2);
            list.pop_opacity();
            list.pop_opacity();
            return;
        }
        const EmulatorSpec &emulator = spec(index);
        if (const std::uint32_t texture = icon_of(index))
        {
            list.image(texture, {x, 168, 120, 120}, gfx::kFullUv, kText, 28);
            list.bordered_rect({x, 168, 120, 120}, 28, kClear, 1, Color::rgb(0xffffff, 0.12f));
        }
        std::string system = L(emulator.system);
        ui::text(list, fonts.semibold, ui::upper(system), x + 152, 205, 24, kText2,
                 gfx::Align::left, 3.4f);
        ui::text(list, fonts.display, emulator.name, x + 148, 283, 88, kText);
        const std::string version = is_installed(index)          ? installed_text(index)
                                    : state(index).latest.empty() ? std::string()
                                                                  : pretty_version(state(index).latest);
        float cursor = x;
        if (!version.empty())
        {
            cursor += ui::text(list, fonts.semibold, version, cursor, 352, 26, kText);
            cursor += ui::text(list, fonts.regular, "  ·  ", cursor, 352, 26, kText2);
        }
        std::string meta;
        if (emulator.prerelease)
            meta += std::string(L(kS.prerelease)) + "  ·  ";
        meta += std::string(emulator.author) + "  ·  " + emulator.license;
        ui::text(list, fonts.regular, meta, cursor, 352, 26, kText2);
        float cx = x;
        for (const ChipSpec &c : status_chips(index))
            cx += chip(list, cx, 376, c.icon, c.tone, c.label) + 14.0f;
        list.push_opacity(1.0f - compact_.value);
        ui::paragraph(list, fonts.regular, L(emulator.description), x, 470, 28, 860, 40,
                      kText.with_alpha(0.86f), 2);
        list.pop_opacity();
        list.pop_opacity();
    }

    void draw_home(gfx::DrawList &list) const
    {
        // Billboard: the new emulator slides in a beat after the old one left.
        if (hero_.running)
        {
            const float t = hero_.progress();
            const float arrive = tween::clamp01((t - 0.2f) / 0.8f);
            draw_hero_content(list, shown_, tween::smoothstep(arrive),
                              28.0f * (1.0f - tween::quint_out(arrive)));
        }
        else
        {
            draw_hero_content(list, shown_, 1.0f, 0.0f);
        }
        list.push_opacity(1.0f - compact_.value);
        for (const Item &item : items_)
        {
            if (!item.nav && item.row < 0)
                draw_button(list, item);
        }
        list.pop_opacity();

        // Shelves: titles, then cards, the focused card last so it sits on top.
        const ui::Fonts &fonts = context_.fonts;
        for (int r = 0; r < static_cast<int>(shelves_.size()); ++r)
        {
            const float y = kShelfTop + static_cast<float>(r) * kShelfPitch + rows_y_.value;
            // A shelf fades as it rises into the billboard's text.
            float visible = tween::clamp01((y - 400.0f) / 140.0f);
            if (r > std::max(row_, 0))
                visible *= 0.5f; // the shelf below only peeks in
            if (visible <= 0.01f)
                continue;
            list.push_opacity(visible);
            ui::text(list, fonts.semibold, L(*shelves_[static_cast<std::size_t>(r)].title), kMargin,
                     y + 26, 26, kText.with_alpha(r == row_ || (row_ < 0 && r == 0) ? 1.0f : 0.75f));
            for (const Item &item : items_)
            {
                if (item.row == r && !focused(item))
                    draw_tile(list, item);
            }
            list.pop_opacity();
        }
        for (const Item &item : items_)
        {
            if (item.row >= 0 && focused(item))
                draw_tile(list, item);
        }
        // The hints sit on a fade so the next shelf's cards do not crowd them.
        list.gradient_rect({0, 880, gfx::kVirtualWidth, 110}, 0, kBg.with_alpha(0.0f),
                           kBg.with_alpha(0.97f));
        list.rounded_rect({0, 990, gfx::kVirtualWidth, 90}, 0, kBg.with_alpha(0.97f));
    }

    void draw_tile(gfx::DrawList &list, const Item &item) const
    {
        const Rect r = item.rect;
        if (r.x > gfx::kVirtualWidth || r.x + r.w < -120.0f)
            return;
        const bool focus = focused(item);
        const float nudge = focus ? ui::shake(nudge_.value, clock_, 10.0f, 9.0f) : 0.0f;
        list.push_transform(focus ? focus_scale(Look::tile) : 1.0f, r.cx(), r.cy(), nudge, 0);
        const ui::Fonts &fonts = context_.fonts;
        const Shelf &shelf = shelves_[static_cast<std::size_t>(item.row)];
        const Tile &tile = shelf.tiles[static_cast<std::size_t>(item.col)];
        if (tile.kind == TileKind::more)
        {
            list.bordered_rect(r, 20, kClear, 2, Color::rgb(0xffffff, 0.16f));
            list.line(r.cx() - 26, r.cy() - 18, r.cx() + 26, r.cy() - 18, 3, kText2);
            list.line(r.cx(), r.cy() - 44, r.cx(), r.cy() + 8, 3, kText2);
            ui::text(list, fonts.regular, L(kS.more_title), r.cx(), r.cy() + 56, 24, kText2,
                     gfx::Align::center);
            list.pop_transform();
            return;
        }
        const int index = tile.emulator;
        if (tile.kind == TileKind::folder)
        {
            const emuland::FolderSpec &folder =
                spec(index).folders[static_cast<std::size_t>(tile.folder)];
            list.bordered_rect(r, 20, Color::rgb(0x151a23, 0.92f), 1, kLine);
            if (const std::uint32_t texture = icon_of(index))
                list.image(texture, {r.x + 24, r.y + 22, 40, 40}, gfx::kFullUv, kText, 11);
            const std::string owner = std::string(spec(index).short_name) + "  ·  " + spec(index).name;
            ui::text(list, fonts.regular, owner, r.x + 76, r.y + 49, 20, kText2);
            ui::text(list, fonts.semibold, L(folder.name), r.x + 24, r.y + 140, 30, kText);
            Icon kind = Icon::dash;
            Color tone = kText2;
            std::string label;
            folder_status(index, tile.folder, &kind, &tone, &label);
            chip(list, r.x + 24, r.y + 158, kind, tone, label, 40, 20);
            list.pop_transform();
            return;
        }
        const bool off = !is_installed(index);
        const std::uint32_t background = background_of(index);
        if (background != 0)
            list.image(background, r, gfx::kFullUv,
                       off ? Color::rgb(0xb0b4ba) : Color::rgb(0xffffff), 20);
        else
            list.rounded_rect(r, 20, kSurface2);
        list.gradient_rect({r.x, r.y + r.h * 0.3f, r.w, r.h * 0.7f}, 20,
                           Color::rgb(0x05070a, 0.0f), Color::rgb(0x05070a, 0.92f));
        if (const std::uint32_t texture = icon_of(index))
        {
            list.image(texture, {r.x + 20, r.y + r.h - 92, 72, 72}, gfx::kFullUv, kText, 18);
            list.bordered_rect({r.x + 20, r.y + r.h - 92, 72, 72}, 18, kClear, 1,
                               Color::rgb(0xffffff, 0.12f));
        }
        ui::text(list, fonts.semibold, spec(index).name, r.x + 108, r.y + r.h - 50, 26, kText);
        ui::text(list, fonts.regular, fit(L(spec(index).system), 20, r.w - 128), r.x + 108,
                 r.y + r.h - 24, 20, kText2);
        // Badge in the corner.
        std::string badge;
        Color tone = kAccent;
        if (tile.kind == TileKind::news)
        {
            const std::string latest =
                state(index).latest.empty() ? std::string() : pretty_version(state(index).latest);
            badge = std::string(is_installed(index) ? L(kS.badge_update) : L(kS.badge_new)) +
                    (latest.empty() ? "" : "  ·  " + latest);
        }
        else if (off)
        {
            badge = installable(index) ? L(kS.not_installed) : L(kS.source_only);
            tone = kText2;
        }
        else if (update_for(index))
        {
            badge = L(kS.badge_update);
        }
        if (!badge.empty())
        {
            const float tw = fonts.semibold.measure(badge, 18);
            const bool dot = tone.r == kAccent.r;
            const float w = 28.0f + tw + (dot ? 17.0f : 0.0f);
            list.rounded_rect({r.x + 16, r.y + 16, w, 36}, 18, Color::rgb(0x0b0e14, 0.82f));
            float tx = r.x + 30;
            if (dot)
            {
                list.circle(tx + 4, r.y + 34, 4.5f, tone);
                tx += 17;
            }
            ui::text(list, fonts.semibold, badge, tx, r.y + 40, 18, tone);
        }
        list.pop_transform();
    }

    // ---------------------------------------------------------------- games

    std::string size_text(std::uint64_t bytes) const
    {
        char text[32];
        if (bytes >= 1000000000ull)
            std::snprintf(text, sizeof(text), "%.1f GB", static_cast<double>(bytes) / 1e9);
        else
            std::snprintf(text, sizeof(text), "%d MB", static_cast<int>(bytes / 1000000ull));
        std::string out = text;
        if (lang_ == Lang::pt)
        {
            for (char &c : out)
                c = c == '.' ? ',' : c;
        }
        return out;
    }

    const char *region_text(const std::string &region) const
    {
        static const std::pair<const char *, Text> kNames[] = {
            {"USA", {"EUA", "USA"}},           {"Europe", {"Europa", "Europe"}},
            {"Japan", {"Japão", "Japan"}},     {"Brazil", {"Brasil", "Brazil"}},
            {"World", {"Mundo", "World"}},     {"Korea", {"Coreia", "Korea"}},
            {"Asia", {"Ásia", "Asia"}},        {"Australia", {"Austrália", "Australia"}},
        };
        for (const auto &[raw, name] : kNames)
        {
            if (region == raw)
                return L(name);
        }
        return region.c_str();
    }

    // A cover the user has none for: the middle of the emulator's own art,
    // dimmed, its icon, and the title set large, as Apple TV's text posters.
    // peek: a row below the focused one shows its art only.
    void draw_cover(gfx::DrawList &list, const Item &item, float alpha, bool peek) const
    {
        const Rect r = item.rect;
        const emuland::Game &g = game(item.a);
        const bool focus = focused(item);
        const float nudge = focus ? ui::shake(nudge_.value, clock_, 10.0f, 9.0f) : 0.0f;
        list.push_opacity(alpha);
        list.push_transform(focus ? focus_scale(Look::cover) : 1.0f, r.cx(), r.cy(), nudge, 0);
        if (focus)
            list.glow(r, 16, 30, kAccent.with_alpha(0.28f));
        const ui::Fonts &fonts = context_.fonts;
        const emuland::Cover cover = emuland::cover_of(g.path);
        const float aspect =
            cover.texture != 0 ? static_cast<float>(cover.width) / static_cast<float>(cover.height)
                               : 0.0f;
        const float card = kCoverW / kCoverH;
        if (cover.texture != 0 && std::fabs(aspect - card) < 0.12f)
        {
            // A box like the card's: fills it, the small difference cut evenly.
            Rect uv = gfx::kFullUv;
            if (aspect > card)
                uv = {0.5f - card / aspect * 0.5f, 0.0f, card / aspect, 1.0f};
            else
                uv = {0.0f, 0.5f - aspect / card * 0.5f, 1.0f, aspect / card};
            list.image(cover.texture, r, uv, kText, 16);
        }
        else if (cover.texture != 0)
        {
            // Another shape (a PS1 jewel case is square): whole, on the card's
            // dark ground, the title under it when there is room.
            list.rounded_rect(r, 16, kSurface);
            const float w = r.w - 24.0f;
            const float h = std::min(w / aspect, r.h - 24.0f);
            const float iw = h * aspect;
            const Rect at{r.cx() - iw * 0.5f, r.y + 12.0f, iw, h};
            list.image(cover.texture, at, gfx::kFullUv, kText, 8);
            const float room = r.y + r.h - (at.y + at.h);
            if (!peek && room > 50.0f)
            {
                const ui::ParagraphLines lines = ui::paragraph_lines(
                    fonts.semibold, g.title, 20, r.w - 32, room > 80.0f ? 2 : 1);
                ui::paragraph(list, fonts.semibold, lines, r.x + 16, at.y + at.h + 34, 20, 25,
                              kText);
            }
        }
        else
        {
            list.rounded_rect(r, 16, kSurface2);
            if (const std::uint32_t art = background_of(g.emulator))
            {
                // The same slice on every card of a system, so the titles carry it.
                const float span = (kCoverW / kCoverH) * (9.0f / 16.0f);
                list.image(art, r, {0.5f - span * 0.5f, 0.0f, span, 1.0f}, Color::rgb(0x5c636d),
                           16);
            }
            list.gradient_rect({r.x, r.y + r.h * 0.25f, r.w, r.h * 0.75f}, 16,
                               Color::rgb(0x05070a, 0.15f), Color::rgb(0x05070a, 0.94f));
            if (const std::uint32_t texture = icon_of(g.emulator))
            {
                list.image(texture, {r.x + 18, r.y + 18, 40, 40}, gfx::kFullUv, kText, 11);
                list.bordered_rect({r.x + 18, r.y + 18, 40, 40}, 11, kClear, 1,
                                   Color::rgb(0xffffff, 0.14f));
            }
            ui::text(list, fonts.semibold, spec(g.emulator).short_name, r.x + 68, r.y + 45, 18,
                     kText.with_alpha(0.8f));
            if (!peek)
            {
                const ui::ParagraphLines lines =
                    ui::paragraph_lines(fonts.semibold, g.title, 26, r.w - 40, 4);
                const float top =
                    r.y + r.h - 26.0f - static_cast<float>(lines.lines.size() - 1) * 32.0f;
                ui::paragraph(list, fonts.semibold, lines, r.x + 20, top, 26, 32, kText);
            }
        }
        list.bordered_rect(r, 16, kClear, 1, Color::rgb(0xffffff, 0.10f));
        list.pop_transform();
        list.pop_opacity();
    }

    void draw_games(gfx::DrawList &list) const
    {
        const ui::Fonts &fonts = context_.fonts;
        if (games().empty())
        {
            ui::text(list, fonts.semibold, L(kS.library_eyebrow), kMargin, 197, 24, kText2,
                     gfx::Align::left, 3.4f);
            ui::text(list, fonts.display, L(kS.no_games_title), kMargin - 2, 270, 64, kText);
            const bool any = !installed_list().empty();
            ui::paragraph(list, fonts.regular, L(any ? kS.no_games_text : kS.no_emulator_games),
                          kMargin, 360, 28, 1100, 42, kText2, 4);
            for (const Item &item : items_)
            {
                if (!item.nav)
                    draw_button(list, item);
            }
            return;
        }
        // The header names what has the focus: the game, or the filter.
        const float in =
            folder_switch_.running ? tween::cubic_out(folder_switch_.progress()) : 1.0f;
        if (focus_.act == Act::play_game && !focus_.nav)
        {
            const emuland::Game &g = game(focus_.a);
            const EmulatorSpec &emulator = spec(g.emulator);
            ui::text(list, fonts.semibold,
                     ui::upper(L(emulator.system)) + "  ·  " + emulator.name, kMargin, 197, 24,
                     kText2, gfx::Align::left, 3.4f);
            ui::text(list, fonts.display, fit(fonts.display, g.title, 64, kRight - kMargin),
                     kMargin - 2, 270, 64, kText);
            std::string meta = size_text(g.size);
            if (!g.format.empty())
                meta += "  ·  " + g.format;
            if (!g.region.empty())
                meta += std::string("  ·  ") + region_text(g.region);
            ui::text(list, fonts.regular, meta, kMargin, 322, 28, kText2);
        }
        else
        {
            const std::string title =
                game_filter_ < 0 ? L(kS.all_games) : L(spec(game_filter_).system);
            ui::text(list, fonts.semibold, L(kS.library_eyebrow), kMargin, 197, 24, kText2,
                     gfx::Align::left, 3.4f);
            ui::text(list, fonts.display, title, kMargin - 2, 270, 64, kText);
            ui::text(list, fonts.regular, games_text(games_of(game_filter_)), kMargin, 322, 28,
                     kText2);
        }
        // Filters.
        for (const Item &item : items_)
        {
            if (item.nav || item.look != Look::folder_tab)
                continue;
            const bool focus = focused(item);
            const bool on = item.a == game_filter_;
            const Rect r = item.rect;
            list.push_transform(focus ? focus_scale(Look::folder_tab) : 1.0f, r.cx(), r.cy(), 0,
                                0);
            if (on)
                list.rounded_rect(r, 30, kText);
            else
                list.bordered_rect(r, 30, Color::rgb(0x151a23, 0.7f), 1, kLine);
            float x = r.x + 28;
            if (item.a >= 0)
            {
                if (const std::uint32_t texture = icon_of(item.a))
                    list.image(texture, {r.x + 9, r.y + 9, 42, 42}, gfx::kFullUv, kText, 21);
                x = r.x + 64;
            }
            x += ui::text(list, fonts.semibold, item.label, x, r.cy() + 8, 22, on ? kInk : kText);
            char count[16];
            std::snprintf(count, sizeof(count), "%d", games_of(item.a));
            ui::text(list, fonts.regular, count, x + 12, r.cy() + 8, 22,
                     on ? kInk.with_alpha(0.6f) : kText2);
            list.pop_transform();
        }
        // The grid: rows that scroll up past the filters fade out; the
        // focused cover is drawn last, on top of its neighbours.
        list.push_opacity(in);
        list.push_transform(1.0f, 0, 0, 0, 14.0f * (1.0f - in));
        const auto visible = [](const Rect &r) { return tween::clamp01((r.y - 360.0f) / 100.0f); };
        for (const Item &item : items_)
        {
            if (item.look == Look::cover && !focused(item) && item.rect.y < gfx::kVirtualHeight)
            {
                // The row below the focused one only peeks in, as on Home.
                const bool peek = item.b > games_row_;
                const float alpha = visible(item.rect) * (peek ? 0.5f : 1.0f);
                if (alpha > 0.01f)
                    draw_cover(list, item, alpha, peek);
            }
        }
        for (const Item &item : items_)
        {
            if (item.look == Look::cover && focused(item))
                draw_cover(list, item, visible(item.rect), false);
        }
        list.pop_transform();
        list.pop_opacity();
        list.gradient_rect({0, 880, gfx::kVirtualWidth, 110}, 0, kBg.with_alpha(0.0f),
                           kBg.with_alpha(0.97f));
        list.rounded_rect({0, 990, gfx::kVirtualWidth, 90}, 0, kBg.with_alpha(0.97f));
    }

    // ---------------------------------------------------------------- details

    void draw_page_header(gfx::DrawList &list, int index, const std::string &eyebrow,
                          const std::string &title) const
    {
        const ui::Fonts &fonts = context_.fonts;
        float x = kMargin;
        if (index >= 0)
        {
            if (const std::uint32_t texture = icon_of(index))
            {
                list.image(texture, {kMargin, 160, 104, 104}, gfx::kFullUv, kText, 24);
                list.bordered_rect({kMargin, 160, 104, 104}, 24, kClear, 1,
                                   Color::rgb(0xffffff, 0.12f));
            }
            x += 132;
        }
        ui::text(list, fonts.semibold, eyebrow, x, 197, 24, kText2, gfx::Align::left, 3.4f);
        ui::text(list, fonts.display, title, x - 2, 258, 64, kText);
    }

    void panel(gfx::DrawList &list, const Rect &r) const
    {
        list.bordered_rect(r, 32, Color::rgb(0x151a23, 0.88f), 1, kLine);
    }

    void panel_title(gfx::DrawList &list, const std::string &title, float x, float y) const
    {
        ui::text(list, context_.fonts.semibold, title, x, y, 22, kText2, gfx::Align::left, 3.1f);
    }

    void draw_details(gfx::DrawList &list) const
    {
        const int index = selected_;
        const EmulatorSpec &emulator = spec(index);
        const ui::Fonts &fonts = context_.fonts;
        draw_page_header(list, index, ui::upper(L(emulator.system)), emulator.name);
        ui::paragraph(list, fonts.regular, L(emulator.description), kMargin, 330, 28, 1300, 40,
                      kText2, 2);
        for (const Item &item : items_)
        {
            if (!item.nav)
                draw_button(list, item);
        }
        // Highlights.
        panel(list, {kMargin, 492, 1000, 262});
        panel_title(list, L(kS.highlights), kMargin + 40, 552);
        for (int i = 0; i < 3; ++i)
        {
            const float y = 590.0f + static_cast<float>(i) * 52.0f;
            icon(list, Icon::check, kMargin + 40, y, 26, kAccent);
            ui::text(list, fonts.regular, L(emulator.highlights[i]), kMargin + 80, y + 21, 26, kText);
        }
        // Requirements.
        panel(list, {kMargin, 778, 1000, 168});
        float tx = kMargin + 40;
        tx += ui::text(list, fonts.semibold, L(kS.requirements), tx, 838, 22, kText2,
                       gfx::Align::left, 3.1f);
        ui::text(list, fonts.regular, std::string("  ·  ") + L(kS.requirements_note), tx, 838, 22,
                 kText2);
        float cx = kMargin + 40;
        for (const char *requirement : emulator.requirements)
            cx += chip(list, cx, 862, Icon::check, kOk, requirement) + 14.0f;
        // Version.
        const Rect v{1168, 380, 656, 470};
        panel(list, v);
        panel_title(list, L(kS.version), v.x + 40, v.y + 60);
        const std::string installed =
            is_installed(index) ? installed_text(index) : std::string("—");
        const std::string latest = state(index).latest.empty()
                                       ? std::string(L(kS.unknown))
                                       : pretty_version(state(index).latest);
        const std::pair<const Text *, std::string> rows[] = {
            {&kS.installed_label, installed},
            {&kS.available_label, latest},
            {&kS.channel, L(emulator.prerelease ? kS.prerelease : kS.stable)},
            {&kS.author, emulator.author},
            {&kS.license, emulator.license},
            {&kS.title_id, emulator.title_id},
            {&kS.repository, std::string("github.com/") + emulator.repo},
        };
        float y = v.y + 116;
        for (const auto &[label, value] : rows)
        {
            ui::text(list, fonts.regular, L(*label), v.x + 40, y, 24, kText2);
            if (label == &kS.repository)
            {
                // Long names: the address gets a line of its own.
                ui::text(list, fonts.mono, value, v.x + 40, y + 38, 20, kText);
                break;
            }
            const bool mono = label == &kS.title_id;
            ui::text(list, mono ? fonts.mono : fonts.semibold, value, v.x + 240, y,
                     mono ? 21 : 24, kText);
            y += 46;
        }
    }

    // ---------------------------------------------------------------- folders

    void draw_folders(gfx::DrawList &list) const
    {
        const ui::Fonts &fonts = context_.fonts;
        const EmulatorSpec &emulator = spec(folder_emulator_);
        // A new emulator's folders rise into place instead of popping in.
        const float in =
            folder_switch_.running ? tween::cubic_out(folder_switch_.progress()) : 1.0f;
        const std::string title = std::string(L(emulator.system)) + "  ·  " + emulator.name;
        list.push_opacity(in);
        ui::text(list, fonts.display, title, kMargin - 2 + 16.0f * (1.0f - in), 232, 64, kText);
        list.pop_opacity();
        for (const Item &item : items_)
        {
            if (item.nav)
                continue;
            const bool moving = item.look != Look::folder_tab;
            if (moving)
            {
                list.push_opacity(in);
                list.push_transform(1.0f, 0, 0, 0, 14.0f * (1.0f - in));
            }
            const bool focus = focused(item);
            const Rect r = item.rect;
            if (item.look == Look::folder_tab)
            {
                const bool on = item.a == folder_emulator_;
                list.push_transform(focus ? focus_scale(Look::folder_tab) : 1.0f, r.cx(), r.cy(),
                                    0, 0);
                if (on)
                    list.rounded_rect(r, 30, kText);
                else
                    list.bordered_rect(r, 30, Color::rgb(0x151a23, 0.7f), 1, kLine);
                if (const std::uint32_t texture = icon_of(item.a))
                    list.image(texture, {r.x + 9, r.y + 9, 42, 42}, gfx::kFullUv, kText, 21);
                ui::text(list, fonts.semibold, item.label, r.x + 64, r.cy() + 8, 22,
                         on ? kInk : kText2);
                list.pop_transform();
            }
            else if (item.look == Look::row)
            {
                const emuland::FolderSpec &f = emulator.folders[static_cast<std::size_t>(item.b)];
                list.push_transform(focus ? focus_scale(Look::row) : 1.0f, r.cx(), r.cy(), 0, 0);
                list.bordered_rect(r, 24, Color::rgb(0x151a23, 0.88f), 1, kLine);
                icon(list, Icon::folder, r.x + 28, r.cy() - 22, 44, kText2);
                ui::text(list, fonts.semibold, L(f.name), r.x + 100, r.y + 33, 28, kText);
                const std::string path = std::string(emulator.data) + "/" + f.path;
                float sx = r.x + 100;
                sx += ui::text(list, fonts.mono, path, sx, r.y + 62, 20, kText2);
                ui::text(list, fonts.regular, std::string("  ·  ") + L(f.note), sx, r.y + 62, 22,
                         kText2);
                Icon kind = Icon::dash;
                Color tone = kText2;
                std::string label;
                folder_status(folder_emulator_, item.b, &kind, &tone, &label);
                const float w = 20.0f + 22.0f + 10.0f + fonts.regular.measure(label, 22) + 20.0f;
                chip(list, r.x + r.w - 28 - w, r.cy() - 23, kind, tone, label);
                list.pop_transform();
            }
            else
            {
                draw_button(list, item);
            }
            if (moving)
            {
                list.pop_transform();
                list.pop_opacity();
            }
        }
        // The layout expected on a USB drive.
        list.push_opacity(in);
        list.push_transform(1.0f, 0, 0, 0, 14.0f * (1.0f - in));
        const Rect p = folder_panel_rect();
        panel(list, p);
        panel_title(list, L(kS.usb_layout), p.x + 40, p.y + 60);
        float y = p.y + 112;
        ui::text(list, fonts.mono, std::string("Manta/") + emulator.usb + "/", p.x + 40, y, 22,
                 kText);
        for (std::size_t f = 0; f < emulator.folders.size(); ++f)
        {
            y += 33;
            const bool last = f + 1 == emulator.folders.size();
            ui::text(list, fonts.mono, std::string(last ? "`-- " : "|-- ") + emulator.folders[f].key + "/",
                     p.x + 40, y, 22, kText2);
        }
        ui::paragraph(list, fonts.regular, L(kS.usb_text), p.x + 40, y + 52, 22, 420, 31, kText2,
                      2);
        list.pop_transform();
        list.pop_opacity();
    }

    // ---------------------------------------------------------------- updates

    void draw_updates(gfx::DrawList &list) const
    {
        const ui::Fonts &fonts = context_.fonts;
        const int n = update_count();
        ui::text(list, fonts.semibold, L(kS.updates_eyebrow), kMargin, 197, 24, kText2,
                 gfx::Align::left, 3.4f);
        ui::text(list, fonts.display, n > 0 ? updates_text(n) : L(kS.all_up_to_date), kMargin - 2,
                 270, 64, kText);
        std::string lead;
        if (working(job_.stage) && job_.emulator < 0)
            lead = L(kS.checking);
        else if (job_.checked_hour >= 0)
        {
            char text[64];
            std::snprintf(text, sizeof(text), "%s %02d:%02d", L(kS.last_check), job_.checked_hour,
                          job_.checked_minute);
            lead = text;
        }
        else if (job_.error == emuland::JobError::rate_limited && job_.retry_hour >= 0)
        {
            char text[96];
            std::snprintf(text, sizeof(text), "%s %02d:%02d", L(kS.limit_lead), job_.retry_hour,
                          job_.retry_minute);
            lead = text;
        }
        else if (job_.error == emuland::JobError::network)
            lead = L(kS.offline);
        else
            lead = L(kS.never_checked);
        ui::text(list, fonts.regular, lead, kMargin, 322, 28, kText2);
        const std::vector<int> rows = installed_list();
        if (rows.empty())
            ui::text(list, fonts.regular, L(kS.none_installed), kMargin, 520, 28, kText2);
        for (std::size_t row = 0; row < rows.size(); ++row)
        {
            const int i = rows[row];
            const float pitch = update_pitch(rows.size());
            const Rect r{kMargin, 472.0f + static_cast<float>(row) * pitch, 1300,
                         std::min(96.0f, pitch - 10.0f)};
            list.bordered_rect(r, 24, Color::rgb(0x151a23, 0.88f), 1, kLine);
            if (const std::uint32_t texture = icon_of(i))
                list.image(texture, {r.x + 14, r.y + 12, 72, 72}, gfx::kFullUv, kText, 18);
            float x = r.x + 112;
            x += ui::text(list, fonts.semibold, spec(i).name, x, r.y + 42, 28, kText);
            ui::text(list, fonts.regular, std::string("  ·  ") + spec(i).short_name, x, r.y + 42,
                     28, kText2);
            std::string sub;
            if (!is_installed(i))
            {
                sub = L(kS.not_installed);
                if (!state(i).latest.empty())
                    sub += "  ·  " + pretty_version(state(i).latest);
            }
            else if (update_for(i))
            {
                sub = installed_text(i) + "  →  " + pretty_version(state(i).latest);
            }
            else
            {
                sub = installed_text(i) + "  ·  " +
                      (state(i).latest.empty() ? L(kS.installed_chip) : L(kS.up_to_date));
            }
            ui::text(list, fonts.regular, sub, r.x + 112, r.y + 76, 22, kText2);
            if (is_installed(i) && !update_for(i))
                icon(list, Icon::check, r.x + r.w - 56, r.cy() - 13, 26, kOk);
        }
        for (const Item &item : items_)
        {
            if (!item.nav)
                draw_button(list, item);
        }
    }

    // ---------------------------------------------------------------- settings

    std::string setting_value(Setting setting) const
    {
        switch (setting)
        {
        case Setting::language:
            return language_choice_ == 1   ? "Português"
                   : language_choice_ == 2 ? "English"
                                           : L(kS.automatic);
        case Setting::reduce_motion:
            return L(context_.settings.reduced_motion ? kS.on : kS.off);
        case Setting::sounds:
            return L(sounds_on_ ? kS.on : kS.off);
        case Setting::covers:
            return L(cover_choice_ ? kS.on : kS.off);
        case Setting::resolution:
            return Settings::kResolutions[context_.settings.resolution %
                                          Settings::kResolutionCount]
                .label;
        default:
            return {};
        }
    }

    void draw_settings(gfx::DrawList &list) const
    {
        const ui::Fonts &fonts = context_.fonts;
        ui::text(list, fonts.semibold, "MANTA", kMargin, 197, 24, kText2, gfx::Align::left, 3.4f);
        ui::text(list, fonts.display, L(kS.settings_title), kMargin - 2, 270, 64, kText);
        const Text *titles[] = {&kS.language, &kS.reduce_motion, &kS.sounds,        &kS.resolution,
                                &kS.about,    &kS.exit,          &kS.covers_setting};
        for (const Item &item : items_)
        {
            if (item.nav)
                continue;
            const Rect r = item.rect;
            const bool focus = focused(item);
            list.push_transform(focus ? focus_scale(Look::row) : 1.0f, r.cx(), r.cy(), 0, 0);
            list.bordered_rect(r, 24, Color::rgb(0x151a23, 0.88f), 1, kLine);
            ui::text(list, fonts.regular, L(*titles[item.a]), r.x + 36, r.cy() + 10, 28, kText);
            const std::string value = setting_value(static_cast<Setting>(item.a));
            float right = r.x + r.w - 36;
            // A chevron drawn with two strokes ("›" is not in the baked set).
            list.line(right - 10, r.cy() - 9, right, r.cy(), 2.5f, kText2);
            list.line(right, r.cy(), right - 10, r.cy() + 9, 2.5f, kText2);
            right -= 30;
            if (!value.empty())
                ui::text(list, fonts.regular, value, right, r.cy() + 9, 26, kText2,
                         gfx::Align::right);
            list.pop_transform();
        }
        char version[64];
        std::snprintf(version, sizeof(version), "Manta v%s", emuland::kVersion);
        ui::text(list, fonts.regular, version, kMargin, 870, 22, kText2);
    }

    // ---------------------------------------------------------------- overlays

    void draw_dialog(gfx::DrawList &list) const
    {
        const ui::Fonts &fonts = context_.fonts;
        const float t = dialog_show_.value;
        list.push_opacity(tween::clamp01(t * 1.3f));
        list.rounded_rect({0, 0, gfx::kVirtualWidth, gfx::kVirtualHeight}, 0,
                          Color::rgb(0x05070a, 0.72f));
        const ui::ParagraphLines body =
            ui::paragraph_lines(fonts.regular, dialog_.body, 28, 912, 12);
        float height = 56 + 64 + 18 + static_cast<float>(body.lines.size()) * 40 + 44 + kButtonH + 56;
        if (dialog_.credits)
            height += 40 + static_cast<float>(emulator_count()) * 60 + 50;
        const Rect d{440, 540 - height * 0.5f + 24.0f * (1.0f - t), 1040, height};
        list.shadow({d.x, d.y + 20, d.w, d.h}, 40, 60, Color::rgb(0x000000, 0.5f));
        list.bordered_rect(d, 40, kSurface, 1, kLine);
        float y = d.y + 56 + 50;
        ui::text(list, fonts.display, dialog_.title, d.x + 64, y, 52, kText);
        y += 18 + 40;
        y = ui::paragraph(list, fonts.regular, body, d.x + 64, y, 28, 40, kText2);
        if (dialog_.credits)
        {
            y += 10;
            for (int i = 0; i < emulator_count(); ++i)
            {
                if (const std::uint32_t texture = icon_of(i))
                    list.image(texture, {d.x + 64, y - 6, 44, 44}, gfx::kFullUv, kText, 12);
                ui::text(list, fonts.semibold, spec(i).name, d.x + 128, y + 26, 24, kText);
                ui::text(list, fonts.regular,
                         std::string(spec(i).author) + "  ·  " + spec(i).license, d.x + 340,
                         y + 26, 24, kText2);
                y += 60;
            }
            ui::text(list, fonts.regular, L(kS.art_credit), d.x + 64, y + 20, 21, kText2);
            y += 50;
        }
        float bx = d.x + 64;
        const float by = d.y + d.h - 56 - kButtonH;
        for (int i = 0; i < static_cast<int>(dialog_.buttons.size()); ++i)
        {
            Item button{Act::dialog, i, 0, {}, i == 0 ? Look::primary : Look::button, -1, -1,
                        dialog_.buttons[static_cast<std::size_t>(i)].label};
            button.rect = {bx, by, button_width(button.label, false), kButtonH};
            bx += button.rect.w + 20;
            draw_button(list, button);
            if (i == dialog_focus_)
            {
                const Rect r = button.rect;
                list.bordered_rect(r.inset(-6), r.h * 0.5f + 6, kClear, 3, kAccent);
            }
        }
        list.pop_opacity();
    }

    void draw_toast(gfx::DrawList &list) const
    {
        if (toast_time_ <= 0.0f)
            return;
        const float alpha = tween::clamp01(std::min(toast_time_ / 0.25f, (2.8f - toast_time_) / 0.2f));
        const ui::Fonts &fonts = context_.fonts;
        const float tw = fonts.regular.measure(toast_text_, 24);
        const Rect r{960 - (tw + 72) * 0.5f, 128 - 16 * (1.0f - alpha), tw + 72, 62};
        list.push_opacity(alpha);
        list.rounded_rect(r, 31, kText);
        ui::text(list, fonts.regular, toast_text_, r.cx(), r.cy() + 8.5f, 24, kInk,
                 gfx::Align::center);
        list.pop_opacity();
    }

    std::string megabytes(std::uint64_t bytes) const
    {
        char text[32];
        std::snprintf(text, sizeof(text), "%.1f", static_cast<double>(bytes) / 1e6);
        std::string out = text;
        if (lang_ == Lang::pt)
        {
            for (char &c : out)
                c = c == '.' ? ',' : c;
        }
        return out;
    }

    // The install in progress: one ring for the current step, the steps
    // beside it, and Circle to cancel (nothing changes until the last step).
    void draw_job(gfx::DrawList &list) const
    {
        const int index = install_label_;
        if (index < 0 || index >= emulator_count())
            return;
        const ui::Fonts &fonts = context_.fonts;
        const EmulatorSpec &emulator = spec(index);
        list.push_opacity(tween::clamp01(job_show_.value * 1.3f));
        list.rounded_rect({0, 0, gfx::kVirtualWidth, gfx::kVirtualHeight}, 0,
                          Color::rgb(0x05070a, 0.72f));
        const Rect d{400, 230 + 24.0f * (1.0f - job_show_.value), 1120, 600};
        list.shadow({d.x, d.y + 20, d.w, d.h}, 40, 60, Color::rgb(0x000000, 0.5f));
        list.bordered_rect(d, 40, kSurface, 1, kLine);

        if (const std::uint32_t texture = icon_of(index))
            list.image(texture, {d.x + 64, d.y + 56, 88, 88}, gfx::kFullUv, kText, 22);
        if (job_.import)
        {
            ui::text(list, fonts.display, std::string(L(kS.importing_title)) + " " + emulator.name,
                     d.x + 176, d.y + 106, 44, kText);
            ui::text(list, fonts.regular,
                     std::string("Manta/") + emulator.usb + "/  →  " + emulator.data, d.x + 176,
                     d.y + 140, 22, kText2);
            const float cx = d.x + 190;
            const float cy = d.y + 360;
            constexpr float kTau = 6.2831853f;
            list.arc(cx, cy, 104, 10, 0.0f, kTau, Color::rgb(0x1c2330));
            if (job_.total > 0)
            {
                const float fraction = std::min(
                    1.0f, static_cast<float>(job_.done) / static_cast<float>(job_.total));
                list.arc(cx, cy, 104, 10, 0.0f, kTau * std::max(0.01f, fraction), kAccent);
                char percent[16];
                std::snprintf(percent, sizeof(percent), "%d%%",
                              static_cast<int>(fraction * 100.0f));
                ui::text(list, fonts.semibold, percent, cx, cy + 16, 44, kText,
                         gfx::Align::center);
                ui::text(list, fonts.semibold,
                         megabytes(job_.done) + " / " + megabytes(job_.total) + " MB", d.x + 380,
                         d.y + 300, 30, kText);
            }
            else
            {
                const float spin = context_.settings.reduced_motion ? 0.0f : clock_ * 4.0f;
                list.arc(cx, cy, 104, 10, spin, kTau * 0.22f, kAccent);
            }
            const ui::ParagraphLines lines =
                ui::paragraph_lines(fonts.regular, L(kS.importing_text), 26, 600, 4);
            ui::paragraph(list, fonts.regular, lines, d.x + 380, d.y + 360, 26, 40, kText2);
            ui::HintLayout layout;
            layout.size = 34;
            layout.text_size = 22;
            layout.cy = d.y + d.h - 58;
            const ui::Hint hints[] = {{ui::Button::circle, L(kS.cancel)}};
            ui::draw_hints(list, fonts, ui::GlyphStyle::dark(), hints, 1, d.x + d.w - 64, true,
                           layout);
            list.pop_opacity();
            return;
        }
        if (job_.uninstall)
        {
            ui::text(list, fonts.display, std::string(L(kS.uninstalling_title)) + " " + emulator.name,
                     d.x + 176, d.y + 106, 44, kText);
            ui::text(list, fonts.regular, std::string("/data/homebrew/") + emulator.title_id,
                     d.x + 176, d.y + 140, 22, kText2);
            const float spin = context_.settings.reduced_motion ? 0.0f : clock_ * 4.0f;
            list.arc(d.x + 190, d.y + 360, 104, 10, 0.0f, 6.2831853f, Color::rgb(0x1c2330));
            list.arc(d.x + 190, d.y + 360, 104, 10, spin, 6.2831853f * 0.22f, kAccent);
            const ui::ParagraphLines lines =
                ui::paragraph_lines(fonts.regular, L(kS.uninstalling_text), 26, 600, 4);
            ui::paragraph(list, fonts.regular, lines, d.x + 380, d.y + 330, 26, 40, kText2);
            list.pop_opacity();
            return;
        }
        const std::string title =
            std::string(L(is_installed(index) ? kS.updating_title : kS.installing_title)) + " " +
            emulator.name;
        ui::text(list, fonts.display, title, d.x + 176, d.y + 106, 44, kText);
        const emuland::Release &release = state(index).release;
        std::string sub = pretty_version(release.tag);
        if (release.size > 0)
            sub += "  ·  " + megabytes(release.size) + " MB";
        sub += std::string("  ·  github.com/") + emulator.repo;
        ui::text(list, fonts.regular, sub, d.x + 176, d.y + 140, 22, kText2);

        // Step and how far it is.
        int step = 0;
        switch (job_.stage)
        {
        case emuland::Stage::downloading:
            step = 1;
            break;
        case emuland::Stage::verifying:
            step = 2;
            break;
        case emuland::Stage::extracting:
            step = 3;
            break;
        case emuland::Stage::installing:
        case emuland::Stage::done:
            step = 4;
            break;
        default:
            step = 0;
            break;
        }
        const bool measured = (step == 1 || step == 3) && job_.total > 0;
        const float fraction =
            measured ? std::min(1.0f, static_cast<float>(job_.done) / static_cast<float>(job_.total))
                     : 0.0f;
        const float cx = d.x + 190;
        const float cy = d.y + 360;
        constexpr float kTau = 6.2831853f;
        list.arc(cx, cy, 104, 10, 0.0f, kTau, Color::rgb(0x1c2330));
        if (measured)
        {
            list.arc(cx, cy, 104, 10, 0.0f, kTau * std::max(0.01f, fraction), kAccent);
            char percent[16];
            std::snprintf(percent, sizeof(percent), "%d%%", static_cast<int>(fraction * 100.0f));
            ui::text(list, fonts.semibold, percent, cx, cy + 16, 44, kText, gfx::Align::center);
        }
        else
        {
            // Steps without a measure: a short arc that turns.
            const float spin = context_.settings.reduced_motion ? 0.0f : clock_ * 4.0f;
            list.arc(cx, cy, 104, 10, spin, kTau * 0.22f, kAccent);
        }

        std::string download = L(kS.step_download);
        if (step == 1 && job_.total > 0)
            download += "  " + megabytes(job_.done) + " / " + megabytes(job_.total) + " MB";
        else if (!release.asset.empty())
            download += "  " + release.asset;
        const std::string steps[5] = {L(kS.step_release), download, L(kS.step_verify),
                                      L(kS.step_extract), L(kS.step_install)};
        for (int i = 0; i < 5; ++i)
        {
            const float y = d.y + 252 + static_cast<float>(i) * 54;
            const float x = d.x + 380;
            if (i < step)
            {
                icon(list, Icon::check, x, y - 20, 26, kOk);
                ui::text(list, fonts.regular, steps[i], x + 44, y, 26, kOk);
            }
            else if (i == step)
            {
                list.circle(x + 13, y - 8, 6, kAccent);
                ui::text(list, fonts.semibold, steps[i], x + 44, y, 26, kText);
            }
            else
            {
                list.circle(x + 13, y - 8, 5, Color::rgb(0x5b6878));
                ui::text(list, fonts.regular, steps[i], x + 44, y, 26, kText2);
            }
        }
        if (job_.queued > 0)
        {
            char queued[48];
            std::snprintf(queued, sizeof(queued), "+%d %s", job_.queued, L(kS.queued));
            ui::text(list, fonts.regular, queued, d.x + 64, d.y + d.h - 50, 22, kText2);
        }
        ui::HintLayout layout;
        layout.size = 34;
        layout.text_size = 22;
        layout.cy = d.y + d.h - 58;
        const ui::Hint hints[] = {{ui::Button::circle, L(kS.cancel)}};
        if (job_.stage != emuland::Stage::installing)
            ui::draw_hints(list, fonts, ui::GlyphStyle::dark(), hints, 1, d.x + d.w - 64, true,
                           layout);
        list.pop_opacity();
    }

    void draw_launch(gfx::DrawList &list) const
    {
        const int index = launching_ >= 0 ? launching_ : selected_;
        if (index >= emulator_count())
            return;
        const ui::Fonts &fonts = context_.fonts;
        list.push_opacity(launch_show_.value);
        list.rounded_rect({0, 0, gfx::kVirtualWidth, gfx::kVirtualHeight}, 0, Color::rgb(0x05070a));
        if (const std::uint32_t texture = background_of(index))
            list.image(texture, {-60, -34, 2040, 1148}, gfx::kFullUv, Color::rgb(0x595959));
        list.rounded_rect({0, 0, gfx::kVirtualWidth, gfx::kVirtualHeight}, 0,
                          Color::rgb(0x05070a, 0.55f));
        if (const std::uint32_t texture = icon_of(index))
        {
            list.shadow({860, 324, 200, 200}, 48, 50, Color::rgb(0x000000, 0.5f));
            list.image(texture, {860, 300, 200, 200}, gfx::kFullUv, kText, 48);
        }
        if (launching_game_ >= 0)
        {
            // A game: its title is the headline, the emulator says where.
            ui::text(list, fonts.semibold,
                     ui::upper(std::string(L(kS.opening_in)) + " " + spec(index).name), 960, 580,
                     24, kText2, gfx::Align::center, 7.2f);
            ui::text(list, fonts.display,
                     fit(fonts.display, game(launching_game_).title, 88, 1600), 960, 680, 88,
                     kText, gfx::Align::center);
            // It can't be handed the game: say so, rather than seem to fail.
            if (spec(index).rom_option == nullptr)
                ui::text(list, fonts.regular,
                         std::string(L(kS.pick_in_menu)) + " " + spec(index).name, 960, 812, 26,
                         kText2, gfx::Align::center);
        }
        else
        {
            ui::text(list, fonts.semibold, ui::upper(L(spec(index).system)), 960, 580, 24,
                     kText2, gfx::Align::center, 7.2f);
            ui::text(list, fonts.display, spec(index).name, 960, 680, 88, kText,
                     gfx::Align::center);
        }
        const float progress = tween::cubic_out(tween::clamp01(launch_time_ / 1.8f));
        list.rounded_rect({800, 740, 320, 4}, 2, Color::rgb(0xffffff, 0.12f));
        list.rounded_rect({800, 740, 320 * progress, 4}, 2, kAccent);
        list.pop_opacity();
    }

    void draw_hints(gfx::DrawList &list) const
    {
        const ui::Fonts &fonts = context_.fonts;
        ui::HintLayout layout;
        layout.size = 34;
        layout.text_size = 22;
        layout.cy = 1020;
        layout.item_gap = 36;
        const ui::GlyphStyle style = ui::GlyphStyle::dark();
        if (dialog_.open)
        {
            const ui::Hint hints[] = {{ui::Button::cross, L(kS.select)},
                                      {ui::Button::circle, L(kS.back)}};
            ui::draw_hints(list, fonts, style, hints, 2, kRight, true, layout);
            return;
        }
        if (screen_ == Screen::home)
        {
            const ui::Hint hints[] = {{ui::Button::cross, L(kS.select)},
                                      {ui::Button::triangle, L(kS.details)},
                                      {ui::Button::square, L(kS.folders)},
                                      {ui::Button::l1, L(kS.menu), ui::Button::r1},
                                      {ui::Button::options, L(kS.settings_tab)}};
            ui::draw_hints(list, fonts, style, hints, 5, kRight, true, layout);
            return;
        }
        if (screen_ == Screen::games && focus_.act == Act::play_game && !focus_.nav)
        {
            const ui::Hint hints[] = {{ui::Button::cross, L(kS.play)},
                                      {ui::Button::square, L(kS.folders)},
                                      {ui::Button::l1, L(kS.menu), ui::Button::r1},
                                      {ui::Button::circle, L(kS.back)}};
            ui::draw_hints(list, fonts, style, hints, 4, kRight, true, layout);
            return;
        }
        const ui::Hint hints[] = {
            {ui::Button::cross, screen_ == Screen::folders ? L(kS.import_label) : L(kS.select)},
            {ui::Button::l1, L(kS.menu), ui::Button::r1},
            {ui::Button::circle, L(kS.back)}};
        ui::draw_hints(list, fonts, style, hints, 3, kRight, true, layout);
    }

    // ---------------------------------------------------------------- state

    static constexpr int kMaxShelves = 8;

    app::Context &context_;
    app::Feedback *feedback_ = nullptr;
    Lang lang_ = Lang::pt;
    int language_choice_ = 0;
    bool cover_choice_ = true; // download the covers that are missing
    bool sounds_on_ = true;
    bool mute_ = false;
    bool repeat_ = false;

    Screen screen_ = Screen::home;
    Screen back_to_ = Screen::home;
    bool came_from_home_ = false;
    std::vector<Item> items_;
    std::vector<Shelf> shelves_;
    Item focus_;
    Item saved_focus_[6];
    int selected_ = 0;
    int shown_ = 0;
    int background_ = 0;
    int previous_background_ = 0;
    int folder_emulator_ = 0;
    int game_filter_ = -1;    // Games: the emulator shown, -1 all
    int games_row_ = 0;       // Games: the focused row of covers
    int launching_game_ = -1; // the game being opened, -1 just the emulator
    int row_ = 0;
    int columns_[kMaxShelves] = {};

    float age_ = 0.0f;
    float clock_ = 0.0f;
    float clock_poll_ = 0.0f;
    int hour_ = 0;
    int minute_ = 0;
    tween::Timer hero_;
    tween::Timer background_fade_;
    tween::Timer folder_switch_;
    tween::Timer screen_fade_;
    tween::Spring rows_y_;
    tween::Spring compact_;
    tween::Spring games_y_;
    ui::Scroller scroll_[kMaxShelves];
    ui::SpringRect ring_;
    ui::Pulse nudge_;

    Dialog dialog_;
    int dialog_focus_ = 0;
    tween::Spring dialog_show_;
    std::string toast_text_;
    float toast_time_ = 0.0f;
    int launching_ = -1;
    float launch_time_ = 0.0f;
    bool launch_called_ = false;
    int launch_result_ = 0;
    float registration_poll_ = 0.0f;
    int dialog_emulator_ = 0; // the one the open dialog is about
    int dialog_game_ = 0;     // the game the "open in which emulator" dialog is about
    // By the emulator whose folder holds the game: the one picked to open it.
    std::vector<int> chosen_player_ = std::vector<int>(emuland::catalog().size(), -1);
    tween::Spring launch_show_;
    emuland::JobStatus job_;
    tween::Spring job_show_;
    bool installing_ = false;
    bool preparing_ = false;     // the USB drive's folders are being made
    bool prepare_ready_ = false; // its result arrived, shown after the minimum time
    float prepare_since_ = 0.0f;
    emuland::JobStatus prepare_result_;
    bool manual_check_ = false;
    int install_label_ = 0;
};

} // namespace

std::unique_ptr<app::Concept> make_emuland(app::Context &context)
{
    return std::make_unique<Emuland>(context);
}

} // namespace hui::concepts
