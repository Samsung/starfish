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

#ifndef __StarFishDOMRectList__
#define __StarFishDOMRectList__

#include <cstdio>
#include "util/String.h"
#include "dom/binding/ScriptWrappable.h"

namespace StarFish {

class DOMRect;

class DOMRectList : public ScriptWrappable {
public:
    static DOMRectList* create()
    {
        return new DOMRectList;
    }
    static DOMRectList* create(const std::vector<DOMQuad>& quads)
    {
        return new DOMRectList(quads);
    }
    unsigned long length() const;
    DOMRect* item(unsigned long index);

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this, instance);
    }

    virtual bool isDOMRectList() const
    {
        return true;
    }

private:
    DOMRectList();
    explicit DOMRectList(const std::vector<DOMQuad>&);

    GCVector<DOMRect*> m_list;
};
}

#endif
