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

#include "StarFishConfig.h"
#include "HTMLCollection.h"
#include "dom/Document.h"
#include "dom/HTMLElement.h"
#include "dom/Traverse.h"

namespace StarFish {

unsigned long HTMLCollection::length() const
{
    return m_activeNodeList.length();
}

Element* HTMLCollection::item(unsigned long index)
{
    return m_activeNodeList.item(index)->asElement();
}

Element* HTMLCollection::namedItem(String* key)
{
    if (key->length()) {
        for (unsigned i = 0; i < m_activeNodeList.length(); i++) {
            Element* elem = m_activeNodeList.item(i)->asElement();
            if (elem->asElement()->asHTMLElement()->id()->equals(key))
                return elem;
            size_t idx = elem->hasAttribute(elem->document()->window()->starFish()->staticStrings()->m_name);
            if (idx != SIZE_MAX) {
                if (elem->getAttribute(idx)->equals(key))
                    return elem;
            }
        }
    }
    return nullptr;
}

}
