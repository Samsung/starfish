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

    TextResource* fetchText(const URL& url);
    ImageResource* fetchImage(const URL& url);

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
    URL m_baseURL;
    std::unordered_map<std::string, ImageData*, std::hash<std::string>, std::equal_to<std::string>,
        gc_allocator<std::pair<std::string, ImageData*>>> m_offlineImageCache;
    std::vector<Resource*, gc_allocator<Resource*>> m_currentLoadingResources;
};
}

#endif
