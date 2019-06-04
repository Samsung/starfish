/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
#include "Starfish.h"

#include "binding/ScriptWrappable.h"
#include "JavaScriptNativeHandler.h"

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
    auto returnValue = m_callback(param->toUTF8NonGCString());
    return String::fromUTF8(returnValue.data(), returnValue.size());
}
}
