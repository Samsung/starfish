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
#include "core/dom/builder/html/HTMLDocumentBuilder.h"
#include "core/dom/HTMLFormControlsCollection.h"
#include "core/dom/HTMLFieldSetElement.h"
#include "core/dom/HTMLLegendElement.h"
#include "core/dom/Node.h"
#include "core/dom/Traverse.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Location.h"
#include "core/page/Window.h"

namespace StarFish {

FormDataSetItem::FormDataSetItem(String* name, String* value, String* type)
    : m_name(name)
    , m_value(value)
    , m_type(type)
{
}

FormSubmitData::FormSubmitData(GCVector<FormDataSetItem*>* formDataSet,
                               ResourceRequest::EncodeType enctype,
                               ResourceRequest::MethodType method)
    : m_formDataSet(formDataSet)
    , m_enctype(enctype)
    , m_method(method)
{
}

HTMLFormElement::HTMLFormElement(Document* document)
    : HTMLFormObject(document)
    , m_elements(nullptr)
    , m_plannedNavigationTaskId((size_t)-1)
{
    setAttribute(starFish()->staticStrings()->m_name, String::emptyString);
}

HTMLFormObject::HTMLFormObject(Document* document)
    : HTMLElement(document)
    , m_disabled(false)
{
}

String* HTMLFormObject::domName()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_name);
}

void HTMLFormObject::setDomName(String* name)
{
    setAttribute(starFish()->staticStrings()->m_name, name);
}

String* HTMLFormObject::type() const
{
    String* typeAttr = getAttributeOrEmpty(starFish()->staticStrings()->m_type);
    typeAttr = typeAttr->toLower();

    if (typeAttr->equals("text")) {
        return typeAttr;
    } else if (typeAttr->equals("email")) {
        return typeAttr;
    } else if (typeAttr->equals("number")) {
        return typeAttr;
    } else if (typeAttr->equals("password")) {
        return typeAttr;
    } else if (typeAttr->equals("url")) {
        return typeAttr;
    } else if (typeAttr->equals("tel")) {
        return typeAttr;
    } else if (typeAttr->equals("search")) {
        return typeAttr;
    } else if (typeAttr->equals("checkbox")) {
        return typeAttr;
    } else if (typeAttr->equals("button")) {
        return typeAttr;
    } else if (typeAttr->equals("radio")) {
        return typeAttr;
    } else if (typeAttr->equals("submit")) {
        return typeAttr;
    }

    return starFish()->staticStrings()->m_text.localName();
}

void HTMLFormObject::setType(String* type)
{
    setAttribute(starFish()->staticStrings()->m_type, type);
}

bool HTMLFormObject::disabled()
{
    if (m_disabled) {
        return true;
    }

    if (!isHTMLFormElement()) {
        HTMLFieldSetElement* fieldSetNode = fieldSet();
        if (fieldSetNode && fieldSetNode->disabled()) {
            Node* firstChild = fieldSetNode->firstChild();
            if (firstChild && firstChild->isHTMLLegendElement() &&
                findAncestor(firstChild, this)) {
                return false;
            }
            return true;
        }
    }

    return false;
}

Node* HTMLFormObject::findAncestor(Node* ancestorToFind, Node* fromThisNode)
{
    for (Node* p = fromThisNode->parentNode(); p; p = p->parentNode()) {
        if (p == nullptr) {
            break;
        } else if (p->isHTMLIFrameElement()) {
            return nullptr;
        } else if (p == ancestorToFind) {
            return p;
        }
    }
    return nullptr;
}

void HTMLFormObject::setDisabled(bool disabled)
{
    m_disabled = disabled;
}

void HTMLFormObject::fireSubmitEvent()
{
    auto fn = [](size_t handle, void* data) {
        Node* node = (Node*)data;
        String* eventType =
            node->starFish()->staticStrings()->m_submit.localName();
        Event* e =
            new Event(node->document(), eventType, EventInit(true, true));
        node->EventTarget::dispatchEvent(node, e);
    };
    starFish()->messageLoop()->addIdler(document()->browsingContext(), fn,
                                        this);
}

HTMLFormElement* HTMLFormObject::form()
{
    for (Node* p = parentNode(); p; p = p->parentNode()) {
        if (p == nullptr) {
            break;
        } else if (p->isHTMLIFrameElement()) {
            return nullptr;
        } else if (p->isHTMLFormElement()) {
            return p->asHTMLFormElement();
        }
    }
    return nullptr;
}

HTMLFieldSetElement* HTMLFormObject::fieldSet()
{
    for (Node* p = parentNode(); p; p = p->parentNode()) {
        if (p == nullptr) {
            break;
        } else if (p->isHTMLIFrameElement()) {
            return nullptr;
        } else if (p->isHTMLFieldSetElement()) {
            return p->asHTMLFieldSetElement();
        }
    }
    return nullptr;
}

void* HTMLFormElement::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLFormElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLFormElement, m_elements));
        HTMLElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLFormElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName HTMLFormElement::name()
{
    return starFish()->staticStrings()->m_formTagName;
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

bool HTMLFormElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }

    if (event->type() == starFish()->staticStrings()->m_submit.localName()) {
        STARFISH_ASSERT(event->target()->isHTMLElement());
        submit(event->target()->asHTMLElement());
        return true;
    }

    return false;
}

void HTMLFormElement::submit()
{
    return submit(nullptr);
}

// https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#concept-form-submit
void HTMLFormElement::submit(HTMLElement* submitter)
{
    GCVector<FormDataSetItem*>* formDataSet = createFormDataSet(submitter);
    String* formAction = String::emptyString;
    HTMLInputElement* inputNode = nullptr;
    if (submitter && submitter->isHTMLInputElement()) {
        inputNode = submitter->asHTMLInputElement();
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

    ResourceRequest::EncodeType formEnctype =
        ResourceRequest::MISSING_OR_INVALID_ENCODETYPE;
    ResourceRequest::MethodType formMethod = ResourceRequest::UNKNOWN_METHOD;
    String* formTarget = String::emptyString;

    if (inputNode) {
        formEnctype = ResourceRequest::toEncodeType(inputNode->formEnctype());
        formMethod = ResourceRequest::toMethodType(inputNode->formMethod());
        formTarget = inputNode->formTarget();
    }

    if (formEnctype == ResourceRequest::MISSING_OR_INVALID_ENCODETYPE) {
        formEnctype = ResourceRequest::toEncodeType(enctype());
        if (formEnctype == ResourceRequest::MISSING_OR_INVALID_ENCODETYPE) {
            formEnctype = ResourceRequest::APPLICATION_X_WWW_FORM_URLENCODED;
        }
    }
    if (formMethod == ResourceRequest::UNKNOWN_METHOD) {
        formMethod = ResourceRequest::toMethodType(method());
        if (formMethod == ResourceRequest::UNKNOWN_METHOD) {
            formMethod = ResourceRequest::GET_METHOD;
        }
    }
    if (formTarget->equals(String::emptyString)) {
        formTarget = target();
    }

    if (!formTarget->equals(String::emptyString)) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }

    if (url->isNetworkURL() || url->isFileURL()) {
        submitData(url, formDataSet, formEnctype, formMethod);
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

void HTMLFormElement::submitData(ResourceURL* url,
                                 GCVector<FormDataSetItem*>* formDataSet,
                                 ResourceRequest::EncodeType enctype,
                                 ResourceRequest::MethodType method)
{
    if (m_plannedNavigationTaskId != (size_t)-1) {
        starFish()->messageLoop()->removeIdler(m_plannedNavigationTaskId);
    }

    DocumentURL* urlToOpen =
        new DocumentURL(url, new FormSubmitData(formDataSet, enctype, method));
    auto fn = [](size_t handle, void* data1, void* data2) {
        HTMLFormElement* formElement = (HTMLFormElement*)data1;
        DocumentURL* urlToOpen = (DocumentURL*)data2;
        formElement->document()->window()->location()->assign(urlToOpen);
        formElement->clearPlannedNavigationTask();
    };

    m_plannedNavigationTaskId = starFish()->messageLoop()->addIdler(
        document()->browsingContext(), fn, this, urlToOpen);
}

void HTMLFormElement::clearPlannedNavigationTask()
{
    m_plannedNavigationTaskId = (size_t)-1;
}

HTMLFormControlsCollection* HTMLFormElement::elements()
{
    if (m_elements) {
        return m_elements;
    }
    m_elements =
        new HTMLFormControlsCollection(this, NodeListImpl::FormElementsFiliter);
    return m_elements;
}

uint32_t HTMLFormElement::length()
{
    if (!m_elements) {
        elements();
    }
    return m_elements->length();
}

Element* HTMLFormElement::defaultIndexedGetter(uint32_t idx)
{
    if (!m_elements) {
        elements();
    }
    return m_elements->item(idx);
}

Element* HTMLFormElement::defaultNamedGetter(String* name)
{
    if (!m_elements) {
        elements();
    }
    return m_elements->namedItem(name);
}

// https://www.w3.org/TR/html5/forms.html#constructing-the-form-data-set
GCVector<FormDataSetItem*>* HTMLFormElement::createFormDataSet(
    HTMLElement* submitter)
{
    GCVector<Element*> inputNodes;
    Traverse::collectDescendants(
        inputNodes, asNode(),
        [this, submitter](Node* node) -> bool {
            // TODO: datalist, img button, and object is not supported
            if (node->isHTMLInputElement()) {
                HTMLInputElement* inputNode = node->asHTMLInputElement();

                if (inputNode->disabled()) {
                    return false;
                }
                if (inputNode->type()->equalsWithoutCase("button") &&
                    (inputNode != submitter)) {
                    return false;
                }
                if (inputNode->type()->equalsWithoutCase("checkbox") &&
                    (!inputNode->checked())) {
                    return false;
                }
                if (inputNode->type()->equalsWithoutCase("radio") &&
                    (!inputNode->checked())) {
                    return false;
                }

                return true;
            }
            return false;
        },
        false);

    GCVector<FormDataSetItem*>* formDataSet =
        new (GC) GCVector<FormDataSetItem*>();
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
