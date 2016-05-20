#include "StarFishConfig.h"
#include "ImageResource.h"

#include "platform/message_loop/MessageLoop.h"
#include "platform/file_io/FileIO.h"
#include "dom/Document.h"
#include "loader/ResourceLoader.h"
#include "platform/network/NetworkRequest.h"

namespace StarFish {

void ImageResource::doLoadFile(void* data)
{
    Resource* res = (Resource*)data;
    // special path for image resource
    if (res->url().isFileURL()) {
        FileIO* fio = FileIO::create();
        // NOTE
        // we should special logic to load file url for image
        // we can pass src of image to platform layer
        // TODO handle decoding failed
        String* path = res->url().urlStringWithoutSearchPart();
        String* filePath = path->substring(7, path->length() - 7);
        bool canLoad = fio->open(filePath);
        delete fio;
        if (!canLoad) {
            res->didLoadFailed();
            return;
        }
        std::string utf8 = filePath->utf8Data();
        auto iter = res->loader()->m_offlineImageCache.find(utf8);
        ImageData* id;
        if (iter == res->loader()->m_offlineImageCache.end()) {
            id = ImageData::create(filePath);
            res->loader()->m_offlineImageCache.insert(std::make_pair(utf8, id));
        } else {
            id = iter->second;
        }
        res->asImageResource()->m_imageData = id;
        res->didLoadFinished();
    } else {
        res->didLoadFailed();
    }
}

void ImageResource::request(bool needsSyncRequest)
{
    if (m_url.isFileURL()) {
        loader()->fetchResourcePreprocess(this);
        if (needsSyncRequest) {
            doLoadFile(this);
        } else {
            pushIdlerHandle(m_loader->m_document->window()->starFish()->messageLoop()->addIdler([](size_t handle, void* data) {
                Resource* res = (Resource*)data;
                res->removeIdlerHandle(handle);
                doLoadFile(data);
            }, this));
        }
    } else {
        Resource::request(needsSyncRequest);
    }
}

void ImageResource::didLoadFinished()
{
    if (!m_url.isFileURL()) {
        m_imageData = ImageData::create(m_networkRequest->responseData().data(), m_networkRequest->responseData().size());
    }
    Resource::didLoadFinished();
}

}
