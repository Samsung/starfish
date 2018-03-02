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

void* HTMLScriptElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLScriptElement)] = { 0 };
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLScriptElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

static bool isJavaScriptType(const char* type, size_t len)
{
    if (strcmp("", type) == 0) {
        return true;
    } else if (len == 15 && memcmp("text/javascript", type, 15) == 0) {
        return true;
    } else if (strcmp("application/javascript", type) == 0) {
        return true;
    } else if (strcmp("application/x-javascript", type) == 0) {
        return true;
    } else if (strcmp("application/octet-stream", type) == 0) {
        return true;
    } else if (strcmp("application/ecmascript", type) == 0) {
        return true;
    } else if (strcmp("text/ecmascript", type) == 0) {
        return true;
    } else if (strcmp("text/plain", type) == 0) {
        return true;
    } else if (strcmp("text/html", type) == 0) {
        return true;
    }
    return false;
}

class DeferredScriptDownloadClient : public ResourceClient {
public:
    DeferredScriptDownloadClient(HTMLScriptElement* script, Resource* res)
        : ResourceClient(res)
        , m_isLoaded(false)
        , m_successToLoad(false)
        , m_responseMIMEType(String::emptyString)
        , m_element(script)
    {
        m_element->document()->m_deferredScriptElements.push_back(
            std::make_pair(m_element, this));
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        m_isLoaded = true;
        didScriptLoaded();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        m_successToLoad = true;
        m_isLoaded = true;
        m_responseMIMEType = m_resource->resourceRequest()->responseMimeType();

        auto& deferredScriptElements =
            m_element->document()->m_deferredScriptElements;
        while (deferredScriptElements.size() &&
               deferredScriptElements.begin()->second->m_isLoaded) {
            auto client = deferredScriptElements.begin()->second;
            auto s =
                client->m_responseMIMEType->toASCIILower()->toUTF8NonGCString();
            if (isJavaScriptType(s.data(), s.length())) {
                String* text = client->m_resource->asTextResource()->text();
                client->m_element->document()->appendCurrentScript(
                    client->m_element);
                evaluateString(
                    client->m_element->window()->scriptBindingInstance(), text,
                    ResourceClient::resource()->url()->urlString());
                client->m_element->document()->popCurrentScript();
            }
            deferredScriptElements.erase(deferredScriptElements.begin());
        }
        didScriptLoaded();
    }

    void didScriptLoaded()
    {
        m_element->m_didScriptExecuted = true;
        if (!m_successToLoad) {
            size_t pos = 0;
            while (true) {
                if (m_element->document()
                        ->m_deferredScriptElements[pos]
                        .second == this) {
                    break;
                }
                pos++;
            }

            m_element->document()->m_deferredScriptElements.erase(pos);
        }

        if (m_element->document()->m_deferredScriptElements.size() == 0 &&
            m_element->document()->documentBuilder() == nullptr) {
            m_element->document()->notifyDomContentLoaded();
        }
    }

    bool m_isLoaded;
    bool m_successToLoad;
    String* m_responseMIMEType;
    HTMLScriptElement* m_element;
};

class ScriptDownloadClient : public ResourceClient {
public:
    ScriptDownloadClient(HTMLScriptElement* script, Resource* res,
                         bool shouldResumeParsing)
        : ResourceClient(res)
        , m_element(script)
        , m_shouldResumeParsing(shouldResumeParsing)
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
        auto s = m_resource->resourceRequest()
                     ->responseMimeType()
                     ->toASCIILower()
                     ->toUTF8NonGCString();
        if (isJavaScriptType(s.data(), s.length())) {
            String* text = m_resource->asTextResource()->text();
            m_element->document()->appendCurrentScript(m_element);
            evaluateString(m_element->window()->scriptBindingInstance(), text,
                           ResourceClient::resource()->url()->urlString());
            m_element->document()->popCurrentScript();
        }
        didScriptLoaded();
    }

    void didScriptLoaded()
    {
        m_element->m_didScriptExecuted = true;
        if (m_shouldResumeParsing) {
            m_element->document()->resumeDocumentParsing();
        }
    }

protected:
    HTMLScriptElement* m_element;
    bool m_shouldResumeParsing;
};

bool HTMLScriptElement::executeScript(bool forceSync, bool inParser)
{
    bool result = executeScriptImpl(forceSync, inParser);
    return result;
}

bool HTMLScriptElement::executeScriptImpl(bool forceSync, bool inParser)
{
    if (m_isParserInserted) {
        return false;
    }

    if (!m_isAlreadyStarted &&
        isInDocumentScopeAndDocumentParticipateInRendering()) {
        Nullable<String*> typeStr =
            getAttribute(starFish()->staticStrings()->m_type);
        if (typeStr.hasValue()) {
            auto utf8Data =
                typeStr.getValue()->toASCIILower()->toUTF8NonGCString();
            if (!isJavaScriptType(utf8Data.data(), utf8Data.length())) {
                return false;
            }
        }
        Nullable<String*> srcStr =
            getAttribute(starFish()->staticStrings()->m_src);
        if (!srcStr.hasValue()) {
            if (!firstChild()) {
                return false;
            }
            String* script = text();
            m_isAlreadyStarted = true;
            document()->appendCurrentScript(this);
            evaluateString(
                window()->scriptBindingInstance(), script,
                String::createASCIIString("HTMLScriptElement innerText"));
            document()->popCurrentScript();
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
                new ResourceURL(url, document()->baseURL()->baseURI()),
                charset);
            if (!async() && defer()) {
                res->addResourceClient(
                    new DeferredScriptDownloadClient(this, res));
            } else {
                bool shouldResumeParsing = inParser && !forceSync && !async();
                res->addResourceClient(
                    new ScriptDownloadClient(this, res, shouldResumeParsing));
            }
            res->addResourceClient(new ElementResourceClient(this, res, true));
            res->request(forceSync
                             ? Resource::ResourceRequestSyncLevel::AlwaysSync
                             : Resource::ResourceRequestSyncLevel::NeverSync,
                         document()->documentURI(), true);
            if (async() || defer()) {
                return false;
            } else {
                return true;
            }
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

void HTMLScriptElement::didNodeInsertedToDocumentTree()
{
    HTMLElement::didNodeInsertedToDocumentTree();
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

QualifiedName HTMLScriptElement::name()
{
    return starFish()->staticStrings()->m_scriptTagName;
}

String* HTMLScriptElement::src()
{
    String* url = getAttributeOrEmpty(starFish()->staticStrings()->m_src);
    if (url->equals(String::emptyString)) {
        return String::emptyString;
    }

    return ResourceURL::mergeDocumentURIWithURIString(
        document()->baseURL()->baseURI(), url);
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

bool HTMLScriptElement::async()
{
    return hasAttribute(starFish()->staticStrings()->m_async) != SIZE_MAX;
}

void HTMLScriptElement::setAsync(bool b)
{
    if (b) {
        setAttribute(starFish()->staticStrings()->m_async, String::emptyString);
    } else {
        removeAttribute(starFish()->staticStrings()->m_async);
    }
}

bool HTMLScriptElement::defer()
{
    return hasAttribute(starFish()->staticStrings()->m_defer) != SIZE_MAX;
}

void HTMLScriptElement::setDefer(bool b)
{
    if (b) {
        setAttribute(starFish()->staticStrings()->m_defer, String::emptyString);
    } else {
        removeAttribute(starFish()->staticStrings()->m_defer);
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
