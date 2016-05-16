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

    Resource* resource()
    {
        return m_resource;
    }

protected:
    Resource* m_resource;
};
}

#endif
