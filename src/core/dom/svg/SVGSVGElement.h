/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGSVGElement__
#define __StarfishSVGSVGElement__

#include "core/dom/svg/SVGElement.h"
#include "core/dom/svg/SVGTransform.h"

namespace Starfish {

class SVGSVGElement : public SVGElement {
public:
    SVGSVGElement(Document* document, const QualifiedName& qname);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGSVGElement() const override;

    virtual void didAttributeChanged(QualifiedName name, Optional<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    virtual void didNodeInserted(Node* parent, Node* newChild) override;
    virtual void didNodeRemoved(Node* parent, Node* oldChild) override;
    virtual void didNodeRemovedFromDocumentTree() override;

    virtual void styleForPresentationAttribute(
        CSSStyleValuePairVectorHolder& cssValues,
        Optional<const MutablePropertyValueList*> cssCustomValues) override;

    virtual bool needsPreserveAspectRatioValue() override
    {
        return true;
    }

    virtual bool needsGeometryAttributes() override
    {
        return true;
    }

    virtual bool needsSizingAttributes() override
    {
        return true;
    }

    virtual bool needsClipPathAttributes()
    {
        return false;
    }

    virtual bool isRenderableElement() override
    {
        return true;
    }

    virtual bool isStructuralElement() override
    {
        return true;
    }

    virtual bool hasViewBox() const override
    {
        return m_hasViewBox;
    }

    virtual Unit::Rect viewBox() const override
    {
        STARFISH_ASSERT(m_hasViewBox);
        return m_viewBox;
    }

    virtual NativeImageData::PreserveAspectRatioAlign preserveAspectRatioAlign()
        override;
    virtual NativeImageData::PreserveAspectRatioMeetOrSlice
    preserveAspectRatioMeetOrSlice() override;

    virtual void updateSVGAttributeNeeded(QualifiedName name);

    STARFISH_SVG_ANIMATED_LENGTH_GETTER(x);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(y);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(width);
    STARFISH_SVG_ANIMATED_LENGTH_GETTER(height);

    SVGNumber* createSVGNumber();
    SVGLength* createSVGLength();
    SVGAngle* createSVGAngle();
    SVGTransform* createSVGTransform();

    void pauseAnimations();
    void unpauseAnimations();

    void connectUseElements();
    const GCVector<std::pair<SVGUseElement*, SVGElement*>>& useElementsPair()
    {
        return m_useElementsPair;
    }

    static void parseViewBox(bool& hasViewBox, Unit::Rect& viewBox,
                             String* value);

    const GCVector<std::pair<AtomicString, GCVector<SVGElement*>>>&
    gradientClientElements()
    {
        return m_gradientClientElements;
    }

    void clearGradientClientElements()
    {
        m_gradientClientElements.clear();
    }

    void registerGradientClientElements(const AtomicString& id,
                                        SVGElement* client);
    void notifyRepaintToGradientClientElements(const AtomicString& id);

protected:
    bool m_hasViewBox{ false };
    Unit::Rect m_viewBox;

    Optional<SVGAnimatedLength*> m_x;
    Optional<SVGAnimatedLength*> m_y;
    Optional<SVGAnimatedLength*> m_width;
    Optional<SVGAnimatedLength*> m_height;

    GCVector<std::pair<SVGUseElement*, SVGElement*>> m_useElementsPair;
    GCVector<std::pair<AtomicString, GCVector<SVGElement*>>>
        m_gradientClientElements;
};
} // namespace Starfish

#endif
