#ifndef __StarFishResource__
#define __StarFishResource__

#include "loader/Resource.h"
#include "loader/ResourceClient.h"

namespace StarFish {

class TextResource;
class ImageResource;
class ResourceLoader;

class Resource : public gc {
public:
    enum State {
        BeforeSend,
        Receiving,
        Finished,
        Failed,
        Canceled,
    };

    virtual ~Resource()
    {

    }

    virtual bool isTextResource()
    {
        return false;
    }

    virtual bool isImageResource()
    {
        return false;
    }

    TextResource* asTextResource()
    {
        STARFISH_ASSERT(isTextResource());
        return (TextResource*)this;
    }

    ImageResource* asImageResource()
    {
        STARFISH_ASSERT(isImageResource());
        return (ImageResource*)this;
    }

    void addResourceClient(ResourceClient* rc)
    {
        m_resourceClients.push_back(rc);
    }

    void addResourceClient(const ResourceClientVector& rc)
    {
        m_resourceClients.insert(m_resourceClients.end(), rc.begin(), rc.end());
    }

    void removeResourceClient(ResourceClient* rc)
    {
        m_resourceClients.erase(std::find(m_resourceClients.begin(), m_resourceClients.end(), rc));
    }

    const URL& url()
    {
        return m_url;
    }

    virtual void request(bool needsSyncRequest = false);
    virtual void cancel();

    virtual void didDataReceived(const char*, size_t length)
    {
        m_state = Receiving;
    }

    virtual void didLoadFinished()
    {
        m_state = Finished;
        auto iter = m_resourceClients.begin();
        while (iter != m_resourceClients.end()) {
            (*iter)->didLoadFinished();
            iter++;
        }
    }

    virtual void didLoadFailed()
    {
        m_state = Failed;
        auto iter = m_resourceClients.begin();
        while (iter != m_resourceClients.end()) {
            (*iter)->didLoadFailed();
            iter++;
        }
    }

    virtual void didLoadCanceled();
    ResourceLoader* loader()
    {
        return m_loader;
    }
protected:
    Resource(const URL& url, ResourceLoader* loader)
        : m_state(BeforeSend)
        , m_url(url)
        , m_loader(loader)
    {
    }

    void pushIdlerHandle(void* handle)
    {
        m_requstedIdlers.push_back(handle);
    }

    State m_state;
    URL m_url;
    ResourceLoader* m_loader;
    ResourceClientVector m_resourceClients;
    std::vector<void*, gc_allocator<void*>> m_requstedIdlers;
};
}

#endif
