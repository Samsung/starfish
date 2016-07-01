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

#ifndef __StarFishElementResourceClient__
#define __StarFishElementResourceClient__

#include "loader/ResourceClient.h"

namespace StarFish {

class Element;
class ElementResourceClient : public ResourceClient {
public:
    ElementResourceClient(Element* element, Resource* resource, bool needsSyncEventDispatch = false)
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
