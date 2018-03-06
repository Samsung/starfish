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

#ifndef __StarFishTouchEvent__
#define __StarFishTouchEvent__

#include "core/dom/UIEvent.h"
#include "core/dom/Touch.h"

namespace StarFish {

// TODO TouchEventInit
// https://w3c.github.io/touch-events/#idl-def-toucheventinit

// Binding interface
// https://w3c.github.io/touch-events/#touchevent-interface
class TouchEvent : public UIEvent {
public:
    TouchEvent(Document* document);
    TouchEvent(Document* document, String* eventType);
    TouchEvent(Document* document, String* eventType, TouchData* data,
               size_t touchCount);
    // TODO Implement offitial constructor
    // TouchEvent(Document* document, String* eventType, TouchEventInit& init);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isTouchEvent() const override;

    TouchList* touches()
    {
        return m_touches;
    }

private:
    TouchList* m_touches;
};
}

#endif
