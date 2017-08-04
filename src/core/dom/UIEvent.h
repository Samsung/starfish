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
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    UIEventInit()
        : EventInit()
        , m_view(nullptr)
        , m_detail(false)
    {
    }

    UIEventInit(bool bubbles, bool cancelable)
        : EventInit(bubbles, cancelable)
        , m_view(nullptr)
        , m_detail(false)
    {
    }

    Window* view() const
    {
        return m_view;
    }

    void setView(Window* view)
    {
        m_view = view;
    }

    int32_t detail() const
    {
        return m_detail;
    }

    void setDetail(int32_t detail)
    {
        m_detail = detail;
    }

private:
    Window* m_view;
    int32_t m_detail;
};

// https://w3c.github.io/uievents/#dictdef-eventmodifierinit
struct EventModifierInit : UIEventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    EventModifierInit();

    bool ctrlKey() const;
    void setCtrlKey(bool ctrlKey);

    bool shiftKey() const;
    void setShiftKey(bool shiftKey);

    bool altKey() const;
    void setAltKey(bool altKey);

    bool metaKey() const;
    void setMetaKey(bool metaKey);

private:
    bool m_ctrlKey;
    bool m_shiftKey;
    bool m_altKey;
    bool m_metaKey;

    // TODO Implement
    // bool m_modifierAltGraph;
    // bool m_modifierCapsLock;
    // bool m_modifierFn;
    // bool m_modifierFnLock;
    // bool m_modifierHyper;
    // bool m_modifierNumLock;
    // bool m_modifierScrollLock;
    // bool m_modifierSuper;
    // bool m_modifierSymbol;
    // bool m_modifierSymbolLock;
};

class UIEvent : public Event {
public:
    UIEvent(Document* document)
        : Event(document)
        , m_view(nullptr)
    {
    }

    UIEvent(Document* document, String* eventType)
        : Event(document, eventType)
        , m_view(nullptr)
    {
    }

    UIEvent(Document* document, String* eventType, const UIEventInit& init)
        : Event(document, eventType, init)
        , m_view(init.view())
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isUIEvent() const override;

    Window* view() const
    {
        return m_view;
    }

    void setView(Window* view)
    {
        m_view = view;
    }

private:
    Window* m_view;
};
}

#endif
