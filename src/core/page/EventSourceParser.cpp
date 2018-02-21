// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "EventSourceParser.h"
#include "core/page/EventSource.h"

namespace StarFish {

EventSourceParser::EventSourceParser(String* lastEventId, Client* client)
    : m_eventType(String::emptyString)
    , m_id(lastEventId)
    , m_lastEventId(lastEventId)
    , m_client(client)
    , m_isRecognizingCRLF(false)
    , m_isRecognizingBOM(true)
    , m_isStopped(false)
{
}

static void append(GCVector<char>& src, const char* dst, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        src.push_back(dst[i]);
    }
}

void EventSourceParser::addBytes(const char* bytes, size_t size)
{
    size_t start = 0;
    const unsigned char kBOM[] = { 0xef, 0xbb, 0xbf };
    for (size_t i = 0; i < size && !m_isStopped; ++i) {
        // As kBOM contains neither CR nor LF, we can think BOM and the line
        // break separately.
        if (m_isRecognizingBOM && m_line.size() + (i - start) == 3) {
            GCVector<char> line = m_line;
            append(line, &bytes[start], i - start);
            STARFISH_ASSERT(line.size() == 3);
            m_isRecognizingBOM = false;
            if (memcmp(line.data(), kBOM, sizeof(kBOM)) == 0) {
                start = i;
                m_line.clear();
                continue;
            }
        }
        if (m_isRecognizingCRLF && bytes[i] == '\n') {
            // This is the latter part of "\r\n".
            m_isRecognizingCRLF = false;
            ++start;
            continue;
        }
        m_isRecognizingCRLF = false;
        if (bytes[i] == '\r' || bytes[i] == '\n') {
            append(m_line, &bytes[start], i - start);
            parseLine();
            m_line.clear();
            start = i + 1;
            m_isRecognizingCRLF = bytes[i] == '\r';
            m_isRecognizingBOM = false;
        }
    }
    if (m_isStopped) {
        return;
    }
    append(m_line, &bytes[start], size - start);
}
void EventSourceParser::EventSourceParser::parseLine()
{
    if (m_line.size() == 0) {
        m_lastEventId = m_id;
        // We dispatch an event when seeing an empty line.
        if (!m_data.empty()) {
            STARFISH_ASSERT(m_data[m_data.size() - 1] == '\n');
            String* data = String::fromUTF8(m_data.data(), m_data.size());
            String* messageType = String::createASCIIString("message");

            m_client->onMessageEvent(m_eventType->isEmpty() ? messageType
                                                            : m_eventType,
                                     data, m_lastEventId);
            m_data.clear();
        }
        m_eventType = String::emptyString;
        return;
    }
    auto fieldNameEnd = std::find(m_line.begin(), m_line.end(), ':');
    auto fieldValueStart = fieldNameEnd;
    if (fieldNameEnd == m_line.end()) {
        fieldNameEnd = m_line.end();
        fieldValueStart = fieldNameEnd;
    } else {
        fieldValueStart = fieldNameEnd + 1;
        if (fieldValueStart < m_line.end() && *fieldValueStart == ' ') {
            ++fieldValueStart;
        }
    }
    size_t fieldValueSize = m_line.end() - fieldValueStart;
    String* fieldName =
        String::fromUTF8(m_line.data(), fieldNameEnd - m_line.begin());

    if (fieldName->equals("event")) {
        m_eventType = String::fromUTF8(
            m_line.data() + (fieldValueStart - m_line.begin()), fieldValueSize);
        return;
    }
    if (fieldName->equals("data")) {
        append(m_data, m_line.data() + (fieldValueStart - m_line.begin()),
               fieldValueSize);
        append(m_data, "\n", 1);
        return;
    }
    if (fieldName->equals("id")) {
        m_id = String::fromUTF8(
            m_line.data() + (fieldValueStart - m_line.begin()), fieldValueSize);
        return;
    }
    if (fieldName->equals("retry")) {
        bool hasOnlyDigits = true;
        for (size_t i = fieldValueStart - m_line.begin();
             i < m_line.size() && hasOnlyDigits; ++i) {
            hasOnlyDigits = String::isASCIIDigit(m_line[i]);
        }
        if (fieldValueStart == m_line.end()) {
            m_client->onReconnectionTimeSet(EventSource::defaultReconnectDelay);
        } else if (hasOnlyDigits) {
            int64_t reconnectionTime = String::parseInt64(String::fromUTF8(
                m_line.data() + (fieldValueStart - m_line.begin()),
                fieldValueSize));

            if (reconnectionTime) {
                m_client->onReconnectionTimeSet(reconnectionTime);
            }
        }
        return;
    }
    // Unrecognized field name. Ignore!
}
}
