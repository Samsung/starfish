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

#include "StarFishConfig.h"

#include "FrameInputBox.h"

#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLTextEditable.h"
#include "core/dom/Text.h"
#include "core/dom/PseudoElement.h"
#include "core/page/Window.h"

#include "core/layout/Frame.h"
#include "core/layout/FrameText.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/modules/canvas/Canvas.h"

namespace StarFish {

// TODO <textarea> & <input> could share same layout code
// Below methods will be removed eventually
//
// * buildFrameTreeForTextEditable -> buildFrameTree
// * layoutForTextEditable -> layout
// * paintCarretForTextEditable -> paintCaret

static ComputedStyle* createStyle(Node* from)
{
    ComputedStyle* childStyle = new ComputedStyle(from->style());
    childStyle->loadResources(from);
    childStyle->arrangeStyleValues(from->style(), from);
    childStyle->setDisplay(DisplayValue::InlineDisplayValue);
    return childStyle;
}

static FrameText* createFrameText(HTMLTextEditable* from)
{
    // Generate pseudo element
    PseudoElement* textElement = new PseudoElement(
        from->document(),
        StyleResolver::PseudoElementType::PseudoElementFormOnly);
    textElement->setParentNode(from);

    // Set style
    ComputedStyle* pseudoStyle = createStyle(from);
    if (from->ignoreLineBreaks()) {
        pseudoStyle->setWhiteSpace(WhiteSpaceValue::PreWhiteSpaceValue);
    }
    textElement->setStyle(pseudoStyle);

    // Edit text value to show
    String* textValue = from->textValue();
    if (!textValue->length()) {
        if (from->isEditableType()) {
            textValue = from->placeholder();
        } else {
            // To prevent height shrink
            textValue = String::createUTF32String(0x202F);
        }
    }

    // Generate text node
    Text* textNode = new Text(from->document(), textValue);
    textNode->setParentNode(textElement);
    ComputedStyle* textStyle = createStyle(textElement);
    // TODO set styles for input elements
    textNode->setStyle(textStyle);

    FrameText* frameText = new FrameText(textNode, textStyle);
    textNode->setFrame(frameText);
    return frameText;
}

FrameInputBox::FrameInputBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

static FrameInputBox* buildFrameTreeForTextEditable(
    Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    STARFISH_ASSERT(current->isHTMLTextEditable());
    FrameInputBox* currentFrame = nullptr;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    if (current->needsFrameTreeBuild() || force) {
        currentFrame = new FrameInputBox(current, nullptr);
        force = true;
    } else {
        currentFrame = current->frame()->asFrameInputBox();
    }
    STARFISH_ASSERT(currentFrame);

    ctx.setCurrentBlockContainer(currentFrame);

    HTMLTextEditable* textEditable = current->asHTMLTextEditable();
    if (textEditable->hasTextValue()) {
        currentFrame->appendChild(createFrameText(textEditable));
    }
    ctx.setCurrentBlockContainer(parent);
    return currentFrame;
}

FrameInputBox* FrameInputBox::buildFrameTree(Node* current,
                                             FrameTreeBuilderContext& ctx,
                                             bool force)
{
    if (current->isHTMLTextEditable() && current->isHTMLTextAreaElement()) {
        return buildFrameTreeForTextEditable(current, ctx, force);
    }

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

        String* visibleValue = inputNode->visibleValue();
        if (visibleValue->length() == 0) {
            // replace emptyString to spaceString for preventing shrink linebox
            // height
            visibleValue = String::createUTF32String(0x202F);
        }

        Text* textNode = new Text(current->document(), visibleValue);
        textNode->setParentNode(textElement);
        ComputedStyle* textStyle = createInputElementStyleFrom(textElement);
        if (inputNode->type()->equalsIgnoreCase("checkbox")) {
            textStyle->setTextAlign(TextAlignValue::CenterTextAlignValue);
        }
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
    childStyle->loadResources(parent);
    childStyle->arrangeStyleValues(parent->style(), parent);
    childStyle->setDisplay(DisplayValue::InlineDisplayValue);
    return childStyle;
}

void FrameInputBox::layoutForTextEditable(
    LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    FrameBlockBox::layout(ctx, resolveWhat);
    HTMLTextEditable* textEditable = node()->asHTMLTextEditable();

    // Align text to center vertically if necessary
    if (textEditable->ignoreLineBreaks()) {
        if (lineBoxes().size() > 0) {
            LineBox* lb = *lineBoxes().begin();
            LayoutUnit availableHeight = contentHeight() - lb->height();
            lb->setY(availableHeight / 2 + paddingTop() + borderTop());
        }
    }

    // Update caret location
    LayoutUnit x, y;
    LayoutUnit fontHeight =
        textEditable->style()->font()->metrics().m_fontHeight;
    bool isLTR = style()->direction() == DirectionValue::LtrDirectionValue;
    size_t caret = textEditable->currentCaretPosition();

    if (!caret) {
        y = paddingTop() + borderTop();
        if (isLTR) {
            x = paddingLeft() + borderLeft();
        } else {
            x = paddingRight() + borderRight();
        }
    } else {
        bool found = false;
        size_t lastEndIndex = 0;
        LayoutUnit lastY = 0;
        // Find textBox put caret inside
        iterateChildFrameBox([&](FrameBox* box) {
            if (!box->isInlineTextBox()) {
                return;
            }
            StringView stringView = box->asInlineTextBox()->text();
            auto absPoint = box->absolutePoint(this);
            lastEndIndex = stringView.end();
            lastY = absPoint.y();
            if (!found && stringView.start() < caret &&
                stringView.end() >= caret) {
                found = true;
                x = absPoint.x();
                y = absPoint.y();
                StringView newStringView(stringView);
                newStringView.setEnd(caret);
                LayoutUnit offset = style()->font()->measureText(newStringView);
                if (isLTR) {
                    x += offset;
                } else {
                    STARFISH_ASSERT(box->width() - offset >= 0);
                    x += (box->width() - offset);
                }
            }
        });
        if (!found) {
            // Last Linebox were removed while processing layout
            STARFISH_ASSERT(lastEndIndex < caret);
            STARFISH_ASSERT(!textEditable->ignoreLineBreaks());
            x = paddingLeft() + borderLeft();
            y = lastY;
        }
    }
    textEditable->currentCaretLayoutLocation().setX(x);
    textEditable->currentCaretLayoutLocation().setY(y);

    LayoutUnit caretThickness = textEditable->caretThickness();
    if (x + caretThickness > contentWidth()) {
        textEditable->ensureRareElementMembers()->m_scrollLeft =
            x + caretThickness - contentWidth();
    }

    // Move scroll top if necessary
    if (!textEditable->ignoreLineBreaks()) {
        LayoutUnit comparingHeight =
            contentHeight() + paddingTop() + borderTop();
        if (y + fontHeight > comparingHeight) {
            textEditable->ensureRareElementMembers()->m_scrollTop =
                y + fontHeight - comparingHeight;
        }
    }
}

void FrameInputBox::layout(LayoutContext& ctx,
                           Frame::LayoutWantToResolve resolveWhat)
{
    if (node()->isHTMLTextEditable() && node()->isHTMLTextAreaElement()) {
        layoutForTextEditable(ctx, resolveWhat);
        return;
    }

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
        style()->loadFont(node());
        // TODO: propagate fontsize
    }

    FrameBlockBox::layout(ctx, resolveWhat);

    if (node()->asHTMLInputElement()->canHaveValue()) {
        if (lineBoxes().size() > 0) {
            LineBox* lb = *lineBoxes().begin();
            LayoutUnit availableHeight = contentHeight() - lb->height();
            lb->setY(availableHeight / 2 + paddingTop() + borderTop());
        }
    }

    HTMLInputElement* e = node()->asHTMLInputElement();
    size_t cPos = e->m_currentCaretPosition;

    bool found = false;
    bool isLTR = style()->direction() == DirectionValue::LtrDirectionValue;
    LayoutUnit caretThickness = e->caretThickness();
    LayoutUnit mostRight;
    LayoutUnit mostLeft;
    LayoutUnit mostTop = borderTop() + paddingTop();
    LayoutUnit x, y;
    iterateChildFrameBox([&](FrameBox* box) {
        if (box->isInlineTextBox()) {
            auto absPoint = box->absolutePoint(this);
            mostRight =
                std::max(mostRight, box->asInlineTextBox()->x() +
                                        box->asInlineTextBox()->width());
            mostLeft = std::max(mostLeft, box->asInlineTextBox()->x());
            mostTop = std::max(absPoint.y(), mostTop);
            if (!found && box->asInlineTextBox()->text().end() == cPos) {
                found = true;

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
        y = mostTop;
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

static void paintCaretForTextEditable(FrameInputBox* from, Canvas* canvas)
{
    STARFISH_ASSERT(from->node()->isHTMLTextEditable());
    HTMLTextEditable* textEditable = from->node()->asHTMLTextEditable();
    if (!textEditable->shouldDrawCaret()) {
        return;
    }

    LayoutUnit caretThickness = textEditable->caretThickness();
    LayoutUnit x, y;
    x = textEditable->currentCaretLayoutLocation().x();
    y = textEditable->currentCaretLayoutLocation().y();
    canvas->save();
    canvas->setColor(textEditable->style()->color());
    canvas->drawRect(LayoutRect(x, y, caretThickness,
                                from->style()->font()->metrics().m_fontHeight));
    canvas->restore();
}

void FrameInputBox::paintCaret(Canvas* canvas)
{
    if (node()->isHTMLTextEditable()) {
        paintCaretForTextEditable(this, canvas);
        return;
    }
    HTMLInputElement* e = node()->asHTMLInputElement();
    size_t cPos = e->m_currentCaretPosition;
    if (!e->m_shouldDrawCaret) {
        return;
    }

    LayoutUnit caretThickness = e->caretThickness();
    LayoutUnit x, y;
    x = e->currentCaretLayoutLocation().x();
    y = e->currentCaretLayoutLocation().y();
    canvas->save();
    canvas->clip(makeRect(BoxValue::ContentBoxBoxValue));
    canvas->setColor(node()->style()->caretColor());
    canvas->drawRect(LayoutRect(x - scrollLeft(), y, caretThickness,
                                style()->font()->metrics().m_fontHeight));
    canvas->restore();
}

void FrameInputBox::paintInlineContentBlock(Canvas* canvas)
{
    FrameBlockBox::paintInlineContentBlock(canvas);
    paintCaret(canvas);
}

void FrameInputBox::paintContent(PaintingContext& ctx)
{
    if (canSkipPaintingStage(ctx)) {
        return;
    }

    FrameBlockBox::paintContent(ctx);

    if (ctx.m_paintingStage == PaintingStage::PaintingNormalFlowInline) {
        paintCaret(ctx.m_canvas);
    }
}
}
