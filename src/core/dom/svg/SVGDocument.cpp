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

namespace Starfish {

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
    } else if (str->m_svglinearGradientTagName == localName ||
               str->m_svglineargradientTagName == localName) {
        // FIXME: SVG tagnames should be case-sensitive
        return new SVGLinearGradientElement(document, qname);
    } else if (str->m_svgradialGradientTagName == localName ||
               str->m_svgradialgradientTagName == localName) {
        return new SVGRadialGradientElement(document, qname);
    } else if (str->m_svgstopTagName == localName) {
        return new SVGStopElement(document, qname);
    } else if (str->m_svgclippathTagName == localName) {
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
    } else {
        return new SVGElement(document, qname);
    }
}
} // namespace Starfish
