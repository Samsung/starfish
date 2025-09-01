/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGImageElement.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "platform/loader/ElementResourceClient.h"
#include "platform/loader/ResourceLoader.h"
#include "core/dom/WebOrigin.h"

namespace Starfish {

void* SVGImageElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGImageElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGImageElement)] = { 0 };
        GC_set_bit(desc, GC_WORD_OFFSET(SVGImageElement, m_imageData));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGImageElement, m_imageResource));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGImageElement, m_x));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGImageElement, m_y));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGImageElement, m_width));
        GC_set_bit(desc, GC_WORD_OFFSET(SVGImageElement, m_height));
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGImageElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
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
        updateImage(m_element->document()->brokenImage());
    }

    virtual void didLoadFinished()
    {
        ResourceClient::didLoadFinished();
        updateImage(m_resource->asImageResource()->imageData());
    }

    void updateImage(NativeImageData* imageData)
    {
        m_element->m_requestErrorType = m_resource->requestErrorType();
        m_element->m_imageResource = nullptr;
        m_element->m_imageData = imageData;
        if (!imageData) {
            return;
        }
        m_element->setNeedsLayout();
        m_element->setNeedsPainting();
    }

protected:
    SVGImageElement* m_element;
};

SVGImageElement::SVGImageElement(Document* document, const QualifiedName& qname)
    : SVGElement(document, qname)
    , m_imageResource(nullptr)
    , m_imageData(nullptr)
    , m_requestErrorType(RequestErrorType::NoError)
{
}

void SVGImageElement::unloadImage()
{
    if (m_imageResource) {
        m_imageResource->cancel();
        m_imageResource = nullptr;
    }
    m_imageData = nullptr;
    m_requestErrorType = RequestErrorType::NoError;
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

    RequestData* reqData = new RequestData();
    reqData->m_url = m_imageResource->url();
    reqData->m_referrer = new ReferrerURL(document()->documentURI());
    reqData->m_destination = RequestDestination::Image;
    reqData->m_syncLevel = RequestSyncLevel::NeverSync;

    m_imageResource->request(reqData, true);
}

void SVGImageElement::didAttributeChanged(QualifiedName name,
                                          Optional<String*> old, String* value,
                                          bool attributeCreated,
                                          bool attributeRemoved)
{
    SVGElement::didAttributeChanged(name, old, value, attributeCreated,
                                    attributeRemoved);
    if (old.hasValue() && old->equals(value)) {
        return;
    }

    StaticStrings* ss = starfish()->staticStrings();
    if (ss->m_href == name || ss->m_xlinkHref == name ||
        (!name.hasPrefix() &&
         ss->m_xlinkHref.hasSameNamespaceURI(name.namespaceURI()) &&
         ss->m_xlinkHref.hasSameLocalName(name.localName()))) {
        if (attributeRemoved) {
            unloadImage();
        } else {
            loadImage(value);
        }
    } else if (name == ss->m_preserveAspectRatio) {
        setNeedsPainting();
    }
}

void SVGImageElement::updateSVGAttributeNeeded(QualifiedName name)
{
    StaticStrings* ss = starfish()->staticStrings();

    if (ss->m_x == name) {
        setAttribute(ss->m_x, x()->baseVal()->valueAsString());
    } else if (ss->m_y == name) {
        setAttribute(ss->m_y, y()->baseVal()->valueAsString());
    } else if (ss->m_width == name) {
        setAttribute(ss->m_width, width()->baseVal()->valueAsString());
    } else if (ss->m_height == name) {
        setAttribute(ss->m_height, height()->baseVal()->valueAsString());
    }
}

void SVGImageElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    SVGElement::styleForPresentationAttribute(cssValues, cssCustomValues);
}

WebOrigin* SVGImageElement::webOrigin()
{
    return WebOrigin::createDocumentOrigin(origin());
}

ResourceURL* SVGImageElement::origin()
{
    if (hasAttribute(starfish()->staticStrings()->m_href) != SIZE_MAX) {
        return new ResourceURL(
            getAttributeOrEmpty(starfish()->staticStrings()->m_href),
            document()->baseURI());
    } else {
        return new ResourceURL(String::emptyString);
    }
}

bool SVGImageElement::hasRequestError()
{
    return m_requestErrorType != RequestErrorType::NoError;
}
} // namespace Starfish
