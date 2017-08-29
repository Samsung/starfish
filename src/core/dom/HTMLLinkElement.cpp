/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "core/dom/HTMLLinkElement.h"

#include "StarFish.h"
#include "core/dom/Document.h"
#include "platform/loader/ElementResourceClient.h"
#include "platform/file/FileIO.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleSheet.h"
#include "core/style/MediaQueryEvaluator.h"

namespace StarFish {

bool isCSSType(const char* type);

void* HTMLLinkElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLLinkElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLLinkElement, m_generatedSheet));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(HTMLLinkElement, m_styleSheetTextResource));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLLinkElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName HTMLLinkElement::name()
{
    return starFish()->staticStrings()->m_linkTagName;
}

String* HTMLLinkElement::href()
{
    String* url = getAttributeOrEmpty(starFish()->staticStrings()->m_href);

    return ResourceURL::mergeDocumentURIWithURIString(
        document()->documentURI()->baseURI(), url);
}

void HTMLLinkElement::setHref(String* href)
{
    setAttribute(starFish()->staticStrings()->m_href, href);
}

String* HTMLLinkElement::rel()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_rel);
}

void HTMLLinkElement::setRel(String* rel)
{
    setAttribute(starFish()->staticStrings()->m_rel, rel);
}

String* HTMLLinkElement::media()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_media);
}

void HTMLLinkElement::setMedia(String* media)
{
    setAttribute(starFish()->staticStrings()->m_media, media);
}

String* HTMLLinkElement::type()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_type);
}

void HTMLLinkElement::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}

ResourceURL* HTMLLinkElement::url()
{
    Nullable<String*> url = getAttribute(starFish()->staticStrings()->m_href);

    if (!url.hasValue()) {
        return nullptr;
    }
    return new ResourceURL(url.getValue(),
                           document()->documentURI()->baseURI());
}

void HTMLLinkElement::didNodeInsertedToDocumentTree()
{
    HTMLElement::didNodeInsertedToDocumentTree();
    checkLoadStyleSheet();
}

void HTMLLinkElement::didNodeRemovedFromDocumentTree()
{
    HTMLElement::didNodeRemovedFromDocumentTree();
    unloadStyleSheetIfExists();
}

void HTMLLinkElement::checkLoadStyleSheet()
{
    if (!isInDocumentScopeAndDocumentParticipateInRendering()) {
        unloadStyleSheetIfExists();
        return;
    }

    Nullable<String*> type = getAttribute(starFish()->staticStrings()->m_type);
    Nullable<String*> href = getAttribute(starFish()->staticStrings()->m_href);
    Nullable<String*> rel = getAttribute(starFish()->staticStrings()->m_rel);

    if (((type.hasValue() &&
          isCSSType(type.getValue()->toLower()->utf8Data())) ||
         !type.hasValue()) &&
        href.hasValue() && !href.getValue()->isEmpty() && rel.hasValue() &&
        rel.getValue()->toLower()->equals("stylesheet")) {
        loadStyleSheet();
    } else {
        unloadStyleSheetIfExists();
    }
}

class StyleSheetDownloadClient : public ResourceClient {
public:
    StyleSheetDownloadClient(HTMLLinkElement* element, Resource* res)
        : ResourceClient(res)
        , m_element(element)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        m_element->m_styleSheetTextResource = nullptr;
        m_element->didStyleSheetLoadComplete();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        String* text = m_resource->asTextResource()->text();

        CSSStyleSheet* sheet = new CSSStyleSheet(m_element, text);
        sheet->parseSheetIfneeds();
        m_element->m_generatedSheet = sheet;
        m_element->document()->styleResolver().addSheet(sheet);

        CSSParser parser(m_element->document());
        parser.makeToken(m_element->media());
        MediaQuerySet* mediaQuerySet = parser.parseMediaQuery();
        sheet->setMediaQuerySet(mediaQuerySet);
        const MediaQueryEvaluator& evaluator =
            m_element->document()->styleResolver().mediaQueryEvaluator();
        if (evaluator.eval(mediaQuerySet)) {
            m_element->window()
                ->browsingContext()
                ->setWholeDocumentNeedsStyleRecalc();
        }

        m_element->m_styleSheetTextResource = nullptr;
        m_element->didStyleSheetLoadComplete();
    }

protected:
    HTMLLinkElement* m_element;
};

void HTMLLinkElement::loadStyleSheet()
{
    unloadStyleSheetIfExists();
    String* urlString =
        getAttributeOrEmpty(starFish()->staticStrings()->m_href);
    ResourceURL* url =
        new ResourceURL(urlString, document()->documentURI()->baseURI());

    if (m_styleSheetTextResource) {
        m_styleSheetTextResource->cancel();
    }
    m_styleSheetTextResource = document()->resourceLoader().fetchText(url);
    m_styleSheetTextResource->addResourceClient(
        new StyleSheetDownloadClient(this, m_styleSheetTextResource));
    m_styleSheetTextResource->addResourceClient(
        new ElementResourceClient(this, m_styleSheetTextResource));
    willStyleSheetLoad();
    m_styleSheetTextResource->request(
        Resource::ResourceRequestSyncLevel::NeverSync,
        document()->documentURI());
}

void HTMLLinkElement::unloadStyleSheetIfExists()
{
    if (m_styleSheetTextResource) {
        m_styleSheetTextResource->cancel();
        m_styleSheetTextResource = nullptr;
    }
    if (m_generatedSheet) {
        document()->styleResolver().removeSheet(m_generatedSheet);
        window()->browsingContext()->setWholeDocumentNeedsStyleRecalc();
        m_generatedSheet = nullptr;
    }
}

void HTMLLinkElement::didAttributeChanged(QualifiedName name, String* old,
                                          String* value, bool attributeCreated,
                                          bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starFish()->staticStrings()->m_href) {
        if (!old->equals(value)) {
            checkLoadStyleSheet();
        }
    } else if (name == starFish()->staticStrings()->m_type) {
        checkLoadStyleSheet();
    } else if (name == starFish()->staticStrings()->m_rel) {
        checkLoadStyleSheet();
    } else if (name == starFish()->staticStrings()->m_media) {
        if (!old->equals(value)) {
            checkLoadStyleSheet();
        }
    }
}

void HTMLLinkElement::willStyleSheetLoad()
{
    STARFISH_ASSERT(isInDocumentScopeAndDocumentParticipateInRendering());
    window()->browsingContext()->markHasPendingStyleSheet();
}

void HTMLLinkElement::didStyleSheetLoadComplete()
{
    window()->browsingContext()->unmarkHasPendingStyleSheet();
}
}
