/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#include "dom/FocusEvent.h"

namespace StarFish {

FocusEventInit::FocusEventInit()
    : FocusEventInit(false, false, false, nullptr)
{
}

FocusEventInit::FocusEventInit(Node* relatedTarget)
    : FocusEventInit(false, false, false, relatedTarget)
{
}

FocusEventInit::FocusEventInit(bool bubbles)
    : FocusEventInit(bubbles, false, false, nullptr)
{
}

FocusEventInit::FocusEventInit(bool bubbles, bool cancelable)
    : FocusEventInit(bubbles, cancelable, false, nullptr)
{
}

FocusEventInit::FocusEventInit(bool bubbles, bool cancelable, bool composed)
    : FocusEventInit(bubbles, cancelable, composed, nullptr)
{
}

FocusEventInit::FocusEventInit(bool bubbles, bool cancelable, bool composed,
                               Node* relatedTarget)
    : EventInit(bubbles, cancelable, composed)
    , relatedTarget(relatedTarget)
{
}
}
