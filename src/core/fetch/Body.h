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

#ifndef __StarfishFetchBody__
#define __StarfishFetchBody__

#include "binding/ScriptWrappable.h"
#include "binding/BlobOrBufferSourceOrUSVStringOrReadableStreamUnion.h"
#include "core/modules/resource_request/ResourceRequest.h"

namespace Starfish {

typedef BlobOrBufferSourceOrUSVStringOrReadableStream BodyInit;

class DOMException;
class ReadableStream;

enum class BodyType {
    Empty,
    ArrayBuffer,
    Blob,
    Document,
    Json,
    Text,
};

class Body : public ResourceRequestClient {
public:
    Promise* arrayBuffer();
    Promise* blob();
    Promise* formData();
    Promise* json();
    Promise* text();

    bool bodyUsed();

    Nullable<BodyInit> bodyInit() const;
    void setBodyInit(const Nullable<BodyInit>& bodyInit);
    void pushResponseData(ResourceRequest* request);

    String* contentType() const
    {
        return m_contentType;
    }

    void copyBody(Body* body);

    void onProgressEvent(ResourceRequest* request, bool isExplicitAction);
    void onReadyStateChange(ResourceRequest* request, bool fromExplicit);

    void createReadableStream();
    ReadableStream* body()
    {
        return m_readableStream;
    }

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

private:
protected:
    Body(ExecutionContext* executionContext);
    Body(ExecutionContext* executionContext, Nullable<BodyInit>& bodyInitValue);

    ExecutionContext* m_executionContext;
    Nullable<BodyInit> m_bodyInit;
    String* m_contentType;
    ResourceRequest* m_resourceRequest;
    ReadableStream* m_readableStream;
    Promise* m_promise;
};
} // namespace Starfish

#endif
