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
class HTMLInputElement : public HTMLFormObject {
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
    String* value();
    void setValue(String* value);

    String* formEnctype();
    void setFormEnctype(String* enctype);

    String* formMethod();
    void setFormMethod(String* method);

    String* formTarget();
    void setFormTarget(String* target);

    String* formAction();
    void setFormAction(String* formAction);

    bool checked();
    void setChecked(bool checked);

    // Other methods
    String* obscurePhrase(String* phrase);
    bool handleDefaultEvent(Event* event) override;

    bool supportsFocus() const override;

protected:
    bool isUserKeyboardInputAllowed();
    bool m_shouldDrawCaret;
    size_t m_caretBlinkingIntervalId;
    size_t m_currentCaretPosition;
    String* m_currentEditingText;
};
}

#endif
