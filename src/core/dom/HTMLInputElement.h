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
    bool isPlaceholderVisible() override;

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

    String* max();
    void setMax(String* max);
    String* min();
    void setMin(String* min);

    String* step();
    void setStep(String* step);

    bool hasActivationBehavior() override;
    void activationBehavior() override;
    void legacyPreActivationBehavior() override;
    void legacyCanceledActivationBehavior() override;

protected:
private:
    void toggleChecked();
    bool isSizableType();
    bool shouldUsePlaceholder();
    void sanitizeValue();

    GCVector<HTMLInputElement*>* radioButtonGroup();
    void resetRadioButtons();
    HTMLInputElement* getCurrentCheckedRadioButton();
    bool isInSameRadioButtonGroup(HTMLInputElement* other);

    double minimum();
    double maximum();
    bool allowedValueStep(double* ret);
    double stepBase();
    double defaultValueForRangeType();
    bool sufferingFromStepMismatch(double val);
    double roundValueToMultiplesOfSteps(double val, double stepVal);
    void setDefaultBookkeepingValues();

    bool m_dirtiness;

    bool m_checkness;
    bool m_dirtyCheckness;
    bool m_previousCheckness;

    bool m_shouldDrawCaret;
    size_t m_caretBlinkingIntervalId;
    size_t m_currentCaretPosition;
    LayoutLocation m_currentCaretLayoutLocation;
    String* m_currentEditingText;

    int32_t m_defaultMinimum;
    int32_t m_defaultMaximum;
    int32_t m_defaultStep;
    int32_t m_stepScaleFactor;

    int32_t m_maxlength;

    HTMLInputElement* m_previousCheckedRadioButton;
};
}

#endif
