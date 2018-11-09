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

#include "binding/ScriptBindingSecurity.h"
#include "core/page/Window.h"
#include "core/dom/Document.h"
#include "core/page/Location.h"
#include "core/dom/WebOrigin.h"
#include <EscargotPublic.h>

namespace Starfish {

// https://html.spec.whatwg.org/multipage/browsers.html#isplatformobjectsameorigin-(-o-)
static bool canAccess(Document* source, Document* target)
{
    if (source->webOrigin()->isSameOriginDomain(target->webOrigin())) {
        return true;
    }
    return false;
}

bool ScriptBindingSecurity::shouldAllowCrossOriginScriptAPIAccessToWindow(
    Escargot::ExecutionStateRef* state, Window* window)
{
    STARFISH_LOG_INFO("Call shouldAllowCrossOriginScriptAPIAccessToWindow\n");
    Window* sourceWindow = nullptr; // TODO : Needs to update Escargot

    if (!sourceWindow || !window) {
        return false;
    }
    return canAccess(sourceWindow->document(), window->document());
}

bool ScriptBindingSecurity::shouldAllowCrossOriginScriptAPIAccessToLocation(
    Escargot::ExecutionStateRef* state, Location* location)
{
    STARFISH_LOG_INFO("Call shouldAllowCrossOriginScriptAPIAccessToLocation\n");
    Window* sourceWindow = nullptr; // TODO : Needs to update Escargot

    if (!sourceWindow || !location) {
        return false;
    }
    return canAccess(sourceWindow->document(), location->document());
}
}