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
#include "core/dom/Text.h"
#include "core/dom/DOMException.h"

namespace StarFish {

String* Text::nodeName()
{
    return starFish()->staticStrings()->m_textLocalName;
}

String* Text::localName()
{
    return starFish()->staticStrings()->m_textLocalName;
}

Text* Text::splitText(unsigned long offset)
{
    // INDEX_SIZE_ERR
    // Raised if the specified offset is negative or greater than
    // the number of 16-bit units in data.
    if (offset < 0 || offset > data()->length()) {
        throw new DOMException(document(), DOMException::Code::INDEX_SIZE_ERR);
    }
    String* oldValue = data();
    String* newA = String::emptyString;
    String* newB = String::emptyString;
    size_t aBegin = 0;
    size_t aCount = offset;
    size_t bBegin = offset;
    size_t bCount = data()->length() - offset;
    if (aCount == 0) {
        newB = oldValue;
    } else if (bCount == 0) {
        newA = oldValue;
    } else {
        newA = oldValue->substring(aBegin, aCount);
        newB = oldValue->substring(bBegin, bCount);
    }
    setData(newA);
    Text* result = new Text(document(), newB);
    if (parentNode()) {
        if (nextSibling()) {
            parentNode()->insertBefore(result, nextSibling());
        } else {
            parentNode()->appendChild(result);
        }
    }
    return result;
}

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
