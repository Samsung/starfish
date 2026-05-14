/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "binding/ScriptBindingInstance.h"
#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/HTMLLinkElement.h"
#include "core/dom/Node.h"
#include "core/dom/NodeList.h"
#include "core/dom/SelectorQuery.h"
#include "core/dom/Traverse.h"
#include "core/layout/Frame.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/style/AncestorSelectorFilter.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSRuleList.h"
#include "core/style/CSSStyleRule.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/MediaList.h"
#include "core/style/MediaQueryEvaluator.h"
#include "core/style/StyleRule.h"
#include "binding/generated/ElementOrProcessingInstructionUnion.h"
#include "binding/generated/MediaListOrDOMStringUnion.h"

namespace Starfish {

class StyleSheetCSSRuleList : public CSSRuleList {
public:
    StyleSheetCSSRuleList(CSSStyleSheet* sheet)
        : m_styleSheet(sheet)
    {
    }

    ScriptBindingInstance* scriptBindingInstance() override
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

static Node* findRootOfStyleSheet(Node* origin)
{
    if (origin->isInShadowRoot()) {
        return origin->parentShadowRoot();
    } else {
        return origin->document();
    }
}

// Constructor for constructable stylesheets
CSSStyleSheet::CSSStyleSheet(ExecutionContext* executionContext,
                             const CSSStyleSheetInit& options)
    : StyleSheet(executionContext)
    , m_sourceString(String::emptyString)
    , m_origin(nullptr)
    , m_root(nullptr)
    , m_ownerRule(nullptr)
    , m_ruleList(nullptr)
    , m_mediaQuerySet(nullptr)
    , m_mediaWrapper(nullptr)
    , m_disabled(options.disabled())
{
    // Parse media option if provided
    const MediaListOrDOMString& media = options.media();
    if (media.isDOMStringValue()) {
        String* mediaString = media.getDOMStringValue();
        if (mediaString && !mediaString->isEmpty()) {
            // Parse media string using CSSParser
            CSSParser parser(executionContext);
            parser.makeToken(mediaString);
            m_mediaQuerySet = parser.parseMediaQuery();
        }
    } else if (media.isMediaListValue()) {
        // If MediaList is provided, use its MediaQuerySet
        m_mediaQuerySet = MediaQuerySet::create((Node*)nullptr);
        MediaList* mediaList = media.getMediaListValue();
        if (mediaList && mediaList->mediaQuerySet()) {
            // Copy the media queries from the provided MediaList
            for (size_t i = 0;
                 i < mediaList->mediaQuerySet()->queryVector().size(); i++) {
                m_mediaQuerySet->addMediaQuery(
                    mediaList->mediaQuerySet()->queryVector()[i]);
            }
        }
    }
}

// Constructor for stylesheets from style/link elements
CSSStyleSheet::CSSStyleSheet(Node* origin, String* str)
    : StyleSheet(origin->executionContext())
    , m_sourceString(str)
    , m_origin(origin)
    , m_root(findRootOfStyleSheet(origin))
    , m_ownerRule(nullptr)
    , m_ruleList(nullptr)
    , m_mediaQuerySet(nullptr)
    , m_mediaWrapper(nullptr)
    , m_disabled(false)
{
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
    return m_origin->document()->baseURL();
}

void CSSStyleSheet::parseSheetIfneeds()
{
    INSTALL_RECORDABLE_PROFILE_TIMER(ProfileKind::kStyle, "Parse Style Sheet");
    if (m_sourceString != String::emptyString) {
        CSSParser parser(m_origin);
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
        CSSSelector* selector = selectorList[i].m_selector;

        // The negation pseudo-class has another simple selector in own data
        // structure.
        if (selector->type() == CSSSelector::Type::PseudoClass &&
            selector->asCSSPseudoSelector()->pseudoType() ==
                CSSSelector::PseudoType::PseudoNot) {
            temp = total + selector->asCSSPseudoSelector()
                               ->pseudoSelectorList()[0]
                               .m_selector->specificityForOneSelector();
        } else {
            temp = total + selector->specificityForOneSelector();
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

static bool compareSpecificity(const std::pair<StyleRule*, ResourceURL*>& r1,
                               const std::pair<StyleRule*, ResourceURL*>& r2)
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
    GCVector<std::pair<CSSStyleDeclaration*, ResourceURL*>>& webFonts,
    MediaQueryResultList* viewportDependentResult,
    MediaQueryResultList* deviceDependentResult)
{
    if (disabled()) {
        return;
    }
    for (unsigned i = 0; i < rules.size(); i++) {
        if (rules[i]->isLoading()) {
            continue;
        }
        if (matchesMediaQueries(origin()->styleResolver().mediaQueryEvaluator(),
                                rules[i]->mediaQuerySet(),
                                viewportDependentResult,
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
    GCVector<std::pair<CSSStyleDeclaration*, ResourceURL*>>& webFonts,
    ResourceURL* url, MediaQueryResultList* viewportDependentResult,
    MediaQueryResultList* deviceDependentResult)
{
    if (disabled()) {
        return;
    }
    auto iter = rules.begin();
    while (iter != rules.end()) {
        auto rule = (*iter);
        if (rule->isStyleRule()) {
            m_styleRules.push_back(std::make_pair((StyleRule*)(*iter), url));
        } else if (rule->isMediaRule()) {
            StyleRuleMedia* media = (StyleRuleMedia*)(*iter);
            auto resolver = origin()->styleResolver();
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
            webFonts.push_back(std::make_pair(
                rule->asStyleRuleFontFace()->styleDeclaration(), url));
        } else if (rule->isSupportsRule()) {
            StyleRuleSupports* supports = rule->asStyleRuleSupports();
            if (supports->isSupported()) {
                collectStyleRules(supports->childRules(), webFonts, url,
                                  viewportDependentResult,
                                  deviceDependentResult);
            }
        } else if (rule->isKeyframesRule() == true) {
            m_keyframes.push_back(rule->asStyleRuleKeyframes());
        }
        iter++;
    }
}

static void invalidateStyleOfMatchedElementWorker(
    Node* parentElement, StyleResolver& styleResolver,
    AncestorSelectorFilter& filter,
    GCVector<std::pair<StyleRule*, ResourceURL*>>& styleRules)
{
    filter.pushNode(parentElement);

    RenderingSiblingIterator iter(parentElement->firstRenderingChild());
    while (true) {
        Optional<Node*> child = iter.next();
        if (!child) {
            break;
        }
        if (child->isElement() && !child->needsStyleRecalc()) {
            StyleResolver& resolver = child->styleResolver();
            StyleResolver::MatchResult result(nullptr);
            AtomicString elementName =
                child->asElement()->name().localNameAtomic();
            AtomicString elementId = child->asElement()->atomicId();
            const GCAtomicTightVector<AtomicString>& elementClasses =
                child->asElement()->classNames();
            bool canUseAncestorSelectorFilter =
                filter.canUseAncestorSelectorFilter(child->asElement());

            for (size_t i = 0; i < styleRules.size(); i++) {
                if (canUseAncestorSelectorFilter &&
                    filter.canIgnoreSelector(styleRules[i].first,
                                             child->asElement())) {
                    continue;
                }
                bool matches =
                    resolver.matchSelector(
                        child->asElement(), elementName, elementId,
                        elementClasses, styleRules[i].first->selectorList(), 0,
                        result, false) == StyleResolver::Match::SelectorMatches;

                if (matches) {
                    child->asElement()->setNeedsStyleRecalc();
                    break;
                }
            }
        }
    }

    iter = RenderingSiblingIterator(parentElement->firstRenderingChild());
    while (true) {
        Optional<Node*> child = iter.next();
        if (!child) {
            break;
        }
        if (child->isShadowRoot()) {
            continue;
        }
        if (child->isElement()) {
            invalidateStyleOfMatchedElementWorker(child.value(), styleResolver,
                                                  filter, styleRules);
        }
    }

    filter.popNode();
}

void CSSStyleSheet::willRemovedFromDocument()
{
    if (m_origin->document()
            ->browsingContext()
            ->needsStyleRecalcForWholeDocument()) {
        return;
    }
    LongTaskFinder t("CSSStyleSheet::willRemovedFromDocument", 1);
    AncestorSelectorFilter filter;
    invalidateStyleOfMatchedElementWorker(m_root, m_root->styleResolver(),
                                          filter, m_styleRules);
}

void CSSStyleSheet::willAddToDocument()
{
    if (m_origin->document()
            ->browsingContext()
            ->needsStyleRecalcForWholeDocument()) {
        return;
    }
    LongTaskFinder t("CSSStyleSheet::willAddToDocument", 1);

    auto viewportDependentResult =
        &m_origin->styleResolver().viewportDependentMediaQueryResults();
    auto deviceDependentResult =
        &m_origin->styleResolver().deviceDependentMediaQueryResults();

    GCVector<std::pair<CSSStyleDeclaration*, ResourceURL*>> webFonts;
    clearStyleRules();
    collectRulesFromImportedSheet(importRules(), webFonts,
                                  viewportDependentResult,
                                  deviceDependentResult);
    collectStyleRules(childRules(), webFonts, url(), viewportDependentResult,
                      deviceDependentResult);

    AncestorSelectorFilter filter;
    invalidateStyleOfMatchedElementWorker(m_root, m_root->styleResolver(),
                                          filter, m_styleRules);
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

String* CSSStyleSheet::title() const
{
    // For constructable stylesheets, m_origin is nullptr
    if (!m_origin) {
        return String::emptyString;
    }
    if (m_origin->isElement()) {
        auto title = m_origin->asElement()->getAttribute(
            m_origin->starfish()->staticStrings()->m_title);
        if (title.hasValue()) {
            return title.getValue();
        }
    }
    return String::emptyString;
}

Optional<ElementOrProcessingInstruction> CSSStyleSheet::ownerNode() const
{
    if (!m_origin) {
        return Optional<ElementOrProcessingInstruction>();
    }
    if (m_origin->isElement()) {
        return ElementOrProcessingInstruction::createElement(
            m_origin->asElement());
    }
    if (m_origin->isProcessingInstruction()) {
        return ElementOrProcessingInstruction::createProcessingInstruction(
            m_origin->asProcessingInstruction());
    }
    return Optional<ElementOrProcessingInstruction>();
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
    // For constructable stylesheets, create an empty MediaList if needed
    if (!m_mediaQuerySet) {
        m_mediaQuerySet = MediaQuerySet::create(m_origin);
    }

    if (!m_mediaWrapper) {
        m_mediaWrapper = new MediaList(m_executionContext, m_mediaQuerySet);
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
    if (index > length()) {
        StringBuilder msg;
        msg.appendString("The index provided (");
        msg.appendString(String::fromInt(index));
        msg.appendString(") is larger than the maximum index (");
        msg.appendString(String::fromInt(length()));
        msg.appendString(").");
        auto s = msg.finalize()->toUTF8NonGCString();
        throw new DOMException(m_executionContext, DOMException::INDEX_SIZE_ERR,
                               s.data());
    }

    GCVector<StyleRuleBase*> rules;
    if (m_origin) {
        CSSParser parser(m_origin);
        RefPtr<CSSToken> token = parser.makeToken(ruleString);
        parser.parseRules(token, rules,
                          CSSParser::RuleListType::TopLevelRuleList, true);
    } else {
        CSSParser parser(m_executionContext);
        RefPtr<CSSToken> token = parser.makeToken(ruleString);
        parser.parseRules(token, rules,
                          CSSParser::RuleListType::TopLevelRuleList, true);
    }

    if (rules.size() != 1) {
        StringBuilder msg;
        msg.appendString("Failed to parse the rule '");
        msg.appendString(ruleString);
        msg.appendString("'.");
        auto s = msg.finalize()->toUTF8NonGCString();
        throw new DOMException(m_executionContext, DOMException::SYNTAX_ERR,
                               s.data());
    }

    bool success = wrapperInsertRule(rules[0], index);
    if (!success) {
        throw new DOMException(m_executionContext,
                               DOMException::HIERARCHY_REQUEST_ERR,
                               "Failed to insert the rule.");
    }

    syncChildRuleWrappers();
    notifyStyleSheetChanged();

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
    if (index >= length()) {
        StringBuilder msg;
        msg.appendString("The index provided (");
        msg.appendString(String::fromInt(index));
        msg.appendString(") is larger than the maximum index (");
        msg.appendString(String::fromInt(length() - 1));
        msg.appendString(").");
        auto s = msg.finalize()->toUTF8NonGCString();
        throw new DOMException(m_executionContext, DOMException::INDEX_SIZE_ERR,
                               s.data());
    }

    bool success = wrapperDeleteRule(index);
    if (!success) {
        throw new DOMException(m_executionContext,
                               DOMException::INVALID_STATE_ERR,
                               "Failed to delete rule");
    }

    syncChildRuleWrappers();
    if (!m_childRuleWrappers.empty()) {
        if (m_childRuleWrappers[index]) {
            m_childRuleWrappers[index]->setParentStyleSheet(nullptr);
        }
        m_childRuleWrappers.erase(m_childRuleWrappers.begin() + index);
    }

    notifyStyleSheetChanged();
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

void CSSStyleSheet::syncChildRuleWrappers()
{
    size_t ruleCount = length();
    if (m_childRuleWrappers.size() != ruleCount) {
        m_childRuleWrappers.resize(ruleCount);
        for (size_t i = 0; i < ruleCount; i++) {
            m_childRuleWrappers[i] =
                ruleAt(i)->createCSSOMWrapper(const_cast<CSSStyleSheet*>(this));
        }
    }
}

void CSSStyleSheet::notifyStyleSheetChanged()
{
    // For constructable stylesheets (m_origin is nullptr), skip style recalc
    // as they are not associated with a document until adopted
    if (m_origin) {
        origin()->styleResolver().setNeedsRecalcRuleSet();
        scriptBindingInstance()
            ->ownerWindow()
            ->browsingContext()
            ->setNeedsStyleSheetsRecalcAndWholeDocumentNeedsStyleRecalc();
    }
}

CSSRule* CSSStyleSheet::item(unsigned index)
{
    unsigned ruleCount = length();
    if (index >= ruleCount) {
        return nullptr;
    }

    syncChildRuleWrappers();
    return m_childRuleWrappers[index];
}

bool CSSStyleSheet::disabled()
{
    return m_disabled;
}

void CSSStyleSheet::setDisabled(bool disabled)
{
    if (m_disabled == disabled) {
        return;
    }
    m_disabled = disabled;

    notifyStyleSheetChanged();
}

} /* namespace Starfish */
