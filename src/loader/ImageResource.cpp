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

#ifdef STARFISH_EFL
void ImageResource::doLoadFile(void* data)
{
    Resource* res = (Resource*)data;
    // special path for image resource
    if (res->url()->isFileURL()) {
        FileIO* fio = FileIO::create();
        // NOTE
        // we should special logic to load file url for image
        // we can pass src of image to platform layer in efl
        String* path = res->url()->urlStringWithoutSearchPart();
        String* filePath = path->substring(7, path->length() - 7);
        bool canLoad = fio->open(filePath);
        delete fio;
        if (!canLoad) {
            res->didLoadFailed();
            return;
        }
        ImageData* id = ImageData::create(filePath);
        if (!id) {
            res->didLoadFailed();
            return;
        }
        res->asImageResource()->m_imageData = id;
        res->didLoadFinished();
    } else {
        res->didLoadFailed();
    }
}
#endif

void ImageResource::request(ResourceRequestSyncLevel syncLevel)
{
#ifdef STARFISH_EFL
    if (m_url->isFileURL()) {
        if (!loader()->requestResourcePreprocess(this, syncLevel)) {
            // cache miss
            if (ResourceRequestSyncLevel::AlwaysSync == syncLevel) {
                doLoadFile(this);
            } else {
                pushIdlerHandle(m_loader->m_document->window()->starFish()->messageLoop()->addIdler([](size_t handle, void* data) {
                    Resource* res = (Resource*)data;
                    res->removeIdlerHandle(handle);
                    res->asImageResource()->doLoadFile(data);
                }, this));
            }
        }
    } else {
        Resource::request(syncLevel);
    }
#else
    Resource::request(syncLevel);
#endif
}

void ImageResource::didLoadFinished()
{
#ifdef STARFISH_EFL
    if (!m_url->isFileURL()) {
        m_imageData = ImageData::create(m_networkRequest->responseData().data(), m_networkRequest->responseData().size());
        if (!m_imageData) {
            Resource::didLoadFailed();
            return;
        }
    }
#else
    m_imageData = ImageData::create(m_networkRequest->responseData().data(), m_networkRequest->responseData().size());
    if (!m_imageData) {
        Resource::didLoadFailed();
        return;
    }
#endif
    Resource::didLoadFinished();
}

}
