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
#include "core/dom/Touch.h"
#include "core/dom/TouchList.h"
#include "core/dom/TouchEvent.h"

namespace StarFish {

TouchEvent::TouchEvent(Document* document)
    : UIEvent(document)
    , m_touches(new TouchList(document))
{
}

TouchEvent::TouchEvent(Document* document, String* eventType)
    : UIEvent(document, eventType)
    , m_touches(new TouchList(document))
{
}

TouchEvent::TouchEvent(Document* document, String* eventType, TouchData* data,
                       size_t touchCount)
    : TouchEvent(document, eventType)
{
    for (size_t i = 0; i < touchCount; i++) {
        m_touches->push_back(new Touch(document, data[i]));
    }
}
}
