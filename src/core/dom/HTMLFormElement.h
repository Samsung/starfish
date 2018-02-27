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

    String* m_name;
    String* m_value;
    String* m_type;
};

class FormSubmitData : public gc {
public:
    FormSubmitData(GCVector<FormDataSetItem*>* formDataSet,
                   ResourceRequest::EncodeType enctype,
                   ResourceRequest::MethodType method);
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

    virtual bool disabled() const;
    virtual void setDisabled(bool disabled);

    virtual HTMLFormElement* form();

    // Other method
    virtual bool supportsFocus();

    virtual bool isHTMLFormControl() const override
    {
        return true;
    }

    virtual void reset()
    {
    }

    int32_t maxLength();
    void setMaxLength(int32_t maxlength);

    bool isAutofocusable();

    bool autofocus();

    void setAutofocus(bool autofocus);

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* val, bool attributeCreated,
                                     bool attributeRemoved) override;
    virtual void didNodeInsertedToDocumentTree() override;

protected:
    HTMLFormControl(Document* document, bool supportTabIndex = true);
    void fireSubmitEvent();

    virtual void fireEvent(QualifiedName& type, bool bubbles, bool cancelable);
    virtual void queueEvent(QualifiedName& type, bool bubbles, bool cancelable);

    virtual bool isDisabled();

    HTMLFormElement* formOwner();
    HTMLSelectElement* select();
    virtual bool isButton(HTMLFormControl* node);

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(HTMLFormControl, m_value));
        HTMLElement::fillGCDescriptor(desc);
    }

    String* m_value;

private:
    Node* findAncestor(Node* ancestorToFind, Node* fromThisNode);
    bool m_supportTabIndex;
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
    virtual QualifiedName name();

    // 4.10 Interface Form
    String* action();
    void setAction(String* name);

    String* enctype();
    void setEnctype(String* enctype);

    String* method();
    void setMethod(String* method);

    String* target();
    void setTarget(String* target);

    void submit();

    HTMLFormControlsCollection* elements();
    uint32_t length();
    Element* defaultIndexedGetter(uint32_t idx);
    Element* defaultNamedGetter(String* name);

    // Other methods
    bool handleDefaultEvent(Event* event) override;

private:
    void submit(HTMLElement* submitter);
    GCVector<FormDataSetItem*>* createFormDataSet(HTMLElement* submitter);
    void submitData(ResourceURL* url, GCVector<FormDataSetItem*>* formDataSet,
                    ResourceRequest::EncodeType encodeType,
                    ResourceRequest::MethodType methodType);
    void clearPlannedNavigationTask();
    bool isFormAssociatedElement(Node* node);
    void computeFormAssociatedElements(Node* parent,
                                       GCVector<HTMLElement*>& list);
    bool isSubmittableElement(Node* node);

    HTMLFormControlsCollection* m_elements;

    size_t m_plannedNavigationTaskId;
};
}

#endif
