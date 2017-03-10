/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "Event.h"
#include "platform/profiling/Profiling.h"

namespace StarFish {

EventInit::EventInit() : bubbles(false), cancelable(false)
{
}

EventInit::EventInit(bool b, bool c) : bubbles(b), cancelable(c)
{
}

Event::Event()
    : ScriptWrappable(this)
    , m_isInitialized(true)
    , m_type(String::emptyString)
    , m_eventPhase(0)
    , m_propagationStopped(false)
    , m_immediatePropagationStopped(false)
    , m_bubbles(false)
    , m_cancelable(false)
    , m_defaultPrevented(false)
    , m_isDispatched(false)
{
    m_timeStamp = timestamp();
}

Event::Event(String* eventType, const EventInit& init)
    : ScriptWrappable(this)
    , m_isInitialized(true)
    , m_type(eventType)
    , m_eventPhase(0)
    , m_propagationStopped(false)
    , m_immediatePropagationStopped(false)
    , m_bubbles(init.bubbles)
    , m_cancelable(init.cancelable)
    , m_defaultPrevented(false)
    , m_isDispatched(false)
{
    m_timeStamp = timestamp();
}

ProgressEventInit::ProgressEventInit()
    : lengthComputable(false), loaded(0), total(0)
{
}

ProgressEventInit::ProgressEventInit(bool b, bool c, bool lengthComputable,
                                     unsigned long long loaded,
                                     unsigned long long total)
    : EventInit(b, c)
    , lengthComputable(lengthComputable)
    , loaded(loaded)
    , total(total)
{
}

ProgressEvent::ProgressEvent(String* eventType, const ProgressEventInit& init)
    : Event(eventType, init)
    , m_lengthComputable(init.lengthComputable)
    , m_loaded(init.loaded)
    , m_total(init.total)
{
    initScriptWrappable(this);
}

unsigned long KeyboardEvent::convertKeyCodeFromEcore(String* key)
{
    if (key->length() == 1) {
        const char* rawdata = key->utf8Data();
        if (rawdata[0] >= '0' && rawdata[0] <= '9') {
            return (unsigned)(rawdata[0] - '0') + (unsigned)KEYBOARD_KEYCODE_0;
        } else if (rawdata[0] >= 'a' && rawdata[0] <= 'z') {
            return (unsigned)(rawdata[0] - 'a') + (unsigned)KEYBOARD_KEYCODE_a;
        }
        // Note: No upper case -> expressed with shift key
    } else if (key->equals("Left")) {
        return KEYBOARD_KEYCODE_LEFT;
    } else if (key->equals("Up")) {
        return KEYBOARD_KEYCODE_UP;
    } else if (key->equals("Right")) {
        return KEYBOARD_KEYCODE_RIGHT;
    } else if (key->equals("Down")) {
        return KEYBOARD_KEYCODE_DOWN;
    } else if (key->equals("Return")) {
        return KEYBOARD_KEYCODE_RETURN;
    } else if (key->equals("BackSpace")) {
        return KEYBOARD_KEYCODE_BACKSPACE;
    } else if (key->equals("XF86Back")) {
        return KEYBOARD_KEYCODE_ESC;
    } else if (key->equals("space")) {
        return KEYBOARD_KEYCODE_SPACE;
    } else if (key->equals("Shift_L")) {
        return KEYBOARD_KEYCODE_SHIFT;
    } else if (key->equals("Control_L")) {
        return KEYBOARD_KEYCODE_CTRL_L;
    } else if (key->equals("Alt_L")) {
        return KEYBOARD_KEYCODE_ALT_L;
    }
    return KEYBOARD_KEYCODE_NONE;
}
}
