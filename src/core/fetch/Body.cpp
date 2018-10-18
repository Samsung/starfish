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
#include "core/util/URL.h"
#include "core/fileapi/Blob.h"

namespace StarFish {

// TODO: find where the mine type should be placed
static const char kTextPlainContentType[] = "text/plain;charset=UTF-8";

Promise* Body::arrayBuffer()
{
    Promise* promise = new Promise(scriptBindingInstance());

    if (m_bodyUsed) {
        auto error = scriptTypeError(scriptBindingInstance(),
                                     String::fromUTF8("Body is locked"));

        promise->reject(createScriptValue(error));
    } else {
        if (m_body.hasValue()) {
            setBodyUsed(true);
            m_promise = promise;

            BodyInit body = m_body.getValue();

            if (body.isUSVStringValue()) {
                auto value = body.getUSVStringValue();
                auto str = value->toUTF8NonGCString();
                void* buffer = calloc(1, str.length());
                memcpy(buffer, str.data(), str.length());
                auto ab = createArrayBuffer(scriptBindingInstance(), buffer,
                                            value->length());

                promise->fulfill(ab);
            } else {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
        } else {
            promise->fulfill(createScriptValue(String::emptyString));
        }
    }

    return promise;
}

Promise* Body::blob()
{
    Promise* promise = new Promise(scriptBindingInstance());

    if (m_bodyUsed) {
        auto error = scriptTypeError(scriptBindingInstance(),
                                     String::fromUTF8("Body is locked"));

        promise->reject(createScriptValue(error));
    } else {
        if (m_body.hasValue()) {
            setBodyUsed(true);
            m_promise = promise;

            BodyInit body = m_body.getValue();

            if (body.isUSVStringValue()) {
                auto value = body.getUSVStringValue();
                auto str = value->toUTF8NonGCString();
                void* buffer = calloc(1, str.length());
                memcpy(buffer, str.data(), str.length());

                auto blob = new Blob(scriptBindingInstance()->ownerDocument(),
                                     value->length(), contentType(), buffer,
                                     false, false);

                promise->fulfill(blob->scriptValue());
            } else {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
        } else {
            promise->fulfill(createScriptValue(String::emptyString));
        }
    }

    return promise;
}

Promise* Body::json()
{
    Promise* promise = new Promise(scriptBindingInstance());

    if (m_bodyUsed) {
        auto error = scriptTypeError(scriptBindingInstance(),
                                     String::fromUTF8("Body is locked"));

        promise->reject(createScriptValue(error));
    } else {
        if (m_body.hasValue()) {
            setBodyUsed(true);
            m_promise = promise;

            BodyInit body = m_body.getValue();

            if (body.isUSVStringValue()) {
                ScriptValue jsonObject = parseJSON(scriptBindingInstance(),
                                                   body.getUSVStringValue());
                promise->fulfill(jsonObject);
            } else {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
        } else {
            promise->fulfill(createScriptValue(String::emptyString));
        }
    }

    return promise;
}

Promise* Body::formData()
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
            setBodyUsed(true);
            m_promise = promise;

            BodyInit body = m_body.getValue();
            if (body.isUSVStringValue()) {
                promise->fulfill(createScriptValue(body.getUSVStringValue()));

            } else if (body.isArrayBufferViewOrArrayBufferValue()) {
                auto byteBuffer = body.getArrayBufferViewOrArrayBufferValue();
                String* text;
                if (byteBuffer.isArrayBufferValue()) {
                    auto arrayBuffer = byteBuffer.getArrayBufferValue();
                    auto buffer = arrayBufferRawData(arrayBuffer);
                    auto size = arrayBufferSize(arrayBuffer);
                    text = String::fromUTF8((const char*)buffer, size);
                } else {
                    // ArrayBufferView case
                    auto arrayBufferView = byteBuffer.getArrayBufferViewValue();
                    auto buffer = arrayBufferViewRawData(arrayBufferView);
                    auto size = arrayBufferViewSize(arrayBufferView);
                    text = String::fromUTF8((const char*)buffer, size);
                }
                promise->fulfill(createScriptValue(text));

            } else if (body.isBlobValue()) {
                String* url = URL::createObjectURL(body.getBlobValue());
                ResourceURL* resUrl =
                    new ResourceURL(url, document()->baseURL()->baseURI());
                if (!m_resourceRequest) {
                    m_resourceRequest = new ResourceRequest(document());
                }
                m_resourceRequest->addResourceRequestClient(this);
                m_resourceRequest->open(ResourceRequest::GET_METHOD, resUrl,
                                        true, document()->documentURI(),
                                        String::emptyString,
                                        String::emptyString);
                m_resourceRequest->send();
            } else {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
        } else {
            promise->fulfill(createScriptValue(String::emptyString));
        }
    }

    return promise;
}

// NOTE: consider creating `ResourceRequestClient` class for Body
void Body::onProgressEvent(ResourceRequest* request, bool isExplicitAction)
{
    ResourceRequest::ProgressState progState = request->progressState();

    if (progState == ResourceRequest::IN_ERROR) {
        auto error = scriptTypeError(scriptBindingInstance(),
                                     String::fromUTF8("Body is locked"));

        m_promise->reject(createScriptValue(error));
    }
}

void Body::onReadyStateChange(ResourceRequest* request, bool fromExplicit)
{
    if (fromExplicit) {
        if (request->readyState() == ResourceRequest::ReadyState::DONE) {
            BodyInit body = m_body.getValue();

            if (body.isBlobValue()) {
                String* text =
                    String::fromUTF8(m_resourceRequest->response().data(),
                                     m_resourceRequest->response().size());
                m_promise->fulfill(createScriptValue(text));
                m_resourceRequest->response().clear();
                m_resourceRequest->response().shrink_to_fit();
            }
        }
    }
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
    } else if (body.isBlobValue()) {
        m_contentType = body.getBlobValue()->type();
    }
}

void Body::copyBody(Body* body)
{
    if (body->contentType()) {
        m_contentType = String::createASCIIString(CSTR(body->contentType()));
    }

    auto srcBody = body->body();
    if (srcBody.hasValue()) {
        BodyInit srcBodyValue = srcBody.getValue();
        if (srcBodyValue.isUSVStringValue()) {
            m_body =
                BodyInit::createUSVString(srcBodyValue.getUSVStringValue());
        } else {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
    }
}
};
