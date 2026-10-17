// Emuland - The online catalog: emulators added without a new Manta version.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "emuland/online_catalog.hpp"

#include "emuland/json.hpp"
#include "emuland/model.hpp"

#include <cstdio>
#include <deque>
#include <set>

namespace emuland
{

namespace
{

constexpr std::size_t kMaxEntries = 32;
constexpr std::size_t kMaxFolders = 12;
constexpr std::size_t kMaxRequirements = 6;
constexpr std::size_t kMaxText = 400;

bool in(char c, const char *allowed)
{
    for (const char *a = allowed; *a != '\0'; ++a)
    {
        if (*a == c)
            return true;
    }
    return false;
}

// Letters, digits and the given punctuation only, 1 to limit characters.
bool plain(const std::string &text, std::size_t limit, const char *punctuation)
{
    if (text.empty() || text.size() > limit)
        return false;
    for (const char c : text)
    {
        const bool alnum = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        if (!alnum && !in(c, punctuation))
            return false;
    }
    return true;
}

bool lower_id(const std::string &text)
{
    if (!plain(text, 32, "-"))
        return false;
    for (const char c : text)
    {
        if (c >= 'A' && c <= 'Z')
            return false;
    }
    return true;
}

// PPSA99203: four capitals and five digits.
bool title_id(const std::string &text)
{
    if (text.size() != 9)
        return false;
    for (std::size_t i = 0; i < 9; ++i)
    {
        const char c = text[i];
        if (i < 4 ? (c < 'A' || c > 'Z') : (c < '0' || c > '9'))
            return false;
    }
    return true;
}

// owner/name, as GitHub allows them.
bool repo(const std::string &text)
{
    const std::size_t slash = text.find('/');
    return slash != std::string::npos && slash > 0 && slash + 1 < text.size() &&
           text.find('/', slash + 1) == std::string::npos && text.find("..") == std::string::npos &&
           plain(text, 100, "-_./");
}

// A folder of its own right under /data: "Remove everything" deletes it.
bool data_folder(const std::string &text)
{
    if (text.rfind("/data/", 0) != 0)
        return false;
    const std::string name = text.substr(6);
    if (!plain(name, 40, "-_.") || name.find("..") != std::string::npos || name[0] == '.')
        return false;
    for (const char *reserved : {"homebrew", "emuland", "etaHEN", "shadowmount", "self-update"})
    {
        if (name == reserved)
            return false;
    }
    return true;
}

// Inside the data folder: "games", "azahar/games"; never up or absolute.
bool relative_path(const std::string &text)
{
    if (text == ".")
        return true;
    if (!plain(text, 80, "-_./") || text[0] == '/' || text.back() == '/')
        return false;
    std::size_t start = 0;
    while (start <= text.size())
    {
        const std::size_t end = text.find('/', start);
        const std::string part = text.substr(start, end == std::string::npos ? end : end - start);
        if (part.empty() || part == "." || part == "..")
            return false;
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    return true;
}

bool readable(const std::string &text)
{
    if (text.size() > kMaxText)
        return false;
    for (const char c : text)
    {
        if (static_cast<unsigned char>(c) < 32)
            return false;
    }
    return true;
}

// "1.10.0" against "1.3.0", part by part.
int compare_versions(const std::string &a, const std::string &b)
{
    int x[3] = {};
    int y[3] = {};
    std::sscanf(a.c_str(), "%d.%d.%d", &x[0], &x[1], &x[2]);
    std::sscanf(b.c_str(), "%d.%d.%d", &y[0], &y[1], &y[2]);
    for (int i = 0; i < 3; ++i)
    {
        if (x[i] != y[i])
            return x[i] < y[i] ? -1 : 1;
    }
    return 0;
}

bool text_pair(const json::Value &value, OnlineText &out, bool required)
{
    if (value.type == json::Value::Type::null)
        return !required;
    if (value.type == json::Value::Type::string)
    {
        out.pt = out.en = value.str();
        return readable(out.pt) && (!required || !out.pt.empty());
    }
    if (value.type != json::Value::Type::object || value["pt"].type != json::Value::Type::string ||
        value["en"].type != json::Value::Type::string)
        return false;
    out.pt = value["pt"].str();
    out.en = value["en"].str();
    return readable(out.pt) && readable(out.en) && (!required || (!out.pt.empty() && !out.en.empty()));
}

bool string_field(const json::Value &value, std::string &out)
{
    if (value.type == json::Value::Type::null)
        return true;
    if (value.type != json::Value::Type::string)
        return false;
    out = value.str();
    return readable(out);
}

int maker_of(const std::string &name)
{
    const char *const names[] = {"sony", "nintendo", "microsoft", "multi"};
    for (int i = 0; i < 4; ++i)
    {
        if (name == names[i])
            return i;
    }
    return -1;
}

bool parse_folder(const json::Value &value, OnlineFolder &folder, std::string &error)
{
    std::string need = "none";
    const bool ok = value.type == json::Value::Type::object && string_field(value["key"], folder.key) &&
                    string_field(value["path"], folder.path) && text_pair(value["name"], folder.name, true) &&
                    text_pair(value["note"], folder.note, false) && string_field(value["need"], need) &&
                    string_field(value["extensions"], folder.extensions) &&
                    text_pair(value["when_empty"], folder.when_empty, false);
    folder.games = value["games"].truthy();
    folder.need = need == "required" ? 2 : need == "optional" ? 1 : need == "none" ? 0 : -1;
    if (!ok || !lower_id(folder.key) || !relative_path(folder.path) || folder.need < 0 ||
        (!folder.extensions.empty() && !plain(folder.extensions, 120, ",")))
    {
        error = "folder \"" + folder.key + "\": bad key, path, need or extensions";
        return false;
    }
    return true;
}

bool parse_entry(const json::Value &value, OnlineEntry &entry, std::string &error)
{
    if (value.type != json::Value::Type::object)
    {
        error = "an emulator is not an object";
        return false;
    }
    std::string maker = "multi";
    const bool fields =
        string_field(value["id"], entry.id) && string_field(value["name"], entry.name) &&
        string_field(value["short_name"], entry.short_name) && text_pair(value["system"], entry.system, true) &&
        string_field(value["author"], entry.author) && string_field(value["license"], entry.license) &&
        string_field(value["repo"], entry.repo) && string_field(value["title_id"], entry.title_id) &&
        string_field(value["data"], entry.data) && string_field(value["usb"], entry.usb) &&
        text_pair(value["needs"], entry.needs, false) &&
        text_pair(value["description"], entry.description, true) &&
        string_field(value["asset_prefix"], entry.asset_prefix) && string_field(value["maker"], maker) &&
        string_field(value["rom_option"], entry.rom_option);
    entry.prerelease = value["prerelease"].truthy();
    entry.maker = maker_of(maker);
    const std::string who = "emulator \"" + entry.id + "\": ";
    if (!fields)
    {
        error = who + "a field has the wrong type or a control character";
        return false;
    }
    if (!lower_id(entry.id) || !plain(entry.name, 32, "-_. ") || !plain(entry.short_name, 12, "-_. /") ||
        entry.author.empty() || entry.license.empty())
    {
        error = who + "bad id, name, short_name, author or license";
        return false;
    }
    if (!title_id(entry.title_id) || entry.title_id == kTitleId || !repo(entry.repo) ||
        !data_folder(entry.data) || !lower_id(entry.usb) || entry.maker < 0)
    {
        error = who + "bad title_id, repo, data, usb or maker";
        return false;
    }
    if ((!entry.asset_prefix.empty() && !plain(entry.asset_prefix, 64, "-_.")) ||
        (!entry.rom_option.empty() && (entry.rom_option[0] != '-' || !plain(entry.rom_option, 24, "-"))))
    {
        error = who + "bad asset_prefix or rom_option";
        return false;
    }
    const json::Value &highlights = value["highlights"];
    for (std::size_t i = 0; i < highlights.items.size() && i < 3; ++i)
    {
        OnlineText text;
        if (!text_pair(highlights.items[i], text, true))
        {
            error = who + "bad highlight";
            return false;
        }
        entry.highlights.push_back(text);
    }
    const json::Value &requirements = value["requirements"];
    for (std::size_t i = 0; i < requirements.items.size() && i < kMaxRequirements; ++i)
    {
        std::string text;
        if (!string_field(requirements.items[i], text) || text.empty())
        {
            error = who + "bad requirement";
            return false;
        }
        entry.requirements.push_back(text);
    }
    const json::Value &folders = value["folders"];
    if (folders.type != json::Value::Type::array || folders.items.empty() ||
        folders.items.size() > kMaxFolders)
    {
        error = who + "needs 1 to 12 folders";
        return false;
    }
    std::set<std::string> keys;
    for (const json::Value &each : folders.items)
    {
        OnlineFolder folder;
        if (!parse_folder(each, folder, error))
        {
            error = who + error;
            return false;
        }
        if (!keys.insert(folder.key).second)
        {
            error = who + "two folders share a key";
            return false;
        }
        entry.folders.push_back(folder);
    }
    return true;
}

bool parse_change(const json::Value &value, OnlineChange &change, std::string &error)
{
    const bool fields = value.type == json::Value::Type::object && string_field(value["id"], change.id) &&
                        string_field(value["repo"], change.repo) &&
                        string_field(value["asset_prefix"], change.asset_prefix) &&
                        string_field(value["rom_option"], change.rom_option);
    if (value["prerelease"].type == json::Value::Type::boolean)
        change.prerelease = value["prerelease"].boolean ? 1 : 0;
    if (!fields || !lower_id(change.id) || (!change.repo.empty() && !repo(change.repo)) ||
        (!change.asset_prefix.empty() && !plain(change.asset_prefix, 64, "-_.")) ||
        (!change.rom_option.empty() && (change.rom_option[0] != '-' || !plain(change.rom_option, 24, "-"))))
    {
        error = "change \"" + change.id + "\": bad id, repo, asset_prefix or rom_option";
        return false;
    }
    return true;
}

const char *keep(const std::string &text)
{
    return text.empty() ? nullptr : keep_text(text);
}

Text keep(const OnlineText &text)
{
    return {keep_text(text.pt), keep_text(text.en)};
}

} // namespace

bool parse_online_catalog(std::string_view document, OnlineCatalog &out, std::string &error)
{
    out = {};
    json::Value root;
    if (!json::parse(document, root) || root.type != json::Value::Type::object)
    {
        error = "not a JSON object";
        return false;
    }
    out.schema = static_cast<int>(root["schema"].number);
    if (out.schema != 1)
    {
        error = "unknown schema";
        return false;
    }
    const json::Value &emulators = root["emulators"];
    if (emulators.items.size() > kMaxEntries)
    {
        error = "too many emulators";
        return false;
    }
    std::set<std::string> ids;
    std::set<std::string> titles;
    std::set<std::string> datas;
    for (const json::Value &value : emulators.items)
    {
        OnlineEntry entry;
        if (!parse_entry(value, entry, error))
            return false;
        if (!ids.insert(entry.id).second || !titles.insert(entry.title_id).second ||
            !datas.insert(entry.data).second)
        {
            error = "emulator \"" + entry.id + "\": id, title_id or data used twice";
            return false;
        }
        std::string needs_version;
        string_field(value["min_version"], needs_version);
        if (!needs_version.empty() && compare_versions(needs_version, kVersion) > 0)
            continue; // for a newer Manta
        out.emulators.push_back(std::move(entry));
    }
    for (const json::Value &value : root["changes"].items)
    {
        OnlineChange change;
        if (!parse_change(value, change, error))
            return false;
        out.changes.push_back(std::move(change));
    }
    error.clear();
    return true;
}

int apply_online_catalog(const OnlineCatalog &catalog)
{
    for (const OnlineChange &change : catalog.changes)
        override_catalog_entry({change.id, change.repo, change.asset_prefix, change.rom_option,
                                change.prerelease});
    // The spans of an entry point here: never moved once made.
    static std::deque<std::vector<FolderSpec>> folder_lists;
    static std::deque<std::vector<const char *>> requirement_lists;
    int added = 0;
    for (const OnlineEntry &entry : catalog.emulators)
    {
        std::vector<FolderSpec> &folders = folder_lists.emplace_back();
        for (const OnlineFolder &folder : entry.folders)
        {
            FolderSpec made{keep_text(folder.key), keep(folder.name), keep_text(folder.path),
                            keep(folder.note)};
            made.need = static_cast<FolderNeed>(folder.need);
            made.games = folder.games;
            made.extensions = keep(folder.extensions);
            if (!folder.when_empty.pt.empty())
                made.when_empty = keep(folder.when_empty);
            folders.push_back(made);
        }
        std::vector<const char *> &requirements = requirement_lists.emplace_back();
        for (const std::string &text : entry.requirements)
            requirements.push_back(keep_text(text));
        EmulatorSpec spec{keep_text(entry.id),
                          keep_text(entry.name),
                          keep_text(entry.short_name),
                          keep(entry.system),
                          keep_text(entry.author),
                          keep_text(entry.license),
                          keep_text(entry.repo),
                          keep_text(entry.title_id),
                          keep_text(entry.data),
                          keep_text(entry.usb),
                          entry.prerelease,
                          keep(entry.needs),
                          keep(entry.description),
                          {{"", ""}, {"", ""}, {"", ""}},
                          requirements,
                          folders};
        for (std::size_t i = 0; i < entry.highlights.size() && i < 3; ++i)
            spec.highlights[i] = keep(entry.highlights[i]);
        spec.asset_prefix = keep(entry.asset_prefix);
        spec.maker = static_cast<Maker>(entry.maker);
        spec.rom_option = keep(entry.rom_option);
        added += add_catalog_entry(spec) ? 1 : 0;
    }
    return added;
}

int new_online_entries(const OnlineCatalog &catalog)
{
    int fresh = 0;
    for (const OnlineEntry &entry : catalog.emulators)
    {
        bool known = false;
        for (const EmulatorSpec &spec : emuland::catalog())
            known = known || entry.id == spec.id || entry.title_id == spec.title_id;
        fresh += known ? 0 : 1;
    }
    return fresh;
}

} // namespace emuland
