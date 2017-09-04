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

class SerializedTypedData;
class ResourceURL;
class WebView;
class HTMLIFrameElement;
class HTMLFormElement;

class HistoryManager : public gc {
    friend class HTMLFormElement;

public:
    const uint32_t MAX_ENTRY_SIZE = 256;
    enum Action { Add, Replace, Intact };
    class HistoryEntry : public gc {
    public:
        HistoryEntry(SerializedTypedData* state, String* title,
                     ResourceURL* url)
            : m_state(state)
            , m_title(title)
            , m_url(url)
        {
        }

        SerializedTypedData* state()
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

        void init(SerializedTypedData* state, String* title, ResourceURL* url)
        {
            m_state = state;
            m_title = title;
            m_url = url;
        }

    private:
        SerializedTypedData* m_state;
        String* m_title;
        ResourceURL* m_url;
    };

    static HistoryManager* create(WebView* webView);
    static HistoryManager* create(HTMLIFrameElement* iframe);

    void go(int delta);
    uint32_t length();
    void pushState(Document* document, ScriptValue state, String* title,
                   Nullable<String*> url);
    void replaceState(Document* document, ScriptValue state, String* title,
                      Nullable<String*> url);
    ScriptValue state(Document* document);

    void push(Document* document, ResourceURL* url);
    void replace(Document* document, ResourceURL* url);

    HistoryEntry* currentEntry();

private:
    HistoryManager(WebView* webView);
    HistoryManager(HTMLIFrameElement* iframe);
    void addHistoryEntry(HistoryEntry* entry);

    enum HistoryManagerOwner {
        OwnerIsWebView,
        OwnerIsHTMLIFrame,
    };
    HistoryManagerOwner m_ower;

    union {
        WebView* m_webView;
        HTMLIFrameElement* m_iframe;
    };

    GCList<HistoryEntry*> m_historyEntries;
    GCList<HistoryEntry*>::iterator m_curEntry;
};
}

#endif
