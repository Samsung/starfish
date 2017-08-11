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
#include "StarFish.h"

#include "core/dom/HTMLFormElement.h"

#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/Node.h"
#include "core/dom/Traverse.h"
#include "core/page/BrowsingContext.h"
#include "core/modules/resource_request/ResourceRequest.h"

namespace StarFish {

class FormResourceRequest : public ResourceRequestClient {
public:
    FormResourceRequest(Document* document)
        : m_resourceRequest(new ResourceRequest(document))
    {
        m_resourceRequest->addResourceRequestClient(this);
    }

    void onReadyStateChange(ResourceRequest* request, bool fromExplicit)
    {
        if (fromExplicit &&
            (request->readyState() == ResourceRequest::ReadyState::DONE)) {
            if (m_resourceRequest->m_responseType ==
                    ResourceRequest::ResponseType::TEXT_RESPONSE ||
                m_resourceRequest->m_responseType ==
                    ResourceRequest::ResponseType::DOCUMENT_RESPONSE ||
                m_resourceRequest->m_responseType ==
                    ResourceRequest::ResponseType::DEFAULT_RESPONSE) {
                TextConverter textConverter(
                    m_resourceRequest->responseMimeType(),
                    String::fromUTF8("UTF-8"),
                    m_resourceRequest->response().data(),
                    m_resourceRequest->response().size());
                String* m_responseText = textConverter.convert(
                    m_resourceRequest->response().data(),
                    m_resourceRequest->response().size(), true);
                m_resourceRequest->response().clear();

                // TODO: navigate to this document
            }
        }
    }

    void open(String* method, String* url)
    {
        m_resourceRequest->open(ResourceRequest::POST_METHOD, url, true,
                                String::emptyString, String::emptyString);
    }

    void setRequestHeader(String* key, String* value)
    {
        m_resourceRequest->setRequestHeader(key, value);
    }

    void send(String* body)
    {
        m_resourceRequest->send(body);
    }

protected:
    ResourceRequest* m_resourceRequest;
};

class FormDataSetItem : public gc {
public:
    FormDataSetItem(String* name, String* value, String* type)
        : m_name(name)
        , m_value(value)
        , m_type(type)
    {
    }

    String* m_name;
    String* m_value;
    String* m_type;
};

HTMLFormElement::HTMLFormElement(Document* document)
    : HTMLElement(document)
    , m_submitter(nullptr)
{
    setAttribute(starFish()->staticStrings()->m_name, String::emptyString);
}

QualifiedName HTMLFormElement::name()
{
    return starFish()->staticStrings()->m_formTagName;
}

String* HTMLFormElement::domName()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_name);
}

void HTMLFormElement::setDomName(String* name)
{
    setAttribute(starFish()->staticStrings()->m_name, name);
}

String* HTMLFormElement::enctype()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_enctype);
}

void HTMLFormElement::setEnctype(String* enctype)
{
    setAttribute(starFish()->staticStrings()->m_enctype, enctype);
}

String* HTMLFormElement::method()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_method);
}

void HTMLFormElement::setMethod(String* method)
{
    setAttribute(starFish()->staticStrings()->m_method, method);
}

String* HTMLFormElement::target()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_target);
}

void HTMLFormElement::setTarget(String* target)
{
    setAttribute(starFish()->staticStrings()->m_target, target);
}

String* HTMLFormElement::action()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_action);
}

void HTMLFormElement::setAction(String* action)
{
    setAttribute(starFish()->staticStrings()->m_action, action);
}

void HTMLFormElement::setSubmitter(Element* elem)
{
    m_submitter = elem;
}

// https://www.w3.org/TR/html5/forms.html#concept-form-submit
void HTMLFormElement::submit()
{
    GCVector<FormDataSetItem*>* formDataSet = createFormDataSet();
    String* formAction = String::emptyString;
    HTMLInputElement* inputNode = nullptr;
    if (m_submitter && m_submitter->isHTMLInputElement()) {
        inputNode = m_submitter->asHTMLInputElement();
        if (inputNode->type()->equalsWithoutCase("button")) {
            formAction = inputNode->formAction();
        }
    }

    if (formAction->equals(String::emptyString)) {
        formAction = action();
    }

    ResourceURL* url;
    if (!formAction->equals(String::emptyString)) {
        if (ResourceURL::isValidURL(formAction)) {
            url = new ResourceURL(formAction);
        } else {
            url = new ResourceURL(formAction,
                                  document()->documentURI()->baseURI());
        }
    } else {
        url = document()->documentURI();
    }

    String* formEnctype = String::emptyString;
    String* formMethod = String::emptyString;
    String* formTarget = String::emptyString;
    if (inputNode) {
        formEnctype = inputNode->formEnctype();
        formMethod = inputNode->formMethod();
        formTarget = inputNode->formTarget();
    }

    if (formEnctype->equals(String::emptyString)) {
        formEnctype = enctype();
        if (formEnctype->equals(String::emptyString)) {
            formEnctype =
                String::createASCIIString("application/x-www-form-urlencoded");
        }
    }
    if (formMethod->equals(String::emptyString)) {
        formMethod = method();
    }
    if (formTarget->equals(String::emptyString)) {
        formTarget = target();
    }

    if (!formTarget->equals(String::emptyString)) {
        STARFISH_ASSERT_NOT_REACHED();
    }

    if (url->isNetworkURL() || url->isFileURL()) {
        if (formMethod->equalsWithoutCase("post")) {
            submitAsEntityBody(url, formEnctype, formDataSet);
        }
    }
}

void HTMLFormElement::submitAsEntityBody(
    ResourceURL* url, String* formEnctype,
    GCVector<FormDataSetItem*>* formDataSet)
{
    String* entityBody = encodeFormDataSet(formEnctype, formDataSet);
    FormResourceRequest* req = new FormResourceRequest(document());
    req->open(String::createASCIIString("post"), url->urlString());
    req->setRequestHeader(String::createASCIIString("content-type"),
                          formEnctype);
    req->setRequestHeader(String::createASCIIString("charset"),
                          String::createASCIIString("utf-8"));
    req->send(entityBody);
}

// https://www.w3.org/TR/html5/forms.html#application/
// x-www-form-urlencoded-encoding-algorithm
String* HTMLFormElement::encodeFormDataSet(
    String* formEnctype, GCVector<FormDataSetItem*>* formDataSet)
{
    String* space = String::spaceString;
    String* plus = String::createASCIIString("+");

    String* result = String::createASCIIString("");
    if (formEnctype->equalsWithoutCase("application/x-www-form-urlencoded")) {
        for (size_t i = 0; i < formDataSet->size(); i++) {
            FormDataSetItem* item = (*formDataSet)[i];
            String* name = item->m_name->replaceAll(space, plus);
            String* value = item->m_value->replaceAll(space, plus);
            String* type = item->m_type->replaceAll(space, plus);

            if (i == 0 && name->equalsWithoutCase("isindex") &&
                type->equalsWithoutCase("text")) {
                result = result->concat(value);
                continue;
            }

            if (name->equalsWithoutCase("_charset_") &&
                type->equalsWithoutCase("hidden")) {
                value = String::createASCIIString("UTF-8");
            }

            if (i > 0) {
                result = result->concat(String::createASCIIString("&"));
            }
            result = result->concat(name);
            result = result->concat(String::createASCIIString("="));
            result = result->concat(value);
        }
    } else if (formEnctype->equalsWithoutCase("multipart/form-data")) {
        // TODO
    } else if (formEnctype->equalsWithoutCase("text/plain")) {
        // TODO
    }

    return result;
}

// https://www.w3.org/TR/html5/forms.html#constructing-the-form-data-set
GCVector<FormDataSetItem*>* HTMLFormElement::createFormDataSet()
{
    GCVector<Element*> inputNodes;
    Traverse::collectDescendants(
        inputNodes, asNode(),
        [this](Node* node) -> bool {
            // TODO: "Checkness" is not supported yet.
            // Collecting inputboxes only now
            if (node->isHTMLInputElement()) {
                HTMLInputElement* inputNode = node->asHTMLInputElement();
                if (inputNode->type()->equalsWithoutCase("button") &&
                    (inputNode != m_submitter)) {
                    return false;
                }
                return true;
            }
            return false;
        },
        false);

    GCVector<FormDataSetItem*>* formDataSet = new GCVector<FormDataSetItem*>();
    for (Element* node : inputNodes) {
        if (node->isHTMLInputElement()) {
            HTMLInputElement* inputNode = node->asHTMLInputElement();
            String* val = inputNode->value();

            if (inputNode->type()->equalsWithoutCase("checkbox") ||
                inputNode->type()->equalsWithoutCase("radio")) {
                if (inputNode->value() == String::emptyString) {
                    val = String::createASCIIString("on");
                }
            }

            formDataSet->push_back(new FormDataSetItem(inputNode->domName(),
                                                       val, inputNode->type()));
        }
    }

    return formDataSet;
}
}
