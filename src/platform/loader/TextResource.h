/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishTextResource__
#define __StarFishTextResource__

#include "platform/loader/Resource.h"
#include "core/util/TextConverter.h"
#include "core/modules/resource_request/ResourceRequest.h"

namespace StarFish {

class TextResource : public Resource {
    friend class ResourceLoader;
    TextResource(ResourceURL* url, ResourceLoader* loader,
                 String* preferredEncoding)
        : Resource(url, loader)
        , m_converter(nullptr)
        , m_preferredEncoding(preferredEncoding)
        , m_text(String::emptyString)
    {
    }

public:
    virtual void prepare()
    {
        if (m_resourceRequest) {
            m_resourceRequest->setRequestHeader(
                String::createASCIIString(HTTPHeaderMap::kAccept),
                String::createASCIIString(
                    "text/html,text/plain,text/javascript,text/"
                    "ecmascript,application/x-javascript,text/*"));
        }
    }

    virtual bool isTextResource()
    {
        return true;
    }

    virtual void didDataReceived(const char* buffer, size_t length);

    virtual size_t contentSize()
    {
        return m_text->contentLength();
    }

    virtual Type type()
    {
        return Type::TextResourceType;
    }

    String* text()
    {
        return m_text;
    }

    String* characterEncoding()
    {
        return m_converter ? m_converter->encoding()
                           : String::createASCIIString("UTF-8");
    }

protected:
    TextConverter* m_converter;
    String* m_preferredEncoding;
    String* m_text;
};
}

#endif
