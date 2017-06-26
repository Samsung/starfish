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

#include "core/style/CSSRule.h"

#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSRule.h"
#include "core/style/CSSRuleList.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleRule.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleSheet.h"
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
        m_propertiesWrapper = new StyleRuleCSSStyleDeclaration(
            m_styleRule->styleDeclaration()->cssValues(),
            this->asCSSStyleRule());
    }

    return m_propertiesWrapper;
}

String* CSSStyleRule::cssText()
{
    StringBuilder result;
    result.appendString(selectorText());
    result.appendString(" { ");

    String* decls = m_styleRule->styleDeclaration()->generateCSSText();
    result.appendString(decls);
    if (!decls->isEmpty()) {
        result.appendChar(' ');
    }
    result.appendChar('}');
    return result.finalize();
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
    CSSSelectorList selectorList = m_styleRule->selectorList();
    return selectorList.selectorText();
}

CSSGroupingRule::CSSGroupingRule(StyleRuleGroup* groupRule,
                                 CSSStyleSheet* parent)
    : CSSRule(parent)
    , m_groupRule(groupRule)
    , m_ruleListWrapper(nullptr)
{
    m_childRuleWrappers.resize(groupRule->childRules().size());
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
    STARFISH_ASSERT(m_childRuleWrappers.size() ==
                    m_groupRule->childRules().size());

    if (index > m_groupRule->childRules().size()) {
        StringBuilder msg;
        msg.appendString("the index ");
        msg.appendString(String::fromInt(index));
        msg.appendString(
            " must be less than or equal to the length of the rule list.");
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INDEX_SIZE_ERR,
                               msg.finalize()->utf8Data());
        return 0;
    }

    CSSParser parser(scriptBindingInstance()->ownerDocument());
    RefPtr<CSSToken> token = parser.makeToken(rule);

    GCVector<StyleRuleBase*> styleRules;
    GCVector<CSSSelectorList*> selectorListContainer;
    parser.parseStyleRule(token, styleRules,
                          CSSParser::AllowedRulesType::RegularRules,
                          &selectorListContainer);

    if (styleRules.size() == 0) {
        StringBuilder msg;
        msg.appendString("the rule '");
        msg.appendString(rule);
        msg.appendString("' is invalid and cannot be parsed.");
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::SYNTAX_ERR,
                               msg.finalize()->utf8Data());
        return 0;
    }

    if (styleRules[0]->isImportRule()) {
        throw new DOMException(
            scriptBindingInstance()->ownerDocument(),
            DOMException::HIERARCHY_REQUEST_ERR,
            "'@import' rules cannot be inserted inside a group rule.");
        return 0;
    }

    // TODO: throw exception for @namespace at-rule.

    m_groupRule->wrapperInsertRule(index, styleRules[0]);
    m_childRuleWrappers.insert(m_childRuleWrappers.begin() + index,
                               (CSSRule*)(nullptr));
    return index;
}

void CSSGroupingRule::deleteRule(unsigned index)
{
    STARFISH_ASSERT(m_childRuleWrappers.size() ==
                    m_groupRule->childRules().size());

    if (index >= m_groupRule->childRules().size()) {
        StringBuilder msg;
        msg.appendString("the index ");
        msg.appendString(String::fromInt(index));
        msg.appendString(" is greater than the length of the rule list.");
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INDEX_SIZE_ERR,
                               msg.finalize()->utf8Data());
        return;
    }

    m_groupRule->wrapperRemoveRule(index);

    if (m_childRuleWrappers[index]) {
        m_childRuleWrappers[index]->setParentRule(nullptr);
    }
    m_childRuleWrappers.erase(m_childRuleWrappers.begin() + index);
}

unsigned CSSGroupingRule::length() const
{
    return m_groupRule->childRules().size();
}

CSSRule* CSSGroupingRule::item(unsigned index)
{
    if (index >= length()) {
        return nullptr;
    }

    STARFISH_ASSERT(m_childRuleWrappers.size() ==
                    m_groupRule->childRules().size());

    if (!m_childRuleWrappers[index]) {
        m_childRuleWrappers[index] =
            m_groupRule->childRules()[index]->createCSSOMWrapper(
                const_cast<CSSGroupingRule*>(this));
    }

    return m_childRuleWrappers[index];
}

void CSSGroupingRule::appendCSSTextForItems(StringBuilder& result)
{
    unsigned size = length();
    for (unsigned i = 0; i < size; ++i) {
        result.appendString("  ");
        result.appendString(item(i)->cssText());
        result.appendChar('\n');
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
    StringBuilder result;
    result.appendString("@media ");
    if (mediaQuerySet()) {
        result.appendString(mediaQuerySet()->mediaText());
        result.appendChar(' ');
    }
    result.appendString("{ \n");
    appendCSSTextForItems(result);
    result.appendChar('}');
    return result.finalize();
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
    StringBuilder result;
    result.appendString("@import url(\"");
    result.appendString(m_importRule->href());
    result.appendString("\")");

    if (m_importRule->mediaQuerySet()) {
        String* mediaText = m_importRule->mediaQuerySet()->mediaText();
        if (!mediaText->isEmpty()) {
            result.appendChar(' ');
            result.appendString(mediaText);
        }
    }
    result.appendChar(';');

    return result.finalize();
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
