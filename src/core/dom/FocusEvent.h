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

#ifndef __StarFishFocusEvent__
#define __StarFishFocusEvent__

#include "core/dom/UIEvent.h"

namespace StarFish {

struct FocusEventInit : UIEventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    FocusEventInit()
        : UIEventInit()
        , m_relatedTarget(nullptr)
    {
    }

    FocusEventInit(bool bubbles, bool cancelable,
                   EventTarget* relatedTarget = nullptr)
        : UIEventInit(bubbles, cancelable)
        , m_relatedTarget(relatedTarget)
    {
    }

    EventTarget* relatedTarget() const
    {
        return m_relatedTarget;
    }
    void setRelatedTarget(EventTarget* relatedTarget)
    {
        m_relatedTarget = relatedTarget;
    }

private:
    EventTarget* m_relatedTarget;
};

class FocusEvent : public UIEvent {
public:
    FocusEvent(Document* document)
        : UIEvent(document)
        , m_relatedTarget(nullptr)
    {
    }
    FocusEvent(Document* document, String* eventType)
        : UIEvent(document, eventType)
        , m_relatedTarget(nullptr)
    {
    }
    FocusEvent(Document* document, String* eventType,
               const FocusEventInit& init)
        : UIEvent(document, eventType, init)
        , m_relatedTarget(init.relatedTarget())
    {
    }

    EventTarget* relatedTarget() const
    {
        return m_relatedTarget;
    }
    void setRelatedTarget(EventTarget* relatedTarget)
    {
        m_relatedTarget = relatedTarget;
    }
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isFocusEvent() const override;

private:
    EventTarget* m_relatedTarget;
};
}

#endif
