// Emuland - A small JSON reader for GitHub's release answers.
// Copyright (C) 2026 Emuland contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "emuland/json.hpp"

#include <cstdlib>

namespace emuland::json
{

namespace
{

constexpr int kMaxDepth = 64;

struct Parser
{
    std::string_view in;
    std::size_t at = 0;

    void skip_space()
    {
        while (at < in.size() &&
               (in[at] == ' ' || in[at] == '\t' || in[at] == '\n' || in[at] == '\r'))
            ++at;
    }

    bool literal(std::string_view word)
    {
        if (in.substr(at, word.size()) != word)
            return false;
        at += word.size();
        return true;
    }

    static void put_utf8(std::string &out, unsigned code)
    {
        if (code < 0x80)
        {
            out += static_cast<char>(code);
        }
        else if (code < 0x800)
        {
            out += static_cast<char>(0xc0 | (code >> 6));
            out += static_cast<char>(0x80 | (code & 0x3f));
        }
        else if (code < 0x10000)
        {
            out += static_cast<char>(0xe0 | (code >> 12));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
            out += static_cast<char>(0x80 | (code & 0x3f));
        }
        else
        {
            out += static_cast<char>(0xf0 | (code >> 18));
            out += static_cast<char>(0x80 | ((code >> 12) & 0x3f));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3f));
            out += static_cast<char>(0x80 | (code & 0x3f));
        }
    }

    bool hex4(unsigned &code)
    {
        if (at + 4 > in.size())
            return false;
        code = 0;
        for (int i = 0; i < 4; ++i)
        {
            const char c = in[at++];
            code <<= 4;
            if (c >= '0' && c <= '9')
                code |= static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f')
                code |= static_cast<unsigned>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F')
                code |= static_cast<unsigned>(c - 'A' + 10);
            else
                return false;
        }
        return true;
    }

    bool string(std::string &out)
    {
        if (at >= in.size() || in[at] != '"')
            return false;
        ++at;
        while (at < in.size())
        {
            const char c = in[at++];
            if (c == '"')
                return true;
            if (static_cast<unsigned char>(c) < 0x20)
                return false;
            if (c != '\\')
            {
                out += c;
                continue;
            }
            if (at >= in.size())
                return false;
            const char e = in[at++];
            switch (e)
            {
            case '"':
            case '\\':
            case '/':
                out += e;
                break;
            case 'b':
                out += '\b';
                break;
            case 'f':
                out += '\f';
                break;
            case 'n':
                out += '\n';
                break;
            case 'r':
                out += '\r';
                break;
            case 't':
                out += '\t';
                break;
            case 'u':
            {
                unsigned code = 0;
                if (!hex4(code))
                    return false;
                // A surrogate pair is one character above U+FFFF.
                if (code >= 0xd800 && code < 0xdc00 && in.substr(at, 2) == "\\u")
                {
                    at += 2;
                    unsigned low = 0;
                    if (!hex4(low) || low < 0xdc00 || low > 0xdfff)
                        return false;
                    code = 0x10000 + ((code - 0xd800) << 10) + (low - 0xdc00);
                }
                put_utf8(out, code);
                break;
            }
            default:
                return false;
            }
        }
        return false;
    }

    bool number(Value &out)
    {
        const std::size_t start = at;
        if (at < in.size() && in[at] == '-')
            ++at;
        while (at < in.size() && ((in[at] >= '0' && in[at] <= '9') || in[at] == '.' ||
                                  in[at] == 'e' || in[at] == 'E' || in[at] == '+' || in[at] == '-'))
            ++at;
        if (at == start)
            return false;
        const std::string text(in.substr(start, at - start));
        char *end = nullptr;
        out.number = std::strtod(text.c_str(), &end);
        out.type = Value::Type::number;
        return end != nullptr && *end == '\0';
    }

    bool value(Value &out, int depth)
    {
        if (depth > kMaxDepth)
            return false;
        skip_space();
        if (at >= in.size())
            return false;
        const char c = in[at];
        if (c == '{')
        {
            ++at;
            out.type = Value::Type::object;
            skip_space();
            if (at < in.size() && in[at] == '}')
            {
                ++at;
                return true;
            }
            for (;;)
            {
                skip_space();
                std::string key;
                if (!string(key))
                    return false;
                skip_space();
                if (at >= in.size() || in[at] != ':')
                    return false;
                ++at;
                out.names.push_back(std::move(key));
                out.items.emplace_back();
                if (!value(out.items.back(), depth + 1))
                    return false;
                skip_space();
                if (at < in.size() && in[at] == ',')
                {
                    ++at;
                    continue;
                }
                if (at < in.size() && in[at] == '}')
                {
                    ++at;
                    return true;
                }
                return false;
            }
        }
        if (c == '[')
        {
            ++at;
            out.type = Value::Type::array;
            skip_space();
            if (at < in.size() && in[at] == ']')
            {
                ++at;
                return true;
            }
            for (;;)
            {
                out.items.emplace_back();
                if (!value(out.items.back(), depth + 1))
                    return false;
                skip_space();
                if (at < in.size() && in[at] == ',')
                {
                    ++at;
                    continue;
                }
                if (at < in.size() && in[at] == ']')
                {
                    ++at;
                    return true;
                }
                return false;
            }
        }
        if (c == '"')
        {
            out.type = Value::Type::string;
            return string(out.text);
        }
        if (literal("true"))
        {
            out.type = Value::Type::boolean;
            out.boolean = true;
            return true;
        }
        if (literal("false"))
        {
            out.type = Value::Type::boolean;
            return true;
        }
        if (literal("null"))
            return true;
        return number(out);
    }
};

} // namespace

const Value &Value::operator[](std::string_view name) const
{
    static const Value kNull;
    if (type != Type::object)
        return kNull;
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        if (names[i] == name)
            return items[i];
    }
    return kNull;
}

bool parse(std::string_view document, Value &out)
{
    Parser parser{document};
    out = Value{};
    if (!parser.value(out, 0))
        return false;
    parser.skip_space();
    return parser.at == document.size();
}

} // namespace emuland::json
