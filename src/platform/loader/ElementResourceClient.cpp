/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Element.h"
#include "core/dom/Document.h"
#include "core/dom/Event.h"
#include "platform/loader/ElementResourceClient.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"

namespace Starfish {

void ElementResourceClient::didLoadFinished()
{
    ResourceClient::didLoadFinished();
    auto fn = [](size_t handle, void* data) {
        Element* element = (Element*)data;
        String* eventType =
            element->starfish()->staticStrings()->m_load.localName();
        Event* e = new Event(element->executionContext(), eventType,
                             EventInit(false, false));
        element->EventTarget::dispatchEventByUA(element, e, true);
    };
    if (m_needsSyncEventDispatch) {
        fn(SIZE_MAX, m_element);
    } else {
        m_element->webView()->messageLoop()->addIdler(m_element->window(), fn,
                                                      m_element);
    }
}

void ElementResourceClient::didLoadFailed()
{
    ResourceClient::didLoadFailed();
    auto fn = [](size_t handle, void* data) {
        Element* element = (Element*)data;
        String* eventType =
            element->starfish()->staticStrings()->m_error.localName();
        Event* e = new Event(element->executionContext(), eventType,
                             EventInit(false, false));
        element->EventTarget::dispatchEventByUA(element, e, true);
    };
    if (m_needsSyncEventDispatch) {
        fn(SIZE_MAX, m_element);
    } else {
        m_element->webView()->messageLoop()->addIdler(m_element->window(), fn,
                                                      m_element);
    }
}
}
