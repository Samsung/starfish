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

#ifndef __StarFishHTMLTextAreaElement__
#define __StarFishHTMLTextAreaElement__

#include "core/dom/HTMLTextEditable.h"

namespace StarFish {

class HTMLTextAreaElement : public HTMLTextEditable {
public:
    HTMLTextAreaElement(Document* document);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    void init(ScriptBindingInstance* instance, void* domObjectPointer) override;
    void didNodeInserted(Node* parent, Node* newChild) override;
    void didAttributeChanged(QualifiedName name, String* old, String* val,
                             bool attributeCreated,
                             bool attributeRemoved) override;
    void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;
    void reset() override;
    bool supportsFocus() const override;
    bool isHTMLTextAreaElement() const override;
    QualifiedName name() override;
    Node* clone() override;
    String* type() override;
    String* value() override;

    String* apiValue();
    void setApiValue(String* value);

    String* defaultValue();
    void setDefaultValue(String* value);

    uint32_t cols();
    void setCols(uint32_t value);

    uint32_t rows();
    void setRows(uint32_t value);
};
}

#endif
