#include "StarFishConfig.h"
#include "Resource.h"

#include "platform/message_loop/MessageLoop.h"
#include "platform/file_io/FileIO.h"
#include "dom/Document.h"
#include "loader/ResourceLoader.h"

namespace StarFish {

void Resource::request(bool needsSyncRequest)
{
    loader()->fetchResourcePreprocess(this);

    auto fn = [](void* data)
    {
        Resource* res = (Resource*)data;
        FileIO* fio = FileIO::create();
        if (fio->open(res->m_url.urlString()->substring(7, res->m_url.urlString()->length() - 7))) {
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
    };

    if (needsSyncRequest) {
        fn(this);
    } else {
        pushIdlerHandle(m_loader->m_document->window()->starFish()->messageLoop()->addIdler(fn, this));
    }
}

void Resource::cancel()
{
    STARFISH_ASSERT(m_state == BeforeSend || m_state == Receiving);
    didLoadCanceled();
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
}

}
