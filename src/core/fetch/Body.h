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

#ifndef __StarFishFetchBody__
#define __StarFishFetchBody__

#include "binding/ScriptWrappable.h"
#include "binding/WindowHoldable.h"
#include "core/fetch/GetSet.h"
#include "binding/BlobOrBufferSourceOrUSVStringUnion.h"
#include "core/modules/resource_request/ResourceRequest.h"

namespace StarFish {

typedef BlobOrBufferSourceOrUSVString BodyInit;
class DOMException;

class Body : public ResourceRequestClient, public WindowHoldable {
public:
    Promise* arrayBuffer();
    Promise* blob();
    Promise* formData();
    Promise* json();
    Promise* text();

    bool bodyUsed() const
    {
        return m_bodyUsed;
    };

    Nullable<BodyInit> body() const;
    void setBody(const BodyInit& body);

    String* contentType() const
    {
        return m_contentType;
    }

    void copyBody(Body* body);

    void onProgressEvent(ResourceRequest* request, bool isExplicitAction);
    void onReadyStateChange(ResourceRequest* request, bool fromExplicit);

private:
    bool m_bodyUsed;

protected:
    Body(Window* window)
        : WindowHoldable(window)
        , m_bodyUsed(false)
        , m_contentType(nullptr)
        , m_resourceRequest(nullptr)
        , m_promise(nullptr)
    {
    }

    void setBodyUsed(bool isUsed)
    {
        m_bodyUsed = isUsed;
    };

    Nullable<BodyInit> m_body;
    String* m_contentType;
    ResourceRequest* m_resourceRequest;
    Promise* m_promise;
};
}

#endif
