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

#include "StarFishConfig.h"
#include "StarFish.h"

#include "binding/ScriptWrappable.h"
#include "binding/StarFishHoldable.h"

#include "JavaScriptNativeHandler.h"
namespace StarFish {

JavaScriptNativeHandler::JavaScriptNativeHandler(
    StarFish* starFish, String* functionName, NativeFunctionPtr nativeCallback)
    : ScriptWrappable(this)
    , StarFishHoldable(starFish)
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
    STARFISH_ASSERT(starFish());
    STARFISH_ASSERT(m_callback);
    return String::fromUTF8(m_callback(param->toUTF8NonGCString()).c_str());
}
}
