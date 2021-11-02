/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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
#if defined(PORT_EVENTLOOP_BACKEND_WINDOWS)

#include "Starfish.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/page/WebView.h"

#include <Windows.h>

namespace Starfish {

#define IDLE_MESSAGE_INVOKE_NAVIGATE (WM_USER + 22)

struct InvokeNavigateData {
    WebView* wv;
    ResourceURL* url;
    ReferrerURL* referrerURL;
    UINT_PTR timerID;
    HistoryManagerAction action;
};

__declspec(thread) InvokeNavigateData* g_invokeNavigateData;

void MessageLoopMixin::invokeNavigate(WebView* wv, ResourceURL* url,
                                      ReferrerURL* referrerURL,
                                      HistoryManagerAction action, bool force)
{
    InvokeNavigateData* data =
        new (GC_MALLOC_UNCOLLECTABLE(sizeof(InvokeNavigateData)))
            InvokeNavigateData();
    g_invokeNavigateData = data;
    data->wv = wv;
    data->url = url;
    data->referrerURL = referrerURL;
    data->action = action;
    PostMessage(NULL, IDLE_MESSAGE_INVOKE_NAVIGATE, (size_t)data, 0);
}

void MessageLoopMixin::onDestroyed()
{
    g_invokeNavigateData = nullptr;
}

void MessageLoopMixin::processMessage(MessageLoopMixin* self,
                                      const MSG& message)
{
    switch (message.message) {
    case IDLE_MESSAGE_INVOKE_NAVIGATE: {
        STARFISH_ASSERT(message.message == IDLE_MESSAGE_INVOKE_NAVIGATE);
        if ((size_t)g_invokeNavigateData == (size_t)message.wParam) {
            g_invokeNavigateData->wv->navigate(
                g_invokeNavigateData->url, g_invokeNavigateData->action,
                g_invokeNavigateData->referrerURL);
            g_invokeNavigateData = nullptr;
        }
        GC_FREE((void*)message.wParam);
    } break;
    default:
        break;
    };
}

} // namespace Starfish

#endif
