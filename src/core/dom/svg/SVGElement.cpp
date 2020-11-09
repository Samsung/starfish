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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/svg/SVGElement.h"
#include "core/dom/Traverse.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace Starfish {

SVGElement::SVGElement(Document* document, const QualifiedName& qname)
    : Element(document, qname)
    , m_preserveAspectRatioValue(
          NativeImageData::PreserveAspectRatioValue::None)
    , m_clipPathElement(nullptr)
{
    STARFISH_ASSERT(namespaceURI().hasValue());
    STARFISH_ASSERT(name().hasSameNamespaceURI(SVG_NAMESPACE));
}

void* SVGElement::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(SVGElement));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(SVGElement)] = { 0 };
        Element::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGElement::didAttributeChanged(QualifiedName name, String* old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved)
{
    Element::didAttributeChanged(name, old, value, attributeCreated,
                                 attributeRemoved);
    StaticStrings* ss = starfish()->staticStrings();

    if (name == ss->m_onload) {
        setAttributeEventListener(ss->m_load, value, this);
    } else if (name == ss->m_onerror) {
        setAttributeEventListener(ss->m_error, value, this);
    }

    if (needsGeometryAttributes()) {
        if (ss->m_x == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsLayout();
        } else if (ss->m_y == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsLayout();
        }
    }

    if (needsSizingAttributes()) {
        if (ss->m_width == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsLayout();
        } else if (ss->m_height == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsLayout();
        }
    }

    if (needsFillAttributes()) {
        if (ss->m_fill == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
        } else if (ss->m_fillOpacity == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
        } else if (ss->m_fillRule == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
        }
    }

    if (needsStrokeAttributes()) {
        if (ss->m_stroke == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
        } else if (ss->m_strokeWidth == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
        } else if (ss->m_strokeOpacity == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
        }
    }

    if (needsTransparentAttributes()) {
        if (ss->m_opacity == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
        }
    }

    if (needsClipPathAttributes()) {
        if (ss->m_clipPath == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
        }
    }

    if (needsPreserveAspectRatioValue()) {
        if (name == starfish()->staticStrings()->m_preserveAspectRatio) {
#define SET_PARV(name)                                      \
    else if (value->equals(#name))                          \
    {                                                       \
        m_preserveAspectRatioValue = NativeImageData::name; \
    }

            if (value->equals("none")) {
                m_preserveAspectRatioValue = NativeImageData::None;
            }
            SET_PARV(xMinYMin)
            SET_PARV(xMidYMin)
            SET_PARV(xMaxYMin)
            SET_PARV(xMinYMid)
            SET_PARV(xMidYMid)
            SET_PARV(xMaxYMid)
            SET_PARV(xMinYMax)
            SET_PARV(xMidYMax)
            SET_PARV(xMaxYMax)
            else
            {
                STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
            }
#undef SET_PARV
        }
    }

    if (isRenderableElement()) {
        if (ss->m_display == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
        }
    }
}

String* SVGElement::xmlbase()
{
    return getAttributeOrEmpty(starfish()->staticStrings()->m_xmlBase);
}

void SVGElement::setXmlbase(String* str)
{
    setAttribute(starfish()->staticStrings()->m_xmlBase, str);
}

SVGElement* SVGElement::ownerSVGElement()
{
    // The nearest ancestor ‘svg’ element. Null if the given element is the
    // outermost svg element.
    Element* e = parentElement();

    while (!e->isSVGSVGElement()) {
        e = e->parentElement();
    }

    return (SVGElement*)e;
}

SVGElement* SVGElement::viewportElement()
{
    // The element which established the current viewport. Often, the nearest
    // ancestor ‘svg’ element. Null if the given element is the outermost svg
    // element.
    return ownerSVGElement();
}

void SVGElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues)
{
    Element::styleForPresentationAttribute(cssValues);
    CSSStyleValuePair pair;

    if (needsGeometryAttributes()) {
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(x, X);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(y, Y);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(x1, X1);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(y1, Y1);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(x2, X2);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(y2, Y2);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(width, Width);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(height, Height);
    }

    if (needsFillAttributes()) {
        String* fill = getAttributeOrEmpty(starfish()->staticStrings()->m_fill);
        if (fill->length()) {
            pair.setKeyKind(CSSStyleValuePair::Fill);

            auto fillStr = fill->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, fillStr.data(),
                                                  fillStr.length());
            if (pair.updateValueFill(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }

        String* fillRule =
            getAttributeOrEmpty(starfish()->staticStrings()->m_fillRule);
        if (fillRule->length()) {
            pair.setKeyKind(CSSStyleValuePair::FillRule);

            auto str = fillRule->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueFillRule(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }

        String* fillOpacity =
            getAttributeOrEmpty(starfish()->staticStrings()->m_fillOpacity);
        if (fillOpacity->length()) {
            pair.setKeyKind(CSSStyleValuePair::FillOpacity);

            auto str = fillOpacity->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueFillOpacity(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }
    }

    if (needsStrokeAttributes()) {
        String* stroke =
            getAttributeOrEmpty(starfish()->staticStrings()->m_stroke);
        if (stroke->length()) {
            pair.setKeyKind(CSSStyleValuePair::Stroke);

            auto str = stroke->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueStroke(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }

        String* strokeWidth =
            getAttributeOrEmpty(starfish()->staticStrings()->m_strokeWidth);
        if (strokeWidth->length()) {
            pair.setKeyKind(CSSStyleValuePair::StrokeWidth);

            auto str = strokeWidth->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueStrokeWidth(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }

        String* strokeOpacity =
            getAttributeOrEmpty(starfish()->staticStrings()->m_strokeOpacity);
        if (strokeOpacity->length()) {
            pair.setKeyKind(CSSStyleValuePair::StrokeOpacity);

            auto str = strokeOpacity->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueStrokeOpacity(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }
    }

    if (needsTransparentAttributes()) {
        String* opacity =
            getAttributeOrEmpty(starfish()->staticStrings()->m_opacity);
        if (opacity->length()) {
            pair.setKeyKind(CSSStyleValuePair::Opacity);

            auto str = opacity->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueOpacity(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }
    }

    if (needsTransformAttributes()) {
        String* transform =
            getAttributeOrEmpty(starfish()->staticStrings()->m_transform);
        if (transform->length()) {
            pair.setKeyKind(CSSStyleValuePair::Transform);

            auto str = transform->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueTransform(tokens, true)) {
                cssValues.push_back(pair);
            }
        }
    }

    if (needsClipPathAttributes()) {
        String* clipPathStr =
            getAttributeOrEmpty(starfish()->staticStrings()->m_clipPath);

        if (clipPathStr->length()) {
            pair.setKeyKind(CSSStyleValuePair::ClipPath);

            auto str = clipPathStr->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueClipPath(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }
    }

    if (isRenderableElement()) {
        // The display property only applies to renderable elements.
        String* displayStr =
            getAttributeOrEmpty(starfish()->staticStrings()->m_display);

        if (displayStr->length()) {
            pair.setKeyKind(CSSStyleValuePair::Display);

            auto str = displayStr->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueDisplay(document(), tokens)) {
                cssValues.push_back(pair);
            }
        }
    }
}

int SVGElement::tabIndex()
{
    if (supportsFocus()) {
        return Element::tabIndex();
    }
    return -1;
}

SVGElement* SVGElement::clipPathElement()
{
    if (!hasClipPath() || !needsClipPathAttributes()) {
        return nullptr;
    }

    if (!m_clipPathElement) {
        String* clipPathStr = style()->clipPath();
        ResourceURL* clipPathURL;
        // In case that SVG element is loaded as an image resource through
        // MockHTMLIFrameElement. At this case, we can find baseURI at its
        // referrerURL.
        if (document()->baseURL()->isDataURL()) {
            clipPathURL = new ResourceURL(clipPathStr, document()->referrer());
        } else {
            clipPathURL = new ResourceURL(clipPathStr, document()->baseURI());
        }
        String* id = clipPathURL->getFragmentIdValue();
        if (!id->isEmpty()) {
            Element* clipPathElement = document()->getElementById(id);
            if (clipPathElement) {
                m_clipPathElement = (SVGElement*)clipPathElement;
            }
        }
    }
    return m_clipPathElement;
}

SVGElement* SVGElement::getSVGElementById(String* id)
{
    Node* descendant = Traverse::findDescendant(this, [this, id](Node* node) {
        if (!node->isSVGElement()) {
            return false;
        }

        String* nodeId = node->asSVGElement()->getAttributeOrEmpty(
            starfish()->staticStrings()->m_id);
        if (nodeId->equals(id)) {
            return true;
        }
        return false;
    });

    if (!descendant) {
        return nullptr;
    }

    return descendant->asSVGElement();
}
} // namespace Starfish
