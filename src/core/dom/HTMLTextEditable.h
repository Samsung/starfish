/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishHTMLTextEditable__
#define __StarFishHTMLTextEditable__

#include "core/dom/HTMLFormElement.h"

namespace StarFish {

class HTMLTextEditable : public HTMLFormControl {
    const int CARET_THICKNESS = 2;

public:
    HTMLTextEditable(Document* document);

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc,
                   GC_WORD_OFFSET(HTMLTextEditable, m_currentEditingText));
        HTMLFormControl::fillGCDescriptor(desc);
    }

    virtual bool isHTMLTextEditable() const override
    {
        return true;
    }

    virtual bool hasTextValue()
    {
        return true;
    }

    virtual String* textValue()
    {
        return m_value;
    }

    virtual bool isEditableType()
    {
        return true;
    }

    virtual LayoutLocation& currentCaretLayoutLocation()
    {
        return m_currentCaretLayoutLocation;
    }

    virtual size_t currentCaretPosition()
    {
        return m_currentCaretPosition;
    }

    virtual bool shouldDrawCaret()
    {
        return m_shouldDrawCaret;
    }

    virtual bool ignoreLineBreaks()
    {
        return false;
    }

    virtual void didStateChanged(int oldState, int newState) override;
    virtual bool handleDefaultEvent(Event* event) override;
    virtual LayoutUnit caretThickness() const;

    bool isMutable() const
    {
        return !disabled() && !readonly();
    }

    bool readonly() const;
    String* placeholder();

protected:
    bool m_dirtyValueFlag;
    bool m_shouldDrawCaret;
    size_t m_caretBlinkingIntervalId;
    size_t m_currentCaretPosition;
    LayoutLocation m_currentCaretLayoutLocation;
    String* m_currentEditingText;
    int32_t m_maxlength;
};
}

#endif
