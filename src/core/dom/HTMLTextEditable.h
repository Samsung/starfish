/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

    bool isMutable()
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
