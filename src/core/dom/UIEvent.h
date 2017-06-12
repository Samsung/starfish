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

#ifndef __StarFishUIEvent__
#define __StarFishUIEvent__

#include "Event.h"

namespace StarFish {

// https://w3c.github.io/uievents/#dictdef-uieventinit
class Window;
struct UIEventInit : EventInit {
public:
    UIEventInit();

    // Constructor for internal use
    UIEventInit(bool bubbles);
    UIEventInit(bool bubbles, bool cancelable);

    Window* view() const;
    void setView(Window*);

    int32_t detail() const;
    void setDetail(int32_t);

private:
    Window* m_view;
    int32_t m_detail;
};

// https://w3c.github.io/uievents/#dictdef-eventmodifierinit
struct EventModifierInit : UIEventInit {
public:
    EventModifierInit();

    // Constructor for internal use
    EventModifierInit(bool bubbles);
    EventModifierInit(bool bubbles, bool cancelable);

    bool ctrlKey() const;
    void setCtrlKey(bool ctrlKey);

    bool shiftKey() const;
    void setShiftKey(bool shiftKey);

    bool altKey() const;
    void setAltKey(bool altKey);

    bool metaKey() const;
    void setMetaKey(bool metaKey);

    bool modifierAltGraph() const;
    void setModifierAltGraph(bool modifierAltGraph);

    bool modifierCapsLock() const;
    void setModifierCapsLock(bool modifierCapsLock);

    bool modifierFn() const;
    void setModifierFn(bool modifierFn);

    bool modifierFnLock() const;
    void setModifierFnLock(bool modifierFnLock);

    bool modifierHyper() const;
    void setModifierHyper(bool modifierHyper);

    bool modifierNumLock() const;
    void setModifierNumLock(bool modifierNumLock);

    bool modifierScrollLock() const;
    void setModifierScrollLock(bool modifierScrollLock);

    bool modifierSuper() const;
    void setModifierSuper(bool modifierSuper);

    bool modifierSymbol() const;
    void setModifierSymbol(bool modifierSymbol);

    bool modifierSymbolLock() const;
    void setModifierSymbolLock(bool modifierSymbolLock);

private:
    bool m_ctrlKey;
    bool m_shiftKey;
    bool m_altKey;
    bool m_metaKey;
    bool m_modifierAltGraph;
    bool m_modifierCapsLock;
    bool m_modifierFn;
    bool m_modifierFnLock;
    bool m_modifierHyper;
    bool m_modifierNumLock;
    bool m_modifierScrollLock;
    bool m_modifierSuper;
    bool m_modifierSymbol;
    bool m_modifierSymbolLock;
};

class UIEvent : public Event {
public:
    UIEvent(Document* document, String* eventType,
            const UIEventInit& init = UIEventInit())
        : Event(document, eventType, init)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isUIEvent() const override;
};
}

#endif
