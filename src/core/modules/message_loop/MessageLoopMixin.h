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

#ifndef __StarfishMessageLoopMixin__
#define __StarfishMessageLoopMixin__

#if defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
#include <Windows.h>
class MessageLoop;
#endif

namespace Starfish {

class WebView;
class ResourceURL;
class ReferrerURL;
enum class HistoryManagerAction;

class MessageLoopMixin {
public:
    void invokeNavigate(WebView* wv, ResourceURL* url, ReferrerURL* referrerURL,
                        HistoryManagerAction action, bool force = false);

    void onDestroyed();

#if defined(PORT_EVENTLOOP_BACKEND_WINDOWS)
    static void processMessage(MessageLoop* self, const MSG& message);
#endif

private:
    void* m_navigateInvokeIdler{ nullptr };
};

} // namespace Starfish

#endif
