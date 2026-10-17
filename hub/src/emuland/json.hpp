// Emuland - A small JSON reader for GitHub's release answers.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Builds a tree of the whole document (a release list is about 40 KB). No
// exceptions: parse() returns false on anything malformed, deeper than 64
// levels or larger than the limit given.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace emuland::json
{

struct Value
{
    enum class Type
    {
        null,
        boolean,
        number,
        string,
        array,
        object,
    };
    Type type = Type::null;
    bool boolean = false;
    double number = 0.0;
    std::string text;                                // string
    std::vector<Value> items;       // array, or an object's values
    std::vector<std::string> names; // object: the name of each of items, in document order

    // The member called name, or a shared null value.
    const Value &operator[](std::string_view name) const;
    const std::string &str() const
    {
        return text;
    }
    std::uint64_t u64() const
    {
        return type == Type::number && number > 0 ? static_cast<std::uint64_t>(number) : 0;
    }
    bool truthy() const
    {
        return type == Type::boolean && boolean;
    }
};

bool parse(std::string_view document, Value &out);

} // namespace emuland::json
