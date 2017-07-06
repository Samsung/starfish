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
