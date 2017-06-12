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
#include "core/modules/window/Window.h"
#include "core/style/CSSRule.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleRule.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/Style.h"
#include "core/style/MediaQuerySet.h"
#include "core/style/CSSRuleList.h"

namespace StarFish {

CSSStyleRule::CSSStyleRule(CSSSelector::Type type, AtomicString selectorText)
    : CSSRule(CSSRule::STYLE_RULE)
    , m_styleDeclaration(new CSSStyleDeclaration(nullptr))
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

String* CSSStyleRule::selectorText() const
{
    String* result = String::emptyString;
    GCDeque<CSSSelector*>* selectors = this->selectorList();
    for (size_t i = 0; i < selectors->size(); ++i) {
        if (i != 0) {
            result->concat(String::createASCIIString(", "));
        }
        result->concat((*selectors)[i]->selectorText().string());
    }

    return result;
}

String* CSSStyleRule::cssText() const
{
    String* result = selectorText();
    result->concat(String::createASCIIString(" { "));
    String* decls = m_styleDeclaration->generateCSSText();
    result->concat(decls);
    if (!decls->equals(String::emptyString)) {
        result->concat(String::createASCIIString(" "));
    }
    result->concat(String::createASCIIString("}"));
    return result;
}

CSSStyleDeclaration* CSSStyleRule::style() const
{
    return m_styleDeclaration;
}

CSSGroupingRule::CSSGroupingRule(RuleType type, GCVector<CSSRule*>& rules)
    : CSSRule(type)
    , m_ruleList(nullptr)
{
    m_childRules.clear();
    m_childRules.assign(rules.begin(), rules.end());
}
CSSGroupingRule::CSSGroupingRule(CSSGroupingRule& o)
    : CSSRule(o.type())
    , m_ruleList(o.m_ruleList)
{
    m_childRules.clear();
    m_childRules.assign(o.childRules().begin(), o.childRules().end());
}

CSSRuleList* CSSGroupingRule::cssRules()
{
    if (!m_ruleList) {
        m_ruleList = new LiveCSSRuleList<CSSGroupingRule>(
            const_cast<CSSGroupingRule*>(this));
    }
    return m_ruleList;
}

unsigned CSSGroupingRule::length() const
{
    return m_childRules.size();
}

CSSRule* CSSGroupingRule::item(unsigned index) const
{
    if (index >= length()) {
        return nullptr;
    }
    CSSRule* rule = m_childRules[index];
    if (!rule->parentRule()) {
        rule->setParentRule(const_cast<CSSGroupingRule*>(this));
    }
    return rule;
}

void CSSGroupingRule::appendCSSTextForItems(String* result) const
{
    unsigned size = length();
    for (unsigned i = 0; i < size; ++i) {
        result->concat(String::createASCIIString("  "));
        result->concat(m_childRules[i]->cssText());
        result->concat(String::createASCIIString("\n"));
    }
}

CSSConditionRule::CSSConditionRule(RuleType type, String* conditionText,
                                   GCVector<CSSRule*>& rules)
    : CSSGroupingRule(type, rules)
    , m_conditionText(conditionText)
{
}

CSSConditionRule::CSSConditionRule(RuleType type, GCVector<CSSRule*>& rules)
    : CSSGroupingRule(type, rules)
{
    m_conditionText = String::emptyString;
}

CSSConditionRule::CSSConditionRule(CSSConditionRule& conditionRule)
    : CSSGroupingRule(conditionRule)
{
    if (conditionRule.m_conditionText) {
        m_conditionText = conditionRule.m_conditionText;
    } else {
        m_conditionText = String::emptyString;
    }
}

CSSMediaRule::CSSMediaRule(MediaQuerySet* media, GCVector<CSSRule*>& rules)
    : CSSConditionRule(CSSRule::MEDIA_RULE, rules)
    , m_mediaQuerySet(media)
{
}

CSSMediaRule::CSSMediaRule(CSSMediaRule& o)
    : CSSConditionRule(o)
{
    if (o.mediaQuerySet()) {
        m_mediaQuerySet = MediaQuerySet::create();
        m_mediaQuerySet->queryVector().clear();
        m_mediaQuerySet->queryVector().assign(
            o.mediaQuerySet()->queryVector().begin(),
            o.mediaQuerySet()->queryVector().end());
    }
}

String* CSSMediaRule::cssText() const
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

CSSImportRule::CSSImportRule(String* href, MediaQuerySet* media)
    : CSSRule(CSSRule::IMPORT_RULE)
    , m_strHref(href)
    , m_mediaQuerySet(media)
    , m_generatedSheet(nullptr)
    , m_styleSheetTextResource(nullptr)
{
}

Document* CSSImportRule::document()
{
    STARFISH_ASSERT(m_parentStyleSheet);
    STARFISH_ASSERT(m_parentStyleSheet->origin());
    return m_parentStyleSheet->origin()->document();
}

void CSSImportRule::willStyleSheetLoad()
{
    document()->window()->markHasPendingStyleSheet();
}

void CSSImportRule::didStyleSheetLoadComplete()
{
    document()->window()->unmarkHasPendingStyleSheet();
}

String* CSSImportRule::cssText() const
{
    String* result = String::emptyString;
    result->concat(String::createASCIIString("@import url(\""));
    result->concat(m_strHref);
    result->concat(String::createASCIIString("\")"));

    if (mediaQuerySet()) {
        String* mediaText = mediaQuerySet()->mediaText();
        if (!mediaText->equals(String::emptyString)) {
            result->concat(String::createASCIIString(" "));
            result->concat(mediaText);
        }
    }
    result->concat(String::createASCIIString(";"));

    return result;
}

class ImportedStyleSheetDownloadClient : public ResourceClient {
public:
    ImportedStyleSheetDownloadClient(CSSImportRule* ownerRule, Resource* res)
        : ResourceClient(res)
        , m_ownerRule(ownerRule)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        m_ownerRule->m_styleSheetTextResource = nullptr;
        m_ownerRule->didStyleSheetLoadComplete();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        String* text = m_resource->asTextResource()->text();

        Document* doc = m_ownerRule->document();
        if (!doc) {
            return;
        }

        CSSStyleSheet* sheet = new CSSStyleSheet(
            m_ownerRule->parentStyleSheet()->origin(), text, m_ownerRule);
        if (sheet) {
            m_ownerRule->m_generatedSheet = sheet;
            doc->styleResolver().addSheet(sheet);
            doc->window()->setWholeDocumentNeedsStyleRecalc();
        }

        m_ownerRule->m_styleSheetTextResource = nullptr;
        m_ownerRule->didStyleSheetLoadComplete();
    }

protected:
    CSSImportRule* m_ownerRule;
};

void CSSImportRule::unloadStyleSheetIfExists()
{
    if (m_styleSheetTextResource) {
        m_styleSheetTextResource->cancel();
        m_styleSheetTextResource = nullptr;
    }
    if (m_generatedSheet) {
        Document* doc = document();
        doc->styleResolver().removeSheet(m_generatedSheet);
        doc->window()->setWholeDocumentNeedsStyleRecalc();
        m_generatedSheet = nullptr;
    }
}

void CSSImportRule::requestStyleSheet()
{
    if (!m_parentStyleSheet || !m_parentStyleSheet->origin()) {
        return;
    }

    Document* doc = document();
    if (!doc) {
        return;
    }

    unloadStyleSheetIfExists();

    ResourceURL* absURL =
        new ResourceURL(m_strHref, doc->documentURI()->baseURI());

    CSSStyleSheet* rootSheet = m_parentStyleSheet;
    for (CSSStyleSheet* sheet = m_parentStyleSheet; sheet;
         sheet = sheet->parentStyleSheet()) {
        if (absURL->getUrlPathString()->equals(
                sheet->url()->getUrlPathString())) {
            return;
        }
        rootSheet = sheet;
    }

    if (m_styleSheetTextResource) {
        m_styleSheetTextResource->cancel();
    }
    m_styleSheetTextResource = doc->resourceLoader().fetchText(absURL);
    m_styleSheetTextResource->addResourceClient(
        new ImportedStyleSheetDownloadClient(this, m_styleSheetTextResource));

    m_styleSheetTextResource->request();
}
}
