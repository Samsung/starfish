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

#include "core/dom/CSSRule.h"
#include "core/dom/CSSStyleDeclaration.h"
#include "core/dom/CSSStyleRule.h"
#include "core/style/Style.h"
#include "core/style/MediaQuerySet.h"

namespace StarFish {

CSSStyleRule::CSSStyleRule(CSSSelector::Type type, String* selectorText)
    : CSSRule(CSSRule::STYLE_RULE)
    , m_styleDeclaration(new CSSStyleDeclaration())
{
    CSSSelector* selector =
        new CSSSelector(type, CSSSelector::RelationType::None, selectorText);
    GCDeque<CSSSelector*>* selectorList = new (GC) GCDeque<CSSSelector*>();
    selectorList->push_back(selector);
    m_selectorList = selectorList;
}

CSSStyleRule::CSSStyleRule(GCDeque<CSSSelector*>* selectorList,
                           CSSStyleDeclaration* decl)
    : CSSRule(CSSRule::STYLE_RULE)
    , m_selectorList(selectorList)
    , m_styleDeclaration(decl)
{
}

CSSStyleRuleGroup::CSSStyleRuleGroup(RuleType type, GCVector<CSSRule*>& rules)
    : CSSRule(type)
{
    m_childRules.clear();
    m_childRules.assign(rules.begin(), rules.end());
}
CSSStyleRuleGroup::CSSStyleRuleGroup(CSSStyleRuleGroup& o)
    : CSSRule(o.type())
{
    m_childRules.clear();
    m_childRules.assign(o.childRules().begin(), o.childRules().end());
}

CSSStyleRuleMedia::CSSStyleRuleMedia(MediaQuerySet* media,
                                     GCVector<CSSRule*>& rules)
    : CSSStyleRuleGroup(CSSRule::MEDIA_RULE, rules)
    , m_mediaQueries(media)
{
}

CSSStyleRuleMedia::CSSStyleRuleMedia(CSSStyleRuleMedia& o)
    : CSSStyleRuleGroup(o)
{
    if (o.mediaQueries()) {
        m_mediaQueries = MediaQuerySet::create();
        m_mediaQueries->queryVector().clear();
        m_mediaQueries->queryVector().assign(
            o.mediaQueries()->queryVector().begin(),
            o.mediaQueries()->queryVector().end());
    }
}
}
