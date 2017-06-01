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
#include "StarFish.h"
#include "core/dom/Element.h"
#include "core/dom/Event.h"
#include "platform/loader/ElementResourceClient.h"
#include "core/modules/message_loop/MessageLoop.h"

namespace StarFish {

void ElementResourceClient::didLoadFinished()
{
    ResourceClient::didLoadFinished();
    auto fn = [](size_t handle, void* data) {
        Element* element = (Element*)data;
        String* eventType =
            element->starFish()->staticStrings()->m_load.localName();
        Event* e =
            new Event(element->document(), eventType, EventInit(false, false));
        element->EventTarget::dispatchEvent(element, e);
    };
    if (m_needsSyncEventDispatch) {
        fn(SIZE_MAX, m_element);
    } else {
        m_element->starFish()->messageLoop()->addIdler(fn, m_element);
    }
}

void ElementResourceClient::didLoadFailed()
{
    ResourceClient::didLoadFailed();
    auto fn = [](size_t handle, void* data) {
        Element* element = (Element*)data;
        String* eventType =
            element->starFish()->staticStrings()->m_error.localName();
        Event* e =
            new Event(element->document(), eventType, EventInit(false, false));
        element->EventTarget::dispatchEvent(element, e);
    };
    if (m_needsSyncEventDispatch) {
        fn(SIZE_MAX, m_element);
    } else {
        m_element->starFish()->messageLoop()->addIdler(fn, m_element);
    }
}
}
