/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/dom/HTMLFormElement.h"

#include "core/dom/Document.h"
#include "core/dom/DOMException.h"
#include "core/dom/Event.h"
#include "core/fetch/FetchUtils.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLButtonElement.h"
#include "core/dom/HTMLSelectElement.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/builder/html/HTMLDocumentBuilder.h"
#include "core/dom/HTMLFormControlsCollection.h"
#include "core/dom/HTMLFieldSetElement.h"
#include "core/dom/HTMLLabelElement.h"
#include "core/dom/HTMLLegendElement.h"
#include "core/dom/HTMLBaseElement.h"
#include "core/dom/Node.h"
#include "platform/loader/ResourceURL.h"
#include "core/dom/Traverse.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Location.h"
#include "core/page/WebView.h"
#include "core/page/Window.h"
#include "core/page/History.h"
#include "browser/history/HistoryManager.h"
#include "core/modules/resource_request/ResourceRequest.h"

namespace Starfish {

FormDataSetItem::FormDataSetItem(String* name, String* value, String* type)
    : m_name(name)
    , m_value(value)
    , m_type(type)
{
}

void* FormDataSetItem::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FormDataSetItem));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FormDataSetItem)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(FormDataSetItem, m_name));
        GC_set_bit(desc, GC_WORD_OFFSET(FormDataSetItem, m_value));
        GC_set_bit(desc, GC_WORD_OFFSET(FormDataSetItem, m_type));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FormDataSetItem));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* FormDataSetItem::toString()
{
    StringBuilder b;
    b.appendString(m_name);
    b.appendChar('=');
    b.appendString(m_value);
    return b.finalize();
}

FormSubmitData::FormSubmitData(GCVector<FormDataSetItem*>* formDataSet,
                               EncodeType enctype, String* method)
    : m_formDataSet(formDataSet)
    , m_enctype(enctype)
    , m_method(method)
{
}

void* FormSubmitData::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FormSubmitData));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FormSubmitData)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(FormSubmitData, m_formDataSet));
        GC_set_bit(desc, GC_WORD_OFFSET(FormSubmitData, m_method));
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FormSubmitData));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* FormSubmitData::toString()
{
    StringBuilder b;
    for (size_t i = 0; i < m_formDataSet->size(); i++) {
        FormDataSetItem* item = (*m_formDataSet)[i];
        b.appendString(item->toString());
        if (i < m_formDataSet->size() - 1) {
            b.appendChar('&');
        }
    }

    return b.finalize();
}

HTMLFormElement::HTMLFormElement(Document* document, const QualifiedName& qname)
    : HTMLFormControl(document, qname, false)
    , m_elements(nullptr)
    , m_plannedNavigationTaskId((size_t)-1)
    , m_isLockedForReset(false)
{
    setAttribute(starfish()->staticStrings()->m_name, String::emptyString);
}

HTMLFormControl::HTMLFormControl(Document* document, const QualifiedName& qname,
                                 bool supportTabIndex)
    : HTMLElement(document, qname)
    , m_value(String::emptyString)
    , m_supportTabIndex(supportTabIndex)
    , m_labels(nullptr)
{
    if (m_supportTabIndex) {
        m_tabIndexWasSetExplicitly = true;
        m_tabIndex = 0;
    }
}

String* HTMLFormControl::domName()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_name);
}

void HTMLFormControl::setDomName(String* name)
{
    setAttribute(starfish()->staticStrings()->m_name, name);
}

String* HTMLFormControl::type()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_type);
}

void HTMLFormControl::setType(String* type)
{
    setAttribute(starfish()->staticStrings()->m_type, type);
}

String* HTMLFormControl::value()
{
    return m_value;
}

void HTMLFormControl::setValue(String* value)
{
    m_value = value;
    setNeedsFrameTreeBuildWithoutSelf();
}

String* HTMLFormControl::formEnctype()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_formEnctype);
}

void HTMLFormControl::setFormEnctype(String* enctype)
{
    setAttribute(starfish()->staticStrings()->m_formEnctype, enctype);
}

String* HTMLFormControl::formMethod()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_formMethod);
}

void HTMLFormControl::setFormMethod(String* method)
{
    setAttribute(starfish()->staticStrings()->m_formMethod, method);
}

String* HTMLFormControl::formTarget()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_formTarget);
}

void HTMLFormControl::setFormTarget(String* target)
{
    setAttribute(starfish()->staticStrings()->m_formTarget, target);
}

String* HTMLFormControl::formAction()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_formAction);
}

void HTMLFormControl::setFormAction(String* formAction)
{
    setAttribute(starfish()->staticStrings()->m_formAction, formAction);
}

bool HTMLFormControl::required()
{
    Nullable<String*> val =
        getAttribute(starfish()->staticStrings()->m_required);
    return val.hasValue();
}

void HTMLFormControl::setRequired(bool required)
{
    if (required) {
        setAttribute(starfish()->staticStrings()->m_required,
                     String::emptyString);
    } else {
        removeAttribute(starfish()->staticStrings()->m_required);
    }
}

bool HTMLFormControl::multiple()
{
    Nullable<String*> val =
        getAttribute(starfish()->staticStrings()->m_multiple);
    return val.hasValue();
}

void HTMLFormControl::setMultiple(bool multiple)
{
    if (multiple) {
        setAttribute(starfish()->staticStrings()->m_multiple,
                     String::emptyString);
    } else {
        removeAttribute(starfish()->staticStrings()->m_multiple);
    }
}

bool HTMLFormControl::disabled()
{
    Nullable<String*> val =
        getAttribute(starfish()->staticStrings()->m_disabled);
    if (val.hasValue()) {
        return val.getValue();
    }

    return false;
}

void HTMLFormControl::setDisabled(bool disabled)
{
    if (disabled) {
        setAttribute(starfish()->staticStrings()->m_disabled,
                     String::emptyString);
    } else {
        removeAttribute(starfish()->staticStrings()->m_disabled);
    }
}

// https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#attr-fe-disabled
bool HTMLFormControl::isDisabled()
{
    if (disabled() && (isHTMLButtonElement() || isHTMLInputElement() ||
                       isHTMLSelectElement() || isHTMLTextAreaElement())) {
        return true;
    }

    HTMLFieldSetElement* fieldSet = nullptr;
    for (Node* p = parentNode(); p; p = p->parentNode()) {
        if (p->isHTMLIFrameElement() || p->isHTMLFormElement()) {
            break;
        } else if (p->isHTMLFieldSetElement()) {
            if (p->asHTMLFieldSetElement()->disabled()) {
                HTMLLabelElement* firstLabel = nullptr;
                for (Node* c = p->firstChild(); c; c = c->nextSibling()) {
                    if (c->isHTMLLabelElement()) {
                        firstLabel = c->asHTMLLabelElement();
                        break;
                    }
                }

                if (!firstLabel) {
                    return true;
                }
                if (!findAncestor(firstLabel, this)) {
                    return true;
                }
            }
        }
    }

    return false;
}

Node* HTMLFormControl::findAncestor(Node* ancestorToFind, Node* fromThisNode)
{
    for (Node* p = fromThisNode->parentNode(); p; p = p->parentNode()) {
        if (p->isHTMLIFrameElement() || p->isHTMLFormElement()) {
            return nullptr;
        } else if (p == ancestorToFind) {
            return p;
        }
    }
    return nullptr;
}

void HTMLFormControl::fireSubmitEvent()
{
    auto fn = [](size_t handle, void* data) {
        Node* node = (Node*)data;
        String* eventType =
            node->starfish()->staticStrings()->m_submit.localName();
        Event* e =
            new Event(node->document(), eventType, EventInit(true, true));
        node->EventTarget::dispatchEventByUA(node, e);
    };
    webView()->messageLoop()->addIdler(document()->browsingContext(), fn, this);
}

void HTMLFormControl::didAttributeChanged(QualifiedName name, String* old,
                                          String* val, bool attributeCreated,
                                          bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, val, attributeCreated,
                                     attributeRemoved);

    if (name == starfish()->staticStrings()->m_disabled) {
        document()->invalidFocusRingCacheIfNeeded();
    } else if (m_supportTabIndex &&
               name == starfish()->staticStrings()->m_tabindex) {
        m_tabIndexWasSetExplicitly = true;
        if (m_tabIndex == -1)
            m_tabIndex = 0;
    }
}

bool HTMLFormControl::isAutofocusable()
{
    return autofocus() && supportsFocus();
}

bool HTMLFormControl::autofocus()
{
    return hasAttribute(starfish()->staticStrings()->m_autofocus) != SIZE_MAX;
}

void HTMLFormControl::setAutofocus(bool autofocus)
{
    if (autofocus) {
        setAttribute(starfish()->staticStrings()->m_autofocus,
                     String::emptyString);
    } else {
        removeAttribute(starfish()->staticStrings()->m_autofocus);
    }
}

void HTMLFormControl::didNodeInsertedToDocumentTree()
{
    HTMLElement::didNodeInsertedToDocumentTree();

    // https://www.w3.org/TR/html5/editing.html#focusing-steps
    if (isAutofocusable()) {
        webView()->messageLoop()->addIdler(
            document()->browsingContext(),
            [](size_t handle, void* data) {
                HTMLFormControl* element = (HTMLFormControl*)data;
                if (!element->document()->browsingContext()->focusedNode()) {
                    element->document()->browsingContext()->setActiveNode(
                        element);
                    element->document()->browsingContext()->setFocusedNode(
                        element, false);
                }
            },
            this);
    }
}

bool HTMLFormControl::isPlaceholderVisible()
{
    return false;
}

bool HTMLFormControl::isLabelable() const
{
    if (isHTMLInputElement()) {
        String* type = getAttributeOrEmpty(starfish()->staticStrings()->m_type);
        if (!type->equals("hidden")) {
            return true;
        }
    } else if (isHTMLButtonElement() || isHTMLSelectElement() ||
               isHTMLTextAreaElement() || isHTMLOutputElement()) {
        return true;
    }

    return false;
}

bool HTMLFormControl::isButton(HTMLFormControl* node)
{
    if (node->type()->equals("submit") || node->type()->equals("button") ||
        node->type()->equals("reset") || node->type()->equals("image")) {
        return true;
    }

    return false;
}

HTMLFormElement* HTMLFormControl::form()
{
    return formOwner();
}

HTMLFormElement* HTMLFormControl::formOwner()
{
    for (Node* p = parentNode(); p; p = p->parentNode()) {
        if (p->isHTMLIFrameElement()) {
            return nullptr;
        } else if (p->isHTMLFormElement()) {
            return p->asHTMLFormElement();
        }
    }
    return nullptr;
}

bool HTMLFormControl::supportsFocus()
{
    return false;
}

int32_t HTMLFormControl::maxLength()
{
    int32_t result = 0;
    Nullable<String*> maxLengthStr =
        getAttribute(starfish()->staticStrings()->m_maxlength);

    if (!maxLengthStr.hasValue()) {
        result = -1;
    } else {
        result = String::parseInt(maxLengthStr.getValue());
        if (result < 0) {
            result = -1;
        }
    }
    return result;
}

void HTMLFormControl::setMaxLength(int32_t maxlength)
{
    int32_t minlength = minLength();
    if (maxlength < 0) {
        COMPOSE_MESSAGE(reason, NOT_POSITIVE,
                        String::fromInt(maxlength)->toUTF8NonGCString().data());
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, "maxLength",
                        "HTMLFormControl", reason);
        throw new DOMException(document(), DOMException::DOM_EXCEPTION, msg);
    } else if (minlength >= 0 && maxlength < minlength) {
        COMPOSE_MESSAGE(reason, EXCEED_MIN_BOUNDARY,
                        String::fromInt(maxlength)->toUTF8NonGCString().data(),
                        String::fromInt(minlength)->toUTF8NonGCString().data());
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, "maxLength",
                        "HTMLFormControl", reason);
        throw new DOMException(document(), DOMException::DOM_EXCEPTION, msg);
    } else {
        setAttribute(starfish()->staticStrings()->m_maxlength,
                     String::fromInt(maxlength));
    }
}

int32_t HTMLFormControl::minLength()
{
    int32_t result = 0;
    Nullable<String*> minLengthStr =
        getAttribute(starfish()->staticStrings()->m_minlength);

    if (!minLengthStr.hasValue()) {
        result = -1;
    } else {
        result = String::parseInt(minLengthStr.getValue());
        if (result < 0) {
            result = -1;
        }
    }
    return result;
}

void HTMLFormControl::setMinLength(int32_t minlength)
{
    int32_t maxlength = maxLength();
    if (minlength < 0) {
        COMPOSE_MESSAGE(reason, NOT_POSITIVE,
                        String::fromInt(minlength)->toUTF8NonGCString().data());
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, "minLength",
                        "HTMLFormControl", reason);
        throw new DOMException(document(), DOMException::DOM_EXCEPTION, msg);
    } else if (maxlength >= 0 && maxlength < minlength) {
        COMPOSE_MESSAGE(reason, EXCEED_MAX_BOUNDARY,
                        String::fromInt(minlength)->toUTF8NonGCString().data(),
                        String::fromInt(maxlength)->toUTF8NonGCString().data());
        COMPOSE_MESSAGE(msg, FAILED_TO_SET_PROPERTY, "minLength",
                        "HTMLFormControl", reason);
        throw new DOMException(document(), DOMException::DOM_EXCEPTION, msg);
    } else {
        setAttribute(starfish()->staticStrings()->m_minlength,
                     String::fromInt(minlength));
    }
}

NodeList* HTMLFormControl::labels()
{
    if (!isLabelable()) {
        return nullptr;
    }

    if (!m_labels) {
        m_labels =
            new NodeList(document(), NodeListImpl::AssociatedLabelElementFilter,
                         this, false);
    }

    return m_labels;
}

void* HTMLFormElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(HTMLFormElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(HTMLFormElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLFormElement, m_elements));
        HTMLFormControl::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(HTMLFormElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

String* HTMLFormElement::enctype()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_enctype);
}

void HTMLFormElement::setEnctype(String* enctype)
{
    setAttribute(starfish()->staticStrings()->m_enctype, enctype);
}

String* HTMLFormElement::encoding()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_enctype);
}

void HTMLFormElement::setEncoding(String* encoding)
{
    setEnctype(encoding);
}

String* HTMLFormElement::method()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_method);
}

void HTMLFormElement::setMethod(String* method)
{
    setAttribute(starfish()->staticStrings()->m_method, method);
}

String* HTMLFormElement::target()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_target);
}

void HTMLFormElement::setTarget(String* target)
{
    setAttribute(starfish()->staticStrings()->m_target, target);
}

String* HTMLFormElement::action()
{
    String* action = getAttributeOrEmpty(starfish()->staticStrings()->m_action);
    if (action->equals(String::emptyString)) {
        action = document()->documentURI()->urlString();
    }
    return action;
}

void HTMLFormElement::setAction(String* action)
{
    setAttribute(starfish()->staticStrings()->m_action, action);
}

// https://html.spec.whatwg.org/multipage/webappapis.html#user-interaction-task-source
void HTMLFormControl::queueEvent(QualifiedName& eventType, bool bubbles,
                                 bool cancelable)
{
    Event* e = new Event(document(), eventType.localName(),
                         EventInit(bubbles, cancelable));

    auto fn = [](size_t handle, void* data, void* data1) {
        EventTarget* element = (EventTarget*)data;
        Event* e = (Event*)data1;
        element->EventTarget::dispatchEventByUA(element, e);
    };
    webView()->messageLoop()->addIdler(document()->browsingContext(), fn, this,
                                       e);
}

void HTMLFormControl::fireEvent(QualifiedName& eventType, bool bubbles,
                                bool cancelable)
{
    Event* e = new Event(document(), eventType.localName(),
                         EventInit(bubbles, cancelable));
    EventTarget::dispatchEventByUA(this, e);
}

bool HTMLFormElement::handleDefaultEvent(Event* event)
{
    if (HTMLElement::handleDefaultEvent(event)) {
        return true;
    }

    if (event->type() == starfish()->staticStrings()->m_submit.localName()) {
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

void HTMLFormElement::reset()
{
    if (m_isLockedForReset) {
        return;
    }
    m_isLockedForReset = true;

    String* eventType = starfish()->staticStrings()->m_reset.localName();
    Event* e = new Event(document(), eventType, EventInit(true, true));

    if (dispatchEventByUA(this, e, true)) {
        auto elms = elements();
        for (size_t i = 0; i < elms->length(); ++i) {
            auto elm = elms->item(i);
            if (elm->isHTMLFormControl() &&
                elm->asHTMLFormControl()->isResettableElement()) {
                elm->asHTMLFormControl()->reset();
            }
        }
    }
    m_isLockedForReset = false;
}

// https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#concept-form-submit
void HTMLFormElement::submit(HTMLElement* submitter)
{
    GCVector<FormDataSetItem*>* formDataSet = createFormDataSet(submitter);
    String* formAction = String::emptyString;
    HTMLFormControl* inputNode = nullptr;
    if (submitter &&
        (submitter->isHTMLInputElement() || submitter->isHTMLButtonElement())) {
        inputNode = submitter->asHTMLFormControl();
        if (inputNode->type()->equals("submit") ||
            inputNode->type()->equals("image")) {
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
            url = new ResourceURL(formAction, document()->baseURL()->baseURI());
        }
    } else {
        url = document()->documentURI();
    }

    EncodeType formEnctype = EncodeType::MissingOrInvalidEncodeType;
    String* formMethod = String::emptyString;
    String* formTarget = String::emptyString;

    if (inputNode) {
        formEnctype = ResourceRequest::toEncodeType(inputNode->formEnctype());
        formMethod = inputNode->formMethod();
        formTarget = inputNode->formTarget();
    }

    if (formEnctype == EncodeType::MissingOrInvalidEncodeType) {
        formEnctype = ResourceRequest::toEncodeType(enctype());
        if (formEnctype == EncodeType::MissingOrInvalidEncodeType) {
            formEnctype = EncodeType::ApplicationXWWWFormURLEncoded;
        }
    }
    if (formMethod == String::emptyString) {
        formMethod = method();
    }
    formMethod = FetchUtils::normalizeMethod(formMethod);

    if (formMethod == String::emptyString) {
        formMethod = String::createASCIIString("GET");
    }

    if (formTarget->equals(String::emptyString)) {
        formTarget = target();
        if (formTarget->equals(String::emptyString)) {
            Node* baseElem =
                document()->childMatchedBy(document(), [](Node* node) {
                    if (node->isHTMLBaseElement() &&
                        !node->asHTMLBaseElement()->target()->equals(
                            String::emptyString)) {
                        return true;
                    }
                    return false;
                });

            if (baseElem) {
                formTarget = baseElem->asHTMLBaseElement()->target();
            }
        }
    }

    if (url->isHTTPFamilyURL() || url->isFileURL()) {
        submitData(url, formDataSet, formEnctype, formMethod);
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

void HTMLFormElement::submitData(ResourceURL* url,
                                 GCVector<FormDataSetItem*>* formDataSet,
                                 EncodeType enctype, String* method)
{
    if (method->equals("GET")) {
        mutateActionUrl(url, formDataSet, enctype, method);
    } else if (method->equals("POST")) {
        submitAsEntityBody(url, formDataSet, enctype, method);
    } else {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    }
}

void HTMLFormElement::mutateActionUrl(ResourceURL* url,
                                      GCVector<FormDataSetItem*>* formDataSet,
                                      EncodeType enctype, String* method)
{
    if (m_plannedNavigationTaskId != (size_t)-1) {
        webView()->messageLoop()->removeIdler(m_plannedNavigationTaskId);
    }

    FormSubmitData* dataToSubmit =
        new FormSubmitData(formDataSet, enctype, method);
    String* urlStr =
        url->urlString()->concat("?")->concat(dataToSubmit->toString());

    DocumentURL* urlToOpen = new DocumentURL(urlStr, dataToSubmit);
    auto fn = [](size_t handle, void* data1, void* data2) {
        HTMLFormElement* formElement = (HTMLFormElement*)data1;
        DocumentURL* urlToOpen = (DocumentURL*)data2;
        formElement->document()->window()->location()->assign(
            urlToOpen, formElement->document()->documentURI());
        formElement->clearPlannedNavigationTask();
    };

    m_plannedNavigationTaskId = webView()->messageLoop()->addIdler(
        document()->browsingContext(), fn, this, urlToOpen);
}

void HTMLFormElement::submitAsEntityBody(
    ResourceURL* url, GCVector<FormDataSetItem*>* formDataSet,
    EncodeType enctype, String* method)
{
    if (enctype == EncodeType::ApplicationXWWWFormURLEncoded) {
        if (m_plannedNavigationTaskId != (size_t)-1) {
            webView()->messageLoop()->removeIdler(m_plannedNavigationTaskId);
        }

        FormSubmitData* dataToSubmit =
            new FormSubmitData(formDataSet, enctype, method);

        DocumentURL* urlToOpen =
            new DocumentURL(url->urlString(), dataToSubmit);
        auto fn = [](size_t handle, void* data1, void* data2) {
            HTMLFormElement* formElement = (HTMLFormElement*)data1;
            DocumentURL* urlToOpen = (DocumentURL*)data2;
            // force open as the url does not change in method="post"
            formElement->document()->window()->location()->assign(
                urlToOpen,
                new ReferrerURL(formElement->document()->documentURI()), true);
            formElement->clearPlannedNavigationTask();
        };

        m_plannedNavigationTaskId = webView()->messageLoop()->addIdler(
            document()->browsingContext(), fn, this, urlToOpen);
    } else if (enctype == EncodeType::MultiPartFormData) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    } else if (enctype == EncodeType::TextPlain) {
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    } else {
        // Do nothing for an invalid enctype
    }
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

// https://html.spec.whatwg.org/multipage/form-control-infrastructure.html#constructing-form-data-set
GCVector<FormDataSetItem*>* HTMLFormElement::createFormDataSet(
    HTMLElement* submitter)
{
    GCVector<FormDataSetItem*>* formDataSet =
        new (GC) GCVector<FormDataSetItem*>();

    // 1-3
    GCVector<HTMLFormControl*> list;
    computeFormSubmittableElements(this, list);
    for (HTMLFormControl* field : list) {
        // TODO: HTML object is not supported
        // 3.1
        // datalist is not supported
        if (field->disabled()) {
            continue;
        } else if ((field != submitter) && isButton(field)) {
            continue;
        } else if (field->isHTMLInputElement()) {
            if (field->type()->equals("checkbox") &&
                (!field->asHTMLInputElement()->checked())) {
                continue;
            } else if (field->type()->equals("radio") &&
                       (!field->asHTMLInputElement()->checked())) {
                continue;
            }
        } else if (!field->isHTMLInputElement() &&
                   field->type()->equals("image") &&
                   (field->domName()->isEmpty())) {
            continue;
        }

        // 3.2, 3.3
        if (field->isHTMLInputElement() && field->type()->equals("image")) {
            // TODO
            continue;
        }

        // 3.4-3.10
        if (field->isHTMLSelectElement()) {
            GCVector<HTMLOptionElement*> optionElements;
            field->asHTMLSelectElement()->computeListOfOptionElements(
                field, optionElements);
            for (HTMLOptionElement* opt : optionElements) {
                if (!opt->disabled() && opt->selected()) {
                    formDataSet->push_back(new FormDataSetItem(
                        opt->domName(), opt->value(), opt->type()));
                }
            }
        } else if (field->isHTMLInputElement() &&
                   (field->type()->equals("checkbox") ||
                    field->type()->equals("radio"))) {
            HTMLInputElement* inputNode = field->asHTMLInputElement();
            String* val = inputNode->value()->trim();
            if (val->equals(String::emptyString)) {
                val = String::createASCIIString("on");
            }
            formDataSet->push_back(new FormDataSetItem(inputNode->domName(),
                                                       val, inputNode->type()));
        } else if (field->isHTMLInputElement() &&
                   field->type()->equals("file")) {
            // TODO: file upload, object
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        } else {
            formDataSet->push_back(new FormDataSetItem(
                field->domName(), field->value(), field->type()));
        }
    }

    return formDataSet;
}

void HTMLFormElement::computeFormSubmittableElements(
    Node* parent, GCVector<HTMLFormControl*>& list)
{
    for (Node* c = parent->firstChild(); c; c = c->nextSibling()) {
        if (c->isHTMLIFrameElement() || c->isHTMLFormElement()) {
            continue;
        }

        if (isSubmittableElement(c) &&
            (c->asHTMLFormControl()->form() == this)) {
            list.push_back(c->asHTMLFormControl());
        } else {
            computeFormSubmittableElements(c, list);
        }
    }
}

bool HTMLFormElement::isFormAssociatedElement(Node* node)
{
    if (node->isHTMLButtonElement() || node->isHTMLFieldSetElement() ||
        node->isHTMLInputElement() || node->isHTMLObjectElement() ||
        /*node->isHTMLOutputElement() ||*/ node->isHTMLSelectElement() ||
        node->isHTMLTextAreaElement() || node->isHTMLImageElement()) {
        return true;
    }

    return false;
}

bool HTMLFormElement::isSubmittableElement(Node* node)
{
    if (node->isHTMLButtonElement() || node->isHTMLInputElement() ||
        node->isHTMLSelectElement() || node->isHTMLTextAreaElement()) {
        return true;
    }
    return false;
}
}
