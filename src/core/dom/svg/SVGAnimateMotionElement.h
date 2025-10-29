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

#ifndef __StarfishSVGSVGAnimateMotionElement__
#define __StarfishSVGSVGAnimateMotionElement__

#include "core/dom/svg/SVGAnimationElement.h"

namespace Starfish {

class Path;

class SVGAnimateMotionElement : public SVGAnimationElement {
public:
    SVGAnimateMotionElement(Document* document, const QualifiedName& qname);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGAnimateMotionElement() const override;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void beginElementAt(float offset) override;

    static Unit::FloatPoint computePoint(
        const GCAtomicVector<Unit::FloatPoint>& pl, float progress);

private:
    GCAtomicVector<Unit::FloatPoint> m_pointList;
};
} // namespace Starfish

#endif
