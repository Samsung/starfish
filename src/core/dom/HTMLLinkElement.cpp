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

#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLLinkElement.h"
#include "platform/loader/ElementResourceClient.h"
#include "platform/file/FileIO.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/window/Window.h"
#include "core/style/CSSParser.h"

namespace StarFish {

bool isCSSType(const char* type);

String* HTMLLinkElement::localName()
{
    return starFish()->staticStrings()->m_linkTagName.localName();
}

QualifiedName HTMLLinkElement::name()
{
    return starFish()->staticStrings()->m_linkTagName;
}

String* HTMLLinkElement::href()
{
    String* url = getAttributeOrEmpty(starFish()->staticStrings()->m_href);

    return URL::getURLString(document()->documentURI()->baseURI(), url);
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

String* HTMLLinkElement::type()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_type);
}

void HTMLLinkElement::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}

URL* HTMLLinkElement::url()
{
    Nullable<String*> url = getAttribute(starFish()->staticStrings()->m_href);

    if (!url.hasValue()) {
        return nullptr;
    }
    return URL::createURL(document()->documentURI()->baseURI(), url.getValue());
}

void HTMLLinkElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    checkLoadStyleSheet();
}

void HTMLLinkElement::didNodeRemovedFromDocumenTree()
{
    HTMLElement::didNodeRemovedFromDocumenTree();
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
        href.hasValue() && rel.hasValue() &&
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

        CSSParser parser(m_element->document());
        CSSStyleSheet* sheet = new CSSStyleSheet(m_element, text);
        if (sheet) {
            m_element->m_generatedSheet = sheet;
            m_element->document()->styleResolver().addSheet(sheet);
            m_element->window()->setWholeDocumentNeedsStyleRecalc();
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
    URL* url = URL::createURL(document()->documentURI()->baseURI(), urlString);

    if (m_styleSheetTextResource) {
        m_styleSheetTextResource->cancel();
    }
    m_styleSheetTextResource = document()->resourceLoader().fetchText(url);
    m_styleSheetTextResource->addResourceClient(
        new StyleSheetDownloadClient(this, m_styleSheetTextResource));
    m_styleSheetTextResource->addResourceClient(
        new ElementResourceClient(this, m_styleSheetTextResource));
    willStyleSheetLoad();
    m_styleSheetTextResource->request();
}

void HTMLLinkElement::unloadStyleSheetIfExists()
{
    if (m_styleSheetTextResource) {
        m_styleSheetTextResource->cancel();
        m_styleSheetTextResource = nullptr;
    }
    if (m_generatedSheet) {
        document()->styleResolver().removeSheet(m_generatedSheet);
        window()->setWholeDocumentNeedsStyleRecalc();
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
    }
}

void HTMLLinkElement::willStyleSheetLoad()
{
    STARFISH_ASSERT(isInDocumentScopeAndDocumentParticipateInRendering());
    window()->markHasPendingStyleSheet();
}

void HTMLLinkElement::didStyleSheetLoadComplete()
{
    window()->unmarkHasPendingStyleSheet();
}
}
