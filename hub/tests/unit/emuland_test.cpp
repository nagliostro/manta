// Emuland - Catalog, game names, versions and image limits.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/save_file.hpp"
#include "core/version.hpp"
#include "emuland/image.h"
#include "emuland/model.hpp"
#include "emuland/online_catalog.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <set>
#include <string>
#include <utility>

#ifndef HUI_SOURCE_DIR
#define HUI_SOURCE_DIR "."
#endif

namespace
{

bool valid_title_id(const char *id)
{
    if (std::strlen(id) != 9)
        return false;
    for (int i = 0; i < 4; ++i)
        if (id[i] < 'A' || id[i] > 'Z')
            return false;
    for (int i = 4; i < 9; ++i)
        if (id[i] < '0' || id[i] > '9')
            return false;
    return true;
}

} // namespace

TEST(EmulandCatalog, EntriesAreWellFormed)
{
    std::set<std::string> ids;
    std::set<std::string> titles;
    for (const emuland::EmulatorSpec &spec : emuland::catalog())
    {
        SCOPED_TRACE(spec.name);
        EXPECT_TRUE(valid_title_id(spec.title_id));
        EXPECT_TRUE(ids.insert(spec.id).second) << "duplicate id";
        EXPECT_TRUE(titles.insert(spec.title_id).second) << "duplicate title ID";
        // Paths the installer deletes on "remove the data too" must stay in /data.
        EXPECT_EQ(std::strncmp(spec.data, "/data/", 6), 0);
        EXPECT_EQ(std::strstr(spec.data, ".."), nullptr);
        EXPECT_NE(std::strchr(spec.repo, '/'), nullptr);
        for (const emuland::FolderSpec &folder : spec.folders)
        {
            EXPECT_EQ(std::strstr(folder.path, ".."), nullptr);
            EXPECT_NE(folder.path[0], '/');
        }
    }
}

TEST(EmulandCatalog, VersionMatchesParamJson)
{
    std::string text;
    ASSERT_TRUE(hui::save::read_file(HUI_SOURCE_DIR "/sce_sys/param.json", &text));
    const std::string content = hui::content_version(text); // "01.003.000"
    const std::string expected = emuland::kVersion;          // "1.3.0"
    int major = 0, minor = 0, patch = 0;
    ASSERT_EQ(std::sscanf(expected.c_str(), "%d.%d.%d", &major, &minor, &patch), 3);
    char built[16];
    std::snprintf(built, sizeof(built), "%02d.%03d.%03d", major, minor, patch);
    EXPECT_EQ(content, built);
}

TEST(EmulandNames, TidiesNoIntroNames)
{
    std::string title;
    std::string region;
    emuland::tidy_game_name("Metal Gear Solid (USA) (Disc 1)", title, region);
    EXPECT_EQ(region, "USA");
    EXPECT_EQ(title.find('('), std::string::npos);
    EXPECT_NE(title.find("Metal Gear Solid"), std::string::npos);
}

TEST(EmulandNames, TitleKeyIgnoresPunctuationAndCase)
{
    EXPECT_EQ(emuland::title_key("Ratchet & Clank"), emuland::title_key("ratchet _ clank"));
    EXPECT_NE(emuland::title_key("Tekken 3"), emuland::title_key("Tekken 2"));
}

TEST(EmulandVersions, SameVersionAcrossTagShapes)
{
    EXPECT_TRUE(emuland::same_version("v1.002.000", "01.002.000"));
    EXPECT_TRUE(emuland::same_version("01.002.000", "01.002.000"));
    EXPECT_FALSE(emuland::same_version("v2.2.0", "01.002.000"));
}

TEST(EmulandImage, RefusesHugeDeclaredPictures)
{
    // A valid PNG header that declares 60000 x 60000 pixels: refused before
    // any pixel memory is taken.
    const unsigned char png[] = {0x89, 'P',  'N',  'G',  0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00,
                                 0x0D, 'I',  'H',  'D',  'R',  0x00, 0x00, 0xEA, 0x60, 0x00, 0x00,
                                 0xEA, 0x60, 0x08, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    int width = 0;
    int height = 0;
    EXPECT_EQ(emuland_decode_image(png, static_cast<int>(sizeof(png)), &width, &height), nullptr);
}

TEST(EmulandImage, RefusesGarbage)
{
    const unsigned char bytes[] = {1, 2, 3, 4, 5, 6, 7, 8};
    int width = 0;
    int height = 0;
    EXPECT_EQ(emuland_decode_image(bytes, static_cast<int>(sizeof(bytes)), &width, &height),
              nullptr);
}

namespace
{

// One valid entry; each test breaks one rule of it.
std::string online_document(const std::string &field, const std::string &value)
{
    std::string entry = R"({"id":"meuemu","name":"MeuEmu","short_name":"GBA",)"
                        R"("system":{"pt":"Game Boy Advance","en":"Game Boy Advance"},)"
                        R"("maker":"nintendo","author":"fulano","license":"GPL-3.0",)"
                        R"("repo":"fulano/MeuEmu-PS5","title_id":"PPSA97777","asset_prefix":"MeuEmu-",)"
                        R"("data":"/data/meuemu","usb":"gba","rom_option":"--rom",)"
                        R"("description":{"pt":"Emulador.","en":"Emulator."},)"
                        R"("folders":[{"key":"jogos","name":{"pt":"Jogos","en":"Games"},)"
                        R"("path":"games","games":true,"extensions":"gba,zip"}]})";
    if (!field.empty())
    {
        const std::string key = "\"" + field + "\":";
        const std::size_t at = entry.find(key);
        const std::size_t start = at + key.size();
        const std::size_t end = entry[start] == '"' ? entry.find('"', start + 1) + 1
                                                    : entry.find_first_of(",}", start);
        entry.replace(start, end - start, value);
    }
    return R"({"schema":1,"emulators":[)" + entry +
           R"(],"changes":[{"id":"psxs5","repo":"SynoPiia/PSXS5"}]})";
}

} // namespace

TEST(EmulandOnlineCatalog, AcceptsAValidEntry)
{
    emuland::OnlineCatalog catalog;
    std::string error;
    ASSERT_TRUE(emuland::parse_online_catalog(online_document("", ""), catalog, error)) << error;
    ASSERT_EQ(catalog.emulators.size(), 1u);
    EXPECT_EQ(catalog.emulators[0].data, "/data/meuemu");
    EXPECT_EQ(catalog.emulators[0].maker, 1);
    ASSERT_EQ(catalog.changes.size(), 1u);
    EXPECT_EQ(emuland::new_online_entries(catalog), 1);
}

TEST(EmulandOnlineCatalog, RefusesDangerousOrMalformedEntries)
{
    const std::pair<const char *, const char *> broken[] = {
        {"data", R"("/data")"},
        {"data", R"("/data/homebrew")"},
        {"data", R"("/data/emuland")"},
        {"data", R"("/data/../etc")"},
        {"data", R"("/user/meuemu")"},
        {"title_id", R"("PPSA98265")"}, // Manta's own
        {"title_id", R"("ppsa97777")"},
        {"repo", R"("fulano")"},
        {"repo", R"("fulano/../x")"},
        {"id", R"("Meu Emu")"},
        {"rom_option", R"("; rm -rf")"},
        {"maker", R"("sega")"},
        {"path", R"("../games")"},
        {"path", R"("/games")"},
    };
    for (const auto &[field, value] : broken)
    {
        emuland::OnlineCatalog catalog;
        std::string error;
        EXPECT_FALSE(emuland::parse_online_catalog(online_document(field, value), catalog, error))
            << field << " = " << value;
    }
    emuland::OnlineCatalog catalog;
    std::string error;
    EXPECT_FALSE(emuland::parse_online_catalog("not json", catalog, error));
    EXPECT_FALSE(emuland::parse_online_catalog(R"({"schema":9})", catalog, error));
}

TEST(EmulandOnlineCatalog, LeavesOutEntriesForANewerManta)
{
    std::string document = online_document("", "");
    document.replace(document.find("\"id\":\"meuemu\""), 0, "\"min_version\":\"99.0.0\",");
    emuland::OnlineCatalog catalog;
    std::string error;
    ASSERT_TRUE(emuland::parse_online_catalog(document, catalog, error)) << error;
    EXPECT_TRUE(catalog.emulators.empty());
}

TEST(EmulandOnlineCatalog, RepositoryFileIsValid)
{
    std::string text;
    ASSERT_TRUE(hui::save::read_file(HUI_SOURCE_DIR "/../catalog/online.json", &text));
    emuland::OnlineCatalog catalog;
    std::string error;
    EXPECT_TRUE(emuland::parse_online_catalog(text, catalog, error)) << error;
}

TEST(EmulandCatalog, MantaIsLastAndHidden)
{
    const auto entries = emuland::catalog();
    ASSERT_FALSE(entries.empty());
    EXPECT_TRUE(entries.back().hidden);
    EXPECT_STREQ(entries.back().title_id, emuland::kTitleId);
    for (std::size_t i = 0; i + 1 < entries.size(); ++i)
        EXPECT_FALSE(entries[i].hidden);
}
