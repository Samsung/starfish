/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishWebOrigin__
#define __StarfishWebOrigin__

namespace Starfish {

class ResourceURL;

class WebOrigin : public gc {
protected:
    WebOrigin();
    WebOrigin(ResourceURL* url, bool isOpaque);

public:
    static WebOrigin* createDocumentOrigin(ResourceURL* url);
    String* serialize() const;
    Nullable<String*> domain() const;
    Nullable<ResourceURL*> url() const
    {
        return m_originalURL;
    }
    bool isOpaque() const
    {
        return m_isOpaque;
    }

    bool isSameOrigin(const WebOrigin* otherWebOrigin) const;
    bool isSameOriginDomain(const WebOrigin* otherWebOrigin) const;

protected:
    Nullable<ResourceURL*> m_originalURL;
    bool m_isOpaque;
};

struct WebOriginHash {
    size_t operator()(const WebOrigin* data) const
    {
        size_t seed = 0;
        return seed ^ std::hash<String*>{}(data->serialize());
    }
};

struct WebOriginEqual {
    bool operator()(const WebOrigin* data1, const WebOrigin* data2) const
    {
        return data1 == data2 ? true : data1->isSameOrigin(data2);
    }
};
} // namespace Starfish

#endif
