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

Element* SVGDocument::createSVGElement(Document* document,
                                       const QualifiedName& qname)
{
    // FIXME: qname is always in lowercase when passed through HTMLTokenizer
    // during document build.
    // However, if qname is passed directly through the js interface, it must be
    // strict on case sensitivity.

    // For example, with the <animateTransform> tag
    /* clang-format off
        const animateTransform1 = svgElement.createElementNS('http://www.w3.org/2000/svg', 'animatetransform')
        console.log(animateTransform1.__proto__); // print SVGElement
        const animateTransform2 = svgElement.createElementNS('http://www.w3.org/2000/svg', 'animateTransform')
        console.log(animateTransform2.__proto__); // print SVGAnimateTransformElement
    clang-format on */

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
    } else if (str->m_svglinearGradientTagName == localName ||
               str->m_svglineargradientTagName == localName) {
        return new SVGLinearGradientElement(document,
                                            str->m_svglinearGradientTagName);
    } else if (str->m_svgradialGradientTagName == localName ||
               str->m_svgradialgradientTagName == localName) {
        return new SVGRadialGradientElement(document,
                                            str->m_svgradialGradientTagName);
    } else if (str->m_svgstopTagName == localName) {
        return new SVGStopElement(document, qname);
    } else if (str->m_svgclippathTagName == localName ||
               str->m_svgclipPathTagName == localName) {
        return new SVGClipPathElement(document, str->m_svgclipPathTagName);
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
    } else if (str->m_svganimateTransformTagName == localName ||
               str->m_svganimatetransformTagName == localName) {
        return new SVGAnimateTransformElement(document, qname);
    } else if (str->m_svgfilterTagName == localName) {
        return new SVGFilterElement(document, qname);
    } else if (str->m_svgfegaussianblurTagName == localName ||
               str->m_svgfeGaussianBlurTagName == localName) {
        return new SVGFEGaussianBlurElement(document,
                                            str->m_svgfeGaussianBlurTagName);
    } else if (str->m_svgfecolormatrixTagName == localName ||
               str->m_svgfeColorMatrixTagName == localName) {
        return new SVGFEColorMatrixElement(document,
                                           str->m_svgfeColorMatrixTagName);
    } else if (str->m_svgfecomponenttransferTagName == localName ||
               str->m_svgfeComponentTransferTagName == localName) {
        return new SVGFEComponentTransferElement(
            document, str->m_svgfeComponentTransferTagName);
    } else if (str->m_svgfeFuncATagName == localName ||
               str->m_svgfefuncaTagName == localName) {
        return new SVGFEFuncAElement(document, str->m_svgfeFuncATagName);
    } else if (str->m_svgfeFuncRTagName == localName ||
               str->m_svgfefuncrTagName == localName) {
        return new SVGFEFuncRElement(document, str->m_svgfeFuncRTagName);
    } else if (str->m_svgfeFuncGTagName == localName ||
               str->m_svgfefuncgTagName == localName) {
        return new SVGFEFuncGElement(document, str->m_svgfeFuncGTagName);
    } else if (str->m_svgfeFuncBTagName == localName ||
               str->m_svgfefuncbTagName == localName) {
        return new SVGFEFuncBElement(document, str->m_svgfeFuncBTagName);
    } else if (str->m_svgfeMergeTagName == localName ||
               str->m_svgfemergeTagName == localName) {
        return new SVGFEMergeElement(document, str->m_svgfeMergeTagName);
    } else if (str->m_svgfeMergeNodeTagName == localName ||
               str->m_svgfemergenodeTagName == localName) {
        return new SVGFEMergeNodeElement(document,
                                         str->m_svgfeMergeNodeTagName);
    } else if (str->m_svgfecompositeTagName == localName ||
               str->m_svgfeCompositeTagName == localName) {
        return new SVGFECompositeElement(document,
                                         str->m_svgfeCompositeTagName);
    } else if (str->m_svgfemorphologyTagName == localName ||
               str->m_svgfeMorphologyTagName == localName) {
        return new SVGFEMorphologyElement(document,
                                          str->m_svgfeMorphologyTagName);
    } else if (str->m_svgfeoffsetTagName == localName ||
               str->m_svgfeOffsetTagName == localName) {
        return new SVGFEOffsetElement(document, str->m_svgfeOffsetTagName);
    } else if (str->m_svgfefloodTagName == localName ||
               str->m_svgfeFloodTagName == localName) {
        return new SVGFEFloodElement(document, str->m_svgfeFloodTagName);
    } else if (str->m_svgfeturbulenceTagName == localName ||
               str->m_svgfeTurbulenceTagName == localName) {
        return new SVGFETurbulenceElement(document,
                                          str->m_svgfeTurbulenceTagName);
    } else if (str->m_svgfeDisplacementMapTagName == localName ||
               str->m_svgfedisplacementmapTagName == localName) {
        return new SVGFEDisplacementMapElement(
            document, str->m_svgfeDisplacementMapTagName);
    } else {
        return new SVGElement(document, qname);
    }
}
} // namespace Starfish
