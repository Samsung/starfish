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
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/HTMLLinkElement.h"
#include "core/dom/Node.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSRuleList.h"
#include "core/style/CSSStyleRule.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/MediaList.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/style/StyleRule.h"

namespace StarFish {

class StyleSheetCSSRuleList : public CSSRuleList {
public:
    StyleSheetCSSRuleList(CSSStyleSheet* sheet)
        : m_styleSheet(sheet)
    {
    }

    ScriptBindingInstance* scriptBindingInstance()
    {
        return m_styleSheet->scriptBindingInstance();
    }

private:
    unsigned length() const override
    {
        return m_styleSheet->length();
    }

    CSSRule* item(unsigned index) const override
    {
        return m_styleSheet->item(index);
    }

    CSSStyleSheet* styleSheet() const override
    {
        return m_styleSheet;
    }

    CSSStyleSheet* m_styleSheet;
};

CSSStyleSheet::CSSStyleSheet(Node* origin, String* str)
    : StyleSheet()
    , m_sourceString(str)
    , m_origin(origin)
    , m_ownerRule(nullptr)
    , m_ruleList(nullptr)
    , m_mediaQuerySet(nullptr)
    , m_mediaWrapper(nullptr)
    , m_ruleSet(nullptr)
{
}

ScriptBindingInstance* CSSStyleSheet::scriptBindingInstance()
{
    return origin()->scriptBindingInstance();
}

void CSSStyleSheet::addStyleRule(std::pair<StyleRule*, ResourceURL*> rule)
{
    m_styleRules.push_back(rule);
}

static void extractValuesforSelector(const CSSSelector* selector, bool& id,
                                     bool& className, bool& tagName)
{
    switch (selector->type()) {
    case CSSSelector::Id:
        id = true;
        break;
    case CSSSelector::Class:
        className = true;
        break;
    case CSSSelector::Tag:
        if (selector->selectorText() != String::createASCIIString('*')) {
            tagName = true;
        }
        break;
    default:
        break;
    }
}

void CSSStyleSheet::addToRuleSet(std::pair<StyleRule*, ResourceURL*> rule)
{
    auto selectorList = rule.first->selectorList();

    bool id = false;
    bool className = false;
    bool tagName = false;

    unsigned size = selectorList.size();
    unsigned i = 0;
    for (; i < size && selectorList[i]->relation() == CSSSelector::SubSelector;
         ++i) {
        extractValuesforSelector(selectorList[i], id, className, tagName);
    }

    if (i < size) {
        extractValuesforSelector(selectorList[i], id, className, tagName);
    }

    if (id) {
        ruleSet()->idRules().push_back(rule);
        return;
    }
    if (className) {
        ruleSet()->classRules().push_back(rule);
        return;
    }
    if (tagName) {
        ruleSet()->tagRules().push_back(rule);
        return;
    }
    ruleSet()->universalRules().push_back(rule);
}

void CSSStyleSheet::addRule(StyleRuleBase* rule)
{
    if (rule->isImportRule()) {
        STARFISH_ASSERT(m_childRules.size() == 0);

        StyleRuleImport* importRule = rule->asStyleRuleImport();
        m_importRules.push_back(importRule);
        m_importRules.back()->setParentStyleSheet(this);
        m_importRules.back()->requestStyleSheet();
        return;
    }

    m_childRules.push_back(rule);
}

void CSSStyleSheet::setOwnerRule(CSSRule* ownerRule)
{
    m_ownerRule = ownerRule;
}

ResourceURL* CSSStyleSheet::url()
{
    if (m_origin->isHTMLLinkElement()) {
        STARFISH_ASSERT(m_origin->asHTMLLinkElement()->href());
        return m_origin->asHTMLLinkElement()->url();
    }
    return m_origin->document()->documentURI();
}

void CSSStyleSheet::parseSheetIfneeds()
{
    if (m_sourceString != String::emptyString) {
        CSSParser parser(m_origin->document());
        parser.parseStyleSheet(m_sourceString, this);
        m_sourceString = String::emptyString;
    }
}

// http://www.w3.org/TR/css3-selectors/#specificity
// We use 256 as the base of the specificity number system.
static unsigned calcSpecificity(CSSSelectorList& selectorList)
{
    // Make sure the result doesn't overflow
    static const unsigned idMask =
        0xff0000; // count the number of ID selectors in the selector (= a)
    static const unsigned classMask =
        0x00ff00; // count the number of class selectors, attributes selectors,
                  // and pseudo-classes in the selector (= b)
    static const unsigned elementMask =
        0x0000ff; // count the number of type selectors and pseudo-elements in
                  // the selector (= c)

    unsigned total = 0;
    unsigned temp = 0;

    for (unsigned i = 0; i < selectorList.size(); i++) {
        CSSSelector* selector = selectorList[i];
        temp = total + selector->specificityForOneSelector();

        // The negation pseudo-class has another simple selector in own data
        // structure.
        if (selector->type() == CSSSelector::Type::PseudoClass &&
            selector->asCSSPseudoSelector()->pseudoType() ==
                CSSSelector::PseudoType::PseudoNot) {
            temp += total +
                    selector->asCSSPseudoSelector()
                        ->pseudoSelectorList()[0]
                        ->specificityForOneSelector();
        }

        // Clamp each component to its max in the case of overflow.
        if ((temp & idMask) < (total & idMask)) {
            total |= idMask;
        } else if ((temp & classMask) < (total & classMask)) {
            total |= classMask;
        } else if ((temp & elementMask) < (total & elementMask)) {
            total |= elementMask;
        } else {
            total = temp;
        }
    }

    return total;
}

static bool compareSpecificity(std::pair<StyleRule*, ResourceURL*> r1,
                               std::pair<StyleRule*, ResourceURL*> r2)
{
    return calcSpecificity(r1.first->selectorList()) <
           calcSpecificity(r2.first->selectorList());
}

CSSStyleSheet* CSSStyleSheet::parentStyleSheet() const
{
    return m_ownerRule ? m_ownerRule->parentStyleSheet() : nullptr;
}

void CSSStyleSheet::sortStyleRulesBySpecificity()
{
    std::stable_sort(m_styleRules.begin(), m_styleRules.end(),
                     compareSpecificity);
}

unsigned CSSSelectorList::specificity()
{
    if (m_specificity == 0) {
        m_specificity = calcSpecificity(*this);
    }
    return m_specificity;
}

bool CSSStyleSheet::matchesMediaQueries(
    const MediaQueryEvaluator& evaluator, MediaQuerySet* mediaQueries,
    MediaQueryResultList* viewportDependentResult,
    MediaQueryResultList* deviceDependentResult)
{
    if (!mediaQueries) {
        return true;
    }

    return evaluator.eval(mediaQueries, viewportDependentResult,
                          deviceDependentResult);
}

void CSSStyleSheet::collectRulesFromImportedSheet(
    GCVector<StyleRuleImport*>& rules,
    std::vector<CSSStyleDeclaration*>& webFonts,
    MediaQueryResultList* viewportDependentResult,
    MediaQueryResultList* deviceDependentResult)
{
    for (unsigned i = 0; i < rules.size(); i++) {
        if (rules[i]->isLoading()) {
            continue;
        }
        if (matchesMediaQueries(
                origin()->document()->styleResolver().mediaQueryEvaluator(),
                rules[i]->mediaQuerySet(), viewportDependentResult,
                deviceDependentResult)) {
            if (rules[i]->styleSheet()->importRules().size() > 0) {
                collectRulesFromImportedSheet(
                    rules[i]->styleSheet()->importRules(), webFonts,
                    viewportDependentResult, deviceDependentResult);
            }
            if (rules[i]->styleSheet()->childRules().size() > 0) {
                ResourceURL* url = new ResourceURL(
                    rules[i]->href(),
                    rules[i]->parentStyleSheet()->url()->urlString());
                collectStyleRules(rules[i]->styleSheet()->childRules(),
                                  webFonts, url, viewportDependentResult,
                                  deviceDependentResult);
            }
        }
    }
}

void CSSStyleSheet::collectStyleRules(
    GCVector<StyleRuleBase*>& rules,
    std::vector<CSSStyleDeclaration*>& webFonts, ResourceURL* url,
    MediaQueryResultList* viewportDependentResult,
    MediaQueryResultList* deviceDependentResult)
{
    auto iter = rules.begin();
    while (iter != rules.end()) {
        auto rule = (*iter);
        if (rule->isStyleRule()) {
            m_styleRules.push_back(std::make_pair((StyleRule*)(*iter), url));
        } else if (rule->isMediaRule()) {
            StyleRuleMedia* media = (StyleRuleMedia*)(*iter);
            auto resolver = origin()->document()->styleResolver();
            const MediaQueryEvaluator& evaluator =
                resolver.mediaQueryEvaluator();
            if (matchesMediaQueries(evaluator, media->mediaQuerySet(),
                                    viewportDependentResult,
                                    deviceDependentResult)) {
                collectStyleRules(media->childRules(), webFonts, url,
                                  viewportDependentResult,
                                  deviceDependentResult);
            }
        } else if (rule->isFontFaceRule()) {
            webFonts.push_back(rule->asStyleRuleFontFace()->styleDeclaration());
        }
        iter++;
    }
}

String* CSSStyleSheet::href() const
{
    if (m_origin->isHTMLLinkElement()) {
        STARFISH_ASSERT(m_origin->asHTMLLinkElement()->href());
        return m_origin->asHTMLLinkElement()->href();
    } else if (m_ownerRule) {
        CSSImportRule* rule = m_ownerRule->asCSSImportRule();
        ResourceURL* url = new ResourceURL(
            rule->href(), rule->parentStyleSheet()->url()->urlString());
        return url->urlString();
    }
    return String::emptyString;
}

void CSSStyleSheet::setMediaQuerySet(MediaQuerySet* mediaQuerySet)
{
    m_mediaQuerySet = mediaQuerySet;

    if (m_mediaWrapper && m_mediaQuerySet) {
        m_mediaWrapper->setMediaQuerySet(m_mediaQuerySet);
    }
}

MediaList* CSSStyleSheet::media()
{
    if (!m_mediaQuerySet) {
        return nullptr;
    }

    if (!m_mediaWrapper) {
        m_mediaWrapper = new MediaList(m_mediaQuerySet);
    }

    return m_mediaWrapper;
}

CSSRuleList* CSSStyleSheet::cssRules()
{
    // TODO: If we add an origin policy, we need to be able to verify that the
    // rules are accessible.
    if (!m_ruleList) {
        m_ruleList = new StyleSheetCSSRuleList(this);
    }
    return m_ruleList;
}

bool CSSStyleSheet::wrapperInsertRule(StyleRuleBase* rule, unsigned index)
{
    // TODO: We need to check security issues.
    STARFISH_ASSERT(index <= length());

    if (index < m_importRules.size() ||
        (index == m_importRules.size() && rule->isImportRule())) {
        if (!rule->isImportRule()) {
            return false;
        }

        StyleRuleImport* importRule = rule->asStyleRuleImport();
        m_importRules.insert(m_importRules.begin() + index, importRule);
        m_importRules[index]->setParentStyleSheet(this);
        m_importRules[index]->requestStyleSheet();

        return true;
    }

    if (rule->isImportRule()) {
        return false;
    }

    index -= m_importRules.size();

    {
        // TODO: need to handle @namespace at-rule
    }

    m_childRules.insert(m_childRules.begin() + index, rule);
    return true;
}

unsigned CSSStyleSheet::insertRule(String* ruleString, unsigned index)
{
    STARFISH_ASSERT(m_childRuleWrappers.empty() ||
                    m_childRuleWrappers.size() == length());

    if (index > length()) {
        StringBuilder msg;
        msg.appendString("The index provided (");
        msg.appendString(String::fromInt(index));
        msg.appendString(") is larger than the maximum index (");
        msg.appendString(String::fromInt(length()));
        msg.appendString(").");
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
        msg.appendString("Failed to parse the rule '");
        msg.appendString(ruleString);
        msg.appendString("'.");
        auto s = msg.finalize()->toUTF8NonGCString();
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::SYNTAX_ERR, s.data());
    }

    bool success = wrapperInsertRule(rules[0], index);
    if (!success) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::HIERARCHY_REQUEST_ERR,
                               "Failed to insert the rule.");
    }

    m_childRuleWrappers.insert(m_childRuleWrappers.begin() + index, nullptr);

    scriptBindingInstance()
        ->ownerWindow()
        ->browsingContext()
        ->setNeedsStyleSheetsRecalc();

    return index;
}

bool CSSStyleSheet::wrapperDeleteRule(unsigned index)
{
    // TODO: We need to check security issues.
    STARFISH_ASSERT(index < length());

    if (index < m_importRules.size()) {
        m_importRules[index]->clearParentStyleSheet();
        m_importRules.erase(m_importRules.begin() + index);
        return true;
    }
    index -= m_importRules.size();

    {
        // TODO: need to handle @namespace at-rule
    }

    m_childRules.erase(m_childRules.begin() + index);
    return true;
}

void CSSStyleSheet::deleteRule(unsigned index)
{
    STARFISH_ASSERT(m_childRuleWrappers.empty() ||
                    m_childRuleWrappers.size() == length());

    if (index >= length()) {
        StringBuilder msg;
        msg.appendString("The index provided (");
        msg.appendString(String::fromInt(index));
        msg.appendString(") is larger than the maximum index (");
        msg.appendString(String::fromInt(length() - 1));
        msg.appendString(").");
        auto s = msg.finalize()->toUTF8NonGCString();
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INDEX_SIZE_ERR, s.data());
    }

    bool success = wrapperDeleteRule(index);
    if (!success) {
        throw new DOMException(scriptBindingInstance()->ownerDocument(),
                               DOMException::INVALID_STATE_ERR,
                               "Failed to delete rule");
    }

    if (!m_childRuleWrappers.empty()) {
        if (m_childRuleWrappers[index]) {
            m_childRuleWrappers[index]->setParentStyleSheet(nullptr);
        }
        m_childRuleWrappers.erase(m_childRuleWrappers.begin() + index);
    }

    scriptBindingInstance()
        ->ownerWindow()
        ->browsingContext()
        ->setNeedsStyleSheetsRecalc();
}

unsigned CSSStyleSheet::length() const
{
    return m_importRules.size() + m_childRules.size();
}

StyleRuleBase* CSSStyleSheet::ruleAt(unsigned index) const
{
    STARFISH_ASSERT(index < length());

    if (index < m_importRules.size()) {
        return m_importRules[index];
    }

    index -= m_importRules.size();
    return m_childRules[index];
}

CSSRule* CSSStyleSheet::item(unsigned index)
{
    unsigned ruleCount = length();
    if (index >= ruleCount) {
        return nullptr;
    }

    if (m_childRuleWrappers.size() == 0) {
        m_childRuleWrappers.resize(ruleCount);
    }

    if (!m_childRuleWrappers[index]) {
        m_childRuleWrappers[index] =
            ruleAt(index)->createCSSOMWrapper(const_cast<CSSStyleSheet*>(this));
    }

    return m_childRuleWrappers[index];
}

} /* namespace StarFish */
