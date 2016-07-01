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

namespace StarFish {

class TextResource : public Resource {
    friend class ResourceLoader;
    TextResource(const URL& url, ResourceLoader* loader)
        : Resource(url, loader)
        , m_text(String::emptyString)
    {
    }
public:
    virtual bool isTextResource()
    {
        return true;
    }

    virtual void didDataReceived(const char* buffer, size_t length)
    {
        Resource::didDataReceived(buffer, length);
        m_text = m_text->concat(String::fromUTF8(buffer, length));
    }

    String* text()
    {
        return m_text;
    }

protected:
    // TextEncoding m_textEncoding;
    String* m_text;
};
}

#endif
