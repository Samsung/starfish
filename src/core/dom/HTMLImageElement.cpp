/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "StarFish.h"
#include "core/dom/Document.h"
#include "core/dom/HTMLImageElement.h"
#include "core/layout/FrameReplacedImage.h"
#include "platform/loader/ElementResourceClient.h"
#include "core/modules/message_loop/MessageLoop.h"

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

        ImageData* imageDataBefore = m_element->imageData();
        ImageData* imageData = m_resource->asImageResource()->imageData();
        STARFISH_ASSERT(imageData);

        LayoutSize sizeBefore(0, 0);
        LayoutSize sizeNow(imageData->width(), imageData->height());

        if (imageDataBefore) {
            sizeBefore =
                LayoutSize(imageDataBefore->width(), imageDataBefore->height());
        }

        m_element->m_imageData = imageData;

        if (m_element->frame()) {
            if (sizeBefore == sizeNow) {
                m_element->setNeedsPainting();
            } else {
                // TODO consider width, height attribute
                m_element->setNeedsLayout();
            }
        } else {
            m_element->setNeedsFrameTreeBuild();
        }
    }

protected:
    HTMLImageElement* m_element;
};

String* HTMLImageElement::localName()
{
    return starFish()->staticStrings()->m_imgTagName.localName();
}

QualifiedName HTMLImageElement::name()
{
    return starFish()->staticStrings()->m_imgTagName;
}

void HTMLImageElement::setSrc(String* src)
{
    setAttribute(starFish()->staticStrings()->m_src, src);
}

String* HTMLImageElement::src()
{
    return getAttributeOrEmpty(starFish()->staticStrings()->m_src);
}

unsigned long HTMLImageElement::width()
{
    String* widthStr =
        getAttributeOrEmpty(starFish()->staticStrings()->m_width);
    return String::parseInt(widthStr);
}

void HTMLImageElement::setWidth(unsigned long width)
{
    setAttribute(starFish()->staticStrings()->m_width, String::fromInt(width));
}

unsigned long HTMLImageElement::height()
{
    String* heightStr =
        getAttributeOrEmpty(starFish()->staticStrings()->m_height);
    return String::parseInt(heightStr);
}

void HTMLImageElement::setHeight(unsigned long height)
{
    setAttribute(starFish()->staticStrings()->m_height,
                 String::fromInt(height));
}

void HTMLImageElement::didAttributeChanged(QualifiedName name, String* old,
                                           String* value, bool attributeCreated,
                                           bool attributeRemoved)
{
    HTMLElement::didAttributeChanged(name, old, value, attributeCreated,
                                     attributeRemoved);
    if (name == starFish()->staticStrings()->m_src) {
        if (value->length() && document()->doesParticipateInRendering()) {
            loadImage(value);
        } else {
            unloadImage();
        }
    } else if (name == starFish()->staticStrings()->m_width ||
               name == starFish()->staticStrings()->m_height) {
        if (frame()) {
            setNeedsLayout();
        }
    }
}

void HTMLImageElement::didNodeAdopted()
{
    HTMLElement::didNodeAdopted();
    if (document()->doesParticipateInRendering()) {
        Nullable<String*> srcStr =
            getAttribute(starFish()->staticStrings()->m_src);
        if (srcStr.hasValue() && srcStr.getValue()->length() > 0) {
            loadImage(srcStr.getValue());
        }
    } else {
        unloadImage();
    }
}

void HTMLImageElement::unloadImage()
{
    if (m_imageResource) {
        m_imageResource->cancel();
        m_imageResource = nullptr;
    }
    m_imageData = nullptr;
    if (frame()) {
        setNeedsLayout();
    }
}

void HTMLImageElement::loadImage(String* src)
{
    unloadImage();
    m_imageResource = document()->resourceLoader().fetchImage(
        new ResourceURL(src, document()->documentURI()->baseURI()));
    m_imageResource->addResourceClient(
        new ImageDownloadClient(this, m_imageResource));
    m_imageResource->addResourceClient(
        new ElementResourceClient(this, m_imageResource));
    m_imageResource->request(
        Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded);
}
}
