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

#ifndef __StarFishUIEvent__
#define __StarFishUIEvent__

#include "Event.h"
#include "core/event/EventModifierData.h"

namespace StarFish {

// https://w3c.github.io/uievents/#dictdef-uieventinit
class Window;
struct UIEventInit : EventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    UIEventInit()
        : EventInit()
        , m_view(nullptr)
        , m_detail(0)
    {
    }

    UIEventInit(bool bubbles, bool cancelable)
        : EventInit(bubbles, cancelable)
        , m_view(nullptr)
        , m_detail(0)
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
    friend class KeyboardEvent;
    friend class MouseEvent;

public:
    EventModifierInit();
    EventModifierInit(EventModifierData& emdata);
    bool ctrlKey() const;
    void setCtrlKey(bool ctrlKey);

    bool shiftKey() const;
    void setShiftKey(bool shiftKey);

    bool altKey() const;
    void setAltKey(bool altKey);

    bool metaKey() const;
    void setMetaKey(bool metaKey);

private:
    EventModifierData m_eventModifierData;
};

class UIEvent : public Event {
public:
    UIEvent(Document* document)
        : Event(document)
        , m_view(nullptr)
        , m_detail(0)
    {
    }

    UIEvent(Document* document, String* eventType)
        : Event(document, eventType)
        , m_view(nullptr)
        , m_detail(0)
    {
    }

    UIEvent(Document* document, String* eventType, const UIEventInit& init)
        : Event(document, eventType, init)
        , m_view(init.view())
        , m_detail(0)
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

    int32_t detail() const
    {
#ifdef PORT_GRAPHIC_BACKEND_GENERAL_BUFFER
        STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
#endif
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
}

#endif
