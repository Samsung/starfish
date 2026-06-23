/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishAPIReplayer__
#define __StarfishAPIReplayer__

#if defined(STARFISH_ENABLE_TEST)

#include "LWEWebView.h"
#include <string>
#include <vector>
#include <utility>
#include <cstdint>

namespace StarfishShell {

struct ReplayEvent {
    std::string type;
    uint64_t tsUs = 0;
    std::string strArg;  // url, data, script, text, ua, object name
    std::string strArg2; // function name (JS interface events)
    double x = 0, y = 0;
    int button = 0, buttons = 0, delta = 0, key = 0;
    int iX = 0, iY = 0;
    int intArg = 0; // cache mode, font size
    size_t uW = 0, uH = 0;
    float dpr = 0;
    std::vector<std::pair<std::string, std::string>> settings; // SetSettings
};

struct ReplayHeader {
    unsigned width = 1920, height = 1080;
    float devicePixelRatio = 1.0f;
};

class APIReplayer {
public:
    bool load(const char* path);
    const ReplayHeader& header() const
    {
        return m_header;
    }
    const std::vector<ReplayEvent>& events() const
    {
        return m_events;
    }

    // Schedules all events via AddTimeout — non-blocking, returns immediately.
    void startReplay(LWE::WebContainer* wc, float speedFactor = 1.0f);

private:
    bool parseLine(const char* line, size_t len);

    ReplayHeader m_header;
    std::vector<ReplayEvent> m_events;
};

} // namespace StarfishShell

#endif // STARFISH_ENABLE_TEST
#endif // __StarfishAPIReplayer__
