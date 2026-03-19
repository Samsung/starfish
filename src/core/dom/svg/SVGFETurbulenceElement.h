/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGFETurbulenceElement__
#define __StarfishSVGFETurbulenceElement__

#include "core/dom/svg/SVGFilterPrimitiveStandardAttributes.h"
#include "core/dom/svg/SVGAnimatedEnumeration.h"

namespace Starfish {

/**
 * @brief The SVGFETurbulenceElement class implements the <feTurbulence> SVG
 * filter primitive.
 *
 * The <feTurbulence> filter primitive creates an image using the Perlin
 * turbulence function. It allows the synthesis of artificial textures like
 * clouds or marble. https://www.w3.org/TR/SVG/filters.html#feTurbulenceElement
 */
class SVGFETurbulenceElement : public SVGFilterPrimitiveStandardAttributes {
public:
    /**
     * @brief The TurbulenceType enum corresponds to the 'type' attribute of the
     * <feTurbulence> element.
     */
    enum TurbulenceType {
        SVG_TURBULENCE_TYPE_UNKNOWN = 0,
        SVG_TURBULENCE_TYPE_FRACTALNOISE,
        SVG_TURBULENCE_TYPE_TURBULENCE,
    };

    /**
     * @brief The StitchType enum corresponds to the 'stitchTiles' attribute of
     * the <feTurbulence> element.
     */
    enum StitchType {
        SVG_STITCHTYPE_UNKNOWN = 0,
        SVG_STITCHTYPE_STITCH,
        SVG_STITCHTYPE_NOSTITCH,
    };

    SVGFETurbulenceElement(Document* document, const QualifiedName& qname);
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGFETurbulenceElement() const override;
    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        MatchedStyleRules<>& matchedRules,
        Optional<const MutablePropertyValueList*> cssCustomValues) override;

    /**
     * @brief Corresponds to the 'baseFrequencyX' attribute of the
     * <feTurbulence> element.
     */
    SVGAnimatedNumber* baseFrequencyX();
    /**
     * @brief Corresponds to the 'baseFrequencyY' attribute of the
     * <feTurbulence> element.
     */
    SVGAnimatedNumber* baseFrequencyY();
    /**
     * @brief Corresponds to the 'numOctaves' attribute of the <feTurbulence>
     * element.
     */
    SVGAnimatedInteger* numOctaves();
    /**
     * @brief Corresponds to the 'seed' attribute of the <feTurbulence> element.
     */
    SVGAnimatedNumber* seed();
    /**
     * @brief Corresponds to the 'stitchTiles' attribute of the <feTurbulence>
     * element.
     */
    SVGAnimatedEnumeration* stitchTiles();
    /**
     * @brief Corresponds to the 'type' attribute of the <feTurbulence> element.
     */
    SVGAnimatedEnumeration* type();

private:
    virtual void computeAttributeChangeDamage(AtomicString attrName) override;

    Optional<SVGAnimatedNumber*> m_baseFrequencyX;
    Optional<SVGAnimatedNumber*> m_baseFrequencyY;
    Optional<SVGAnimatedInteger*> m_numOctaves;
    Optional<SVGAnimatedNumber*> m_seed;
    Optional<SVGAnimatedEnumeration*> m_stitchTiles;
    Optional<SVGAnimatedEnumeration*> m_type;
};
} // namespace Starfish

#endif
