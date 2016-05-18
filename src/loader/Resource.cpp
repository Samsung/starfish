#include "StarFishConfig.h"
#include "Resource.h"

#include "platform/message_loop/MessageLoop.h"
#include "platform/file_io/FileIO.h"
#include "dom/Document.h"
#include "loader/ResourceLoader.h"

namespace StarFish {

void Resource::doLoad(void* data)
{
    Resource* res = (Resource*)data;
    FileIO* fio = FileIO::create();
    String* path = res->m_url.urlStringWithoutSearchPart();
    if (fio->open(path->substring(7, path->length() - 7))) {
        size_t siz = fio->length();

        char* fileContents = (char*)malloc(siz + 1);
        fio->read(fileContents, sizeof(char), siz);
        fileContents[siz] = 0;
        res->didDataReceived(fileContents, siz);
        free(fileContents);
        fio->close();
        res->didLoadFinished();
    } else {
        STARFISH_LOG_ERROR("failed to load %s\n", res->m_url.urlString()->utf8Data());
        res->didLoadFailed();
    }
    delete fio;
}

void Resource::request(bool needsSyncRequest)
{
    loader()->fetchResourcePreprocess(this);
    if (needsSyncRequest) {
        doLoad(this);
    } else {
        pushIdlerHandle(m_loader->m_document->window()->starFish()->messageLoop()->addIdler([](size_t handle, void* data) {
            Resource* res = (Resource*)data;
            res->removeIdlerHandle(handle);
            doLoad(data);
        }, this));
    }
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
}

void Resource::didLoadFailed()
{
    m_state = Failed;
    auto iter = m_resourceClients.begin();
    while (iter != m_resourceClients.end()) {
        (*iter)->didLoadFailed();
        iter++;
    }
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
}

}
