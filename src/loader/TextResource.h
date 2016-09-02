/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishTextResource__
#define __StarFishTextResource__

#include "loader/Resource.h"
#include "util/TextConverter.h"
#include "platform/network/NetworkRequest.h"

namespace StarFish {

class TextResource : public Resource {
    friend class ResourceLoader;
    TextResource(URL* url, ResourceLoader* loader, String* preferredEncoding)
        : Resource(url, loader)
        , m_converter(nullptr)
        , m_preferredEncoding(preferredEncoding)
        , m_text(String::emptyString)
    {
    }
public:
    virtual bool isTextResource()
    {
        return true;
    }

    virtual void didDataReceived(const char* buffer, size_t length);

    virtual size_t contentSize()
    {
        return m_text->isASCIIString() ? m_text->length() : 4 * m_text->length();
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
        return m_converter ? m_converter->encoding() : String::createASCIIString("UTF-8");
    }

protected:
    TextConverter* m_converter;
    String* m_preferredEncoding;
    String* m_text;
};
}

#endif
