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

#ifndef __StarfishSVGImageElement__
#define __StarfishSVGImageElement__

#include "core/dom/svg/SVGElement.h"

namespace Starfish {

class SVGSVGElement;
class ImageResource;
enum class RequestErrorType;

class SVGImageElement : public SVGElement {
    friend class SVGImageDownloadClient;

public:
    SVGImageElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGImageElement() const override;

    virtual bool needsGeometryAttributes() override
    {
        return true;
    }

    virtual bool needsSizingAttributes() override
    {
        return true;
    }

    virtual bool needsPreserveAspectRatioValue() override
    {
        return true;
    }

    virtual bool isRenderableElement() override
    {
        return true;
    }

    virtual void didAttributeChanged(QualifiedName name, Nullable<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name);

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Nullable<const MutablePropertyValueList*> cssCustomValues) override;

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(x);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(y);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(width);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(height);

    NativeImageData* imageData()
    {
        return m_imageData;
    }

    WebOrigin* webOrigin();
    bool hasRequestError();

protected:
    void unloadImage();
    void loadImage(String* src);
    ResourceURL* origin();

    ImageResource* m_imageResource{ nullptr };
    NativeImageData* m_imageData{ nullptr };
    RequestErrorType m_requestErrorType;

    SVGAnimatedLength* m_x{ nullptr };
    SVGAnimatedLength* m_y{ nullptr };
    SVGAnimatedLength* m_width{ nullptr };
    SVGAnimatedLength* m_height{ nullptr };
};
} // namespace Starfish

#endif
