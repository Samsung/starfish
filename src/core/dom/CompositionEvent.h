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

#ifndef __StarFishCompositionEvent__
#define __StarFishCompositionEvent__

#include "core/dom/UIEvent.h"

namespace StarFish {

class Window;

struct CompositionEventData {
};

struct CompositionEventInit : UIEventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    CompositionEventInit()
        : UIEventInit()
        , m_data(String::emptyString)
    {
    }

    CompositionEventInit(String* data)
        : UIEventInit()
        , m_data(data)
    {
    }

    CompositionEventInit(bool bubbles, bool cancelable)
        : UIEventInit(bubbles, cancelable)
        , m_data(String::emptyString)
    {
    }

    String* data() const
    {
        return m_data;
    }

    void setData(String* data)
    {
        m_data = data;
    }

private:
    String* m_data;
};

class CompositionEvent : public UIEvent {
public:
    CompositionEvent(Document* document)
        : UIEvent(document)
        , m_data(String::emptyString)
    {
    }

    CompositionEvent(Document* document, String* eventType)
        : UIEvent(document, eventType)
        , m_data(String::emptyString)
    {
    }

    CompositionEvent(Document* document, String* eventType,
                     const CompositionEventInit& init)
        : UIEvent(document, eventType, init)
        , m_data(init.data())
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isCompositionEvent() const override;

    String* data() const
    {
        return m_data;
    }

    void setData(String* data)
    {
        m_data = data;
    }

private:
    String* m_data;
};
}

#endif
