// Emuland - Installs and updates emulators from their GitHub releases.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// The public side is in emuland/model.hpp (start_check, start_install,
// cancel_job, job_status). This header is what the console glue and the PC
// test need besides: where things live, and what was installed before.

#pragma once

#include "emuland/model.hpp"

#include <curl/curl.h>

#include <string>
#include <vector>

// Prepares a curl handle for the platform: console_curl_setup() on the PS5,
// the system's certificates on a PC.
extern "C" void emuland_net_setup(CURL *easy);

namespace emuland::installer
{

struct Paths
{
    std::string homebrew = "/data/homebrew"; // where apps are installed
    std::string state = "/data/emuland";     // records, downloads, backups
    std::string registered = "/user";        // the console's copies of sce_sys
    std::string sandboxes = "/mnt/sandbox";  // a running title has <TID>_<n> here
    std::string data = "/data";              // stands for /data in the catalog's paths
    // Where the system mounts USB drives (exFAT or FAT32).
    std::vector<std::string> usb = {"/mnt/usb0", "/mnt/usb1", "/mnt/usb2", "/mnt/usb3",
                                    "/mnt/usb4", "/mnt/usb5", "/mnt/usb6", "/mnt/usb7"};
    // A drive counts only when mounted (its own device); the PC test's
    // folders aren't.
    bool usb_mounts_only = true;
    // ShadowMountPlus's HTTP API (its docs/api.md); empty: not asked.
    std::string shadowmount = "http://127.0.0.1:10101/api/v1";
};

// Before the first job (the PC test points everything at a scratch folder).
void set_paths(const Paths &paths);

// The tag Emuland installed for a title, empty if it did not install it.
std::string installed_tag(const char *title_id);
// The newest release seen for a catalog entry (kept on disk between runs).
bool cached_release(std::size_t index, Release &out);
// Removes the staging folders an interrupted install left in homebrew/.
void clean_leftovers();
// Sends the uninstall helper payload (helper/uninstall_helper.c) to the ELF
// loader: once Emuland closes it asks ShadowMountPlus to take the titles in
// <state>/uninstall-request.txt off the home screen. True when sent. The PS5
// side provides it; the PC test answers false.
bool send_uninstall_helper();

} // namespace emuland::installer
