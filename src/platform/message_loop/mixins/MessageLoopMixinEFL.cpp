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
#if defined(PORT_EVENTLOOP_BACKEND_EFL)

#include "Starfish.h"
#include "core/modules/message_loop/MessageLoopMixin.h"
#include "binding/ScriptBindingInstance.h"
#include "core/modules/threading/Thread.h"
#include "core/modules/threading/Locker.h"
#include "core/page/WebView.h"

#include <Ecore.h>

#if defined(PORT_WINDOW_BACKEND_EFL)
#include <Elementary.h>
#endif

namespace Starfish {

struct InvokeNavigateData : public gc {
    WebView* wv;
    ResourceURL* url;
    ReferrerURL* referrerURL;
    Ecore_Animator* idler;
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
        ecore_animator_freeze(data->idler);
        ecore_animator_del(data->idler);
        delete data;
    }

    InvokeNavigateData* data = new InvokeNavigateData();
    m_navigateInvokeIdler = data;
    data->extra = &m_navigateInvokeIdler;
    data->wv = wv;
    data->url = url;
    data->referrerURL = referrerURL;
    data->action = action;
    data->idler = ecore_animator_add(
        [](void* d) -> Eina_Bool {
            InvokeNavigateData* data = (InvokeNavigateData*)d;
            data->wv->navigate(data->url, data->action, data->referrerURL);
            *(data->extra) = nullptr;
            delete data;
            return ECORE_CALLBACK_CANCEL;
        },
        data);
}

void MessageLoopMixin::onDestroyed()
{
    if (m_navigateInvokeIdler) {
        ecore_animator_freeze(
            ((InvokeNavigateData*)m_navigateInvokeIdler)->idler);
        ecore_animator_del(((InvokeNavigateData*)m_navigateInvokeIdler)->idler);
        delete ((InvokeNavigateData*)m_navigateInvokeIdler);
        m_navigateInvokeIdler = nullptr;
    }
}

} // namespace Starfish

#endif
