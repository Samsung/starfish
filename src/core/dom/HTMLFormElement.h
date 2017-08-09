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

namespace StarFish {

class FormDataSetItem;

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
    void submitAsEntityBody(ResourceURL* url, String* formEnctype,
                            GCVector<FormDataSetItem*>* formDataSet);
    String* encodeFormDataSet(String* formEnctype,
                              GCVector<FormDataSetItem*>* formDataSet);

    Element* m_submitter;
};
}

#endif
