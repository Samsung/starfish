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
#include "core/dom/svg/SVGSVGElement.h"
#include "core/dom/svg/SVGUseElement.h"
#include "core/dom/svg/SVGAnimateElement.h"
#include "core/dom/Traverse.h"
#include "core/style/CSSParser.h"
#include "core/style/CSSStyleDeclaration.h"

namespace Starfish {

SVGElement::SVGElement(Document* document, const QualifiedName& qname)
    : Element(document, qname)
    , m_preserveAspectRatioAlign(
          NativeImageData::PreserveAspectRatioAlign::None)
    , m_preserveAspectRatioMeetOrSlice(
          NativeImageData::PreserveAspectRatioMeetOrSlice::Meet)
    , m_clipPathElement(nullptr)
    , m_maskElement(nullptr)
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
        SVGElement::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(SVGElement));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

void SVGElement::didAttributeChanged(QualifiedName name, Optional<String*> old,
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
        } else if (ss->m_x1 == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsLayout();
        } else if (ss->m_y1 == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsLayout();
        } else if (ss->m_x2 == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsLayout();
        } else if (ss->m_y2 == name) {
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
            setNeedsLayout();
            setNeedsPainting();
        } else if (ss->m_strokeLineCap == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsLayout();
            setNeedsPainting();
        } else if (ss->m_strokeLineJoin == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsLayout();
            setNeedsPainting();
        } else if (ss->m_strokeOpacity == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
        } else if (ss->m_strokeDasharray == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
        } else if (ss->m_strokeDashoffset == name) {
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
        if (ss->m_preserveAspectRatio == name) {
            GCVector<StringView> result;
            StringUtils::tokenize(value, " ", 1, result);

            StringView align;
            StringView meetOrSlice;

            if (result.size() > 1) {
                align = result.at(0);
                meetOrSlice = result.at(1);
            } else if (result.size()) {
                align = result.at(0);
            }

#define SET_PARV(name)                                      \
    else if (align.equals(#name))                           \
    {                                                       \
        m_preserveAspectRatioAlign = NativeImageData::name; \
    }

            if (align.equals("none")) {
                m_preserveAspectRatioAlign = NativeImageData::None;
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
                STARFISH_UNSUPPORTED("Unsupported svg attribute align value %s",
                                     align.toUTF8NonGCString().data());
            }
#undef SET_PARV

            if (meetOrSlice.equals("meet") || meetOrSlice.isEmpty()) {
                m_preserveAspectRatioMeetOrSlice = NativeImageData::Meet;
            } else if (meetOrSlice.equals("slice")) {
                m_preserveAspectRatioMeetOrSlice = NativeImageData::Slice;
            }
        }
    }

    if (isRenderableElement()) {
        if (ss->m_display == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
        }
    }

    if (needsMaskAttributes()) {
        if (ss->m_mask == name || ss->m_maskType == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsPainting();
            m_maskElement = nullptr;
        }
    }

    if (needsTransformAttributes()) {
        if (ss->m_transform == name || ss->m_transformOrigin == name) {
            setNeedsStyleRecalc(StyleChangeReason::JustNeedsRecalcSelf);
            setNeedsLayout();
            Traverse::traverseIncludingShadowDOM(
                this, [](Node* nd) { nd->setNeedsPainting(); });
        }
    }
}

void SVGElement::didNodeInserted(Node* parent, Node* newChild)
{
    Element::didNodeInserted(parent, newChild);
    if (newChild->isSVGAnimateElement()) {
        SVGAnimateElement* animate = newChild->asSVGAnimateElement();
        Optional<Element*> maybeTargetElement = animate->targetElement();
        if (maybeTargetElement && this == maybeTargetElement.value()) {
            animate->beginElement();
        }
    }
}

void SVGElement::didNodeRemoved(Node* parent, Node* oldChild)
{
    Element::didNodeRemoved(parent, oldChild);
    if (isInDocumentScopeAndDocumentParticipateInRendering()) {
        parent->setNeedsFrameTreeBuild();
        auto owner = ownerSVGElement();
        if (owner && owner->isSVGSVGElement()) {
            const auto& s = owner->asSVGSVGElement()->useElementsPair();
            for (auto e : s) {
                if (e.first->asNode() == parent ||
                    e.first->asNode() == oldChild ||
                    e.second->asNode() == parent ||
                    e.second->asNode() == oldChild) {
                    parent->setNeedsStyleRecalc();
                    break;
                }
            }
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

Optional<SVGElement*> SVGElement::ownerSVGElement()
{
    // The nearest ancestor ‘svg’ element. Null if the given element is the
    // outermost svg element.

    if (isSVGSVGElement()) {
        return nullptr;
    }

    Element* e = this;
    while (e && !e->isSVGSVGElement()) {
        if (e->isShadowRoot()) {
            e = e->asShadowRoot()->host();
        } else if (e->parentNode() && e->parentNode()->isShadowRoot()) {
            e = e->parentNode()->asShadowRoot()->host();
        } else {
            e = e->parentElement();
        }
    }

    return (SVGElement*)e;
}

Optional<SVGElement*> SVGElement::viewportElement()
{
    // The element which established the current viewport. Often, the nearest
    // ancestor ‘svg’ element. Null if the given element is the outermost svg
    // element.
    return ownerSVGElement();
}

void SVGElement::styleForPresentationAttribute(
    CSSStyleValuePairVectorHolder& cssValues,
    Optional<const MutablePropertyValueList*> cssCustomValues)
{
    Element::styleForPresentationAttribute(cssValues);
    CSSStyleValuePair pair;

    if (needsGeometryAttributes()) {
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(x, X, cssCustomValues);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(y, Y, cssCustomValues);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(x1, X1, cssCustomValues);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(y1, Y1, cssCustomValues);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(x2, X2, cssCustomValues);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(y2, Y2, cssCustomValues);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(width, Width,
                                                   cssCustomValues);
        STARFISH_SVG_PRESENTATION_ATTRIBUTE_LENGTH(height, Height,
                                                   cssCustomValues);
    }

#define UPDATE_SVG_PRESENTATION_ATTRIBUTE(name, keyName)                \
    {                                                                   \
        String* value = getAttributeOrVarReferencedValue(               \
            starfish()->staticStrings()->m_##name, cssCustomValues);    \
        if (value->length()) {                                          \
            pair.setKeyKind(CSSStyleValuePair::keyName);                \
            auto u8str = value->toUTF8NonGCString();                    \
            CSSTokenVector tokens;                                      \
            CSSStyleDeclaration::tokenizeCSSValue(tokens, u8str.data(), \
                                                  u8str.length());      \
            if (pair.updateValue##keyName(document(), tokens)) {        \
                cssValues.push_back(pair);                              \
            }                                                           \
        }                                                               \
    }

    if (needsFillAttributes()) {
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(fill, Fill);
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(fillRule, FillRule);
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(fillOpacity, FillOpacity);
    }

    if (needsStrokeAttributes()) {
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(stroke, Stroke);
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(strokeWidth, StrokeWidth);
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(strokeOpacity, StrokeOpacity);
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(strokeLineCap, StrokeLineCap);
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(strokeLineJoin, StrokeLineJoin);
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(strokeMiterLimit, StrokeMiterLimit);
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(strokeDasharray, StrokeDasharray);
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(strokeDashoffset, StrokeDashoffset);
    }

    if (needsTransparentAttributes()) {
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(opacity, Opacity);
    }

    if (needsTransformAttributes()) {
        String* transform = getAttributeOrVarReferencedValue(
            starfish()->staticStrings()->m_transform, cssCustomValues);
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
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(transformOrigin, TransformOrigin);
    }

    if (needsClipPathAttributes()) {
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(clipPath, ClipPath);
    }

    if (isRenderableElement()) {
        // The display property only applies to renderable elements.
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(display, Display);
    }

    // https://www.w3.org/TR/SVG11/masking.html#MaskProperty
    if (needsMaskAttributes()) {
        // Value:  <funciri> | none | inherit
        // <FuncIRI> : Functional notation for an IRI: "url(" <IRI> ")".
        String* maskStr = getAttributeOrVarReferencedValue(
            starfish()->staticStrings()->m_mask, cssCustomValues);
        if (!maskStr->isEmpty()) {
            pair.setKeyKind(CSSStyleValuePair::MaskImage);

            auto str = maskStr->toUTF8NonGCString();
            CSSTokenVector tokens;
            CSSStyleDeclaration::tokenizeCSSValue(tokens, str.data(),
                                                  str.length());
            if (pair.updateValueMaskImage(tokens, false)) {
                STARFISH_ASSERT(pair.valueKind() ==
                                CSSStyleValuePair::ValueKind::ValueListKind);
                STARFISH_ASSERT(pair.multiValue()->size() == 1);

                if (pair.multiValue()->at(0).valueKind() ==
                    CSSStyleValuePair::ValueKind::UrlValueKind) {
                    cssValues.push_back(pair);
                }
            }
        }
        UPDATE_SVG_PRESENTATION_ATTRIBUTE(maskType, MaskType);
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

SVGElement* SVGElement::maskElement()
{
    if (!hasMask() || !needsMaskAttributes()) {
        return nullptr;
    }

    if (!m_maskElement) {
        ImageValue* image = style()->maskImage(0);
        STARFISH_RELEASE_ASSERT(image->type() ==
                                ImageValueType::ValueType::URL);

        ResourceURL* maskURL;
        // In case that SVG element is loaded as an image resource through
        // MockHTMLIFrameElement. At this case, we can find baseURI at its
        // referrerURL.
        if (document()->baseURL()->isDataURL()) {
            maskURL =
                new ResourceURL(image->urlValue(), document()->referrer());
        } else {
            maskURL = new ResourceURL(image->urlValue(), document()->baseURI());
        }
        String* id = maskURL->getFragmentIdValue();
        if (!id->isEmpty()) {
            Element* maskElement = document()->getElementById(id);
            if (maskElement) {
                m_maskElement = (SVGElement*)maskElement;
            }
        }
    }
    return m_maskElement;
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
