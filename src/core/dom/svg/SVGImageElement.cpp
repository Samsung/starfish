/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "StarFishConfig.h"
#include "StarFish.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGImageElement.h"
#include "platform/loader/ElementResourceClient.h"
#include "core/modules/message_loop/MessageLoop.h"

namespace StarFish {

void* SVGImageElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGImageElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGImageElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(SVGImageElement, m_imageData));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGImageElement, m_imageResource));
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGImageElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

QualifiedName SVGImageElement::name()
{
    return starFish()->staticStrings()->m_svgimageTagName;
}

class SVGImageDownloadClient : public ResourceClient {
public:
    SVGImageDownloadClient(SVGImageElement* element, Resource* res)
        : ResourceClient(res)
        , m_element(element)
    {
    }

    virtual void didLoadFailed()
    {
        ResourceClient::didLoadFailed();
        updateImage(nullptr);
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        updateImage(m_resource->asImageResource()->imageData());
    }

    void updateImage(NativeImageData* imageData)
    {
        m_element->m_imageResource = nullptr;
        m_element->m_imageData = imageData;
        if (!imageData) {
            return;
        }
        if (m_element->frame()) {
            m_element->setNeedsPainting();
        }
    }

protected:
    SVGImageElement* m_element;
};

void SVGImageElement::unloadImage()
{
    if (m_imageResource) {
        m_imageResource->cancel();
        m_imageResource = nullptr;
    }
    m_imageData = nullptr;
    if (frame()) {
        setNeedsPainting();
    }
}

void SVGImageElement::loadImage(String* src)
{
    unloadImage();
    m_imageResource = document()->resourceLoader().fetchImage(
        new ResourceURL(src, document()->baseURL()->baseURI()));
    m_imageResource->addResourceClient(
        new SVGImageDownloadClient(this, m_imageResource));
    m_imageResource->addResourceClient(
        new ElementResourceClient(this, m_imageResource));
    m_imageResource->request(
        Resource::ResourceRequestSyncLevel::SyncIfAlreadyLoaded,
        document()->documentURI(), true);
}

void SVGImageElement::didAttributeChanged(QualifiedName name, String* old,
                                          String* value, bool attributeCreated,
                                          bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);

    StaticStrings* ss = starFish()->staticStrings();
    if (ss->m_href == name) {
        if (attributeRemoved) {
            unloadImage();
        } else {
            loadImage(value);
        }
    } else if (name == ss->m_preserveAspectRatio) {
        setNeedsPainting();
    }
}

void SVGImageElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    SVGElement::styleForPresentationAttribute(cssValues);
}
}
