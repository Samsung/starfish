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

#ifndef __StarFishElementResourceClient__
#define __StarFishElementResourceClient__

#include "platform/loader/ResourceClient.h"

namespace StarFish {

class Element;
class ElementResourceClient : public ResourceClient {
public:
    ElementResourceClient(Element* element, Resource* resource,
                          bool needsSyncEventDispatch = false)
        : ResourceClient(resource)
        , m_needsSyncEventDispatch(needsSyncEventDispatch)
        , m_element(element)
    {
    }
    virtual void didLoadFinished();
    virtual void didLoadFailed();

protected:
    bool m_needsSyncEventDispatch;
    Element* m_element;
};
}

#endif
