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

#ifndef __StarFishSVGImageElement__
#define __StarFishSVGImageElement__

#include "core/dom/svg/SVGElement.h"

namespace StarFish {

class SVGSVGElement;
class ImageResource;

class SVGImageElement : public SVGElement {
    friend class SVGImageDownloadClient;

public:
    SVGImageElement(Document* document)
        : SVGElement(document)
        , m_imageResource(nullptr)
        , m_imageData(nullptr)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGImageElement() const override;

    virtual QualifiedName name();

    virtual bool needsGeometryAttributes()
    {
        return true;
    }

    virtual bool needsSizingAttributes()
    {
        return true;
    }

    virtual bool needsPreserveAspectRatioValue()
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(x);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(y);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(width);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(height);

    NativeImageData* imageData()
    {
        return m_imageData;
    }

protected:
    void unloadImage();
    void loadImage(String* src);
    ImageResource* m_imageResource;
    NativeImageData* m_imageData;
};
}

#endif
