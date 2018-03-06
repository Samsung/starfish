/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
