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

#ifndef __StarfishSVGSVGAnimateTransformElement__
#define __StarfishSVGSVGAnimateTransformElement__

#include "core/dom/svg/SVGAnimationElement.h"

namespace Starfish {

enum class TransformType {
    Translate,
    Scale,
    Rotate,
    SkewX,
    SkewY,
};

class SVGAnimateTransformElement : public SVGAnimationElement {
public:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        SVGAnimationElement::fillGCDescriptor(desc);
        // Fill GC descriptor here if needed
    }

    SVGAnimateTransformElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGAnimateTransformElement() const override;

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;
    virtual void beginElementAt(float offset) override;

private:
    bool parseType(TransformType& type);

    virtual bool parseFromTo(CSSStyleValuePair::KeyKind keyKind,
                             const String* value,
                             CSSStyleValuePair& output) override;

    bool toCSSTransfromValue(const TransformType type, String* value,
                             String** transformValue);

    Optional<TransformType> m_type;
};
} // namespace Starfish

#endif
