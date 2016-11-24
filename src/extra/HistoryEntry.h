/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#ifndef __StarFishHistoryEntry__
#define __StarFishHistoryEntry__

namespace StarFish {

class URL;
class String;

class HistoryEntry : public gc {
public:
    HistoryEntry(String* state, String* title, URL* url, bool isPushState)
        : m_state(state)
        , m_title(title)
        , m_url(url)
        , m_isPushState(isPushState)
    {
    }
    inline String* state() { return m_state; }
    inline String* title() { return m_title; }
    inline URL* url() { return m_url; }
    inline bool isPushState() { return m_isPushState; }

    void replaceState(String* state, String* title, URL* url)
    {
        m_state = state;
        m_title = title;
        m_url = url;
    }
private:
    String* m_state;
    String* m_title;
    URL* m_url;
    bool m_isPushState;
};

}
#endif
