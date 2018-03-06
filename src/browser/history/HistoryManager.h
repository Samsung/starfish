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

    bool go(int delta);
    bool canGo(int delta);
    void clear();

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
    bool checkHistoryEntry(int delta, bool changeCurrentEntry);

    enum HistoryManagerOwner {
        OwnerIsWebView,
        OwnerIsHTMLIFrame,
    };
    HistoryManagerOwner m_owner;

    union {
        WebView* m_webView;
        HTMLIFrameElement* m_iframe;
    };

    GCList<HistoryEntry*> m_historyEntries;
    GCList<HistoryEntry*>::iterator m_curEntry;
};
}

#endif
