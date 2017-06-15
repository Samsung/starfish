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
#include "core/style/CSSStyleRule.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/CSSRule.h"
#include "core/style/CSSRuleList.h"
#include "core/style/StyleRule.h"
#include "core/style/MediaQuerySet.h"

namespace StarFish {

CSSRule::CSSRule(CSSStyleSheet* parent)
    : ScriptWrappable(this)
    , m_parentIsRule(false)
    , m_parentStyleSheet(parent)
{
}

ScriptBindingInstance* CSSRule::scriptBindingInstance()
{
    return parentStyleSheet()->scriptBindingInstance();
}

CSSStyleRule::CSSStyleRule(StyleRule* styleRule, CSSStyleSheet* parent)
    : CSSRule(parent)
    , m_styleRule(styleRule)
    , m_propertiesWrapper(nullptr)
{
}

CSSStyleDeclaration* CSSStyleRule::style()
{
    if (!m_propertiesWrapper) {
        m_propertiesWrapper = new CSSStyleDeclaration();
    }

    return m_propertiesWrapper;
}

String* CSSStyleRule::cssText()
{
    String* result = selectorText();
    result->concat(String::createASCIIString(" { "));

    String* decls = m_styleRule->styleDeclaration()->generateCSSText();
    result->concat(decls);
    if (!decls->equals(String::emptyString)) {
        result->concat(String::createASCIIString(" "));
    }
    result->concat(String::createASCIIString("}"));
    return result;
}

String* CSSStyleRule::selectorText() const
{
    // TODO : Do we need to cache this information?
    return generateSelectorText();
}

void CSSStyleRule::setSelectorText(String* selectorText)
{
    // TODO : implement
}

String* CSSStyleRule::generateSelectorText() const
{
    String* result = String::emptyString;
    GCDeque<CSSSelector*> selectors = m_styleRule->selectorList();
    for (size_t i = 0; i < selectors.size(); ++i) {
        if (i != 0) {
            result->concat(String::createASCIIString(", "));
        }
        result->concat(selectors[i]->selectorText().string());
    }

    return result;
}

CSSGroupingRule::CSSGroupingRule(StyleRuleGroup* groupRule,
                                 CSSStyleSheet* parent)
    : CSSRule(parent)
    , m_groupRule(groupRule)
    , m_ruleListWrapper(nullptr)
{
    m_childRuleWrappers.reserve(groupRule->childRules().size());
}

CSSRuleList* CSSGroupingRule::cssRules()
{
    if (!m_ruleListWrapper) {
        m_ruleListWrapper = new LiveCSSRuleList<CSSGroupingRule>(
            const_cast<CSSGroupingRule*>(this));
    }
    return m_ruleListWrapper;
}

unsigned CSSGroupingRule::insertRule(String* rule, unsigned index)
{
    // TODO : implement
    return 0; // or throw Exception
}

void CSSGroupingRule::deleteRule(unsigned index)
{
    // TODO : implement
    // or throw Exception
}

unsigned CSSGroupingRule::length() const
{
    return m_childRuleWrappers.size();
}

CSSRule* CSSGroupingRule::item(unsigned index) const
{
    if (index >= length()) {
        return nullptr;
    }

    STARFISH_ASSERT(m_childRuleWrappers.size() ==
                    m_groupRule->childRules().size());
    CSSRule* rule = m_childRuleWrappers[index];
    if (!rule) {
        rule = m_groupRule->childRules()[index]->createCSSOMWrapper(
            const_cast<CSSGroupingRule*>(this));
    }

    return rule;
}

void CSSGroupingRule::appendCSSTextForItems(String* result)
{
    unsigned size = length();
    for (unsigned i = 0; i < size; ++i) {
        result->concat(String::createASCIIString("  "));
        result->concat(item(i)->cssText());
        result->concat(String::createASCIIString("\n"));
    }
}

CSSConditionRule::CSSConditionRule(StyleRuleCondition* condition_rule,
                                   CSSStyleSheet* parent)
    : CSSGroupingRule(condition_rule, parent)
{
}

String* CSSConditionRule::conditionText() const
{
    return static_cast<StyleRuleCondition*>(m_groupRule)->conditionText();
}

CSSMediaRule::CSSMediaRule(StyleRuleMedia* mediaRule, CSSStyleSheet* parent)
    : CSSConditionRule(mediaRule, parent)
{
}

String* CSSMediaRule::cssText()
{
    String* result = String::emptyString;
    result->concat(String::createASCIIString("@media "));
    if (mediaQuerySet()) {
        result->concat(mediaQuerySet()->mediaText());
        result->concat(String::createASCIIString(" "));
    }
    result->concat(String::createASCIIString("{ \n"));
    appendCSSTextForItems(result);
    result->concat(String::createASCIIString("}"));
    return result;
}

String* CSSMediaRule::conditionText() const
{
    // TODO : implement
    return String::emptyString;
}

// MediaList* CSSMediaRule::media()
// {
//    // TODO : implement
//    return nullptr;
// }

MediaQuerySet* CSSMediaRule::mediaQuerySet() const
{
    return static_cast<StyleRuleMedia*>(m_groupRule)->mediaQuerySet();
}

CSSImportRule::CSSImportRule(StyleRuleImport* importRule, CSSStyleSheet* parent)
    : CSSRule(parent)
    , m_importRule(importRule)
    , m_styleSheetWrapper(nullptr)
{
}

String* CSSImportRule::cssText()
{
    String* result = String::emptyString;
    result->concat(String::createASCIIString("@import url(\""));
    result->concat(m_importRule->href());
    result->concat(String::createASCIIString("\")"));

    if (m_importRule->mediaQuerySet()) {
        String* mediaText = m_importRule->mediaQuerySet()->mediaText();
        if (!mediaText->equals(String::emptyString)) {
            result->concat(String::createASCIIString(" "));
            result->concat(mediaText);
        }
    }
    result->concat(String::createASCIIString(";"));

    return result;
}

String* CSSImportRule::href() const
{
    return m_importRule->href();
}

// MediaList* CSSImportRule::media() const
// {
//    // TODO : implement
//    return nullptr;
// }

CSSStyleSheet* CSSImportRule::styleSheet() const
{
    // TODO : implement
    return nullptr;
}
}
