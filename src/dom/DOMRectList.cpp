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

#include "dom/DOMRect.h"
#include "dom/DOMRectList.h"
#include "dom/DOMQuad.h"

namespace StarFish {

DOMRectList::DOMRectList()
    : ScriptWrappable(this)
{
}

DOMRectList::DOMRectList(const std::vector<DOMQuad>& quads)
    : ScriptWrappable(this)
{
    m_list.reserve(quads.size());
    for (size_t i = 0; i < quads.size(); ++i) {
        m_list.push_back(new DOMRect(quads[i].bounds()));
    }
}

static ESValue readCallbackFunction(const ESValue& key, ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    DOMRectList* self = (DOMRectList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isDOMRectList());
    uint32_t idx = key.toIndex();
    if (idx < self->length()) {
        return self->item(idx)->scriptValue();
    }
    return ESValue(ESValue::ESDeletedValue);
}

static bool writeCallbackFunction(const ESValue& key, const ESValue& val,
                                  ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    return false;
}

static ESValueVector enumerateCallbackFunction(ESObject* obj)
{
    STARFISH_ASSERT(obj->extraData() == kEscargotObjectCheckMagic);
    DOMRectList* self = (DOMRectList*)obj->extraPointerData();
    STARFISH_ASSERT(self->isDOMRectList());
    size_t len = self->length();
    ESValueVector v(len);
    for (size_t i = 0; i < len; i++) {
        v[i] = ESValue(i);
    }
    return v;
}

void DOMRectList::init(ScriptBindingInstance* instance)
{
    scriptObject()->set__proto__(
        fetchData(instance)->fnDOMRectList()->protoType());
    scriptObject()->setPropertyInterceptor(readCallbackFunction,
                                           writeCallbackFunction,
                                           enumerateCallbackFunction, true);
}

size_t DOMRectList::length() const
{
    return m_list.size();
}

DOMRect* DOMRectList::item(unsigned long index)
{
    if (index >= m_list.size()) {
        return nullptr;
    }

    return m_list[index];
}
}
