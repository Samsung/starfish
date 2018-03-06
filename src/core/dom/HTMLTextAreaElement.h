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
