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

#ifndef __StarFishStorage__
#define __StarFishStorage__

#include "binding/ScriptWrappable.h"
#include "binding/WindowHoldable.h"

namespace StarFish {

class Storage : public ScriptWrappable, public WindowHoldable {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isStorage() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return WindowHoldable::scriptBindingInstance();
    }

    // 4.1 Storage interface in IDL
    virtual unsigned long length() = 0;
    virtual Nullable<String*> key(unsigned long index) = 0;
    virtual Nullable<String*> getItem(String* key) = 0;
    virtual void setItem(String* key, String* value) = 0;
    virtual void removeItem(String* key) = 0;
    virtual void clear() = 0;

protected:
    Storage(Window* window)
        : ScriptWrappable(this)
        , WindowHoldable(window)
    {
    }
};
}

#endif
