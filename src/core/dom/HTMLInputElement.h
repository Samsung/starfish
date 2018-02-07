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

#ifndef __StarFishHTMLInputElement__
#define __StarFishHTMLInputElement__

#include "core/dom/HTMLFormElement.h"

namespace StarFish {
class Event;
class HTMLInputElement : public HTMLFormControl {
    const int DEFAULT_SIZE = 20;
    const int CARET_THICKNESS = 2;
    friend class FrameInputBox;

public:
    HTMLInputElement(Document* document);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isHTMLInputElement() const override;

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;
    virtual void didStateChanged(int oldState, int newState) override;
    // 4.4 Interface Node
    virtual QualifiedName name();

    // 4.10 Interface Input
    String* type() override;
    bool checked();
    void setChecked(bool checked);
    bool defaultChecked();
    void setDefaultChecked(bool checked);

    String* placeholder();
    void setPlaceholder(String* target);

    uint32_t size();
    void setSize(String* size);

    // Other methods
    bool handleDefaultEvent(Event* event) override;
    bool canHaveValue();
    bool supportsFocus() override;

    LayoutUnit caretThickness() const;
    LayoutLocation& currentCaretLayoutLocation()
    {
        return m_currentCaretLayoutLocation;
    }

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

    static String* obscurePhrase(String* phrase);
    static String* checkboxTickSymbol();

    String* visibleValue();
    bool isEditableType();

    bool firstDefaultValue();
    void setFirstDefaultValue(bool firstDefaultValue);

    String* defaultValue();
    void setDefaultValue(String* defaultValue);
    String* value() override;
    void setValue(String* value) override;

    int32_t maxLength();
    void setMaxLength(int32_t maxlength);

protected:
private:
    void toggleChecked();
    bool isSizableType();
    void updateInputboxValue(String* value);
    bool shouldUsePlaceholder();

    bool m_checkness;
    bool m_dirtiness;

    bool m_shouldDrawCaret;
    size_t m_caretBlinkingIntervalId;
    size_t m_currentCaretPosition;
    LayoutLocation m_currentCaretLayoutLocation;
    String* m_currentEditingText;
    int32_t m_maxlength;
};
}

#endif
