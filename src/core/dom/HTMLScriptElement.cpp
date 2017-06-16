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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLScriptElement.h"
#include "core/dom/Text.h"
#include "core/dom/builder/html/HTMLDocumentBuilder.h"
#include "core/dom/parser/HTMLParser.h"
#include "core/page/Window.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "platform/loader/ElementResourceClient.h"

namespace StarFish {

static bool isJavaScriptType(const char* type)
{
    if (strcmp("", type) == 0) {
        return true;
    } else if (strcmp("text/javascript", type) == 0) {
        return true;
    } else if (strcmp("application/javascript", type) == 0) {
        return true;
    } else if (strcmp("application/x-javascript", type) == 0) {
        return true;
    } else if (strcmp("application/ecmascript", type) == 0) {
        return true;
    } else if (strcmp("text/ecmascript", type) == 0) {
        return true;
    }
    return false;
}

class ScriptDownloadClient : public ResourceClient {
public:
    ScriptDownloadClient(HTMLScriptElement* script, Resource* res,
                         bool forceSync, bool inParser)
        : ResourceClient(res)
        , m_element(script)
        , m_forceSync(forceSync)
        , m_inParser(inParser)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        didScriptLoaded();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        if (isJavaScriptType(m_resource->resourceRequest()
                                 ->responseMimeType()
                                 ->toLower()
                                 ->utf8Data()) ||
            m_resource->resourceRequest()
                ->responseMimeType()
                ->toLower()
                ->equals("text/plain") ||
            m_resource->resourceRequest()
                ->responseMimeType()
                ->toLower()
                ->equals("text/html") ||
            m_resource->resourceRequest()
                ->responseMimeType()
                ->toLower()
                ->equals("application/json")) {
            String* text = m_resource->asTextResource()->text();
            evaluateString(m_element->window()->scriptBindingInstance(), text,
                           ResourceClient::resource()->url()->urlString());
        }
        didScriptLoaded();
    }

    void didScriptLoaded()
    {
        m_element->m_didScriptExecuted = true;
        if (m_inParser && !m_forceSync) {
            m_element->document()->resumeDocumentParsing();
        }
    }

protected:
    HTMLScriptElement* m_element;
    bool m_forceSync;
    bool m_inParser;
};

bool HTMLScriptElement::executeScript(bool forceSync, bool inParser)
{
    if (m_isParserInserted) {
        return false;
    }

    if (!m_isAlreadyStarted &&
        isInDocumentScopeAndDocumentParticipateInRendering()) {
        Nullable<String*> typeStr =
            getAttribute(starFish()->staticStrings()->m_type);
        if (typeStr.hasValue() &&
            !isJavaScriptType(typeStr.getValue()->toLower()->utf8Data())) {
            return false;
        }
        Nullable<String*> srcStr =
            getAttribute(starFish()->staticStrings()->m_src);
        if (!srcStr.hasValue()) {
            if (!firstChild()) {
                return false;
            }
            String* script = text();
            m_isAlreadyStarted = true;
            evaluateString(
                window()->scriptBindingInstance(), script,
                String::createASCIIString("HTMLScriptElement innerText"));
            m_didScriptExecuted = true;
            return false;
        } else {
            String* url = srcStr.getValue();
            m_isAlreadyStarted = true;

            if (!url->length()) {
                return false;
            }

            String* charset =
                getAttributeOrEmpty(starFish()->staticStrings()->m_charset)
                    ->trim();
            TextResource* res = document()->resourceLoader().fetchText(
                new ResourceURL(url, document()->documentURI()->baseURI()),
                charset);
            res->addResourceClient(
                new ScriptDownloadClient(this, res, forceSync, inParser));
            res->addResourceClient(new ElementResourceClient(this, res, true));
            res->request(forceSync
                             ? Resource::ResourceRequestSyncLevel::AlwaysSync
                             : Resource::ResourceRequestSyncLevel::NeverSync);
            return true;
        }
    }
    return false;
}

void HTMLScriptElement::didAttributeChanged(QualifiedName name, String* old,
                                            String* value,
                                            bool attributeCreated,
                                            bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starFish()->staticStrings()->m_src) {
        executeScript();
    }
}

void HTMLScriptElement::didNodeInsertedToDocumenTree()
{
    HTMLElement::didNodeInsertedToDocumenTree();
    executeScript();
}

void HTMLScriptElement::didCharacterDataModified(String* before, String* after)
{
    HTMLElement::didCharacterDataModified(before, after);
    executeScript();
}

void HTMLScriptElement::didNodeInserted(Node* parent, Node* newChild)
{
    HTMLElement::didNodeInserted(parent, newChild);
    executeScript();
}

String* HTMLScriptElement::localName()
{
    return starFish()->staticStrings()->m_scriptTagName.localName();
}

QualifiedName HTMLScriptElement::name()
{
    return starFish()->staticStrings()->m_scriptTagName;
}

String* HTMLScriptElement::src()
{
    String* url = getAttributeOrEmpty(starFish()->staticStrings()->m_src);

    return ResourceURL::mergeDocumentURIWithURIString(
        document()->documentURI()->baseURI(), url);
}

void HTMLScriptElement::setSrc(String* src)
{
    setAttribute(starFish()->staticStrings()->m_src, src);
}

String* HTMLScriptElement::type()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_type);
}

void HTMLScriptElement::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}

String* HTMLScriptElement::charset()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_charset);
}

void HTMLScriptElement::setCharset(String* charset)
{
    setAttribute(starFish()->staticStrings()->m_charset, charset);
}

String* HTMLScriptElement::text()
{
    String* str = String::emptyString;
    for (Node* child = firstChild(); child != nullptr;
         child = child->nextSibling()) {
        if (child->nodeType() == TEXT_NODE) {
            STARFISH_ASSERT(child->textContent().hasValue());
            str = str->concat(child->textContent().getValue());
        }
    }
    return str;
}

void HTMLScriptElement::setText(String* s)
{
    if (firstChild() && firstChild()->isText()) {
        firstChild()->asText()->setData(s);
    } else {
        setTextContent(s);
    }
}

Node* HTMLScriptElement::clone()
{
    HTMLScriptElement* n = HTMLElement::clone()->asHTMLScriptElement();
    n->m_isAlreadyStarted = m_isAlreadyStarted;
    n->m_didScriptExecuted = m_didScriptExecuted;
    return n;
}
}
