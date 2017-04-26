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
#include "dom/Document.h"
#include "dom/HTMLScriptElement.h"
#include "dom/Text.h"
#include "dom/builder/html/HTMLDocumentBuilder.h"
#include "dom/parser/HTMLParser.h"
#include "platform/message_loop/MessageLoop.h"
#include "platform/network/NetworkRequest.h"
#include "platform/window/Window.h"
#include "loader/ElementResourceClient.h"

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
        if (isJavaScriptType(m_resource->networkRequest()
                                 ->responseMimeType()
                                 ->toLower()
                                 ->utf8Data()) ||
            m_resource->networkRequest()->responseMimeType()->toLower()->equals(
                "text/plain") ||
            m_resource->networkRequest()->responseMimeType()->toLower()->equals(
                "text/html") ||
            m_resource->networkRequest()->responseMimeType()->toLower()->equals(
                "application/json")) {
            String* text = m_resource->asTextResource()->text();
            m_element->document()->window()->scriptBindingInstance()->evaluate(
                text);
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
        String* typeAttr = getAttribute(
            document()->window()->starFish()->staticStrings()->m_type);
        if (!typeAttr->equals(String::emptyString) &&
            !isJavaScriptType(typeAttr->toLower()->utf8Data())) {
            return false;
        }
        size_t idx = hasAttribute(
            document()->window()->starFish()->staticStrings()->m_src);
        if (idx == SIZE_MAX) {
            if (!firstChild()) {
                return false;
            }
            String* script = text();
            m_isAlreadyStarted = true;
            document()->window()->scriptBindingInstance()->evaluate(script);
            m_didScriptExecuted = true;
            return false;
        } else {
            String* url = getAttribute(idx);
            m_isAlreadyStarted = true;

            if (!url->length()) {
                return false;
            }

            String* charset = getAttribute(document()
                                               ->window()
                                               ->starFish()
                                               ->staticStrings()
                                               ->m_charset)
                                  ->trim();
            TextResource* res = document()->resourceLoader().fetchText(
                URL::createURL(document()->documentURI()->baseURI(), url),
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
    if (name == document()->window()->starFish()->staticStrings()->m_src) {
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
    return document()
        ->window()
        ->starFish()
        ->staticStrings()
        ->m_scriptTagName.localName();
}

QualifiedName HTMLScriptElement::name()
{
    return document()->window()->starFish()->staticStrings()->m_scriptTagName;
}

String* HTMLScriptElement::src()
{
    String* url =
        getAttribute(document()->window()->starFish()->staticStrings()->m_src);

    return URL::getURLString(document()->documentURI()->baseURI(), url);
}

void HTMLScriptElement::setSrc(String* src)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_src, src);
}

String* HTMLScriptElement::type()
{
    return getAttribute(
        document()->window()->starFish()->staticStrings()->m_type);
}

void HTMLScriptElement::setType(String* type)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_type,
                 type);
}

String* HTMLScriptElement::charset()
{
    return getAttribute(
        document()->window()->starFish()->staticStrings()->m_charset);
}

void HTMLScriptElement::setCharset(String* charset)
{
    setAttribute(document()->window()->starFish()->staticStrings()->m_charset,
                 charset);
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
    HTMLScriptElement* n = HTMLElement::clone()
                               ->asElement()
                               ->asHTMLElement()
                               ->asHTMLScriptElement();
    n->m_isAlreadyStarted = m_isAlreadyStarted;
    n->m_didScriptExecuted = m_didScriptExecuted;
    return n;
}
}
