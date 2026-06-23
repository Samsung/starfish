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

#if defined(STARFISH_ENABLE_TEST)

#include "APIRecorder.h"

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <cstring>

namespace LWERecord {

uint64_t currentMicros()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

std::string escapeJsonString(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (c < 0x20) {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", (unsigned)c);
                out += buf;
            } else {
                out += (char)c;
            }
        }
    }
    return out;
}

APIRecorder& APIRecorder::instance()
{
    static APIRecorder s_instance;
    return s_instance;
}

void APIRecorder::initialize()
{
    if (m_initialized)
        return;
    m_initialized = true;

    const char* path = getenv("STARFISH_API_RECORD");

    std::string pathFromFile;
    if (!path || path[0] == '\0') {
        // Fall back to a trigger file so recording can be activated without
        // env vars (useful on Tizen where the app environment is sealed).
        // fopen failure (no file, no permission) is silently ignored.
        FILE* trigger = fopen("/tmp/starfish_api_record", "r");
        if (trigger) {
            char buf[4096];
            if (fgets(buf, sizeof(buf), trigger)) {
                size_t len = strlen(buf);
                if (len > 0 && buf[len - 1] == '\n')
                    buf[len - 1] = '\0';
                if (buf[0] != '\0') {
                    pathFromFile = buf;
                    path = pathFromFile.c_str();
                }
            }
            fclose(trigger);
        }
    }

    if (!path || path[0] == '\0')
        return;

    m_file = fopen(path, "w");
    if (!m_file) {
        fprintf(stderr, "[APIRecorder] Failed to open: %s\n", path);
        return;
    }

    const char* noFlush = getenv("STARFISH_API_RECORD_NO_FLUSH");
    m_noFlush = (noFlush && noFlush[0] == '1');

    m_startUs = currentMicros();
    fprintf(stderr, "[APIRecorder] Recording to: %s\n", path);
}

void APIRecorder::recordHeader(unsigned w, unsigned h, float dpr,
                               const char* font, const char* locale,
                               const char* tz)
{
    if (!m_file)
        return;
    fprintf(m_file,
            "{\"type\":\"header\",\"ts_us\":0,\"args\":"
            "{\"version\":1,\"w\":%u,\"h\":%u,\"dpr\":%.3f,"
            "\"font\":\"%s\",\"locale\":\"%s\",\"tz\":\"%s\"}}\n",
            w, h, dpr, escapeJsonString(font ? font : "").c_str(),
            escapeJsonString(locale ? locale : "").c_str(),
            escapeJsonString(tz ? tz : "").c_str());
    if (!m_noFlush)
        fflush(m_file);
}

void APIRecorder::recordEvent(const char* type, const char* argsJson)
{
    if (!m_file)
        return;
    uint64_t tsUs = currentMicros() - m_startUs;
    fprintf(m_file, "{\"type\":\"%s\",\"ts_us\":%" PRIu64 ",\"args\":%s}\n",
            type, tsUs, argsJson);
    if (!m_noFlush)
        fflush(m_file);
}

void APIRecorder::recordStringEvent(const char* type, const char* key,
                                    const std::string& value, size_t maxLen)
{
    if (!m_file)
        return;
    const std::string& src =
        value.size() > maxLen ? value.substr(0, maxLen) : value;
    std::string escaped = escapeJsonString(src);
    uint64_t tsUs = currentMicros() - m_startUs;
    fprintf(m_file,
            "{\"type\":\"%s\",\"ts_us\":%" PRIu64
            ",\"args\":{\"%s\":\"%s\"}}\n",
            type, tsUs, key, escaped.c_str());
    if (!m_noFlush)
        fflush(m_file);
}

void APIRecorder::recordTwoStringEvent(const char* type, const char* key1,
                                       const std::string& v1, const char* key2,
                                       const std::string& v2)
{
    if (!m_file)
        return;
    std::string e1 = escapeJsonString(v1);
    std::string e2 = escapeJsonString(v2);
    uint64_t tsUs = currentMicros() - m_startUs;
    fprintf(m_file,
            "{\"type\":\"%s\",\"ts_us\":%" PRIu64
            ",\"args\":{\"%s\":\"%s\",\"%s\":\"%s\"}}\n",
            type, tsUs, key1, e1.c_str(), key2, e2.c_str());
    if (!m_noFlush)
        fflush(m_file);
}

void APIRecorder::finalize()
{
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
    m_initialized = false;
}

} // namespace LWERecord

#endif // STARFISH_ENABLE_TEST
