/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */
#ifndef __SettingsBoolean__
#define __SettingsBoolean__

#include <cctype>
#include <string>

// Settings are stored as strings, and UpdateSetting() is public API, so
// embedders spell boolean values by hand. Writers emit the canonical spelling;
// readers accept any casing so that values written before the spellings were
// unified keep working.
//
// This header is deliberately not part of the delegate interface that the
// loader shares with the API library: it is included by the implementation
// side only, so the two libraries can never end up disagreeing on what a
// boolean setting means.

namespace LWEDelegate {

inline const char* ToBoolString(bool value)
{
    return value ? "True" : "False";
}

inline bool ParseBool(const std::string& value)
{
    // An absent key reads back as an empty string and stays false, which is
    // what every reader assumed before this helper existed.
    static const char kTrue[] = "true";
    const size_t length = sizeof(kTrue) - 1;
    if (value.length() != length) {
        return false;
    }
    for (size_t i = 0; i < length; i++) {
        if (std::tolower(static_cast<unsigned char>(value[i])) != kTrue[i]) {
            return false;
        }
    }
    return true;
}

} // namespace LWEDelegate

#endif
