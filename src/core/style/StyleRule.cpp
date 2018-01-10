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
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/style/CSSRule.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleRule.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/StyleRule.h"
#include "core/style/MediaQuerySet.h"
#include "core/style/AncestorSelectorFilter.h"

namespace StarFish {

CSSRule* StyleRuleBase::createCSSOMWrapper(CSSStyleSheet* parentSheet) const
{
    return createCSSOMWrapper(parentSheet, 0);
}

CSSRule* StyleRuleBase::createCSSOMWrapper(CSSRule* parentRule) const
{
    return createCSSOMWrapper(0, parentRule);
}

CSSRule* StyleRuleBase::createCSSOMWrapper(CSSStyleSheet* parentSheet,
                                           CSSRule* parentRule) const
{
    CSSRule* rule = nullptr;
    StyleRuleBase* self = const_cast<StyleRuleBase*>(this);

    switch (type()) {
    case STYLE_RULE:
        rule = new CSSStyleRule(self->asStyleRule(), parentSheet);
        break;
    case MEDIA_RULE:
        rule = new CSSMediaRule(self->asStyleRuleMedia(), parentSheet);
        break;
    case IMPORT_RULE:
        rule = new CSSImportRule(self->asStyleRuleImport(), parentSheet);
        break;
    case FONT_FACE_RULE:
        rule = new CSSFontFaceRule(self->asStyleRuleFontFace(), parentSheet);
        break;
    default:
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
        return nullptr;
    }

    if (parentRule) {
        rule->setParentRule(parentRule);
    }

    return rule;
}

StyleRule::StyleRule(CSSSelectorList&& selectorList, CSSStyleDeclaration* decl)
    : StyleRuleBase(StyleRuleBase::STYLE_RULE)
    , m_selectorList(std::move(selectorList))
    , m_styleDeclaration(decl)
    , m_order(0)
    , m_isUARule(false)
{
    initFlagsRelatedWithSelectorList();
}

void StyleRule::initFlagsRelatedWithSelectorList()
{
    m_isSimpleIDSelector = false;
    m_isSimpleClassSelector = false;
    m_isSimpleTagSelector = false;

    unsigned size = m_selectorList.size();
    if (size == 1) {
        if (m_selectorList[0]->type() == CSSSelector::Id) {
            m_isSimpleIDSelector = true;
        } else if (m_selectorList[0]->type() == CSSSelector::Class) {
            m_isSimpleClassSelector = true;
        } else if (m_selectorList[0]->type() == CSSSelector::Tag) {
            m_isSimpleTagSelector = true;
        }
    }

    AncestorSelectorFilter::computeIdentifierHash(this);
}

void* StyleRule::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word obj_bitmap[GC_BITMAP_SIZE(StyleRule)] = { 0 };
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StyleRule, m_selectorList));
        GC_set_bit(obj_bitmap, GC_WORD_OFFSET(StyleRule, m_styleDeclaration));
        descr = GC_make_descriptor(obj_bitmap, GC_WORD_LEN(StyleRule));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void StyleRule::wrapperTakeSelectorList(CSSSelectorList& selectors)
{
    m_selectorList = std::move(selectors);
    initFlagsRelatedWithSelectorList();
}

StyleRuleGroup::StyleRuleGroup(RuleType type, GCVector<StyleRuleBase*>& rules)
    : StyleRuleBase(type)
{
    m_childRules.assign(rules.begin(), rules.end());
}
StyleRuleGroup::StyleRuleGroup(StyleRuleGroup& o)
    : StyleRuleBase(o.type())
{
    m_childRules.assign(o.childRules().begin(), o.childRules().end());
}

void StyleRuleGroup::wrapperInsertRule(unsigned index, StyleRuleBase* rule)
{
    m_childRules.insert(m_childRules.begin() + index, rule);
}

void StyleRuleGroup::wrapperRemoveRule(unsigned index)
{
    m_childRules.erase(m_childRules.begin() + index);
}

StyleRuleCondition::StyleRuleCondition(RuleType type, String* conditionText,
                                       GCVector<StyleRuleBase*>& rules)
    : StyleRuleGroup(type, rules)
    , m_conditionText(conditionText)
{
}

StyleRuleCondition::StyleRuleCondition(RuleType type,
                                       GCVector<StyleRuleBase*>& rules)
    : StyleRuleGroup(type, rules)
{
    m_conditionText = String::emptyString;
}

StyleRuleCondition::StyleRuleCondition(StyleRuleCondition& conditionRule)
    : StyleRuleGroup(conditionRule)
    , m_conditionText(conditionRule.m_conditionText)
{
}

StyleRuleMedia::StyleRuleMedia(MediaQuerySet* media,
                               GCVector<StyleRuleBase*>& rules)
    : StyleRuleCondition(StyleRuleBase::MEDIA_RULE, rules)
    , m_mediaQuerySet(media)
{
}

StyleRuleMedia::StyleRuleMedia(StyleRuleMedia& o)
    : StyleRuleCondition(o)
{
    if (o.mediaQuerySet()) {
        m_mediaQuerySet = MediaQuerySet::create(o.mediaQuerySet()->document());
        m_mediaQuerySet->queryVector().assign(
            o.mediaQuerySet()->queryVector().begin(),
            o.mediaQuerySet()->queryVector().end());
    }
}

StyleRuleImport::StyleRuleImport(String* href, MediaQuerySet* media)
    : StyleRuleBase(StyleRuleBase::IMPORT_RULE)
    , m_strHref(href)
    , m_mediaQuerySet(media)
    , m_generatedSheet(nullptr)
    , m_styleSheetTextResource(nullptr)
    , m_parentStyleSheet(nullptr)
    , m_loading(false)
{
}

Document* StyleRuleImport::document()
{
    STARFISH_ASSERT(m_parentStyleSheet);
    STARFISH_ASSERT(m_parentStyleSheet->origin());
    return m_parentStyleSheet->origin()->document();
}

void StyleRuleImport::willStyleSheetLoad()
{
    document()->window()->browsingContext()->markHasPendingStyleSheet();
}

void StyleRuleImport::didStyleSheetLoadComplete()
{
    document()->window()->browsingContext()->unmarkHasPendingStyleSheet();
}

class ImportedStyleSheetDownloadClient : public ResourceClient {
public:
    ImportedStyleSheetDownloadClient(StyleRuleImport* ownerRule, Resource* res)
        : ResourceClient(res)
        , m_ownerRule(ownerRule)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        m_ownerRule->m_styleSheetTextResource = nullptr;
        m_ownerRule->didStyleSheetLoadComplete();
        m_ownerRule->m_loading = false;
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();

        if (m_ownerRule->m_generatedSheet) {
            m_ownerRule->m_generatedSheet->clearOwnerRule();
        }

        String* text = m_resource->asTextResource()->text();
        Document* doc = m_ownerRule->document();
        if (!doc) {
            return;
        }

        CSSStyleSheet* sheet =
            new CSSStyleSheet(m_ownerRule->parentStyleSheet()->origin(), text);
        if (sheet) {
            m_ownerRule->m_generatedSheet = sheet;
            m_ownerRule->m_generatedSheet->parseSheetIfneeds();
            m_ownerRule->m_loading = false;
            doc->window()->browsingContext()->setNeedsStyleSheetsRecalc();
        }

        m_ownerRule->m_styleSheetTextResource = nullptr;
        m_ownerRule->didStyleSheetLoadComplete();
    }

protected:
    StyleRuleImport* m_ownerRule;
};

bool StyleRuleImport::isLoading() const
{
    return m_loading || !m_generatedSheet;
}

void StyleRuleImport::unloadStyleSheetIfExists()
{
    if (m_styleSheetTextResource) {
        m_styleSheetTextResource->cancel();
        m_styleSheetTextResource = nullptr;
    }
    if (m_generatedSheet) {
        Document* doc = document();
        doc->styleResolver().removeSheet(m_generatedSheet);
        doc->window()->browsingContext()->setNeedsStyleSheetsRecalc();
        m_generatedSheet = nullptr;
    }
}

void StyleRuleImport::requestStyleSheet()
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
        new ResourceURL(m_strHref, m_parentStyleSheet->url()->urlString());
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

    m_styleSheetTextResource->request(
        Resource::ResourceRequestSyncLevel::NeverSync,
        document()->documentURI(), true);
    m_loading = true;
}

StyleRuleFontFace::StyleRuleFontFace(CSSStyleDeclaration* decl)
    : StyleRuleBase(StyleRuleBase::FONT_FACE_RULE)
    , m_styleDeclaration(decl)
{
}
}
