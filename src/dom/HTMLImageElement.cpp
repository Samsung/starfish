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
        m_element->m_imageData = m_element->document()->brokenImage();

        m_element->setNeedsFrameTreeBuild();
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();

        std::pair<Length, Length> sizeBefore = m_element->intrinsicSize();
        ImageData* imageData = m_resource->asImageResource()->imageData();
        STARFISH_ASSERT(imageData);

        m_element->m_imageData = imageData;

        std::pair<Length, Length> size = m_element->intrinsicSize();
        if (m_element->frame()) {
            if (sizeBefore.first == size.first && sizeBefore.second == size.second) {
                m_element->setNeedsPainting();
            } else {
                m_element->setNeedsLayout();
            }
        } else {
            m_element->setNeedsFrameTreeBuild();
        }
        m_element->m_imageResource = nullptr;
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
