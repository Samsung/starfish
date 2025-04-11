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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/extra/Console.h"
#include "core/inspector/Inspector.h"
#include "core/page/WebBase.h"
#include "core/modules/profiling/Profiling.h"

#if defined(STARFISH_ENABLE_TEST) && defined(STARFISH_WEBWORKER_NOT_HOST)
void starfishRecordTestFailure();
#endif

namespace Starfish {

Console::Console(WebBase* webBase)
    : m_webBase(webBase)
{
}

void Console::log(String* data)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_webBase->inspector()) {
        m_webBase->inspector()->sendInfoMessage(data);
    }
#endif
    printMessage(
        LogLevel::Log,
        AtomicString::createAtomicString(m_webBase->starfish(), "console.log"),
        data);
}

void Console::info(String* data)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_webBase->inspector()) {
        m_webBase->inspector()->sendInfoMessage(data);
    }
#endif
    printMessage(
        LogLevel::Info,
        AtomicString::createAtomicString(m_webBase->starfish(), "console.info"),
        data);
}

void Console::error(String* data)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_webBase->inspector()) {
        m_webBase->inspector()->sendErrorMessage(data);
    }
#endif
    printMessage(LogLevel::Error,
                 AtomicString::createAtomicString(m_webBase->starfish(),
                                                  "console.error"),
                 data);
}

void Console::warn(String* data)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_webBase->inspector()) {
        m_webBase->inspector()->sendWarnMessage(data);
    }
#endif
    printMessage(
        LogLevel::Warn,
        AtomicString::createAtomicString(m_webBase->starfish(), "console.warn"),
        data);
}

void Console::debug(String* data)
{
#if defined(STARFISH_ENABLE_INSPECTOR)
    if (m_webBase->inspector()) {
        m_webBase->inspector()->sendDebugMessage(data);
    }
#endif
    printMessage(LogLevel::Log,
                 AtomicString::createAtomicString(m_webBase->starfish(),
                                                  "console.debug"),
                 data);
}

void Console::time(String* label)
{
    if (m_times.find(label) != m_times.end()) {
        STARFISH_LOG_ERROR("Timer %s has already been started",
                           label->toUTF8NonGCString().c_str());
        return;
    }

    m_times[label] = longTickCount();
}

void Console::timeLog(String* label, Optional<String*> data)
{
    if (m_times.find(label) == m_times.end()) {
        STARFISH_LOG_ERROR("Timer '%s' does not exist",
                           label->toUTF8NonGCString().c_str());
        return;
    }

    String* timeString = makeTimeString(label, data);
    printMessage(LogLevel::Log,
                 AtomicString::createAtomicString(m_webBase->starfish(),
                                                  "console.timeLog"),
                 timeString);
}

void Console::timeEnd(String* label)
{
    if (m_times.find(label) == m_times.end()) {
        STARFISH_LOG_ERROR("Timer '%s' does not exist",
                           label->toUTF8NonGCString().c_str());
        return;
    }

    String* timeString = makeTimeString(label, nullptr);
    printMessage(LogLevel::Info,
                 AtomicString::createAtomicString(m_webBase->starfish(),
                                                  "console.timeEnd"),
                 timeString);

    m_times.erase(label);
}

void Console::group(String* data)
{
    // Insert indentation only until supprot interactive groups.
    //
    // Interactive groups are only meaningful if the interactive console view is
    // supported. In our case the Inspector represents it. but this currently
    // doesn't work properly and needs further improvement.

    m_groupStack.push_back(data);
    printMessage(LogLevel::Log,
                 AtomicString::createAtomicString(m_webBase->starfish(),
                                                  "console.group"),
                 data);
}

void Console::groupCollapsed(String* data)
{
    // Forward to group() until supprot interactive groups.
    group(data);
}

void Console::groupEnd()
{
    if (m_groupStack.size()) {
        m_groupStack.pop_back();
    }
}

void Console::assertion(bool condition, Optional<String*> data)
{
    if (!condition) {
        printMessage(LogLevel::Error,
                     AtomicString::createAtomicString(m_webBase->starfish(),
                                                      "Assertion failed"),
                     data.hasValue()
                         ? data.value()
                         : AtomicString::createAtomicString(
                               m_webBase->starfish(), "console.assert"));
#if defined(STARFISH_ENABLE_TEST) && defined(STARFISH_WEBWORKER_NOT_HOST)
        starfishRecordTestFailure();
#endif
    } else {
#if defined(STARFISH_ENABLE_TEST)
        printMessage(LogLevel::Error,
                     AtomicString::createAtomicString(m_webBase->starfish(),
                                                      "Assertion ok"),
                     data.hasValue()
                         ? data.value()
                         : AtomicString::createAtomicString(
                               m_webBase->starfish(), "console.assert"));
#endif
    }
}

String* Console::makeTimeString(String* label, Optional<String*> data)
{
    uint64_t now = longTickCount();
    uint64_t start = m_times[label];
    double elapsed = (now - start) / 1000.0;
    std::string elapsedStr = std::to_string(elapsed);

    StringBuilder builder;
    builder.appendString(label);
    builder.appendString(": ");
    builder.appendString(elapsedStr.c_str(), elapsedStr.length());
    builder.appendString("ms");

    if (data) {
        builder.appendChar(' ');
        builder.appendString(data.value());
    }

    return builder.finalize();
}

void Console::printMessage(LogLevel level, String* tag, String* message)
{
    StringBuilder builder;
    builder.appendString(tag);
    builder.appendChar(':');
    builder.appendChar(' ');

    if (m_groupStack.size()) {
        // Start from index 1.
        for (size_t i = 1; i < m_groupStack.size(); ++i) {
            builder.appendString("  ");
        }
    }

    builder.appendString(message);
    String* finalMessage = builder.finalize();

#if defined(STARFISH_TIZEN_PROD_TV) && !defined(STARFISH_ENABLE_TEST)
    finalMessage->peekUTF8Buffer(
        [](const char* buf, size_t len, void* data) -> size_t {
            STARFISH_LOG_ERROR("%s", buf);
            return 0;
        },
        nullptr);
#else
    switch (level) {
    case LogLevel::Log:
    case LogLevel::Info:
        finalMessage->peekUTF8Buffer(
            [](const char* buf, size_t len, void* data) -> size_t {
                STARFISH_LOG_INFO("%s", buf);
                return 0;
            },
            nullptr);
        break;
    case LogLevel::Warn:
        finalMessage->peekUTF8Buffer(
            [](const char* buf, size_t len, void* data) -> size_t {
                STARFISH_LOG_WARN("%s", buf);
                return 0;
            },
            nullptr);
        break;
    case LogLevel::Error:
        finalMessage->peekUTF8Buffer(
            [](const char* buf, size_t len, void* data) -> size_t {
                STARFISH_LOG_ERROR("%s", buf);
                return 0;
            },
            nullptr);
        break;
    }
#endif
}

} // namespace Starfish
