/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#include "core/dom/svg/SVGDocument.h"
#include "core/dom/svg/SVGSVGElement.h"
#include "core/dom/svg/SVGRectElement.h"
#include "core/dom/svg/SVGPathElement.h"
#include "core/dom/svg/SVGGElement.h"
#include "core/dom/svg/SVGPolygonElement.h"
#include "core/dom/svg/SVGPolylineElement.h"
#include "core/dom/svg/SVGCircleElement.h"
#include "core/dom/svg/SVGStyleElement.h"
#include "StarFish.h"

namespace StarFish {

Element* SVGDocument::createSVGElement(Document* document,
                                       AtomicString localName)
{
    StaticStrings* str = document->starFish()->staticStrings();
    if (str->m_svgsvgTagName == localName) {
        return new SVGSVGElement(document);
    } else if (str->m_svgrectTagName == localName) {
        return new SVGRectElement(document);
    } else if (str->m_svggTagName == localName) {
        return new SVGGElement(document);
    } else if (str->m_svgpathTagName == localName) {
        return new SVGPathElement(document);
    } else if (str->m_svgcircleTagName == localName) {
        return new SVGCircleElement(document);
    } else if (str->m_svgpolygonTagName == localName) {
        return new SVGPolygonElement(document);
    } else if (str->m_svgpolylineTagName == localName) {
        return new SVGPolylineElement(document);
    } else if (str->m_svgstyleTagName == localName) {
        return new SVGStyleElement(document);
    } else {
        return new SVGNamedElement(
            document, QualifiedName(str->m_svgNamespaceURI, localName));
    }
}
}
