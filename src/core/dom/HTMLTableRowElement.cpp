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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/HTMLTableRowElement.h"
#include "core/dom/HTMLTableElement.h"
#include "HTMLCollection.h"

namespace StarFish {

QualifiedName HTMLTableRowElement::name()
{
    return starFish()->staticStrings()->m_trTagName;
}

String* HTMLTableRowElement::ch()
{
    Nullable<String*> ret = getAttribute(starFish()->staticStrings()->m_char);
    if (ret.hasValue()) {
        return ret.getValue();
    }
    // TODO : Set defualt Value that is the decimal point character for the
    // current language as set by the lang attribute
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return String::createASCIIString(".");
}

void HTMLTableRowElement::setCh(String* ch)
{
    setAttribute(starFish()->staticStrings()->m_char, ch);
}

inline HTMLTableElement* findTable(const HTMLTableRowElement& row)
{
    auto* parent = row.parentNode();
    if (parent->isHTMLTableElement()) {
        return parent->asHTMLTableElement();
    }
    if (parent->isHTMLTableSectionElement()) {
        auto* grandparent = parent->parentNode();
        if (grandparent->isHTMLTableElement()) {
            return grandparent->asHTMLTableElement();
        }
    }
    return nullptr;
}

int32_t HTMLTableRowElement::rowIndex()
{
    HTMLTableElement* table = findTable(*this);
    if (!table) {
        return -1;
    }

    HTMLCollection* rows = table->rows();
    size_t length = rows->length();
    for (size_t i = 0; i < length; i++) {
        if (rows->item(i) == this) {
            return i;
        }
    }
    return -1;
}
}
