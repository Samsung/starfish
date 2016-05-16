#include "StarFishConfig.h"
#include "dom/Document.h"
#include "HTMLImageElement.h"

#include "loader/ElementResourceClient.h"
#include "platform/message_loop/MessageLoop.h"
#include "layout/FrameReplacedImage.h"

namespace StarFish {

class ImageDownloadClient : public ResourceClient {
public:
    ImageDownloadClient(HTMLImageElement* element, Resource* res)
        : ResourceClient(res)
        , m_element(element)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        m_element->m_imageResource = nullptr;
        m_element->m_imageData = nullptr;
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();

        LayoutSize sizeBefore(0, 0);
        LayoutSize size(0, 0);
        sizeBefore = m_element->intrinsicSize();
        ImageData* imageData = m_resource->asImageResource()->imageData();
        STARFISH_ASSERT(imageData);

        m_element->m_imageData = imageData;

        size = m_element->intrinsicSize();
        if (m_element->frame()) {
            if (sizeBefore == size) {
                m_element->setNeedsPainting();
            } else {
                m_element->setNeedsLayout();
            }
        } else {
            m_element->setNeedsFrameTreeBuild();
        }
    }
protected:
    HTMLImageElement* m_element;
};

void HTMLImageElement::didAttributeChanged(QualifiedName name, String* old, String* value, bool attributeCreated, bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated, attributeRemoved);
    if (name == document()->window()->starFish()->staticStrings()->m_src) {
        if (m_imageResource) {
            m_imageResource->cancel();
        }
        m_imageResource = document()->resourceLoader()->fetchImage(URL(document()->documentURI().baseURI(), value));
        m_imageResource->addResourceClient(new ImageDownloadClient(this, m_imageResource));
        m_imageResource->addResourceClient(new ElementResourceClient(this, m_imageResource));
        m_imageResource->request();
    } else if (name == document()->window()->starFish()->staticStrings()->m_width
        || name == document()->window()->starFish()->staticStrings()->m_height) {
        if (frame()) {
            setNeedsLayout();
        }
    }
}

}
