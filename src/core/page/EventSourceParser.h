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

#ifndef __StarFishEventSourceParser__
#define __StarFishEventSourceParser__

namespace StarFish {

class EventSourceParser : public gc {
public:
    class Client : public gc {
    public:
        virtual void onMessageEvent(String* type, String* data,
                                    String* lastEventId) = 0;
        virtual void onReconnectionTimeSet(
            unsigned long long reconnectionTime) = 0;
    };

    EventSourceParser(String* lastEventId, Client* client);
    String* lastEventId() const
    {
        return m_lastEventId;
    }

    void stop()
    {
        m_isStopped = true;
    }

    void addBytes(const char* bytes, size_t size);

private:
    void parseLine();
    String* fromUTF8(const char* bytes, size_t);

    GCVector<char> m_line;
    String* m_eventType;
    GCVector<char> m_data;

    String* m_id;
    String* m_lastEventId;
    Client* m_client;

    bool m_isRecognizingCRLF;
    bool m_isRecognizingBOM;
    bool m_isStopped;
};
}

#endif
