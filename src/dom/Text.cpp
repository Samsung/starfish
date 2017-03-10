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
#include "Text.h"
#include "Traverse.h"

#include "Document.h"

namespace StarFish {

String* Text::nodeName()
{
    return document()->window()->starFish()->staticStrings()->m_textLocalName;
}

String* Text::localName()
{
    return document()->window()->starFish()->staticStrings()->m_textLocalName;
}

// Text* Text::splitText(unsigned long offset)
// {
//    return nullptr;
// }

String* Text::wholeText()
{
    GCVector<Node*> pSiblings;
    GCVector<Node*> nSiblings;
    Node* sibling = previousSibling();
    while (sibling) {
        if (sibling->isText()) {
            pSiblings.push_back(sibling);
        }
        sibling = sibling->previousSibling();
    }

    sibling = nextSibling();
    while (sibling) {
        if (sibling->isText()) {
            nSiblings.push_back(sibling);
        }
        sibling = sibling->nextSibling();
    }

    String* str = String::createASCIIString("");
    std::for_each(pSiblings.rbegin(), pSiblings.rend(),
                  [&](Node* n) { str = str->concat(n->asText()->data()); });

    str = str->concat(data());

    std::for_each(nSiblings.begin(), nSiblings.end(),
                  [&](Node* n) { str = str->concat(n->asText()->data()); });

    return str;
}
}
