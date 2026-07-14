/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#include "binding/ScriptWrappable.h"
#include "JavaScriptNativeHandler.h"
#include "core/modules/message_loop/MessageLoop.h"

namespace Starfish {

JavaScriptNativeHandler::JavaScriptNativeHandler(
    WebView* wv, String* functionName, NativeFunctionPtr nativeCallback)
    : ScriptWrappable(this)
    , WebViewHoldable(wv)
    , m_name(functionName)
    , m_callback(nativeCallback)
{
    GC_REGISTER_FINALIZER_NO_ORDER(
        this,
        [](void* obj, void* cd) {
            ((JavaScriptNativeHandler*)obj)->~JavaScriptNativeHandler();
        },
        NULL, NULL, NULL);
}

String* JavaScriptNativeHandler::callNativeHandler(String* param)
{
    STARFISH_ASSERT(param != nullptr);
    STARFISH_ASSERT(m_callback != nullptr);
    std::string nonGCParam = param->toUTF8NonGCString();
    std::string returnValue;
    // The embedder's callback may assume it runs on the process main thread
    // (that used to be guaranteed pre-isolated-thread-mode). Exclude the
    // process main thread while it runs instead of hopping the call onto it:
    // the callback still executes here, on the LWE thread, which is the only
    // thread with valid bdwgc state in this engine.
    MessageLoop::runWithProcessMainThreadPausedSync(
        [this, &nonGCParam, &returnValue]() {
            returnValue = m_callback(nonGCParam);
        });
    return String::fromUTF8(returnValue.data(), returnValue.size());
}
} // namespace Starfish
