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

#ifndef __StarFishHistory__
#define __StarFishHistory__

#include "binding/ScriptWrappable.h"
#include "platform/window/Window.h"
#include "dom/Document.h"

namespace StarFish {

class HistoryEntry;

class History : public ScriptWrappable {
public:
    History(StarFish* starFish);
    virtual ~History()
    {
    }

    StarFish* starFish()
    {
        return m_starFish;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isHistory() const override
    {
        return true;
    }

    size_t length();
    ScriptValue state();

    void back();
    void forward();
    void go(int delta);

    void pushState(ScriptValue state, String* title, String* url);
    void replaceState(ScriptValue state, String* title, String* url);

    void setHistory(ScriptValue state, String* title, URL* url,
                    bool isPushState = false);

    HistoryEntry* currentHistoryEntry()
    {
        return m_historyEntries[m_offset];
    }

    /*
    // TODO: implement functions below
    void setScrollRestoration(const String& value);
    String scrollRestoration();
    bool stateChanged() const;
    bool isSameAsCurrentState(StateObject*) const;
    */
protected:
    StarFish* m_starFish;
    GCVector<HistoryEntry*> m_historyEntries;
    size_t m_offset;
    URL* getURL();

    bool navigate(int offset);
    GCVector<HistoryEntry*>& history();
    bool isPushState();
};
}
#endif
