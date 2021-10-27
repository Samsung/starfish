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
#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "FrameOptionBox.h"
#include "core/dom/HTMLOptionElement.h"
#include "core/dom/HTMLSelectElement.h"
#include "core/dom/PseudoElement.h"
#include "core/dom/Text.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameSelectBox.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameText.h"
#include "core/layout/FrameTreeBuilder.h"
namespace Starfish {

static ComputedStyle* createStyleFrom(Node* node)
{
    ComputedStyle* childStyle = new ComputedStyle(node->style());
    childStyle->loadResources(node);
    childStyle->arrangeStyleValues(node->style(), node);
    childStyle->setDisplay(DisplayValue::InlineDisplayValue);
    return childStyle;
}

void* FrameOptionBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameOptionBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameOptionBox)] = { 0 };
        FrameOptionBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameOptionBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

FrameOptionBox::FrameOptionBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

FrameSelectBox* FrameOptionBox::selectBox()
{
    for (Frame* p = parent(); p; p = p->parent()) {
        if (p->node() && (p->node()->isHTMLIFrameElement() ||
                          p->node()->isHTMLFormElement())) {
            return nullptr;
        }

        if (p->isFrameSelectBox()) {
            return p->asFrameSelectBox();
        }
    }

    return nullptr;
}

FrameOptionBox* FrameOptionBox::buildFrameTree(Node* currentNode,
                                               FrameTreeBuilderContext& ctx,
                                               bool force)
{
    FrameOptionBox* currentFrame = nullptr;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    if (currentNode->needsFrameTreeBuild() || force) {
        currentFrame = new FrameOptionBox(currentNode, nullptr);
        force = true;
    } else {
        currentFrame = currentNode->frame()->asFrameOptionBox();
    }

    if (currentNode->childNeedsFrameTreeBuild() || force) {
        ctx.setCurrentBlockContainer(currentFrame);
        if (currentNode->asHTMLOptionElement()->hasLabel()) {
            // Generate pseudo element
            PseudoElement* textElement =
                new PseudoElement(currentNode->document(), nullptr,
                                  PseudoElementType::PseudoElementFormOnly);
            textElement->setParentNode(currentNode);

            // Set style
            ComputedStyle* pseudoStyle = createStyleFrom(currentNode);
            pseudoStyle->setWhiteSpace(WhiteSpaceValue::PreWhiteSpaceValue);
            textElement->setStyle(pseudoStyle);

            HTMLOptionElement* option = currentNode->asHTMLOptionElement();
            // Edit text value to show
            String* textValue = option->label();
            // Generate text node
            Text* textNode = new Text(currentNode->document(), textValue);
            textNode->setParentNode(textElement);
            ComputedStyle* textStyle = createStyleFrom(textElement);
            textNode->setStyle(textStyle);

            FrameText* frameText = new FrameText(textNode, textStyle);
            textNode->setFrame(frameText);
            currentFrame->appendChild(frameText);
        }
        ctx.setCurrentBlockContainer(parent);
    }

    STARFISH_ASSERT(currentFrame);
    return currentFrame;
}

void FrameOptionBox::layout(LayoutContext& ctx,
                            Frame::LayoutWantToResolve resolveWhat)
{
    if (node() && node()->isHTMLOptionElement() &&
        node()->asHTMLOptionElement()->isDisabled()) {
        return;
    }

    STARFISH_ASSERT(node()->isHTMLOptionElement());
    HTMLOptionElement* optionNode = node()->asHTMLOptionElement();
    HTMLSelectElement* selectNode = optionNode->selectElement();

    if (!selectNode) {
        FrameBlockBox::layout(ctx, resolveWhat);
    } else {
        if (selectNode->displaySize() == 1) {
            if (optionNode->selectedness() &&
                selectBox()->m_drawOptionsCount == 0) {
                if (resolveWhat & ResolveHeight) {
                    selectBox()->m_drawOptionsCount++;
                }
                FrameBlockBox::layout(ctx, resolveWhat);
            }
        } else if (selectNode->displaySize() > 1 &&
                   selectBox()->m_drawOptionsCount <
                       selectNode->displaySize()) {
            if (resolveWhat & ResolveHeight) {
                selectBox()->m_drawOptionsCount++;
            }
            FrameBlockBox::layout(ctx, resolveWhat);
        }
    }
}
} // namespace Starfish
