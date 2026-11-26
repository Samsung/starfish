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
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGAnimateElement.h"
#include "core/dom/svg/SVGAnimateTransformElement.h"
#include "core/dom/svg/SVGAnimateMotionElement.h"
#include "core/dom/svg/SVGSVGElement.h"
#include "core/dom/svg/SVGRectElement.h"
#include "core/dom/svg/SVGPathElement.h"
#include "core/dom/svg/SVGGElement.h"
#include "core/dom/svg/SVGPolygonElement.h"
#include "core/dom/svg/SVGPolylineElement.h"
#include "core/dom/svg/SVGCircleElement.h"
#include "core/dom/svg/SVGEllipseElement.h"
#include "core/dom/svg/SVGImageElement.h"
#include "core/dom/svg/SVGTextElement.h"
#include "core/dom/svg/SVGScriptElement.h"
#include "core/dom/svg/SVGStyleElement.h"
#include "core/dom/svg/SVGLineElement.h"
#include "core/dom/svg/SVGUseElement.h"
#include "core/dom/svg/SVGDefsElement.h"
#include "core/dom/svg/SVGLinearGradientElement.h"
#include "core/dom/svg/SVGRadialGradientElement.h"
#include "core/dom/svg/SVGStopElement.h"
#include "core/dom/svg/SVGClipPathElement.h"
#include "core/dom/svg/SVGMaskElement.h"
#include "core/dom/svg/SVGMPathElement.h"
#include "core/dom/svg/SVGTSpanElement.h"
#include "core/dom/svg/SVGMarkerElement.h"
#include "core/dom/svg/SVGSwitchElement.h"
#include "core/dom/svg/SVGSymbolElement.h"
#include "core/dom/svg/SVGFilterElement.h"
#include "core/dom/svg/SVGFEGaussianBlurElement.h"
#include "core/dom/svg/SVGFEColorMatrixElement.h"
#include "core/dom/svg/SVGFEComponentTransferElement.h"
#include "core/dom/svg/SVGFEMergeElement.h"
#include "core/dom/svg/SVGFEMergeNodeElement.h"
#include "core/dom/svg/SVGComponentTransferFunctionElement.h"
#include "core/dom/svg/SVGFECompositeElement.h"
#include "core/dom/svg/SVGFEMorphologyElement.h"
#include "core/dom/svg/SVGFEOffsetElement.h"
#include "core/dom/svg/SVGFEFloodElement.h"
#include "core/dom/svg/SVGFETurbulenceElement.h"
#include "core/dom/svg/SVGFEDisplacementMapElement.h"

namespace Starfish {

QualifiedName SVGDocument::adjustSVGTagName(Starfish* starfish,
                                            const QualifiedName& qname)
{
    // HTML § 13.2.6.5 ("adjust SVG tag names"): HTMLTokenizer lowercases all
    // tag names; restore the spec-defined camelCase spellings for SVG elements
    // inserted by the HTML parser.
    auto data = qname.localName()->bufferAccessData();
    if (!data.hasASCIIData() || data.length < 6 || data.length > 19) {
        return qname;
    }

    struct TagCaseEntry {
        const char* lower;
        const char* adjusted;
        uint8_t length;
    };
    static constexpr TagCaseEntry kTable[] = {
        { "altglyph", "altGlyph", 8 },
        { "altglyphdef", "altGlyphDef", 11 },
        { "altglyphitem", "altGlyphItem", 12 },
        { "animatecolor", "animateColor", 12 },
        { "animatemotion", "animateMotion", 13 },
        { "animatetransform", "animateTransform", 16 },
        { "clippath", "clipPath", 8 },
        { "feblend", "feBlend", 7 },
        { "fecolormatrix", "feColorMatrix", 13 },
        { "fecomponenttransfer", "feComponentTransfer", 19 },
        { "fecomposite", "feComposite", 11 },
        { "feconvolvematrix", "feConvolveMatrix", 16 },
        { "fediffuselighting", "feDiffuseLighting", 17 },
        { "fedisplacementmap", "feDisplacementMap", 17 },
        { "fedistantlight", "feDistantLight", 14 },
        { "fedropshadow", "feDropShadow", 12 },
        { "feflood", "feFlood", 7 },
        { "fefunca", "feFuncA", 7 },
        { "fefuncb", "feFuncB", 7 },
        { "fefuncg", "feFuncG", 7 },
        { "fefuncr", "feFuncR", 7 },
        { "fegaussianblur", "feGaussianBlur", 14 },
        { "feimage", "feImage", 7 },
        { "femerge", "feMerge", 7 },
        { "femergenode", "feMergeNode", 11 },
        { "femorphology", "feMorphology", 12 },
        { "feoffset", "feOffset", 8 },
        { "fepointlight", "fePointLight", 12 },
        { "fespecularlighting", "feSpecularLighting", 18 },
        { "fespotlight", "feSpotLight", 11 },
        { "fetile", "feTile", 6 },
        { "feturbulence", "feTurbulence", 12 },
        { "foreignobject", "foreignObject", 13 },
        { "glyphref", "glyphRef", 8 },
        { "lineargradient", "linearGradient", 14 },
        { "radialgradient", "radialGradient", 14 },
        { "textpath", "textPath", 8 },
    };

    const char* target = data.asciiData();
    size_t targetLen = data.length;
    size_t lo = 0;
    size_t hi = sizeof(kTable) / sizeof(kTable[0]);
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        const TagCaseEntry& e = kTable[mid];
        size_t minLen = e.length < targetLen ? e.length : targetLen;
        int cmp = std::memcmp(e.lower, target, minLen);
        if (cmp == 0 && e.length == targetLen) {
            return QualifiedName(qname.namespaceURI().getValue(),
                                 AtomicString::createAtomicString(
                                     starfish, e.adjusted, e.length));
        }
        if (cmp < 0 || (cmp == 0 && e.length < targetLen)) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return qname;
}

Element* SVGDocument::createSVGElement(Document* document,
                                       const QualifiedName& qname)
{
    StaticStrings* str = document->starfish()->staticStrings();
    AtomicString localName = qname.localNameAtomic();

    if (str->m_svgsvgTagName == localName) {
        return new SVGSVGElement(document, qname);
    } else if (str->m_svgrectTagName == localName) {
        return new SVGRectElement(document, qname);
    } else if (str->m_svggTagName == localName) {
        return new SVGGElement(document, qname);
    } else if (str->m_svgpathTagName == localName) {
        return new SVGPathElement(document, qname);
    } else if (str->m_svgcircleTagName == localName) {
        return new SVGCircleElement(document, qname);
    } else if (str->m_svgpolygonTagName == localName) {
        return new SVGPolygonElement(document, qname);
    } else if (str->m_svgpolylineTagName == localName) {
        return new SVGPolylineElement(document, qname);
    } else if (str->m_svgimageTagName == localName) {
        return new SVGImageElement(document, qname);
    } else if (str->m_svgtextTagName == localName) {
        return new SVGTextElement(document, qname);
    } else if (str->m_svgstyleTagName == localName) {
        return new SVGStyleElement(document, qname);
    } else if (str->m_svglineTagName == localName) {
        return new SVGLineElement(document, qname);
    } else if (str->m_svgellipseTagName == localName) {
        return new SVGEllipseElement(document, qname);
    } else if (str->m_svguseTagName == localName) {
        return new SVGUseElement(document, qname);
    } else if (str->m_svgdefsTagName == localName) {
        return new SVGDefsElement(document, qname);
    } else if (str->m_svglinearGradientTagName == localName) {
        return new SVGLinearGradientElement(document, qname);
    } else if (str->m_svgradialGradientTagName == localName) {
        return new SVGRadialGradientElement(document, qname);
    } else if (str->m_svgstopTagName == localName) {
        return new SVGStopElement(document, qname);
    } else if (str->m_svgclipPathTagName == localName) {
        return new SVGClipPathElement(document, qname);
    } else if (str->m_svgscriptTagName == localName) {
        return new SVGScriptElement(document, qname);
    } else if (str->m_svgmaskTagName == localName) {
        return new SVGMaskElement(document, qname);
    } else if (str->m_svgtspanTagName == localName) {
        return new SVGTSpanElement(document, qname);
    } else if (str->m_svgmarkerTagName == localName) {
        return new SVGMarkerElement(document, qname);
    } else if (str->m_svgswitchTagName == localName) {
        return new SVGSwitchElement(document, qname);
    } else if (str->m_svgsymbolTagName == localName) {
        return new SVGSymbolElement(document, qname);
    } else if (str->m_svganimateTagName == localName) {
        return new SVGAnimateElement(document, qname);
    } else if (str->m_svganimateTransformTagName == localName) {
        return new SVGAnimateTransformElement(document, qname);
    } else if (str->m_svgfilterTagName == localName) {
        return new SVGFilterElement(document, qname);
    } else if (str->m_svgfeGaussianBlurTagName == localName) {
        return new SVGFEGaussianBlurElement(document, qname);
    } else if (str->m_svgfeColorMatrixTagName == localName) {
        return new SVGFEColorMatrixElement(document, qname);
    } else if (str->m_svgfeComponentTransferTagName == localName) {
        return new SVGFEComponentTransferElement(document, qname);
    } else if (str->m_svgfeFuncATagName == localName) {
        return new SVGFEFuncAElement(document, qname);
    } else if (str->m_svgfeFuncRTagName == localName) {
        return new SVGFEFuncRElement(document, qname);
    } else if (str->m_svgfeFuncGTagName == localName) {
        return new SVGFEFuncGElement(document, qname);
    } else if (str->m_svgfeFuncBTagName == localName) {
        return new SVGFEFuncBElement(document, qname);
    } else if (str->m_svgfeMergeTagName == localName) {
        return new SVGFEMergeElement(document, qname);
    } else if (str->m_svgfeMergeNodeTagName == localName) {
        return new SVGFEMergeNodeElement(document, qname);
    } else if (str->m_svgfeCompositeTagName == localName) {
        return new SVGFECompositeElement(document, qname);
    } else if (str->m_svgfeMorphologyTagName == localName) {
        return new SVGFEMorphologyElement(document, qname);
    } else if (str->m_svgfeOffsetTagName == localName) {
        return new SVGFEOffsetElement(document, qname);
    } else if (str->m_svgfeFloodTagName == localName) {
        return new SVGFEFloodElement(document, qname);
    } else if (str->m_svgfeTurbulenceTagName == localName) {
        return new SVGFETurbulenceElement(document, qname);
    } else if (str->m_svgfeDisplacementMapTagName == localName) {
        return new SVGFEDisplacementMapElement(document, qname);
    } else if (str->m_svganimateMotionTagName == localName) {
        return new SVGAnimateMotionElement(document, qname);
    } else if (str->m_svgmpathTagName == localName) {
        return new SVGMPathElement(document, qname);
    } else {
        return new SVGElement(document, qname);
    }
}
} // namespace Starfish
