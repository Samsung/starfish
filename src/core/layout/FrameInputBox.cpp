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

#include "FrameInputBox.h"

#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/Text.h"
#include "core/dom/PseudoElement.h"
#include "core/page/Window.h"

#include "core/layout/Frame.h"
#include "core/layout/FrameText.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/modules/canvas/Canvas.h"

namespace StarFish {

FrameInputBox::FrameInputBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

FrameInputBox* FrameInputBox::buildFrameTree(Node* current,
                                             FrameTreeBuilderContext& ctx,
                                             bool force)
{
    if (current->asHTMLInputElement()->type()->equals("hidden")) {
        return nullptr;
    }

    FrameInputBox* currentFrame = nullptr;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    if (current->needsFrameTreeBuild() || force) {
        currentFrame = new FrameInputBox(current, nullptr);
        force = true;
    } else {
        currentFrame = current->frame()->asFrameInputBox();
    }

    ctx.setCurrentBlockContainer(currentFrame);

    STARFISH_ASSERT(current->isHTMLInputElement());

    HTMLInputElement* inputNode = current->asHTMLInputElement();
    if (inputNode->canHaveValue()) {
        // NOTE: Input boxes display the text value assigned to "value"
        // attribute. To integrate with current layout, a tmp pseudo element is
        // created to display the text value.
        PseudoElement* textElement = new PseudoElement(
            current->document(),
            StyleResolver::PseudoElementType::PseudoElementFormOnly);
        textElement->setParentNode(current);
        ComputedStyle* pseudoStyle = createInputElementStyleFrom(current);
        pseudoStyle->setWhiteSpace(WhiteSpaceValue::PreWhiteSpaceValue);
        textElement->setStyle(pseudoStyle);

        Text* textNode =
            new Text(current->document(), inputNode->visibleValue());
        textNode->setParentNode(textElement);
        ComputedStyle* textStyle = createInputElementStyleFrom(textElement);
        textNode->setStyle(textStyle);

        FrameText* frameText = new FrameText(textNode, textStyle);
        textNode->setFrame(frameText);
        currentFrame->appendChild(frameText);
    }

    ctx.setCurrentBlockContainer(parent);

    STARFISH_ASSERT(currentFrame);
    return currentFrame;
}

ComputedStyle* FrameInputBox::createInputElementStyleFrom(Node* parent)
{
    ComputedStyle* childStyle = new ComputedStyle(parent->style());
    childStyle->loadResources(parent, true);
    childStyle->arrangeStyleValues(parent->style(), true);
    childStyle->setDisplay(DisplayValue::InlineDisplayValue);
    return childStyle;
}

void FrameInputBox::layout(LayoutContext& ctx,
                           Frame::LayoutWantToResolve resolveWhat)
{
    if ((node()->asHTMLInputElement()->type()->equals("checkbox"))) {
        Length fontSize;
        bool parentHasFixedHeight = ctx.parentHasFixedHeight(this);
        if (style()->width().isAuto() || style()->height().isAuto() ||
            !style()->height().isDefinite(parentHasFixedHeight)) {
            fontSize = Length(Length::Fixed, DEFAULT_FONT_SIZE);
        } else {
            LayoutUnit parentContentHeight;
            if (parentHasFixedHeight) {
                parentContentHeight = ctx.parentFixedHeight(this);
            }

            LayoutUnit width = style()->width().specifiedValue(
                ctx.parentContentWidth(this), this);
            LayoutUnit height =
                style()->height().specifiedValue(parentContentHeight, this);
            fontSize = Length(Length::Fixed, std::min(width, height));
        }

        style()->setFontSize(fontSize);
        style()->loadFont(node(), fontSize.fixed());
        // TODO: propagate fontsize
    }

    FrameBlockBox::layout(ctx, resolveWhat);

    HTMLInputElement* e = node()->asHTMLInputElement();
    size_t cPos = e->m_currentCaretPosition;

    bool found = false;
    bool isLTR = style()->direction() == DirectionValue::LtrDirectionValue;
    LayoutUnit caretThickness = e->caretThickness();
    LayoutUnit mostRight;
    LayoutUnit mostLeft;
    LayoutUnit x, y;
    iterateChildFrameBox([&](FrameBox* box) {
        if (box->isInlineTextBox()) {
            mostRight =
                std::max(mostRight, box->asInlineTextBox()->x() +
                                        box->asInlineTextBox()->width());
            mostLeft = std::max(mostLeft, box->asInlineTextBox()->x());
            if (!found && box->asInlineTextBox()->text().end() == cPos) {
                found = true;

                auto absPoint = box->absolutePoint(this);
                x = absPoint.x();
                y = absPoint.y();
                if (isLTR) {
                    x += box->width();
                }
            }
        }
    });

    if (!found && (e->state() & Node::NodeStateFocused)) {
        String* value = e->value();
        y = paddingTop() + borderTop();
        if (isLTR) {
            x = paddingLeft() + borderLeft();
            if (value->length()) {
                x += mostRight;
            }
        } else {
            x = paddingRight() + borderRight();
            if (value->length()) {
                x = mostLeft;
            }
        }
    }

    e->currentCaretLayoutLocation().setX(x);
    e->currentCaretLayoutLocation().setY(y);

    if (x + caretThickness > contentWidth()) {
        node()->asElement()->ensureRareElementMembers()->m_scrollLeft =
            x + caretThickness - contentWidth();
    }
}

void FrameInputBox::paint(PaintingContext& ctx)
{
    FrameBlockBox::paint(ctx);

    if (ctx.m_paintingStage == PaintingStage::PaintingNormalFlowInline) {
        HTMLInputElement* e = node()->asHTMLInputElement();
        size_t cPos = e->m_currentCaretPosition;

        if (!e->m_shouldDrawCaret) {
            return;
        }

        LayoutUnit caretThickness = e->caretThickness();
        LayoutUnit x, y;
        x = e->currentCaretLayoutLocation().x();
        y = e->currentCaretLayoutLocation().y();
        ctx.m_canvas->save();
        ctx.m_canvas->clip(makeRect(BoxValue::ContentBoxBoxValue));
        ctx.m_canvas->setColor(node()->style()->color());
        ctx.m_canvas->drawRect(
            LayoutRect(x - scrollLeft(), y, caretThickness,
                       style()->font()->metrics().m_fontHeight));
        ctx.m_canvas->restore();
    }
}
}
