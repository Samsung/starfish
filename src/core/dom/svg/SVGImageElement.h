/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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
