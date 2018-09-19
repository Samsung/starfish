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
#include "core/fetch/Body.h"
#include "core/dom/Document.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/dom/DOMException.h"

namespace StarFish {

// TODO: find where the mine type should be placed
static const char kTextPlainContentType[] = "text/plain;charset=UTF-8";

Promise* Body::arrayBuffer()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

Promise* Body::blob()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

Promise* Body::json()
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}

struct BodyPromiseHandle : public gc {
    Body* body;
    Promise* promise;
    BodyPromiseHandle(Body* b, Promise* p)
    {
        body = b;
        promise = p;
    }
};

Promise* Body::text()
{
    Promise* promise = new Promise(scriptBindingInstance());

    if (m_bodyUsed) {
        auto error = scriptTypeError(scriptBindingInstance(),
                                     String::fromUTF8("Body is locked"));

        promise->reject(createScriptValue(error));
    } else {
        if (m_body.hasValue()) {
            m_bodyUsed = true;
            BodyInit body = m_body.getValue();
            if (body.isUSVStringValue()) {
                // Check if a string from bindings is encoded in UTF8
                promise->fulfill(createScriptValue(body.getUSVStringValue()));
            }
        } else {
            promise->fulfill(createScriptValue(String::emptyString));
        }
    }

    return promise;
}

Nullable<BodyInit> Body::body() const
{
    // TODO: in case that m_bodyUsed is True
    return m_body;
}

void Body::setBody(const BodyInit& body)
{
    m_body = body;

    // extract Body : https://fetch.spec.whatwg.org/#body-mixin
    if (body.isUSVStringValue()) {
        m_contentType = String::createASCIIString(kTextPlainContentType);
    }
}
};
