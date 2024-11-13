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
#include "core/dom/Document.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/style/CSSRule.h"
#include "core/style/CSSStyleDeclaration.h"
#include "core/style/CSSStyleRule.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/CSSParser.h"
#include "core/style/StyleRule.h"
#include "core/style/MediaQuerySet.h"
#include "core/style/AncestorSelectorFilter.h"
#include "core/csp/ContentSecurityPolicy.h"
#include "platform/loader/ResourceLoader.h"

namespace Starfish {

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
    case CSSRule::Type::STYLE_RULE:
        rule = new CSSStyleRule(self->asStyleRule(), parentSheet);
        break;
    case CSSRule::Type::MEDIA_RULE:
        rule = new CSSMediaRule(self->asStyleRuleMedia(), parentSheet);
        break;
    case CSSRule::Type::IMPORT_RULE:
        rule = new CSSImportRule(self->asStyleRuleImport(), parentSheet);
        break;
    case CSSRule::Type::FONT_FACE_RULE:
        rule = new CSSFontFaceRule(self->asStyleRuleFontFace(), parentSheet);
        break;
    case CSSRule::Type::SUPPORTS_RULE:
        rule = new CSSSupportsRule(self->asStyleRuleSupports(), parentSheet);
        break;
    case CSSRule::Type::COUNTER_STYLE_RULE:
        rule = new CSSCounterStyleRule(self->asStyleRuleCounterStyle(),
                                       parentSheet);
        break;
    case CSSRule::Type::NAMESPACE_RULE:
        rule = new CSSNamespaceRule(self->asStyleRuleNamespace(), parentSheet);
        break;
    case CSSRule::Type::KEYFRAME_RULE:
        rule = new CSSKeyframeRule(self->asStyleRuleKeyframe(), parentSheet);
        break;
    case CSSRule::Type::KEYFRAMES_RULE:
        rule = new CSSKeyframesRule(self->asStyleRuleKeyframes(), parentSheet);
        break;
    default:
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        return nullptr;
    }

    if (parentRule) {
        rule->setParentRule(parentRule);
    }

    return rule;
}

StyleRule::StyleRule(CSSSelectorList&& selectorList, CSSStyleDeclaration* decl)
    : StyleRuleBase(CSSRule::Type::STYLE_RULE)
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
    m_isSimplePseudoClassHostSelector = false;
    m_isPseudoClassHostSelector = false;

    unsigned size = m_selectorList.size();
    if (size == 1) {
        if (m_selectorList[0].m_selector->type() == CSSSelector::Type::Id) {
            m_isSimpleIDSelector = true;
        } else if (m_selectorList[0].m_selector->type() ==
                   CSSSelector::Type::Class) {
            m_isSimpleClassSelector = true;
        } else if (m_selectorList[0].m_selector->type() ==
                   CSSSelector::Type::Tag) {
            m_isSimpleTagSelector = true;
        } else if (m_selectorList[0].m_selector->type() ==
                       CSSSelector::Type::PseudoClass &&
                   m_selectorList[0].m_selector->pseudotype() ==
                       CSSSelector::PseudoType::PseudoHost) {
            m_isSimplePseudoClassHostSelector = true;
            m_isPseudoClassHostSelector = true;
        }
    } else {
        for (unsigned i = 0; i < size; ++i) {
            CSSSelector* selector = m_selectorList[i].m_selector;
            if (selector->type() == CSSSelector::Type::PseudoClass &&
                selector->pseudotype() == CSSSelector::PseudoType::PseudoHost) {
                m_isPseudoClassHostSelector = true;
            }
        }
    }

    AncestorSelectorFilter::computeIdentifierHash(this);
}

void* StyleRule::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(StyleRule));
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

StyleRuleGroup::StyleRuleGroup(CSSRule::Type type,
                               GCVector<StyleRuleBase*>& rules)
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

StyleRuleCondition::StyleRuleCondition(CSSRule::Type type,
                                       String* conditionText,
                                       GCVector<StyleRuleBase*>& rules)
    : StyleRuleGroup(type, rules)
    , m_conditionText(conditionText)
{
}

StyleRuleCondition::StyleRuleCondition(CSSRule::Type type,
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
    : StyleRuleCondition(CSSRule::Type::MEDIA_RULE, rules)
    , m_mediaQuerySet(media)
{
}

StyleRuleMedia::StyleRuleMedia(StyleRuleMedia& o)
    : StyleRuleCondition(o)
    , m_mediaQuerySet(nullptr)
{
    if (o.mediaQuerySet()) {
        m_mediaQuerySet = MediaQuerySet::create(o.mediaQuerySet()->document());
        m_mediaQuerySet->queryVector().assign(
            o.mediaQuerySet()->queryVector().begin(),
            o.mediaQuerySet()->queryVector().end());
    }
}

StyleRuleImport::StyleRuleImport(String* href, MediaQuerySet* media)
    : StyleRuleBase(CSSRule::Type::IMPORT_RULE)
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
            m_ownerRule->m_generatedSheet->willAddToDocument();
            m_ownerRule->m_generatedSheet->root()
                ->styleResolver()
                .setNeedsRecalcRuleSet();
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
        m_generatedSheet->willRemovedFromDocument();
        m_parentStyleSheet->origin()->styleResolver().removeSheet(
            m_generatedSheet);
        document()->window()->browsingContext()->setNeedsStyleSheetsRecalc();
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

    if (!document()->contentSecurityPolicy()->allowSource(
            CSPDirectives::StyleSrc, absURL)) {
        return;
    }

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

    RequestData* reqData = new RequestData();
    reqData->m_url = absURL;
    reqData->m_referrer = new ReferrerURL(document()->documentURI());
    reqData->m_destination = RequestDestination::Style;
    reqData->m_syncLevel = RequestSyncLevel::NeverSync;

    m_styleSheetTextResource->request(reqData, true);
    m_loading = true;
}

StyleRuleFontFace::StyleRuleFontFace(CSSStyleDeclaration* decl)
    : StyleRuleBase(CSSRule::Type::FONT_FACE_RULE)
    , m_styleDeclaration(decl)
{
}

StyleRuleNamespace::StyleRuleNamespace(String* namespaceURI, String* prefix)
    : StyleRuleBase(CSSRule::Type::NAMESPACE_RULE)
    , m_namespaceURI(namespaceURI)
    , m_prefix(prefix)
{
}

StyleRuleKeyframe::StyleRuleKeyframe(GCAtomicVector<double>& keyList,
                                     CSSStyleDeclaration* decl)
    : StyleRuleBase(CSSRule::Type::KEYFRAME_RULE)
    , m_keyList(std::move(keyList))
    , m_styleDeclaration(decl)
{
    STARFISH_ASSERT(decl != nullptr);
}

String* StyleRuleKeyframe::keyText()
{
    STARFISH_ASSERT(!m_keyList.empty());
    StringBuilder keyText;
    for (unsigned i = 0; i < m_keyList.size(); ++i) {
        if (i) {
            keyText.appendString(", ");
        }
        keyText.appendString(String::fromDouble(m_keyList[i] * 100));
        keyText.appendChar('%');
    }
    return keyText.finalize();
}

bool StyleRuleKeyframe::setKeyText(Document* doc, String* text)
{
    STARFISH_ASSERT(doc != nullptr);
    STARFISH_ASSERT(text != nullptr);
    STARFISH_ASSERT(!text->isEmpty());

    CSSParser parser(doc);
    RefPtr<CSSToken> token = parser.makeToken(text);
    GCAtomicVector<double> keys;
    parser.parseKeyframeKeyList(token, keys);

    if (keys.empty()) {
        return false;
    }

    m_keyList = keys;
    return true;
}

String* StyleRuleKeyframe::cssText()
{
    StringBuilder result;
    result.appendString(keyText());
    result.appendString(" { ");
    String* decls = styleDeclaration()->cssText();
    result.appendString(decls);
    result.appendString(" }");

    return result.finalize();
}

StyleRuleKeyframes::StyleRuleKeyframes(String* name,
                                       GCVector<StyleRuleBase*>& keyframes)
    : StyleRuleBase(CSSRule::Type::KEYFRAMES_RULE)
    , m_name(name)
    , m_keyframes(std::move(keyframes))
    , m_version(0)
{
    STARFISH_ASSERT(name != nullptr);
}

void StyleRuleKeyframes::wrapperAppendKeyframe(StyleRuleKeyframe* keyframe)
{
    STARFISH_ASSERT(keyframe != nullptr);
    m_keyframes.push_back(keyframe);
    styleChanged();
}

void StyleRuleKeyframes::wrapperRemoveKeyframe(int index)
{
    m_keyframes.erase(m_keyframes.begin() + index);
    styleChanged();
}

int StyleRuleKeyframes::findKeyframeIndex(Document* doc, String* key) const
{
    STARFISH_ASSERT(doc != nullptr);
    STARFISH_ASSERT(key != nullptr);
    CSSParser parser(doc);
    RefPtr<CSSToken> token = parser.makeToken(key);

    GCAtomicVector<double> keyList;
    parser.parseKeyframeKeyList(token, keyList);

    if (keyList.size() == 0) {
        return -1;
    }

    for (size_t i = m_keyframes.size(); i--;) {
        if (static_cast<StyleRuleKeyframe*>(m_keyframes[i])->keyList() ==
            keyList) {
            return i;
        }
    }
    return -1;
}

StyleRuleSupports::StyleRuleSupports(String* conditionText, bool isSupported,
                                     GCVector<StyleRuleBase*>& rules)
    : StyleRuleCondition(CSSRule::Type::SUPPORTS_RULE, conditionText, rules)
    , m_isSupported(isSupported)
{
}

bool StyleRuleSupports::eval(Document* doc, String* conditionText)
{
    CSSParser parser(doc);
    parser.makeToken(conditionText);
    return parser.parseSupportsCondition();
}
} // namespace Starfish
