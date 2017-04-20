/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "dom/CSSStyleDeclaration.h"
#include "dom/CSSStyleRule.h"
#include "style/Style.h"

namespace StarFish {

CSSStyleRule::CSSStyleRule(CSSSelector::Type type, String* selectorText,
                           Document* document)
    : ScriptWrappable(this)
    , m_styleDeclaration(new CSSStyleDeclaration(document))
    , m_document(document)
{
    CSSSelector* selector =
        new CSSSelector(type, CSSSelector::RelationType::None, selectorText);
    GCDeque<CSSSelector*>* selectorList = new (GC) GCDeque<CSSSelector*>();
    selectorList->push_back(selector);
    m_selectorList = selectorList;
}

CSSStyleRule::CSSStyleRule(GCDeque<CSSSelector*>* selectorList,
                           Document* document, CSSStyleDeclaration* decl)
    : ScriptWrappable(this)
    , m_selectorList(selectorList)
    , m_styleDeclaration(decl)
    , m_document(document)
{
}
}
