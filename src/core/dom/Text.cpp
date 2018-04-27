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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Text.h"
#include "core/dom/Traverse.h"
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
    if (offset > data()->length()) {
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

Node* Text::mergeWithTextSiblings()
{
    if (!length()) {
        Node* next = Traverse::nextPostOrder(this, nullptr);
        remove();
        return next;
    }

    while (Node* next = nextSibling()) {
        if (next->nodeType() != TEXT_NODE || next->isCDATASection()) {
            break;
        }

        Text* nextText = next->asText();
        if (!nextText->length()) {
            nextText->remove();
            continue;
        }

        appendData(nextText->data());
        nextText->setData(String::emptyString);
        nextText->remove();
    }

    return Traverse::nextPostOrder(this, nullptr);
}
}
