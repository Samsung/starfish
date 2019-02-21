/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#if defined(PORT_EVENTLOOP_BACKEND_LIBUV)

#include "Starfish.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/page/Window.h"
#include "core/page/ScriptContext.h"
#include "core/page/WebView.h"

#include <uv.h>

namespace Starfish {

static void on_close_handle(uv_handle_t* handle)
{
    free(handle);
}

struct InvokeNavigateData : public gc {
    WebView* wv;
    ResourceURL* url;
    ReferrerURL* referrerURL;
    uv_timer_t* idler;
    HistoryManagerAction action;
    void** extra;

    static void* operator new(size_t s)
    {
        return GC_MALLOC_UNCOLLECTABLE(s);
    }
};

void MessageLoopMixin::invokeNavigate(WebView* wv, ResourceURL* url,
                                      ReferrerURL* referrerURL,
                                      HistoryManagerAction action, bool force)
{
    if (m_navigateInvokeIdler != nullptr) {
        auto data = ((InvokeNavigateData*)m_navigateInvokeIdler);
        uv_timer_stop(data->idler);
        uv_close((uv_handle_t*)data->idler, on_close_handle);
        delete data;
    }

    InvokeNavigateData* data = new InvokeNavigateData();
    m_navigateInvokeIdler = data;
    data->extra = &m_navigateInvokeIdler;
    data->wv = wv;
    data->url = url;
    data->referrerURL = referrerURL;
    data->action = action;
    data->idler = (uv_timer_t*)malloc(sizeof(uv_timer_t));
    uv_timer_init(uv_default_loop(), data->idler);
    data->idler->data = data;
    uv_timer_start(
        data->idler,
        [](uv_timer_t* handle) {
            InvokeNavigateData* data = (InvokeNavigateData*)handle->data;
            data->wv->navigate(data->url, data->action, data->referrerURL);
            *(data->extra) = nullptr;
            uv_timer_stop(handle);
            delete data;
            uv_close((uv_handle_t*)handle, on_close_handle);
        },
        0, 0);
}

void MessageLoopMixin::onDestroyed()
{
    if (m_navigateInvokeIdler) {
        uv_timer_stop(((InvokeNavigateData*)m_navigateInvokeIdler)->idler);
        uv_close(
            (uv_handle_t*)((InvokeNavigateData*)m_navigateInvokeIdler)->idler,
            on_close_handle);
        delete ((InvokeNavigateData*)m_navigateInvokeIdler);
        m_navigateInvokeIdler = nullptr;
    }
}

} // namespace Starfish

#endif
