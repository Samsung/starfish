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
    pseudoStyle->setWhiteSpace(WhiteSpaceValue::PreWhiteSpaceValue);
    textElement->setStyle(pseudoStyle);

    // Edit text value to show
    String* textValue = from->visibleValue();
    if (textValue->length() == 0) {
        if (from->isHTMLTextAreaElement() ||
            (from->isEditableType() && from->placeholder()->length() != 0)) {
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

void* FrameInputBox::operator new(size_t size)
{
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameInputBox)] = { 0 };
        FrameInputBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameInputBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

FrameInputBox::FrameInputBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

FrameInputBox* FrameInputBox::buildFrameTree(Node* current,
                                             FrameTreeBuilderContext& ctx,
                                             bool force)
{
    if (current->isHTMLInputElement() &&
        current->asHTMLInputElement()->type()->equals("hidden")) {
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

    STARFISH_ASSERT(current->isHTMLTextEditable());

    HTMLTextEditable* textEditable = current->asHTMLTextEditable();
    if (textEditable->shouldCreateFrameText()) {
        currentFrame->appendChild(createFrameText(textEditable));
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

void FrameInputBox::layout(LayoutContext& ctx,
                           Frame::LayoutWantToResolve resolveWhat)
{
    HTMLTextEditable* textEditable = node()->asHTMLTextEditable();
    if (textEditable->isHTMLInputElement() &&
        (textEditable->asHTMLInputElement()->type()->equals("checkbox"))) {
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
        style()->loadFont(textEditable);
        // TODO: propagate fontsize
    }

    FrameBlockBox::layout(ctx, resolveWhat);

    // Align text to center vertically if necessary
    if (textEditable->shouldCreateFrameText() &&
        textEditable->ignoreLineBreaks()) {
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
    size_t cPos = textEditable->currentCaretPosition();

    if (!cPos) {
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
            if (!found && stringView.start() < cPos &&
                stringView.end() >= cPos) {
                found = true;
                x = absPoint.x();
                y = absPoint.y();
                StringView newStringView(stringView);
                newStringView.setEnd(cPos);
                LayoutUnit offset = style()->font()->measureText(newStringView);
                if (isLTR) {
                    x += offset;
                } else {
                    STARFISH_ASSERT(box->width() - offset >= 0);
                    x += (box->width() - offset);
                }
            }
        });
        if (!found && !textEditable->ignoreLineBreaks()) {
            // Last Linebox were removed while processing layout
            STARFISH_ASSERT(lastEndIndex < cPos);
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

void FrameInputBox::paintCaret(Canvas* canvas)
{
    STARFISH_ASSERT(node()->isHTMLTextEditable());
    HTMLTextEditable* textEditable = node()->asHTMLTextEditable();
    if (!textEditable->shouldDrawCaret()) {
        return;
    }

    LayoutUnit caretThickness = textEditable->caretThickness();
    LayoutUnit x, y;
    x = textEditable->currentCaretLayoutLocation().x();
    y = textEditable->currentCaretLayoutLocation().y();
    canvas->save();
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
