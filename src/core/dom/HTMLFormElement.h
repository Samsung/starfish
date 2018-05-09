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

#ifndef __StarFishHTMLFormElement__
#define __StarFishHTMLFormElement__

#include "core/dom/HTMLElement.h"
#include "core/modules/resource_request/ResourceRequest.h"

namespace StarFish {

class DocumentBuilder;
class HTMLFieldSetElement;
class HTMLSelectElement;
class HTMLFormControlsCollection;
class ResourceURL;

class FormDataSetItem : public gc {
public:
    FormDataSetItem(String* name, String* value, String* type);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    String* toString();

    String* m_name;
    String* m_value;
    String* m_type;
};

class FormSubmitData : public gc {
public:
    FormSubmitData(GCVector<FormDataSetItem*>* formDataSet,
                   ResourceRequest::EncodeType enctype,
                   ResourceRequest::MethodType method);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    String* toString();

    GCVector<FormDataSetItem*>* m_formDataSet;
    ResourceRequest::EncodeType m_enctype;
    ResourceRequest::MethodType m_method;
};

class HTMLFormControl : public HTMLElement {
public:
    // Common method for Form-related nodes
    virtual String* domName();
    virtual void setDomName(String* name);

    String* formEnctype();
    void setFormEnctype(String* enctype);

    String* formMethod();
    void setFormMethod(String* method);

    String* formTarget();
    void setFormTarget(String* target);

    String* formAction();
    void setFormAction(String* formAction);

    virtual String* type();
    virtual void setType(String* type);
    virtual bool required();
    virtual void setRequired(bool required);
    virtual bool multiple();
    virtual void setMultiple(bool multiple);

    virtual String* value();
    virtual void setValue(String* value);

    virtual bool disabled() override;
    virtual void setDisabled(bool disabled);

    virtual HTMLFormElement* form();

    // Other method
    virtual bool supportsFocus() override;

    virtual bool isHTMLFormControl() const override
    {
        return true;
    }

    virtual void reset()
    {
    }

    int32_t maxLength();
    void setMaxLength(int32_t maxlength);

    int32_t minLength();
    void setMinLength(int32_t minlength);

    NodeList* labels();

    bool isAutofocusable();

    bool autofocus();

    void setAutofocus(bool autofocus);

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* val, bool attributeCreated,
                                     bool attributeRemoved) override;
    virtual void didNodeInsertedToDocumentTree() override;

    virtual bool isPlaceholderVisible();

    virtual bool isLabelable() const override;
    virtual bool isListedElement() = 0;
    virtual bool isResettableElement() = 0;

protected:
    HTMLFormControl(Document* document, bool supportTabIndex = true);
    void fireSubmitEvent();

    virtual void fireEvent(QualifiedName& type, bool bubbles, bool cancelable);
    virtual void queueEvent(QualifiedName& type, bool bubbles, bool cancelable);

    virtual bool isDisabled();

    HTMLFormElement* formOwner();
    virtual bool isButton(HTMLFormControl* node);

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLFormControl, m_value));
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLFormControl, m_labels));
        HTMLElement::fillGCDescriptor(desc);
    }

    String* m_value;

private:
    Node* findAncestor(Node* ancestorToFind, Node* fromThisNode);
    bool m_supportTabIndex;
    NodeList* m_labels;
};

class HTMLFormElement : public HTMLFormControl {
public:
    HTMLFormElement(Document* document);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLFormElement() const override;

    // 4.4 Interface Node
    virtual QualifiedName name() override;

    // 4.10 Interface Form
    String* action();
    void setAction(String* name);

    String* enctype();
    void setEnctype(String* enctype);

    String* encoding();
    void setEncoding(String* encoding);

    String* method();
    void setMethod(String* method);

    String* target();
    void setTarget(String* target);

    void submit();

    void reset();

    HTMLFormControlsCollection* elements();
    uint32_t length();
    Element* defaultIndexedGetter(uint32_t idx);
    Element* defaultNamedGetter(String* name);

    // Other methods
    bool handleDefaultEvent(Event* event) override;

    virtual bool isListedElement() override
    {
        return false;
    }

    virtual bool isResettableElement() override
    {
        return false;
    }

private:
    void submit(HTMLElement* submitter);
    GCVector<FormDataSetItem*>* createFormDataSet(HTMLElement* submitter);
    void submitData(ResourceURL* url, GCVector<FormDataSetItem*>* formDataSet,
                    ResourceRequest::EncodeType encodeType,
                    ResourceRequest::MethodType methodType);
    void clearPlannedNavigationTask();
    bool isFormAssociatedElement(Node* node);
    void computeFormSubmittableElements(Node* parent,
                                        GCVector<HTMLFormControl*>& list);
    bool isSubmittableElement(Node* node);

    HTMLFormControlsCollection* m_elements;

    size_t m_plannedNavigationTaskId;
    bool m_isLockedForReset;
};
}

#endif
