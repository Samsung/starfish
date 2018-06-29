/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "core/dom/HTMLLinkElement.h"

#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/DOMTokenList.h"
#include "platform/loader/ElementResourceClient.h"
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
    STARFISH_ASSERT(size == sizeof(HTMLLinkElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLLinkElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLLinkElement, m_generatedSheet));
        GC_set_bit(desc,
                   GC_WORD_OFFSET(HTMLLinkElement, m_styleSheetTextResource));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLLinkElement, m_relList));
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
        document()->baseURL()->baseURI(), url);
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

DOMTokenList* HTMLLinkElement::relList()
{
    if (!m_relList) {
        m_relList = new DOMTokenList(this, starFish()->staticStrings()->m_rel);
    }
    return m_relList;
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

String* HTMLLinkElement::referrerPolicy()
{
    return getAttributeOrEmpty(
        document()->starFish()->staticStrings()->m_referrerpolicy);
}

void HTMLLinkElement::setReferrerPolicy(String* policy)
{
    if (ReferrerURL::isValidPolicy(policy)) {
        setAttribute(document()->starFish()->staticStrings()->m_referrerpolicy,
                     policy);
    }
}

StyleSheet* HTMLLinkElement::sheet()
{
    return m_generatedSheet;
}

ResourceURL* HTMLLinkElement::url()
{
    Nullable<String*> url = getAttribute(starFish()->staticStrings()->m_href);

    if (!url.hasValue()) {
        return nullptr;
    }
    return new ResourceURL(url.getValue(), document()->baseURL()->baseURI());
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
          isCSSType(
              type.getValue()->toASCIILower()->toUTF8NonGCString().data())) ||
         !type.hasValue()) &&
        href.hasValue() && !href.getValue()->isEmpty() && rel.hasValue()) {
        String* relString = rel.getValue();
        GCVector<StringView> tokens;
        DOMTokenList::tokenize(relString, tokens);
        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i].equals("stylesheet")) {
                loadStyleSheet();
                return;
            }
        }
    }

    unloadStyleSheetIfExists();
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
            m_element->window()->browsingContext()->setNeedsStyleSheetsRecalc();
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
        new ResourceURL(urlString, document()->baseURL()->baseURI());
    ResourceURL* rUrl =
        new ReferrerURL(document()->documentURI(), referrerPolicy());

    if (m_styleSheetTextResource) {
        m_styleSheetTextResource->cancel();
    }

    m_styleSheetTextResource = nullptr;

    if (document()->preloadScanner()) {
        auto ps = document()->preloadScanner();
        for (size_t i = 0; i < ps->preloadedCSS().size(); i++) {
            TextResource* res = ps->preloadedJS()[i];
            if (*res->url() == *url) {
                if (res->isReceiving()) {
                    m_styleSheetTextResource = res;
                } else if (res->isFinished()) {
                    StyleSheetDownloadClient client(this, res);
                    willStyleSheetLoad();
                    client.didLoadFinished();
                    return;
                }
                break;
            }
        }
    }

    if (!m_styleSheetTextResource) {
        m_styleSheetTextResource = document()->resourceLoader().fetchText(url);
    }
    m_styleSheetTextResource->addResourceClient(
        new StyleSheetDownloadClient(this, m_styleSheetTextResource));
    m_styleSheetTextResource->addResourceClient(
        new ElementResourceClient(this, m_styleSheetTextResource));
    willStyleSheetLoad();
    m_styleSheetTextResource->request(
        Resource::ResourceRequestSyncLevel::NeverSync, rUrl, true);
}

void HTMLLinkElement::unloadStyleSheetIfExists()
{
    if (m_styleSheetTextResource) {
        m_styleSheetTextResource->cancel();
        m_styleSheetTextResource = nullptr;
    }
    if (m_generatedSheet) {
        document()->styleResolver().removeSheet(m_generatedSheet);
        window()->browsingContext()->setNeedsStyleSheetsRecalc();
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
