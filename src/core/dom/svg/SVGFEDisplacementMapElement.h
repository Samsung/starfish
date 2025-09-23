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

#ifndef __StarfishSVGFEDisplacementMapElement__
#define __StarfishSVGFEDisplacementMapElement__

#include "core/dom/svg/SVGFilterPrimitiveStandardAttributes.h"
#include "core/dom/svg/SVGAnimatedEnumeration.h"
#include "core/dom/svg/SVGAnimatedString.h"

namespace Starfish {

class SVGFEDisplacementMapElement
    : public SVGFilterPrimitiveStandardAttributes {
public:
    enum ChannelSelector {
        SVG_CHANNEL_UNKNOWN = 0,
        SVG_CHANNEL_R,
        SVG_CHANNEL_G,
        SVG_CHANNEL_B,
        SVG_CHANNEL_A,
    };

    SVGFEDisplacementMapElement(Document* document, const QualifiedName& qname);
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGFEDisplacementMapElement() const override;
    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void updateSVGAttributeNeeded(QualifiedName name) override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Optional<const MutablePropertyValueList*> cssCustomValues) override;

    SVGAnimatedString* in1();
    SVGAnimatedString* in2();
    SVGAnimatedNumber* scale();
    SVGAnimatedEnumeration* xChannelSelector();
    SVGAnimatedEnumeration* yChannelSelector();

private:
    Optional<SVGAnimatedString*> m_in1;
    Optional<SVGAnimatedString*> m_in2;
    Optional<SVGAnimatedNumber*> m_scale;
    Optional<SVGAnimatedEnumeration*> m_xChannelSelector;
    Optional<SVGAnimatedEnumeration*> m_yChannelSelector;
};
} // namespace Starfish

#endif
