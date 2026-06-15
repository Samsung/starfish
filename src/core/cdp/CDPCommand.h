/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPCommand__)
#define __StarfishCDPCommand__

#include "rapidjson/document.h"
#include <cstdint>
#include <string>

namespace Starfish {

class CDPDispatcher;

// Request context handed to a domain handler. Main-thread only, stack/short
// lived. GC: not inherited.
class CDPCommand {
public:
    CDPCommand(CDPDispatcher* d, Optional<int64_t> id,
               const std::string& sessionId, rapidjson::Value* params);

    CDPDispatcher* dispatcher()
    {
        return m_dispatcher;
    }
    rapidjson::Value* params()
    {
        return m_params;
    } // nullable
    const std::string& sessionId() const
    {
        return m_sessionId;
    }
    bool hasId() const
    {
        return m_id.hasValue();
    }

    void sendResult(rapidjson::Value& result, rapidjson::Document& doc);
    void sendResultEmpty();
    void sendError(int code, const char* message);

    // Emit an event (no id). sessionId of this command is attached if present.
    void sendEvent(const char* method, rapidjson::Value& params,
                   rapidjson::Document& doc);

private:
    void emit(rapidjson::Document& doc);

    CDPDispatcher* m_dispatcher;
    Optional<int64_t> m_id;
    std::string m_sessionId;
    rapidjson::Value* m_params;
};

} // namespace Starfish

#endif
