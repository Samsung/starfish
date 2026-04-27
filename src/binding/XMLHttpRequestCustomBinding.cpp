/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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
#include "core/dom/DOMException.h"
#include "core/modules/resource_request/ResourceRequest.h"
#include "core/xml/XMLHttpRequest.h"

#include <EscargotPublic.h>
using namespace Escargot;

namespace Starfish {

ValueRef* sendXMLHttpRequestFunction(ExecutionStateRef* state,
                                     ValueRef* thisValue, size_t argc,
                                     ValueRef** argv, bool isNewExpression)
{
    GENERATE_THIS_AND_CHECK_TYPE(XMLHttpRequest);

    ValueRef* arg0 = (argc > 0) ? argv[0] : ValueRef::createUndefined();

    if (!arg0->isUndefinedOrNull() && arg0->isObject()) {
        ObjectRef* obj = arg0->asObject();
        if (obj->isArrayBufferObject()) {
            ArrayBufferObjectRef* ab = obj->asArrayBufferObject();
            originalObj->resourceRequest()->setBinaryRequestBody(
                (const char*)ab->rawBuffer(), ab->byteLength());
            try {
                originalObj->send(String::emptyString);
            } catch (DOMException* e) {
                state->throwException(e->scriptValue());
                STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            }
            return ValueRef::createUndefined();
        }
        if (obj->isArrayBufferView()) {
            ArrayBufferViewRef* view = obj->asArrayBufferView();
            originalObj->resourceRequest()->setBinaryRequestBody(
                (const char*)view->rawBuffer(), view->byteLength());
            try {
                originalObj->send(String::emptyString);
            } catch (DOMException* e) {
                state->throwException(e->scriptValue());
                STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            }
            return ValueRef::createUndefined();
        }
    }

    Optional<String*> value0;
    if (!arg0->isUndefinedOrNull()) {
        value0 = toBrowserString(state, arg0);
    }
    try {
        originalObj->send(value0);
    } catch (DOMException* e) {
        state->throwException(e->scriptValue());
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    return ValueRef::createUndefined();
}

} // namespace Starfish
