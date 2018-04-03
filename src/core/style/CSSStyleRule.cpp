/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#include "core/style/MediaList.h"

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
            m_styleRule->styleDeclaration(), this->asCSSStyleRule());
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
    result.appendChar('}');
    return result.finalize();
}

String* CSSStyleRule::selectorText() const
{
    return generateSelectorText();
}

void CSSStyleRule::setSelectorText(String* selectorText)
{
    CSSParser parser(scriptBindingInstance()->ownerDocument());
    parser.makeToken(selectorText);

    bool isValid = true;
    GCVector<CSSSelectorList*> list;
    parser.parseSelector(list, isValid);

    if (!isValid || list.size() == 0) {
        return;
    }

    // TODO: Now, our engine has a list of CSSSelectorLists that are separated
    // by ','.
    // However, it should have only one CSSSelectorList like 'div, p, span'.
    m_styleRule->wrapperTakeSelectorList(*list[0]);

    scriptBindingInstance()
        ->ownerWindow()
        ->browsingContext()
        ->setNeedsStyleSheetsRecalc();
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

unsigned CSSGroupingRule::insertRule(String* ruleString, unsigned index)
{
    STARFISH_ASSERT(m_childRuleWrappers.size() ==
                    m_groupRule->childRules().size());

    if (index > m_groupRule->childRules().size()) {
        StringBuilder msg;
        msg.appendString("the index ");
        msg.appendString(String::fromInt(index));
        msg.appendString(
            " must be less than or equal to the length of the rule list.");
        auto s = msg.finalize()->toUTF8NonGCString();
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INDEX_SIZE_ERR, s.data());
    }

    CSSParser parser(scriptBindingInstance()->ownerDocument());
    RefPtr<CSSToken> token = parser.makeToken(ruleString);

    GCVector<StyleRuleBase*> rules;
    parser.parseRules(token, rules, CSSParser::RuleListType::TopLevelRuleList);

    if (rules.size() != 1) {
        StringBuilder msg;
        msg.appendString("the rule '");
        msg.appendString(ruleString);
        msg.appendString("' is invalid and cannot be parsed.");
        auto s = msg.finalize()->toUTF8NonGCString();
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::SYNTAX_ERR, s.data());
    }

    if (rules[0]->isImportRule()) {
        throw new DOMException(
            scriptBindingInstance()->ownerDocument(),
            DOMException::HIERARCHY_REQUEST_ERR,
            "'@import' rules cannot be inserted inside a group rule.");
    }

    {
        // TODO: throw exception for @namespace at-rule.
    }

    m_groupRule->wrapperInsertRule(index, rules[0]);
    m_childRuleWrappers.insert(m_childRuleWrappers.begin() + index,
                               (CSSRule*)(nullptr));

    scriptBindingInstance()
        ->ownerWindow()
        ->browsingContext()
        ->setNeedsStyleSheetsRecalc();
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
        auto s = msg.finalize()->toUTF8NonGCString();
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INDEX_SIZE_ERR, s.data());
    }

    m_groupRule->wrapperRemoveRule(index);

    if (m_childRuleWrappers[index]) {
        m_childRuleWrappers[index]->setParentRule(nullptr);
    }
    m_childRuleWrappers.erase(m_childRuleWrappers.begin() + index);

    scriptBindingInstance()
        ->ownerWindow()
        ->browsingContext()
        ->setNeedsStyleSheetsRecalc();
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
    , m_mediaWrapper(nullptr)
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
    if (!mediaQuerySet()) {
        return String::emptyString;
    }

    return mediaQuerySet()->mediaText();
}

MediaList* CSSMediaRule::media()
{
    if (!mediaQuerySet()) {
        return nullptr;
    }
    if (!m_mediaWrapper) {
        m_mediaWrapper = new MediaList(mediaQuerySet());
    }
    return m_mediaWrapper;
}

MediaQuerySet* CSSMediaRule::mediaQuerySet() const
{
    return static_cast<StyleRuleMedia*>(m_groupRule)->mediaQuerySet();
}

CSSImportRule::CSSImportRule(StyleRuleImport* importRule, CSSStyleSheet* parent)
    : CSSRule(parent)
    , m_importRule(importRule)
    , m_styleSheetWrapper(nullptr)
    , m_mediaWrapper(nullptr)
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

MediaList* CSSImportRule::media()
{
    if (!m_mediaWrapper) {
        m_mediaWrapper = new MediaList(m_importRule->mediaQuerySet());
    }
    return m_mediaWrapper;
}

CSSStyleSheet* CSSImportRule::styleSheet()
{
    if (!m_importRule->styleSheet()) {
        return nullptr;
    }

    if (!m_styleSheetWrapper) {
        m_styleSheetWrapper = m_importRule->styleSheet();
        m_styleSheetWrapper->setOwnerRule(this);
    }

    return m_styleSheetWrapper;
}

CSSFontFaceRule::CSSFontFaceRule(StyleRuleFontFace* styleRule,
                                 CSSStyleSheet* parent)
    : CSSRule(parent)
    , m_styleRule(styleRule)
    , m_propertiesWrapper(nullptr)
{
}

CSSStyleDeclaration* CSSFontFaceRule::style()
{
    if (!m_propertiesWrapper) {
        m_propertiesWrapper = new StyleRuleCSSStyleDeclaration(
            m_styleRule->styleDeclaration(), this->asCSSStyleRule());
    }

    return m_propertiesWrapper;
}

String* CSSFontFaceRule::cssText()
{
    StringBuilder result;
    result.appendString("@font-face { ");

    String* decls = m_styleRule->styleDeclaration()->generateCSSText();
    result.appendString(decls);
    result.appendChar('}');
    return result.finalize();
}

CSSSupportsRule::CSSSupportsRule(StyleRuleSupports* supportsRule,
                                 CSSStyleSheet* parent)
    : CSSConditionRule(supportsRule, parent)
{
}

String* CSSSupportsRule::cssText()
{
    StringBuilder result;
    result.appendString("@supports ");
    result.appendString(conditionText());
    result.appendString(" {\n");
    appendCSSTextForItems(result);
    result.appendChar('}');
    return result.finalize();
}

CSSCounterStyleRule::CSSCounterStyleRule(StyleRuleCounterStyle* styleRule,
                                         CSSStyleSheet* parent)
    : CSSRule(parent)
{
}

String* CSSCounterStyleRule::cssText()
{
    // TODO: print @counter-style after parsing the rule
    StringBuilder ret;
    ret.appendString("@counter-style { ");
    ret.appendChar('}');
    return ret.finalize();
}
}
