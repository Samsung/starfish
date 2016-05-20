#include "StarFishConfig.h"
#include "Resource.h"

#include "platform/message_loop/MessageLoop.h"
#include "platform/file_io/FileIO.h"
#include "dom/Document.h"
#include "loader/ResourceLoader.h"
#include "platform/network/NetworkRequest.h"

namespace StarFish {

class ResourceNetworkRequestClient : public NetworkRequestClient {
public:
    ResourceNetworkRequestClient(Resource* resource)
        : m_resource(resource)
    {
    }

    virtual void onProgressEvent(NetworkRequest* request, bool isExplicitAction)
    {
        if (request->progressState() == NetworkRequest::LOAD) {
            m_resource->didDataReceived(request->responseData().data(), request->responseData().size());
            m_resource->didLoadFinished();
        } else if (request->progressState() == NetworkRequest::ERROR) {
            m_resource->didLoadFailed();
        } else if (request->progressState() == NetworkRequest::TIMEOUT) {
            m_resource->didLoadFailed();
        }
    }

protected:
    Resource* m_resource;
};

void Resource::request(bool needsSyncRequest)
{
    loader()->fetchResourcePreprocess(this);
    m_networkRequest = new NetworkRequest(m_loader->document());
    m_networkRequest->addNetworkRequestClient(new ResourceNetworkRequestClient(this));
    m_networkRequest->open(NetworkRequest::GET_METHOD, m_url.urlString(), !needsSyncRequest);
    m_networkRequest->send();
}

void Resource::cancel()
{
    STARFISH_ASSERT(m_state == BeforeSend || m_state == Receiving);
    didLoadCanceled();
}

void Resource::didDataReceived(const char*, size_t length)
{
    m_state = Receiving;
}

void Resource::didLoadFinished()
{
    m_state = Finished;
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didLoadFinished();
        iter++;
    }
    m_networkRequest = nullptr;
}

void Resource::didLoadFailed()
{
    m_state = Failed;
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didLoadFailed();
        iter++;
    }
    m_networkRequest = nullptr;
}

void Resource::didLoadCanceled()
{
    m_state = Canceled;
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didLoadCanceled();
        iter++;
    }
    auto iter2 = m_requstedIdlers.begin();
    while (iter2 != m_requstedIdlers.end()) {
        m_loader->m_document->window()->starFish()->messageLoop()->removeIdler(*iter2);
        iter2++;
    }
    m_requstedIdlers.clear();
    m_networkRequest = nullptr;
}

}
