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
#include "binding/DocumentHoldable.h"

namespace StarFish {

class HistoryManager;
class HTMLFormElement;
class HTMLResourceClient;
class NetworkURLResourceRequestJobDelegate;
class StarFish;
class URL;

class History : public ScriptWrappable, public DocumentHoldable {
    friend class HTMLFormElement;
    friend class HTMLResourceClient;
    friend class NetworkURLResourceRequestJobDelegate;

public:
    History(Document* doc);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHistory() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return DocumentHoldable::scriptBindingInstance();
    }

    uint32_t length();
    ScriptValue state();

    void back();
    void forward();
    void go(int delta);
    bool canGoBack();
    bool canGoForward();

    void pushState(ScriptValue state, String* title, Nullable<String*> url);
    void replaceState(ScriptValue state, String* title, Nullable<String*> url);

private:
    HistoryManager* historyManager();
};
}
#endif
