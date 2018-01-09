/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishSVGSVGElement__
#define __StarFishSVGSVGElement__

#include "core/dom/svg/SVGElement.h"
#include "core/modules/canvas/image/NativeImageData.h"

#define STARFISH_DEFAULT_SVG_WIDTH 300
#define STARFISH_DEFAULT_SVG_HEIGHT 150

namespace StarFish {

class SVGSVGElement : public SVGElement {
public:
    SVGSVGElement(Document* document)
        : SVGElement(document)
        , m_hasViewBox(false)
        , m_viewBox(0, 0, 0, 0)
        , m_preserveAspectRatioValue(NativeImageData::xMidYMid)
    {
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGSVGElement() const override;

    virtual QualifiedName name();

    virtual void didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues) override;

    bool hasViewBox() const
    {
        return m_hasViewBox;
    }

    Unit::Rect viewBox() const
    {
        STARFISH_ASSERT(m_hasViewBox);
        return m_viewBox;
    }

    NativeImageData::PreserveAspectRatioValue preserveAspectRatioValue()
    {
        return m_preserveAspectRatioValue;
    }

protected:
    bool m_hasViewBox;
    Unit::Rect m_viewBox;
    NativeImageData::PreserveAspectRatioValue m_preserveAspectRatioValue;
};
}

#endif
