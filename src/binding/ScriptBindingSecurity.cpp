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
#include "PlatformIntegrationData.h"
#include "core/page/Window.h"
#include "core/dom/Document.h"
#include "core/page/Location.h"
#include "core/dom/WebOrigin.h"
#include "core/page/WebView.h"
#include <EscargotPublic.h>

namespace Starfish {

// https://html.spec.whatwg.org/multipage/browsers.html#isplatformobjectsameorigin-(-o-)
static bool canAccess(Document* source, Document* target)
{
    if (source->webView()->getWebSecurityMode() ==
        LWE::WebSecurityMode::Disable) {
        return true;
    }

    if (source->webOrigin()->isSameOriginDomain(target->webOrigin())) {
        return true;
    }
    return false;
}

bool ScriptBindingSecurity::shouldAllowCrossOriginScriptAPIAccessToWindow(
    Escargot::ExecutionStateRef* state, Window* window)
{
    Window* sourceWindow =
        (Window*)state->resolveCallerLexicalGlobalObject()->extraData();

    if (!sourceWindow || !window) {
        return false;
    }
    if (!canAccess(sourceWindow->document(), window->document())) {
        throw new DOMException(sourceWindow->executionContext(),
                               DOMException::Code::SECURITY_ERR);
    }
    return true;
}

bool ScriptBindingSecurity::shouldAllowCrossOriginScriptAPIAccessToLocation(
    Escargot::ExecutionStateRef* state, Location* location)
{
    Window* sourceWindow =
        (Window*)state->resolveCallerLexicalGlobalObject()->extraData();

    if (!sourceWindow || !location) {
        return false;
    }
    if (!canAccess(sourceWindow->document(), location->document())) {
        throw new DOMException(sourceWindow->executionContext(),
                               DOMException::Code::SECURITY_ERR);
    }
    return true;
}
}
