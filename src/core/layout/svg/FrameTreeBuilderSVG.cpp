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
#include "core/layout/svg/FrameSVGClipPathBox.h"
#include "core/layout/svg/FrameSVGInvisibleBox.h"
#include "core/layout/svg/FrameSVGUseBox.h"
#include "core/layout/svg/FrameSVGViewportContextBox.h"
#include "core/layout/svg/FrameSVGMaskBox.h"
#include "core/layout/FrameBlockBox.h"

#include "core/dom/Document.h"

namespace Starfish {

static bool shouldVisitChild(SVGElement* svgElement)
{
    if (svgElement->isSVGSVGElement() || svgElement->isSVGGElement()
            || svgElement->isSVGDefsElement() || svgElement->isSVGUseElement()
            || svgElement->isSVGClipPathElement()
            || svgElement->isSVGMaskElement()
            || svgElement->isSVGSwitchElement()
            || svgElement->isSVGAnimateElement()) {
        return true;
    }
    return false;
}

Frame* FrameTreeBuilder::buildSVGFrameTree(SVGElement* svgElement,
                                           Optional<Frame*> parentFrame, bool force)
{
    ComputedStyle* style = svgElement->style();
    if (!style || style->display() == DisplayValue::NoneDisplayValue) {
        FrameTreeBuilder::clearTree(svgElement);
        svgElement->setFrame(nullptr);
        svgElement->clearNeedsFrameTreeBuild();
        return nullptr;
    }

    Optional<Frame*> newFrame;
    if (svgElement->needsFrameTreeBuild() || force) {
        if (svgElement->frame()) {
            clearTree(svgElement);
        }

        if (svgElement->isSVGSVGElement()) {
            bool isInnerSVG = false;
            auto e = svgElement->renderingParentElement();
            while (e) {
                if (e->isSVGSVGElement()) {
                    isInnerSVG = true;
                    break;
                }
                e = e->renderingParentElement();
            }
            if (isInnerSVG) {
                newFrame = new FrameSVGViewportContextBox(svgElement);
            } else {
                newFrame = new FrameSVGSVGBox(svgElement);
            }
        } else if (svgElement->isSVGRectElement()) {
            newFrame = new FrameSVGRectBox(svgElement);
        } else if (svgElement->isSVGGElement()) {
            newFrame = new FrameSVGBox(svgElement);
        } else if (svgElement->isSVGPathElement()) {
            newFrame = new FrameSVGPathBox(svgElement);
        } else if (svgElement->isSVGPolygonElement()) {
            newFrame = new FrameSVGPolygonBox(svgElement);
        } else if (svgElement->isSVGPolylineElement()) {
            newFrame = new FrameSVGPolylineBox(svgElement);
        } else if (svgElement->isSVGCircleElement()) {
            newFrame = new FrameSVGCircleBox(svgElement);
        } else if (svgElement->isSVGImageElement()) {
            newFrame = new FrameSVGImageBox(svgElement);
        } else if (svgElement->isSVGEllipseElement()) {
            newFrame = new FrameSVGEllipseBox(svgElement);
        } else if (svgElement->isSVGLineElement()) {
            newFrame = new FrameSVGLineBox(svgElement);
        } else if (svgElement->isSVGTextElement()) {
            newFrame = new FrameSVGTextBox(svgElement);

            ComputedStyle* style = new ComputedStyle(svgElement->style());
            style->setDisplay(DisplayValue::BlockDisplayValue);
            style->loadResources(svgElement);
            style->arrangeStyleValues(svgElement->style(), svgElement);
            style->setWhiteSpace(WhiteSpaceValue::NoWrapWhiteSpaceValue);
            FrameBlockBox* box = new FrameBlockBox(nullptr, style);
            newFrame->appendChild(box);

            String* content = String::emptyString;
            for (Node* child = svgElement->firstChild(); child != nullptr;
                 child = child->nextSibling()) {
                if (child->isSVGTSpanElement() && (!child->style()->x().isAuto() ||
                                                   !child->style()->y().isAuto())) {
                    buildSVGFrameTree(child->asSVGElement(), newFrame.value(), true);
                } else if (child->isText() || child->isElement()) {
                    STARFISH_ASSERT(child->textContent().hasValue());
                    content = child->textContent().getValue();
                    Text* textNode = new Text(svgElement->document(), content);
                    ComputedStyle* textStyle = new ComputedStyle(style);
                    textStyle->loadResources(svgElement);
                    textStyle->arrangeStyleValues(svgElement->style(), svgElement);
                    textStyle->setColor(style->fill()->color());
                    textNode->setStyle(textStyle);

                    auto ft = new FrameText(textNode, textStyle);
                    textNode->setFrame(ft);
                    box->appendChild(ft);
                }
            }
        } else if (svgElement->isSVGDefsElement()) {
            newFrame = new FrameSVGInvisibleBox(svgElement);
        } else if (svgElement->isSVGUseElement()) {
            newFrame = new FrameSVGUseBox(svgElement);
        } else if (svgElement->isSVGClipPathElement()) {
            newFrame = new FrameSVGClipPathBox(svgElement);
        } else if (svgElement->isSVGMaskElement()) {
            newFrame = new FrameSVGMaskBox(svgElement);
        } else if (svgElement->isSVGSwitchElement()) {
            newFrame = new FrameSVGBox(svgElement);
        } else if (svgElement->isSVGTSpanElement()) {
            auto txt = svgElement->textContent();
            String* content = String::emptyString;
            if (txt.hasValue()) {
                content = txt.getValue();
            }
            newFrame = new FrameSVGTextBox(svgElement);

            ComputedStyle* style = new ComputedStyle(svgElement->style());
            style->setDisplay(DisplayValue::BlockDisplayValue);
            style->loadResources(svgElement);
            style->arrangeStyleValues(svgElement->style(), svgElement);
            style->setWhiteSpace(WhiteSpaceValue::NoWrapWhiteSpaceValue);
            FrameBlockBox* box = new FrameBlockBox(nullptr, style);
            newFrame->appendChild(box);

            Text* textNode = new Text(svgElement->document(), content);
            ComputedStyle* textStyle = new ComputedStyle(style);
            textStyle->loadResources(svgElement);
            textStyle->arrangeStyleValues(svgElement->style(), svgElement);
            textStyle->setColor(style->fill()->color());
            textNode->setStyle(textStyle);

            auto ft = new FrameText(textNode, textStyle);
            textNode->setFrame(ft);
            box->appendChild(ft);
        } else if (svgElement->isSVGAnimateElement()) {
            newFrame = new FrameSVGInvisibleBox(svgElement);
        }

        // update clipPath element
        if (svgElement->hasClipPath()) {
            svgElement->clipPathElement();
            if (newFrame && newFrame->isFrameSVGBox()) {
                newFrame->asFrameSVGBox()->markHasClipPath();
            }
        }

        if (svgElement->hasMask()) {
            if (newFrame && newFrame->isFrameSVGBox()) {
                newFrame->asFrameSVGBox()->markHasMask();
            }
        }
    }

    svgElement->clearNeedsFrameTreeBuild();

    if (newFrame) {
        force = true;
        if (parentFrame) {
            Optional<Element*> prevElement = svgElement->previousElementSibling();
            while (prevElement.hasValue() && !prevElement->frame()) {
                prevElement = prevElement->previousElementSibling();
            }
            if (prevElement) {
                parentFrame->insertBefore(prevElement->frame()->next(), newFrame.value());
            } else {
                parentFrame->insertBefore(parentFrame->firstChild(), newFrame.value());
            }
        }
        svgElement->setFrame(newFrame.value());
    }

    if (shouldVisitChild(svgElement) && (svgElement->childNeedsFrameTreeBuild() || force)) {
        Element* e = svgElement->firstElementChild();
        if (svgElement->isSVGUseElement()) {
            e = svgElement->internalEnsureShadowRoot()->firstElementChild();
        }
        while (e) {
            if (e->isSVGElement()) {
                buildSVGFrameTree(e->asSVGElement(), svgElement->frame(), force);
            }
            e = e->nextElementSibling();
        }
    }
    svgElement->clearChildNeedsFrameTreeBuild();
    return svgElement->frame();
}
} // namespace Starfish
