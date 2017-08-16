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

class FormDataSetItem : public gc {
public:
    FormDataSetItem(String* name, String* value, String* type);

    String* m_name;
    String* m_value;
    String* m_type;
};

class FormSubmitData : public gc {
public:
    FormSubmitData(GCVector<FormDataSetItem*>* formDataSet, String* formEnctype,
                   String* method);

    GCVector<FormDataSetItem*>* m_formDataSet;
    String* m_formEnctype;
    String* m_method;
};

class HTMLFormElement : public HTMLElement {
public:
    HTMLFormElement(Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLFormElement() const override;

    // 4.4 Interface Node
    virtual QualifiedName name();

    // 4.10 Interface Form
    String* domName();
    void setDomName(String* name);

    String* action();
    void setAction(String* name);

    String* enctype();
    void setEnctype(String* enctype);

    String* method();
    void setMethod(String* method);

    String* target();
    void setTarget(String* target);

    void submit();

    // Other methods
    void setSubmitter(Element* elem);

private:
    GCVector<FormDataSetItem*>* createFormDataSet();
    void submitData(ResourceURL* url, GCVector<FormDataSetItem*>* formDataSet,
                    String* formEnctype, String* methodType);

    Element* m_submitter;
};
}

#endif
