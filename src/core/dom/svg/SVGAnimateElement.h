/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGAnimateElement__
#define __StarfishSVGAnimateElement__

#include "core/dom/svg/SVGAnimationElement.h"

namespace Starfish {

class SVGAnimateElement : public SVGAnimationElement {
public:
    static inline void fillGCDescriptor(GC_word* desc)
    {
        SVGAnimationElement::fillGCDescriptor(desc);
        // Fill GC descriptor here if needed
        GC_set_bit(desc,
                   GC_WORD_OFFSET(SVGAnimateElement, m_animationKeyframes));
    }

    SVGAnimateElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGAnimateElement() const override;

    virtual void beginElement() override;

    virtual void beginElementAt(float offset) override;

    Optional<AnimationKeyframes*> animationKeyframes()
    {
        return m_animationKeyframes;
    }

protected:
    void AddAnimationKeyframe(CSSStyleValuePair::KeyKind keyKind,
                              AnimationKeyframes* animationKeyframes,
                              const GCVector<CSSStyleValuePair>& values);

    Optional<AnimationKeyframes*> m_animationKeyframes;
};
} // namespace Starfish

#endif
