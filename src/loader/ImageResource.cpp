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
    if (res->url()->isFileURL()) {
        FileIO* fio = FileIO::create();
        // NOTE
        // we should special logic to load file url for image
        // we can pass src of image to platform layer
        // TODO handle decoding failed
        String* path = res->url()->urlStringWithoutSearchPart();
        String* filePath = path->substring(7, path->length() - 7);
        bool canLoad = fio->open(filePath);
        delete fio;
        if (!canLoad) {
            res->didLoadFailed();
            return;
        }
        std::string utf8 = filePath->utf8Data();
        auto iter = res->loader()->m_imageCache.find(utf8);
        ImageData* id;
        if (iter == res->loader()->m_imageCache.end()) {
            id = ImageData::create(filePath);
            if (!id) {
                res->didLoadFailed();
                return;
            }
            res->loader()->m_imageCache.insert(std::make_pair(utf8, id));
        } else {
            id = iter->second;
        }
        res->asImageResource()->m_imageData = id;
        res->didLoadFinished();
    } else {
        res->didLoadFailed();
    }
}

void ImageResource::request(ResourceRequestSyncLevel syncLevel)
{
    if (m_url->isFileURL()) {
        loader()->fetchResourcePreprocess(this);
        if (ResourceRequestSyncLevel::AlwaysSync == syncLevel) {
            doLoadFile(this);
        } else if (ResourceRequestSyncLevel::SyncIfAlreadyLoaded == syncLevel) {
            FileIO* fio = FileIO::create();
            String* path = url()->urlStringWithoutSearchPart();
            String* filePath = path->substring(7, path->length() - 7);
            bool canLoad = fio->open(filePath);
            delete fio;
            if (!canLoad) {
                request(ResourceRequestSyncLevel::NeverSync);
                return;
            }
            std::string utf8 = filePath->utf8Data();
            auto iter = loader()->m_imageCache.find(utf8);
            if (iter != loader()->m_imageCache.end()) {
                request(ResourceRequestSyncLevel::AlwaysSync);
            } else {
                request(ResourceRequestSyncLevel::NeverSync);
            }
        } else {
            pushIdlerHandle(m_loader->m_document->window()->starFish()->messageLoop()->addIdler([](size_t handle, void* data) {
                Resource* res = (Resource*)data;
                res->removeIdlerHandle(handle);
                ImageResourceDoLoadFile(data);
            }, this));
        }
    } else {
        Resource::request(syncLevel);
    }
}

void ImageResource::didLoadFinished()
{
    if (!m_url->isFileURL()) {
        m_imageData = ImageData::create(m_networkRequest->responseData().data(), m_networkRequest->responseData().size());
        if (!m_imageData) {
            Resource::didLoadFailed();
            return;
        }
    }
    Resource::didLoadFinished();
}

}
