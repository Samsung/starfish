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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/DocumentFragment.h"
#include "core/dom/Text.h"
#include "core/dom/Traverse.h"
#include "core/dom/HTMLElement.h"

namespace StarFish {

String* DocumentFragment::nodeName()
{
    return starFish()->staticStrings()->m_documentFragmentLocalName.string();
}

Node* DocumentFragment::clone()
{
    DocumentFragment* newNode = new DocumentFragment(document());
    return newNode;
}

Element* DocumentFragment::getElementById(String* id)
{
    if (id->length() == 0) {
        return nullptr;
    }
    return (Element*)Traverse::findDescendant(this, [&](Node* child) {
        if (child->isHTMLElement() && child->asHTMLElement()->hasId() &&
            child->asHTMLElement()->id()->equals(id)) {
            return true;
        } else {
            return false;
        }
    });
}
}
