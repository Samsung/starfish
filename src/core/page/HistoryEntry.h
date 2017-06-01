/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "binding/ScriptWrappable.h"

namespace StarFish {

class URL;

class HistoryEntry : public gc {
public:
    HistoryEntry(ScriptValue state, String* title, ResourceURL* url,
                 bool isPushState)
        : m_state(state)
        , m_title(title)
        , m_url(url)
        , m_isPushState(isPushState)
    {
    }
    inline ScriptValue state()
    {
        return m_state;
    }
    inline String* title()
    {
        return m_title;
    }
    inline ResourceURL* url()
    {
        return m_url;
    }
    inline bool isPushState()
    {
        return m_isPushState;
    }

    void replaceState(ScriptValue state, String* title, ResourceURL* url)
    {
        m_state = state;
        m_title = title;
        m_url = url;
    }

private:
    ScriptValue m_state;
    String* m_title;
    ResourceURL* m_url;
    bool m_isPushState;
};
}
#endif
