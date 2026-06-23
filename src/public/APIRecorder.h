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

#ifndef __StarfishAPIRecorder__
#define __StarfishAPIRecorder__

#if defined(STARFISH_ENABLE_TEST)

#include <cstdint>
#include <cstdio>
#include <string>

namespace LWERecord {

uint64_t currentMicros();
std::string escapeJsonString(const std::string& s);

class APIRecorder {
public:
    static APIRecorder& instance();

    // Parse STARFISH_API_RECORD env var and open the output file. No-op after
    // first call.
    void initialize();

    bool isRecording() const
    {
        return m_file != nullptr;
    }

    void recordHeader(unsigned w, unsigned h, float dpr, const char* font,
                      const char* locale, const char* tz);
    void recordEvent(const char* type, const char* argsJson);
    // For string-valued args: handles JSON escaping and optional truncation.
    void recordStringEvent(const char* type, const char* key,
                           const std::string& value, size_t maxLen = 65535);
    // For events with two string args (e.g. object name + function name).
    void recordTwoStringEvent(const char* type, const char* key1,
                              const std::string& v1, const char* key2,
                              const std::string& v2);
    void finalize();

private:
    APIRecorder() = default;
    ~APIRecorder()
    {
        finalize();
    }

    FILE* m_file = nullptr;
    uint64_t m_startUs = 0;
    bool m_initialized = false;
    bool m_noFlush = false;
};

} // namespace LWERecord

// Activates recording initialization (call from WebContainer::Create*).
#define STARFISH_API_RECORD_INIT() \
    LWERecord::APIRecorder::instance().initialize()

// Records the WebContainer creation parameters as a header event.
#define STARFISH_API_RECORD_HEADER(w, h, dpr, font, locale, tz) \
    LWERecord::APIRecorder::instance().recordHeader(w, h, dpr, font, locale, tz)

// Records a zero-argument API event (pass "{}" as argsJson).
#define STARFISH_API_RECORD_EVENT(type, argsJson)                           \
    do {                                                                    \
        if (LWERecord::APIRecorder::instance().isRecording())               \
            LWERecord::APIRecorder::instance().recordEvent(type, argsJson); \
    } while (0)

// Records an API event with a single string argument.
#define STARFISH_API_RECORD_EVENT_STR(type, key, str)                       \
    do {                                                                    \
        if (LWERecord::APIRecorder::instance().isRecording())               \
            LWERecord::APIRecorder::instance().recordStringEvent(type, key, \
                                                                 str);      \
    } while (0)

// Records an API event with two string arguments.
#define STARFISH_API_RECORD_EVENT_STR2(type, key1, v1, key2, v2)     \
    do {                                                             \
        if (LWERecord::APIRecorder::instance().isRecording())        \
            LWERecord::APIRecorder::instance().recordTwoStringEvent( \
                type, key1, v1, key2, v2);                           \
    } while (0)

// Records an API event with numeric arguments (uses snprintf internally).
#define STARFISH_API_RECORD_EVENT_FMT(type, fmt, ...)                    \
    do {                                                                 \
        if (LWERecord::APIRecorder::instance().isRecording()) {          \
            char _args[256];                                             \
            snprintf(_args, sizeof(_args), fmt, ##__VA_ARGS__);          \
            LWERecord::APIRecorder::instance().recordEvent(type, _args); \
        }                                                                \
    } while (0)

#else // !STARFISH_ENABLE_TEST — production builds: all macros are no-ops

#define STARFISH_API_RECORD_INIT() ((void)0)
#define STARFISH_API_RECORD_HEADER(...) ((void)0)
#define STARFISH_API_RECORD_EVENT(type, args) ((void)0)
#define STARFISH_API_RECORD_EVENT_STR(t, k, s) ((void)0)
#define STARFISH_API_RECORD_EVENT_STR2(t, k1, v1, k2, v2) ((void)0)
#define STARFISH_API_RECORD_EVENT_FMT(t, f, ...) ((void)0)

#endif // STARFISH_ENABLE_TEST

#endif // __StarfishAPIRecorder__
