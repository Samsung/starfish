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

#ifndef __StarFishResourceClient__
#define __StarFishResourceClient__

namespace StarFish {

class Resource;
class ResourceClient;

typedef std::vector<ResourceClient*, gc_allocator<ResourceClient*>> ResourceClientVector;

class ResourceClient : public gc {
public:
    ResourceClient(Resource* resource)
        : m_resource(resource)
    {
    }

    virtual ~ResourceClient()
    {

    }

    virtual void didLoadFinished()
    {

    }

    virtual void didLoadFailed()
    {

    }

    virtual void didLoadCanceled()
    {

    }

    virtual void didDataReceived(const char*, size_t length)
    {

    }

    Resource* resource()
    {
        return m_resource;
    }

protected:
    Resource* m_resource;
};
}

#endif
