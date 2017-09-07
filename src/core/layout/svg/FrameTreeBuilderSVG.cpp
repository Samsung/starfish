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
#include "core/layout/FrameTreeBuilder.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/svg/SVGElement.h"

#include "core/layout/svg/FrameSVGSVGBox.h"
#include "core/layout/svg/FrameSVGRectBox.h"

namespace StarFish {

Frame* FrameTreeBuilder::buildSVGFrameTree(SVGElement* svgElement)
{
    Frame* parentFrame = svgElement->parentElement()->frame();

    ComputedStyle* style = svgElement->style();
    if (!style || style->display() == DisplayValue::NoneDisplayValue) {
        FrameTreeBuilder::clearTree(svgElement);
        svgElement->setFrame(nullptr);
        svgElement->clearNeedsFrameTreeBuild();
        return nullptr;
    }

    bool shouldContinue = false;

    Frame* currentFrame = nullptr;
    if (svgElement->isSVGSVGElement()) {
        shouldContinue = true;
        currentFrame = new FrameSVGSVGBox(svgElement);
    } else if (svgElement->isSVGRectElement()) {
        shouldContinue = true;
        currentFrame = new FrameSVGRectBox(svgElement);
    }

    svgElement->clearNeedsFrameTreeBuild();

    if (shouldContinue) {
        parentFrame->appendChild(currentFrame);
        svgElement->setFrame(currentFrame);

        if (svgElement->childNeedsFrameTreeBuild()) {
            Element* e = svgElement->firstElementChild();
            while (e) {
                if (e->isSVGElement())
                    buildSVGFrameTree(e->asSVGElement());
                e = e->nextElementSibling();
            }

            svgElement->clearChildNeedsFrameTreeBuild();
        }
        return currentFrame;
    }

    return nullptr;
}
}
