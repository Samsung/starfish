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

#include "binding/ScriptWrappable.h"

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

    uint32_t length() const;
    DOMRect* item(uint32_t index);

    virtual void init(ScriptBindingInstance* instance) override;
    virtual void postInit(ScriptBindingInstance* instance) override;
    virtual bool isDOMRectList() const override;

private:
    DOMRectList();
    explicit DOMRectList(const std::vector<DOMQuad>&);

    GCVector<DOMRect*> m_list;
};
}

#endif
