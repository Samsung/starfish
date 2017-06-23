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

#ifndef __StarFishHistoryManager__
#define __StarFishHistoryManager__

#include "binding/ScriptWrappable.h"

namespace StarFish {

class ResourceURL;
class WebView;

class HistoryManager : public gc {
public:
    const uint32_t MAX_ENTRY_SIZE = 256;
    class HistoryEntry : public gc {
    public:
        HistoryEntry(ScriptValue state, String* title, ResourceURL* url)
            : m_state(state)
            , m_title(title)
            , m_url(url)
        {
        }

        ScriptValue state()
        {
            return m_state;
        }

        String* title()
        {
            return m_title;
        }

        ResourceURL* url()
        {
            return m_url;
        }

        void init(ScriptValue state, String* title, ResourceURL* url)
        {
            m_state = state;
            m_title = title;
            m_url = url;
        }

    private:
        ScriptValue m_state;
        String* m_title;
        ResourceURL* m_url;
    };

    static HistoryManager* create(WebView* webView);

    void go(int delta);
    uint32_t length();
    void pushState(ScriptValue state, String* title, Nullable<String*> url);
    void replaceState(ScriptValue state, String* title, Nullable<String*> url);
    ScriptValue state();

    void push(ResourceURL* url);

private:
    HistoryManager(WebView* webView);
    void addHistoryEntry(HistoryEntry* entry);
    HistoryEntry* currentEntry();

    WebView* m_webView;
    GCList<HistoryEntry*> m_historyEntries;
    std::list<HistoryEntry*,
              gc_allocator_ignore_off_page<HistoryEntry*>>::iterator m_curEntry;
};
}

#endif
