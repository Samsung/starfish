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

class BodyResourceRequestClient : public ResourceRequestClient {
public:
    BodyResourceRequestClient(Body* body)
        : m_body(body)
    {
    }

    // NOTE: consider creating `ResourceRequestClient` class for Body
    virtual void onProgressEvent(ResourceRequest* request,
                                 bool isExplicitAction) override
    {
        ProgressState progState = request->progressState();

        if (progState == ProgressState::InError) {
            auto error = scriptTypeError(
                m_body->executionContext()->scriptBindingInstance(),
                String::fromUTF8("Body is locked"));

            m_body->m_promise->reject(createScriptValue(error));
        }
    }

    virtual void onReadyStateChange(ResourceRequest* request,
                                    bool fromExplicit) override
    {
        if (fromExplicit) {
            if (request->readyState() == ReadyState::Done) {
                BodyInit body = m_body->m_bodyInit.getValue();

                if (body.isBlobValue()) {
                    String* text = String::fromUTF8(
                        m_body->m_resourceRequest->response().data(),
                        m_body->m_resourceRequest->response().size());
                    m_body->m_promise->fulfill(createScriptValue(text));
                    m_body->m_resourceRequest->response().clear();
                    m_body->m_resourceRequest->response().shrink_to_fit();
                }
            }
        }
    }

private:
    Body* m_body;
};

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

bool Body::bodyDisturbedOrLocked()
{
    return m_readableStream ? m_readableStream->isDisturbedOrLocked() : false;
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
                STARFISH_UNIMPLEMENTED();
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
                STARFISH_UNIMPLEMENTED();
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
                STARFISH_UNIMPLEMENTED();
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
    STARFISH_UNIMPLEMENTED();
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
            if (body.isUSVStringValue() ||
                body.isArrayBufferViewOrArrayBufferValue()) {
                String* text = extractTextFromBodyInit();
                m_promise->fulfill(createScriptValue(text));
            } else if (body.isBlobValue()) {
                String* url = URL::createObjectURL(body.getBlobValue());
                if (!m_resourceRequest) {
                    m_resourceRequest = new ResourceRequest(executionContext());
                }
                m_resourceRequest->addResourceRequestClient(
                    new BodyResourceRequestClient(this));

                RequestData* reqData = new RequestData();
                reqData->m_url = new ResourceURL(
                    url, executionContext()->baseURL()->baseURI());
                reqData->m_syncLevel = RequestSyncLevel::NeverSync;
                m_resourceRequest->open(reqData, new HeadersData());
                m_resourceRequest->send();
            } else {
                STARFISH_UNIMPLEMENTED();
            }
        } else {
            m_readableStream->resolveData(m_promise, executionContext(),
                                          BodyType::Text);
        }
    }
    m_readableStream->close();
    return m_promise;
}

ArrayBuffer* Body::extractArrayBuffer()
{
    STARFISH_UNIMPLEMENTED();
    return nullptr;
}

ArrayBufferView* Body::extractArrayBufferView()
{
    STARFISH_UNIMPLEMENTED();
    return nullptr;
}

Blob* Body::extractBlob()
{
    STARFISH_UNIMPLEMENTED();
    return nullptr;
}

String* Body::extractText()
{
    String* result = String::emptyString;
    createReadableStream();
    if (m_readableStream->isDisturbedOrLocked()) {
        return result;
    }
    m_readableStream->lock();
    result = extractTextFromBodyInit();
    m_readableStream->close();
    return result;
}

String* Body::extract()
{
    String* result = String::emptyString;
    if (isTextType()) {
        // Currently, it is possible only in case of text because resource
        // request supports only string type.
        result = extractText();
    } else if (isArrayBufferType()) {
        STARFISH_UNIMPLEMENTED();
    } else if (isArrayBufferViewType()) {
        STARFISH_UNIMPLEMENTED();
    } else if (isBlobType()) {
        STARFISH_UNIMPLEMENTED();
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
    return result;
}

String* Body::extractTextFromBodyInit()
{
    if (m_bodyInit.hasValue()) {
        BodyInit body = m_bodyInit.getValue();
        if (body.isUSVStringValue()) {
            return body.getUSVStringValue();
        } else if (body.isArrayBufferViewOrArrayBufferValue()) {
            auto byteBuffer = body.getArrayBufferViewOrArrayBufferValue();
            if (byteBuffer.isArrayBufferValue()) {
                auto arrayBuffer = byteBuffer.getArrayBufferValue();
                auto buffer = arrayBufferRawData(arrayBuffer);
                auto size = arrayBufferByteSize(arrayBuffer);
                return String::fromUTF8((const char*)buffer, size);
            } else {
                auto arrayBufferView = byteBuffer.getArrayBufferViewValue();
                auto buffer = arrayBufferViewRawData(arrayBufferView);
                auto size = arrayBufferViewByteSize(arrayBufferView);
                return String::fromUTF8((const char*)buffer, size);
            }
        }
    }
    return String::emptyString;
}

Nullable<BodyInit> Body::bodyInit() const
{
    // TODO: in case that m_bodyUsed is True
    return m_bodyInit;
}

void Body::setBodyInit(const Nullable<BodyInit>& bodyInitValue)
{
    m_bodyInit = bodyInitValue;

    if (!m_bodyInit.hasValue()) {
        m_readableStream = nullptr;
        return;
    }

    // extract Body : https://fetch.spec.whatwg.org/#body-mixin
    createReadableStream();
    m_readableStream->releaseLock();

    auto bodyInit = m_bodyInit.getValue();
    if (bodyInit.isUSVStringValue()) {
        m_contentType = String::createASCIIString(kTextPlainContentType);
    } else if (bodyInit.isBlobValue()) {
        m_contentType = bodyInit.getBlobValue()->type();
    } else {
        STARFISH_UNIMPLEMENTED();
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
    auto contentType = body->contentType()->toUTF8NonGCString();
    m_contentType = String::fromUTF8(contentType.data(), contentType.size());

    auto srcBody = body->bodyInit();
    if (srcBody.hasValue()) {
        BodyInit srcBodyValue = srcBody.getValue();
        if (srcBodyValue.isUSVStringValue()) {
            auto bodyValueString =
                srcBodyValue.getUSVStringValue()->toUTF8NonGCString();
            m_bodyInit = BodyInit::createUSVString(String::fromUTF8(
                bodyValueString.data(), bodyValueString.size()));
        } else {
            STARFISH_UNIMPLEMENTED();
        }
    } else {
        if (body->body()) {
            createReadableStream();
            auto srcBuffer = body->body()->streamBuffer();
            auto destBuffer = m_readableStream->streamBuffer();

            if (destBuffer->size() > 0) {
                destBuffer->clear();
            }

            destBuffer->push(srcBuffer->data(), srcBuffer->size());
            destBuffer->setType(srcBuffer->type());
            destBuffer->setMimeType(srcBuffer->mineType());
        }
    }
}

bool Body::isArrayBufferType()
{
    if (m_bodyInit.hasValue()) {
        BodyInit body = m_bodyInit.getValue();
        if (body.isArrayBufferViewOrArrayBufferValue()) {
            if (body.getArrayBufferViewOrArrayBufferValue()
                    .isArrayBufferValue()) {
                return true;
            }
        }
    }
    return false;
}

bool Body::isArrayBufferViewType()
{
    if (m_bodyInit.hasValue()) {
        BodyInit body = m_bodyInit.getValue();
        if (body.isArrayBufferViewOrArrayBufferValue()) {
            if (body.getArrayBufferViewOrArrayBufferValue()
                    .isArrayBufferViewValue()) {
                return true;
            }
        }
    }
    return false;
}

bool Body::isBlobType()
{
    if (m_bodyInit.hasValue() && m_bodyInit.getValue().isBlobValue()) {
        return true;
    }
    return false;
}

bool Body::isTextType()
{
    String* contentTypeStr = contentType();
    if (contentTypeStr) {
        return contentTypeStr->equals(kTextPlainContentType);
    }
    return false;
}
}; // namespace Starfish
