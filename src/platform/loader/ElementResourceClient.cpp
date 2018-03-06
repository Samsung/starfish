/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
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
        element->EventTarget::dispatchEventByUA(element, e, true);
    };
    if (m_needsSyncEventDispatch) {
        fn(SIZE_MAX, m_element);
    } else {
        m_element->starFish()->messageLoop()->addIdler(
            m_element->document()->browsingContext(), fn, m_element);
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
        element->EventTarget::dispatchEventByUA(element, e, true);
    };
    if (m_needsSyncEventDispatch) {
        fn(SIZE_MAX, m_element);
    } else {
        m_element->starFish()->messageLoop()->addIdler(
            m_element->document()->browsingContext(), fn, m_element);
    }
}
}
