#include "StarFishConfig.h"
#include "dom/Document.h"
#include "ResourceLoader.h"

#include "platform/message_loop/MessageLoop.h"
#include "platform/file_io/FileIO.h"

namespace StarFish {

ResourceLoader::ResourceLoader(Document& document)
    : m_inDocumentOpenState(false)
    , m_pendingResourceCountWhileDocumentOpening(0)
    , m_document(&document)
    , m_baseURL(String::emptyString, document.documentURI().baseURI())
{

}

TextResource* ResourceLoader::fetchText(const URL& url)
{
    TextResource* res = new TextResource(url, this);
    return res;
}

ImageResource* ResourceLoader::fetchImage(const URL& url)
{
    ImageResource* res = new ImageResource(url, this);
    return res;
}

class DocumentOnLoadChecker : public ResourceClient {
public:
    DocumentOnLoadChecker(Resource* res)
        : ResourceClient(res)
    {
    }
    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        checkFire();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        checkFire();
    }

    virtual void didLoadCanceled()
    {
        ResourceClient::didLoadCanceled();
        checkFire();
    }

    void checkFire()
    {
        STARFISH_ASSERT(m_resource->loader()->m_pendingResourceCountWhileDocumentOpening > 0);
        m_resource->loader()->m_pendingResourceCountWhileDocumentOpening--;
        m_resource->loader()->fireDocumentOnLoadEventIfNeeded();
    }
};

class ResourceAliveChecker : public ResourceClient {
public:
    ResourceAliveChecker(Resource* res)
        : ResourceClient(res)
    {
        std::vector<Resource*, gc_allocator<Resource*>>& v =  m_resource->loader()->m_currentLoadingResources;
        v.push_back(m_resource);
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        clearAlive();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        clearAlive();
    }

    virtual void didLoadCanceled()
    {
        ResourceClient::didLoadCanceled();
        clearAlive();
    }

    void clearAlive()
    {
        std::vector<Resource*, gc_allocator<Resource*>>& v =  m_resource->loader()->m_currentLoadingResources;
        STARFISH_ASSERT(std::find(v.begin(), v.end(), m_resource) != v.end());
        v.erase(std::find(v.begin(), v.end(), m_resource));
    }
};

void ResourceLoader::fetchResourcePreprocess(Resource* res)
{
    if (m_inDocumentOpenState) {
        m_pendingResourceCountWhileDocumentOpening++;
        res->addResourceClient(new DocumentOnLoadChecker(res));
        res->addResourceClient(new ResourceAliveChecker(res));
    }
}

void ResourceLoader::fireDocumentOnLoadEventIfNeeded()
{
    if (m_pendingResourceCountWhileDocumentOpening == 0 && m_inDocumentOpenState) {
        m_inDocumentOpenState = false;
        m_document->window()->dispatchLoadEvent();
    }
}

void ResourceLoader::cancelAllOfPendingRequests()
{
    std::vector<Resource*, gc_allocator<Resource*>>& v = m_currentLoadingResources;
    while (v.size()) {
        v[0]->cancel();
    }
}

}
