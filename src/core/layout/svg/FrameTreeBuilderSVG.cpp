/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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
#include "core/layout/FrameTreeBuilder.h"
#include "core/dom/Node.h"
#include "core/dom/svg/SVGElement.h"
#include "core/dom/svg/SVGUseElement.h"
#include "core/dom/Text.h"
#include "core/dom/ShadowRoot.h"
#include "core/style/AncestorSelectorFilter.h"
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/layout/svg/FrameSVGSVGBox.h"
#include "core/layout/svg/FrameSVGRectBox.h"
#include "core/layout/svg/FrameSVGPathBox.h"
#include "core/layout/svg/FrameSVGPolygonBox.h"
#include "core/layout/svg/FrameSVGPolylineBox.h"
#include "core/layout/svg/FrameSVGCircleBox.h"
#include "core/layout/svg/FrameSVGEllipseBox.h"
#include "core/layout/svg/FrameSVGImageBox.h"
#include "core/layout/svg/FrameSVGTextBox.h"
#include "core/layout/svg/FrameSVGLineBox.h"
#include "core/layout/FrameBlockBox.h"

#include "core/dom/Document.h"

namespace Starfish {

Frame* FrameTreeBuilder::buildSVGFrameTree(SVGElement* svgElement,
                                           Frame* parentFrame)
{
    ComputedStyle* style = svgElement->style();
    if (!style || style->display() == DisplayValue::NoneDisplayValue) {
        FrameTreeBuilder::clearTree(svgElement);
        svgElement->setFrame(nullptr);
        svgElement->clearNeedsFrameTreeBuild();
        return nullptr;
    }

    bool shouldContinue = false;
    bool shouldVisitChild = false;

    Frame* currentFrame = nullptr;
    if (svgElement->isSVGSVGElement()) {
        shouldContinue = true;
        shouldVisitChild = true;
        currentFrame = new FrameSVGSVGBox(svgElement);
    } else if (svgElement->isSVGRectElement()) {
        shouldContinue = true;
        currentFrame = new FrameSVGRectBox(svgElement);
    } else if (svgElement->isSVGGElement()) {
        shouldContinue = true;
        currentFrame = new FrameSVGBox(svgElement);
        shouldVisitChild = true;
    } else if (svgElement->isSVGPathElement()) {
        shouldContinue = true;
        currentFrame = new FrameSVGPathBox(svgElement);
    } else if (svgElement->isSVGPolygonElement()) {
        shouldContinue = true;
        currentFrame = new FrameSVGPolygonBox(svgElement);
    } else if (svgElement->isSVGPolylineElement()) {
        shouldContinue = true;
        currentFrame = new FrameSVGPolylineBox(svgElement);
    } else if (svgElement->isSVGCircleElement()) {
        shouldContinue = true;
        currentFrame = new FrameSVGCircleBox(svgElement);
    } else if (svgElement->isSVGImageElement()) {
        shouldContinue = true;
        currentFrame = new FrameSVGImageBox(svgElement);
    } else if (svgElement->isSVGEllipseElement()) {
        shouldContinue = true;
        currentFrame = new FrameSVGEllipseBox(svgElement);
    } else if (svgElement->isSVGLineElement()) {
        shouldContinue = true;
        currentFrame = new FrameSVGLineBox(svgElement);
    } else if (svgElement->isSVGTextElement()) {
        shouldContinue = true;
        auto txt = svgElement->textContent();
        String* content = String::emptyString;
        if (txt.hasValue()) {
            content = txt.getValue();
        }
        currentFrame = new FrameSVGTextBox(svgElement);

        ComputedStyle* style = new ComputedStyle(svgElement->style());
        style->setDisplay(DisplayValue::BlockDisplayValue);
        style->loadResources(svgElement);
        style->arrangeStyleValues(svgElement->style(), svgElement);
        style->setWhiteSpace(WhiteSpaceValue::NoWrapWhiteSpaceValue);
        FrameBlockBox* box = new FrameBlockBox(nullptr, style);
        currentFrame->appendChild(box);

        Text* textNode = new Text(svgElement->document(), content);
        ComputedStyle* textStyle = new ComputedStyle(style);
        textStyle->loadResources(svgElement);
        textStyle->arrangeStyleValues(svgElement->style(), svgElement);
        textNode->setStyle(textStyle);

        auto ft = new FrameText(textNode, textStyle);
        textNode->setFrame(ft);
        box->appendChild(ft);
    } else if (svgElement->isSVGDefsElement()) {
    } else if (svgElement->isSVGUseElement()) {
        // Style resolve for shadow tree of SVGUseElement
        {
            currentFrame = new FrameSVGBox(svgElement);
            parentFrame->appendChild(currentFrame);
            svgElement->setFrame(currentFrame);

            ShadowRoot* sr = svgElement->asElement()->shadowRoot();
            Document* document = svgElement->document();

            if (sr->hasChildNodes()) {
                // build frame tree
                Node* shadowFirstChild = sr->firstChild();
                if (shadowFirstChild->isSVGElement()) {
                    buildSVGFrameTree(shadowFirstChild->asSVGElement(),
                                      currentFrame);
                }
            }

            svgElement->clearNeedsFrameTreeBuild();
            svgElement->clearChildNeedsFrameTreeBuild();
            return currentFrame;
        }
    }

    svgElement->clearNeedsFrameTreeBuild();

    if (shouldContinue) {
        if (!svgElement->isSVGSVGElement() || parentFrame->isFrameSVGSVGBox()) {
            if (currentFrame->isFrameSVGSVGBox()) {
                ((FrameSVGSVGBox*)currentFrame)->setInnerSVG(true);
            }
            parentFrame->appendChild(currentFrame);
        }
        svgElement->setFrame(currentFrame);

        if (shouldVisitChild) {
            Element* e = svgElement->firstElementChild();
            while (e) {
                if (e->isSVGElement())
                    buildSVGFrameTree(e->asSVGElement(),
                                      e->parentElement()->frame());
                e = e->nextElementSibling();
            }
        }

        svgElement->clearChildNeedsFrameTreeBuild();
        return currentFrame;
    }

    return nullptr;
}
}
