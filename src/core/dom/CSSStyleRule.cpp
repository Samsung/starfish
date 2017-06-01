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

#include "core/dom/Document.h"
#include "core/dom/CSSRule.h"
#include "core/dom/CSSStyleDeclaration.h"
#include "core/dom/CSSStyleRule.h"
#include "core/style/Style.h"
#include "core/style/MediaQuerySet.h"
#include "core/modules/window/Window.h"

namespace StarFish {

CSSStyleRule::CSSStyleRule(CSSSelector::Type type, AtomicString selectorText)
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
    , m_mediaQuerySet(media)
{
}

CSSStyleRuleMedia::CSSStyleRuleMedia(CSSStyleRuleMedia& o)
    : CSSStyleRuleGroup(o)
{
    if (o.mediaQuerySet()) {
        m_mediaQuerySet = MediaQuerySet::create();
        m_mediaQuerySet->queryVector().clear();
        m_mediaQuerySet->queryVector().assign(
            o.mediaQuerySet()->queryVector().begin(),
            o.mediaQuerySet()->queryVector().end());
    }
}

CSSStyleRuleImport::CSSStyleRuleImport(String* href, MediaQuerySet* media)
    : CSSRule(CSSRule::IMPORT_RULE)
    , m_strHref(href)
    , m_mediaQuerySet(media)
    , m_parentStyleSheet(nullptr)
    , m_generatedSheet(nullptr)
    , m_styleSheetTextResource(nullptr)
{
}

Document* CSSStyleRuleImport::document()
{
    STARFISH_ASSERT(m_parentStyleSheet);
    STARFISH_ASSERT(m_parentStyleSheet->origin());
    return m_parentStyleSheet->origin()->document();
}

void CSSStyleRuleImport::willStyleSheetLoad()
{
    document()->window()->markHasPendingStyleSheet();
}

void CSSStyleRuleImport::didStyleSheetLoadComplete()
{
    document()->window()->unmarkHasPendingStyleSheet();
}

class ImportedStyleSheetDownloadClient : public ResourceClient {
public:
    ImportedStyleSheetDownloadClient(CSSStyleRuleImport* ownerRule,
                                     Resource* res)
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
    CSSStyleRuleImport* m_ownerRule;
};

void CSSStyleRuleImport::unloadStyleSheetIfExists()
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

void CSSStyleRuleImport::requestStyleSheet()
{
    if (!m_parentStyleSheet || !m_parentStyleSheet->origin()) {
        return;
    }

    Document* doc = document();
    if (!doc) {
        return;
    }

    unloadStyleSheetIfExists();

    URL* absURL = URL::createURL(doc->documentURI()->baseURI(), m_strHref);

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
