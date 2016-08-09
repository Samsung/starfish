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

#ifndef __StarFishResourceLoader__
#define __StarFishResourceLoader__

#include "loader/Resource.h"
#include "loader/TextResource.h"
#include "loader/ImageResource.h"

namespace StarFish {

class ResourceLoader : public gc {
    friend class Resource;
    friend class ImageResource;
    friend class DocumentOnLoadChecker;
    friend class ResourceAliveChecker;
public:
    ResourceLoader(Document& doc);

    Resource* fetch(URL* url);
    TextResource* fetchText(URL* url, String* preferredEncoding = String::emptyString);
    ImageResource* fetchImage(URL* url);

    void markDocumentOpenState()
    {
        m_inDocumentOpenState = true;
        m_pendingResourceCountWhileDocumentOpening = 1;
    }

    void notifyEndParseDocument()
    {
        STARFISH_ASSERT(m_pendingResourceCountWhileDocumentOpening > 0);
        m_pendingResourceCountWhileDocumentOpening--;
        fireDocumentOnLoadEventIfNeeded();
    }

    void cancelAllOfPendingRequests();
    Document* document()
    {
        return m_document;
    }
private:
    void fetchResourcePreprocess(Resource* res);
    void fireDocumentOnLoadEventIfNeeded();
    bool m_inDocumentOpenState;
    size_t m_pendingResourceCountWhileDocumentOpening;
    Document* m_document;
    URL* m_baseURL;
    std::unordered_map<std::string, ImageData*, std::hash<std::string>, std::equal_to<std::string>,
        gc_allocator<std::pair<std::string, ImageData*>>> m_offlineImageCache;
    std::vector<Resource*, gc_allocator<Resource*>> m_currentLoadingResources;
};
}

#endif
