// Emuland - The online catalog: emulators added without a new Manta version.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// catalog/online.json in Manta's repository (catalog/README.md has the
// format). Manta fetches it with each check, keeps the last valid copy in
// /data/emuland/catalog.json and applies that copy at the next start, before
// the catalog is fixed: new entries join the shelves, and an entry may move a
// built-in emulator to another repository or release file name.
//
// The document says where Manta downloads apps from and which data folder
// "Remove everything" deletes, so it is checked strictly: any entry that
// breaks a rule makes the whole document invalid, and nothing applies.

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace emuland
{

struct OnlineText
{
    std::string pt;
    std::string en;
};

struct OnlineFolder
{
    std::string key;
    OnlineText name;
    std::string path;
    OnlineText note;
    int need = 0; // FolderNeed: 0 none, 1 optional, 2 required
    bool games = false;
    std::string extensions; // empty: every entry
    OnlineText when_empty;
};

struct OnlineEntry
{
    std::string id;
    std::string name;
    std::string short_name;
    OnlineText system;
    std::string author;
    std::string license;
    std::string repo;
    std::string title_id;
    std::string data;
    std::string usb;
    bool prerelease = false;
    OnlineText needs;
    OnlineText description;
    std::vector<OnlineText> highlights; // up to three
    std::vector<std::string> requirements;
    std::vector<OnlineFolder> folders;
    std::string asset_prefix; // empty: source only
    int maker = 3;            // Maker: 0 sony, 1 nintendo, 2 microsoft, 3 multi
    std::string rom_option;
};

struct OnlineChange
{
    std::string id;
    std::string repo;
    std::string asset_prefix;
    std::string rom_option;
    int prerelease = -1;
};

struct OnlineCatalog
{
    int schema = 0;
    std::vector<OnlineEntry> emulators;
    std::vector<OnlineChange> changes;
};

// True when the document is a valid catalog this version can use; error
// names the first rule broken. Entries asking for a newer Manta
// ("min_version") are left out, not refused.
bool parse_online_catalog(std::string_view document, OnlineCatalog &out, std::string &error);

// Adds the entries and changes to the catalog (before it is fixed). Returns
// how many entries were added.
int apply_online_catalog(const OnlineCatalog &catalog);

// Ids in the document that the running catalog lacks (to tell the user a
// restart shows new emulators).
int new_online_entries(const OnlineCatalog &catalog);

} // namespace emuland
