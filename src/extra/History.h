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

#ifndef __StarFishHistory__
#define __StarFishHistory__

#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class HistoryEntry;

class History : public ScriptWrappable {
public:
    History(StarFish* starFish);
    virtual ~History() { }

    StarFish* starFish()
    {
        return m_starFish;
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual Type type()
    {
        return ScriptWrappable::Type::HistoryObject;
    }

    int length();

    void back();
    void forward();
    void go(int delta);

    void setHistory(URL* url);

/*
    // TODO: implement functions below
    StateObject* state();
    void pushState(StateObject* data, const String& title, const String& url);
    void replaceState(StateObject* data, const String& title, const String& url);
    void setScrollRestoration(const String& value);
    String scrollRestoration();
    bool stateChanged() const;
    bool isSameAsCurrentState(StateObject*) const;
*/
protected:
    StarFish* m_starFish;
    URL* getURL(int delta);
    int historyForwardListCount();
    int historyBackListCount();
    bool navigateBackForward(int offset);
    int& offset();
    std::vector<HistoryEntry*, gc_allocator_ignore_off_page<HistoryEntry*>>& history();
};

}
#endif
