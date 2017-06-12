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

#include "StarFishConfig.h"
#include "core/dom/FocusEvent.h"

namespace StarFish {

FocusEventInit::FocusEventInit()
    : FocusEventInit(false)
{
}

FocusEventInit::FocusEventInit(bool bubbles)
    : FocusEventInit(bubbles, false)
{
}

FocusEventInit::FocusEventInit(bool bubbles, bool cancelable)
    : UIEventInit(bubbles, cancelable)
    , m_relatedTarget(nullptr)
{
}

EventTarget* FocusEventInit::relatedTarget() const
{
    return m_relatedTarget;
}

void FocusEventInit::setRelatedTarget(EventTarget* relatedTarget)
{
    m_relatedTarget = relatedTarget;
}
}
