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
#include "core/dom/HTMLLinkElement.h"
#include "core/dom/Node.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSRuleList.h"
#include "core/style/CSSStyleRule.h"
#include "core/style/CSSStyleSheet.h"
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

ScriptBindingInstance* CSSStyleSheet::scriptBindingInstance()
{
    return origin()->scriptBindingInstance();
}

void CSSStyleSheet::addStyleRule(std::pair<StyleRule*, ResourceURL*> rule)
{
    m_styleRules.push_back(rule);
}

void CSSStyleSheet::addRule(StyleRuleBase* rule)
{
    if (rule->isImportRule()) {
        STARFISH_ASSERT(m_allRules.size() == 0);

        StyleRuleImport* importRule = rule->asStyleRuleImport();
        m_importRules.push_back(importRule);
        m_importRules.back()->setParentStyleSheet(this);
        m_importRules.back()->requestStyleSheet();
        return;
    }

    m_allRules.push_back(rule);
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
static unsigned specificity(CSSSelctorList& selectorList)
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
    return specificity(r1.first->selectorList()) <
           specificity(r2.first->selectorList());
}

CSSStyleSheet* CSSStyleSheet::parentStyleSheet()
{
    return m_ownerRule ? m_ownerRule->parentStyleSheet() : nullptr;
}

void CSSStyleSheet::sortStyleRulesBySpecificity()
{
    std::stable_sort(m_styleRules.begin(), m_styleRules.end(),
                     compareSpecificity);
}

bool CSSStyleSheet::matchesMediaQueries(const MediaQueryEvaluator& evaluator,
                                        MediaQuerySet* mediaQueries)
{
    if (!mediaQueries) {
        return true;
    }

    return evaluator.eval(mediaQueries);
}

void CSSStyleSheet::collectRulesForImportedSheet()
{
    StyleRuleImport* importRule = ownerRule();

    if (matchesMediaQueries(
            origin()->document()->styleResolver().mediaQueryEvaluator(),
            importRule->mediaQuerySet())) {
        collectStyleRules(allRules(), importRule->styleSheet()->url());
    }
}

void CSSStyleSheet::collectStyleRules(GCVector<StyleRuleBase*>& rules,
                                      ResourceURL* url)
{
    auto resolver = origin()->document()->styleResolver();
    auto iter = rules.begin();
    while (iter != rules.end()) {
        if ((*iter)->isStyleRule()) {
            m_styleRules.push_back(std::make_pair((StyleRule*)(*iter), url));
        } else if ((*iter)->isMediaRule()) {
            StyleRuleMedia* media = (StyleRuleMedia*)(*iter);
            const MediaQueryEvaluator& evaluator =
                resolver.mediaQueryEvaluator();
            if (matchesMediaQueries(evaluator, media->mediaQuerySet())) {
                collectStyleRules(media->childRules(), url);
            }
        }
        iter++;
    }
}

String* CSSStyleSheet::href() const
{
    if (m_origin->isHTMLLinkElement()) {
        STARFISH_ASSERT(m_origin->asHTMLLinkElement()->href());
        return m_origin->asHTMLLinkElement()->href();
    }
    return String::emptyString;
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

unsigned CSSStyleSheet::length() const
{
    return m_importRules.size() + m_allRules.size();
}

StyleRuleBase* CSSStyleSheet::ruleAt(unsigned index) const
{
    STARFISH_ASSERT(index < length());

    if (index < m_importRules.size()) {
        return m_importRules[index];
    }

    index -= m_importRules.size();
    return m_allRules[index];
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
