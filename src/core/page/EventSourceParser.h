/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarFishEventSourceParser__
#define __StarFishEventSourceParser__

namespace StarFish {

class EventSourceParser : public gc {
public:
    class Client : public gc {
    public:
        virtual ~Client()
        {
        }
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
