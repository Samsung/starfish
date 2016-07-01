/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "dom/Document.h"
#include "HTMLLinkElement.h"

#include "loader/ElementResourceClient.h"
#include "platform/message_loop/MessageLoop.h"
#include "style/CSSParser.h"
#include "platform/file_io/FileIO.h"

namespace StarFish {

bool isCSSType(const char* type);

URL HTMLLinkElement::href()
{
    size_t href = hasAttribute(document()->window()->starFish()->staticStrings()->m_href);
    if (href != SIZE_MAX) {
        String* url = getAttribute(href);
        return URL(document()->documentURI().baseURI(), url);
    }
    return URL(String::emptyString, String::emptyString);
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
    if (!isInDocumentScope()) {
        unloadStyleSheetIfExists();
        return;
    }

    size_t type = hasAttribute(document()->window()->starFish()->staticStrings()->m_type);
    size_t href = hasAttribute(document()->window()->starFish()->staticStrings()->m_href);
    size_t rel = hasAttribute(document()->window()->starFish()->staticStrings()->m_rel);

    if (((type != SIZE_MAX && isCSSType(getAttribute(type)->toLower()->utf8Data())) || type == SIZE_MAX)
        && href != SIZE_MAX
        && rel != SIZE_MAX && getAttribute(rel)->toLower()->equals("stylesheet")) {
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
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        String* text = m_resource->asTextResource()->text();

        CSSParser parser(m_element->document());
        CSSStyleSheet* sheet = new CSSStyleSheet(m_element, text);
        if (sheet) {
            m_element->m_generatedSheet = sheet;
            m_element->document()->styleResolver()->addSheet(sheet);
            m_element->document()->window()->setWholeDocumentNeedsStyleRecalc();
        }

        m_element->m_styleSheetTextResource = nullptr;
    }
protected:
    HTMLLinkElement* m_element;
};

void HTMLLinkElement::loadStyleSheet()
{
    unloadStyleSheetIfExists();
    size_t href = hasAttribute(document()->window()->starFish()->staticStrings()->m_href);

    String* urlString = getAttribute(href);
    URL url = URL(document()->documentURI().baseURI(), urlString);

    if (m_styleSheetTextResource) {
        m_styleSheetTextResource->cancel();
    }
    if (url) {
        m_styleSheetTextResource = document()->resourceLoader()->fetchText(url);
        m_styleSheetTextResource->addResourceClient(new StyleSheetDownloadClient(this, m_styleSheetTextResource));
        m_styleSheetTextResource->addResourceClient(new ElementResourceClient(this, m_styleSheetTextResource));
        m_styleSheetTextResource->request();
    }
}

void HTMLLinkElement::unloadStyleSheetIfExists()
{
    if (m_styleSheetTextResource) {
        m_styleSheetTextResource->cancel();
        m_styleSheetTextResource = nullptr;
    }
    if (m_generatedSheet) {
        document()->styleResolver()->removeSheet(m_generatedSheet);
        document()->window()->setWholeDocumentNeedsStyleRecalc();
        m_generatedSheet = nullptr;
    }
}

void HTMLLinkElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    if (name == document()->window()->starFish()->staticStrings()->m_href) {
        if (!old->equals(value))
            checkLoadStyleSheet();
    } else if (name == document()->window()->starFish()->staticStrings()->m_type) {
        checkLoadStyleSheet();
    } else if (name == document()->window()->starFish()->staticStrings()->m_rel) {
        checkLoadStyleSheet();
    }
}

}
