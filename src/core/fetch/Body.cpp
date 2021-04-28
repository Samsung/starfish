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
#include "core/fetch/Body.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/dom/DOMException.h"
#include "core/util/URL.h"
#include "core/fileapi/Blob.h"
#include "core/fetch/stream/ReadableStream.h"
#include "core/fetch/stream/ReadableStreamBuffer.h"
#include "core/fetch/stream/ReadableStreamDefaultReader.h"

namespace Starfish {

// TODO: find where the mine type should be placed
static const char kTextPlainContentType[] = "text/plain;charset=UTF-8";

Body::Body(ExecutionContext* executionContext)
    : m_executionContext(executionContext)
    , m_bodyInit(nullptr)
    , m_contentType(String::emptyString)
    , m_resourceRequest(nullptr)
    , m_readableStream(nullptr)
    , m_promise(new Promise(executionContext->scriptBindingInstance()))
{
}

Body::Body(ExecutionContext* executionContext, Nullable<BodyInit>& body)
    : Body(executionContext)
{
    if (body.hasValue()) {
        auto bodyValue = body.getValue();
        if (bodyValue.isReadableStreamValue()) {
            m_readableStream = bodyValue.getReadableStreamValue();
        }
    }
}

void Body::createReadableStream()
{
    if (m_readableStream == nullptr) {
        m_readableStream = new ReadableStream(executionContext());
    }
}

bool Body::bodyUsed()
{
    return m_readableStream ? m_readableStream->disturbed() : false;
}

Promise* Body::arrayBuffer()
{
    createReadableStream();

    if (m_readableStream->isDisturbedOrLocked()) {
        auto error =
            scriptTypeError(executionContext()->scriptBindingInstance(),
                            String::fromUTF8("Body is locked"));

        m_promise->reject(createScriptValue(error));
    } else {
        if (m_bodyInit.hasValue()) {
            m_readableStream->lock();

            BodyInit body = m_bodyInit.getValue();

            if (body.isUSVStringValue()) {
                auto value = body.getUSVStringValue();
                auto str = value->toUTF8NonGCString();
                void* buffer = calloc(1, str.length());
                memcpy(buffer, str.data(), str.length());
                auto scriptArrayBuffer = createScriptArrayBuffer(
                    executionContext()->scriptBindingInstance(), buffer,
                    value->length());

                m_promise->fulfill(createScriptValue(scriptArrayBuffer));
            } else if (body.isArrayBufferViewOrArrayBufferValue()) {
                auto arrayValue = body.getArrayBufferViewOrArrayBufferValue();
                // FIXME getting ArrayBuffer of ArrayBufferView
                ScriptValue value =
                    arrayValue.isArrayBufferValue()
                        ? createScriptValue(arrayValue.getArrayBufferValue())
                        : createScriptValue(
                              arrayValue.getArrayBufferViewValue());
                m_promise->fulfill(value);
            } else {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
        } else {
            m_readableStream->resolveData(m_promise, executionContext(),
                                          BodyType::ArrayBuffer);
        }
    }

    m_readableStream->close();
    return m_promise;
}

Promise* Body::blob()
{
    createReadableStream();

    if (m_readableStream->isDisturbedOrLocked()) {
        auto error =
            scriptTypeError(executionContext()->scriptBindingInstance(),
                            String::fromUTF8("Body is locked"));

        m_promise->reject(createScriptValue(error));
    } else {
        if (m_bodyInit.hasValue()) {
            m_readableStream->lock();

            BodyInit body = m_bodyInit.getValue();

            if (body.isUSVStringValue()) {
                auto value = body.getUSVStringValue();
                auto str = value->toUTF8NonGCString();
                void* buffer = calloc(1, str.length());
                memcpy(buffer, str.data(), str.length());

                auto blob = new Blob(executionContext(), value->length(),
                                     contentType(), buffer, false, false, true);

                m_promise->fulfill(blob->scriptValue());
            } else {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
        } else {
            m_readableStream->resolveData(m_promise, executionContext(),
                                          BodyType::Blob);
        }
    }

    m_readableStream->close();
    return m_promise;
}

Promise* Body::json()
{
    createReadableStream();

    if (m_readableStream->isDisturbedOrLocked()) {
        auto error =
            scriptTypeError(executionContext()->scriptBindingInstance(),
                            String::fromUTF8("Body is locked"));

        m_promise->reject(createScriptValue(error));
    } else {
        if (m_bodyInit.hasValue()) {
            m_readableStream->lock();

            BodyInit body = m_bodyInit.getValue();

            if (body.isUSVStringValue()) {
                ScriptValue jsonObject =
                    parseJSON(executionContext()->scriptBindingInstance(),
                              body.getUSVStringValue());
                m_promise->fulfill(jsonObject);
            } else {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
        } else {
            m_readableStream->resolveData(m_promise, executionContext(),
                                          BodyType::Json);
        }
    }

    m_readableStream->close();
    return m_promise;
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
    createReadableStream();

    if (m_readableStream->isDisturbedOrLocked()) {
        auto error =
            scriptTypeError(executionContext()->scriptBindingInstance(),
                            String::fromUTF8("Body is locked"));

        m_promise->reject(createScriptValue(error));
    } else {
        if (m_bodyInit.hasValue()) {
            m_readableStream->lock();

            BodyInit body = m_bodyInit.getValue();
            if (body.isUSVStringValue()) {
                m_promise->fulfill(createScriptValue(body.getUSVStringValue()));

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
                m_promise->fulfill(createScriptValue(text));

            } else if (body.isBlobValue()) {
                String* url = URL::createObjectURL(body.getBlobValue());
                if (!m_resourceRequest) {
                    m_resourceRequest = new ResourceRequest(executionContext());
                }
                m_resourceRequest->addResourceRequestClient(this);

                RequestData* reqData = new RequestData();
                reqData->m_url = new ResourceURL(
                    url, executionContext()->baseURL()->baseURI());
                reqData->m_syncLevel = RequestSyncLevel::NeverSync;
                m_resourceRequest->open(reqData);
                m_resourceRequest->send();
            } else {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
        } else {
            m_readableStream->resolveData(m_promise, executionContext(),
                                          BodyType::Text);
        }
    }

    m_readableStream->close();
    return m_promise;
}

// NOTE: consider creating `ResourceRequestClient` class for Body
void Body::onProgressEvent(ResourceRequest* request, bool isExplicitAction)
{
    ProgressState progState = request->progressState();

    if (progState == ProgressState::InError) {
        auto error =
            scriptTypeError(executionContext()->scriptBindingInstance(),
                            String::fromUTF8("Body is locked"));

        m_promise->reject(createScriptValue(error));
    }
}

void Body::onReadyStateChange(ResourceRequest* request, bool fromExplicit)
{
    if (fromExplicit) {
        if (request->readyState() == ReadyState::Done) {
            BodyInit body = m_bodyInit.getValue();

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

Nullable<BodyInit> Body::bodyInit() const
{
    // TODO: in case that m_bodyUsed is True
    return m_bodyInit;
}

void Body::setBodyInit(const Nullable<BodyInit>& bodyInitValue)
{
    m_bodyInit = bodyInitValue;

    // extract Body : https://fetch.spec.whatwg.org/#body-mixin
    createReadableStream();
    m_readableStream->releaseLock();

    if (bodyInitValue.hasValue()) {
        auto bodyInit = bodyInitValue.getValue();
        if (bodyInit.isUSVStringValue()) {
            m_contentType = String::createASCIIString(kTextPlainContentType);
        } else if (bodyInit.isBlobValue()) {
            m_contentType = bodyInit.getBlobValue()->type();
        }
    }
}

void Body::pushResponseData(ResourceRequest* request)
{
    createReadableStream();
    auto response = request->response();
    auto responseType = request->bodyType();

    auto streamBuffer = m_readableStream->streamBuffer();
    streamBuffer->setType(responseType);
    streamBuffer->setMimeType(request->responseMimeType());
    streamBuffer->push(response.data(), response.size());
}

void Body::copyBody(Body* body)
{
    m_contentType = body->contentType();

    auto srcBody = body->bodyInit();
    if (srcBody.hasValue()) {
        BodyInit srcBodyValue = srcBody.getValue();
        if (srcBodyValue.isUSVStringValue()) {
            m_bodyInit =
                BodyInit::createUSVString(srcBodyValue.getUSVStringValue());
        } else {
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
    }
}
}; // namespace Starfish
