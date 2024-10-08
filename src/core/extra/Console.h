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

#ifndef __StarfishConsole__
#define __StarfishConsole__

#include <cstdint>

namespace Starfish {

class WebBase;

#define CONSOLE_APIS(F) \
    F(time)             \
    F(timeLog)          \
    F(timeEnd)          \
    F(group)            \
    F(groupCollapsed)   \
    F(groupEnd)         \
    F(log)              \
    F(info)             \
    F(error)            \
    F(warn)             \
    F(debug)

// https://console.spec.whatwg.org/#loglevel-severity
enum class LogLevel : uint8_t {
    Log, // log(), trace(), dir(), dirxml(), group(), groupCollapsed(), debug(),
         // timeLog()
    Info,  // count(), info(), timeEnd()
    Warn,  // warn(), countReset()
    Error, // error(), assert()
};

class Console : public gc {
public:
    Console(WebBase* webBase);
    void log(String* data);
    void info(String* data);
    void error(String* data);
    void warn(String* data);
    void debug(String* data);
    void time(String* label);
    void timeLog(String* label, Optional<String*> data);
    void timeEnd(String* label);
    void group(String* data);
    void groupCollapsed(String* data);
    void groupEnd();

protected:
    String* makeTimeString(String* label, Optional<String*> data);
    void printMessage(LogLevel level, String* tag, String* dataessage);

    WebBase* m_webBase = nullptr;
    GCUnorderedMap<String*, uint64_t> m_times;
    GCVector<Optional<String*>> m_groupStack;
};
} // namespace Starfish

#endif
