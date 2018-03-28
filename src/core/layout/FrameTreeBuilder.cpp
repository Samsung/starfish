/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/HTMLInputElement.h"
#include "core/dom/HTMLListContainer.h"
#include "core/dom/PseudoElement.h"
#include "core/dom/Text.h"
#include "core/dom/svg/SVGSVGElement.h"
#include "core/page/Window.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameText.h"
#include "core/layout/FrameInline.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameFlexibleBox.h"
#include "core/layout/FrameGridBox.h"
#include "core/layout/FrameReplaced.h"
#include "core/layout/FrameReplacedImage.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableRowBox.h"
#include "core/layout/FrameTableSectionBox.h"
#include "core/layout/FrameTableCellBox.h"
#include "core/layout/FrameTableColBox.h"
#include "core/layout/FrameTableCaptionBox.h"
#include "core/layout/FrameTableObjectBox.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/layout/FrameInputBox.h"
#include "core/layout/FrameButtonBox.h"
#include "core/layout/FrameSelectBox.h"
#include "core/layout/FrameOptGroupBox.h"
#include "core/layout/FrameOptionBox.h"
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "core/layout/FrameReplacedVideo.h"
#endif
#include "core/layout/FrameReplacedIFrame.h"
#include "core/layout/FrameReplacedObject.h"
#include "core/layout/FrameLineBreak.h"
#include "core/layout/svg/FrameSVGSVGBox.h"

namespace StarFish {
void dump(Frame* frm, unsigned depth);

FrameTreeBuilderContext::FrameTreeBuilderContext(
    FrameBlockBox* currentBlockContainer)
{
    m_isInFrameInlineFlow = false;
    m_isInFrameFlexFlow = false;
    m_isInFrameGridFlow = false;
    m_lastAnonymousTableObjectParent = nullptr;
    setCurrentBlockContainer(currentBlockContainer);
}

void FrameTreeBuilderContext::setCurrentBlockContainer(
    FrameBlockBox* blockContainer)
{
    m_currentBlockContainer = blockContainer;
    m_isInFrameFlexFlow = blockContainer->isFrameFlexibleBox();
    m_isInFrameGridFlow = blockContainer->isFrameGridBox();
}

FrameBlockBox* FrameTreeBuilderContext::currentBlockContainer()
{
    return m_currentBlockContainer;
}

void FrameTreeBuilderContext::setLastAnonymousTableObjectParent(
    FrameTableObjectBox* parent)
{
    m_lastAnonymousTableObjectParent = parent;
}

FrameTableObjectBox* FrameTreeBuilderContext::lastAnonymousTableObjectParent()
{
    return m_lastAnonymousTableObjectParent;
}

std::unordered_map<Node*, FrameInline*>&
FrameTreeBuilderContext::frameInlineItem()
{
    return m_frameInlineItem;
}

bool FrameTreeBuilderContext::isInFrameInlineFlow() const
{
    return m_isInFrameInlineFlow;
}

void FrameTreeBuilderContext::setIsInFrameInlineFlow(bool b)
{
    m_isInFrameInlineFlow = b;
}

bool FrameTreeBuilderContext::isInFrameFlexFlow() const
{
    return m_isInFrameFlexFlow;
}

bool FrameTreeBuilderContext::isInFrameGridFlow() const
{
    return m_isInFrameGridFlow;
}

void FrameTreeBuilderContext::setIsInFrameFlexFlow(bool b)
{
    m_isInFrameFlexFlow = b;
}

void FrameTreeBuilderContext::setIsInFrameGridFlow(bool b)
{
    m_isInFrameGridFlow = b;
}

bool FrameTreeBuilderContext::isInFrameTableFlow() const
{
    return m_currentBlockContainer->isFrameTableObjectBox() ||
           m_lastAnonymousTableObjectParent != nullptr;
}

void FrameTreeBuilder::clearTree(Node* current)
{
    current->markNeedsFrameTreeBuild();
    if (!current->frame())
        return;
    current->setFrame(nullptr);
    Node* n = current->firstChild();
    while (n) {
        clearTree(n);
        n = n->nextSibling();
    }
}

void FrameTreeBuilder::needsFrameTreeBuildFromChildrenOfThisFrame(Frame* f)
{
    Frame* parent = f;
    while (parent->firstChild()) {
        parent->removeChild(parent->firstChild());
    }

    Node* node = f->node()->firstChild();
    while (node) {
        FrameTreeBuilder::clearTree(node);
        node = node->nextSibling();
    }

    node = parent->node();
    while (node) {
        node->markChildNeedsFrameTreeBuild();
        node = node->parentNode();
    }
}

Frame* FrameTreeBuilder::findNearestBlock(Frame* f)
{
    if (!f) {
        return nullptr;
    } else {
        while (f) {
            if (!f->isAnonymous() &&
                (f->isBlockLevel() || f->isFrameTableCellBox())) {
                break;
            }
            f = f->parent();
        }
        return f;
    }
}

template <typename T>
static T* createAnonymousBlockBox(FrameBlockBox* blockContainer, Node* node,
                                  DisplayValue display)
{
    ComputedStyle* style = new ComputedStyle(blockContainer->style());
    style->setDisplay(display);
    style->loadResources(node);
    style->arrangeStyleValues(blockContainer->style(), node);

    return new T(nullptr, style);
}

template <typename T>
static T* wrapWithAnonymousBlockBox(FrameBlockBox* blockContainer, Node* node,
                                    DisplayValue display, Frame* frame)
{
    T* blockBox = createAnonymousBlockBox<T>(blockContainer, node, display);
    blockBox->appendChild(frame);
    blockContainer->appendChild(blockBox);
    return blockBox;
}

void FrameTreeBuilder::insertFlexItemChild(FrameBlockBox* blockContainer,
                                           Frame* currentFrame,
                                           Node* currentNode,
                                           FrameTreeBuilderContext& ctx)
{
    bool isFlexItem = currentFrame->isBlockLevel() &&
                      !currentFrame->isFrameLineBreak() &&
                      !currentFrame->isAbsolutePositioned();
    if (isFlexItem) {
        blockContainer->appendChild(currentFrame);
        currentFrame->markFlexItem();
    } else {
        Frame* last = blockContainer->lastChild();

        if (last && last->isAnonymous()) {
            last->appendChild(currentFrame);
            last->markFlexItem();
        } else {
            Frame* f = wrapWithAnonymousBlockBox<FrameBlockBox>(
                blockContainer, currentNode, DisplayValue::BlockDisplayValue,
                currentFrame);
            f->markFlexItem();
        }
    }
}

void FrameTreeBuilder::insertGridItemChild(FrameBlockBox* blockContainer,
                                           Frame* currentFrame,
                                           Node* currentNode,
                                           FrameTreeBuilderContext& ctx)
{
    // FIXME
    bool isGridItem =
        currentFrame->isBlockLevel() && !currentFrame->isFrameLineBreak();
    if (isGridItem) {
        blockContainer->appendChild(currentFrame);
        currentFrame->markGridItem();
    }
}

static bool isProperTableChild(Frame* child, Frame* parent)
{
    if (parent->isFrameTableBox()) {
        return (child->isFrameTableColBox() &&
                child->style()->display() ==
                    DisplayValue::TableColumnGroupDisplayValue) ||
               child->isFrameTableCaptionBox() ||
               child->isFrameTableSectionBox();
    } else if (parent->isFrameTableColBox() &&
               parent->style()->display() ==
                   DisplayValue::TableColumnGroupDisplayValue) {
        return child->isFrameTableColBox() &&
               child->style()->display() ==
                   DisplayValue::TableColumnDisplayValue;
    } else if (parent->isFrameTableSectionBox()) {
        return child->isFrameTableRowBox();
    } else if (parent->isFrameTableRowBox()) {
        return child->isFrameTableCellBox();
    } else if (parent->isFrameTableCaptionBox() ||
               parent->isFrameTableCellBox()) {
        return !child->isFrameTableObjectBox() || child->isFrameTableBox();
    } else {
        return false;
    }
}

static FrameTableObjectBox* findProperAnonymousTableObjectParent(
    Frame* child, Frame* lastAnonymousTableObjectParent)
{
    Frame* parent = lastAnonymousTableObjectParent;
    while (!parent->isFrameTableBox() && parent->isAnonymous()) {
        if (isProperTableChild(child, parent)) {
            return parent->asFrameTableObjectBox();
        }
        parent = parent->parent();
    }

    if (parent->isFrameTableBox() && parent->isAnonymous()) {
        if (isProperTableChild(child, parent)) {
            return parent->asFrameTableObjectBox();
        }
    }
    return nullptr;
}

FrameTableObjectBox* FrameTreeBuilder::createAnonymousTableObjectParent(
    FrameBlockBox* blockContainer,
    FrameTableObjectBox* lastAnonymousTableObjectParent, Frame* currentFrame,
    Node* currentNode, FrameTreeBuilderContext& ctx)
{
    Frame* parent = nullptr;
    if ((currentFrame->isFrameTableColBox() &&
         currentFrame->style()->display() ==
             DisplayValue::TableColumnGroupDisplayValue) ||
        currentFrame->isFrameTableCaptionBox() ||
        currentFrame->isFrameTableSectionBox()) {
        if (ctx.isInFrameInlineFlow()) {
            parent = createAnonymousBlockBox<FrameTableBox>(
                blockContainer, currentNode,
                DisplayValue::InlineTableDisplayValue);
        } else {
            parent = createAnonymousBlockBox<FrameTableBox>(
                blockContainer, currentNode, DisplayValue::TableDisplayValue);
        }
    } else if ((currentFrame->isFrameTableColBox() &&
                currentFrame->style()->display() ==
                    DisplayValue::TableColumnDisplayValue)) {
        parent = createAnonymousBlockBox<FrameTableColBox>(
            blockContainer, currentNode,
            DisplayValue::TableColumnGroupDisplayValue);
    } else if (currentFrame->isFrameTableRowBox()) {
        parent = createAnonymousBlockBox<FrameTableSectionBox>(
            blockContainer, currentNode,
            DisplayValue::TableRowGroupDisplayValue);
    } else if (currentFrame->isFrameTableCellBox()) {
        parent = createAnonymousBlockBox<FrameTableRowBox>(
            blockContainer, currentNode, DisplayValue::TableRowDisplayValue);
    } else {
        parent = createAnonymousBlockBox<FrameTableCellBox>(
            blockContainer, currentNode, DisplayValue::TableCellDisplayValue);
    }

    if (parent->isFrameTableBox()) {
        insertChild(blockContainer, parent, currentNode, ctx);
    } else {
        insertTableObjectChild(blockContainer, lastAnonymousTableObjectParent,
                               parent, currentNode, ctx);
    }

    return parent->asFrameTableObjectBox();
}

void FrameTreeBuilder::insertTableObjectChild(
    FrameBlockBox* blockContainer,
    FrameTableObjectBox* lastAnonymousTableObjectParent, Frame* currentFrame,
    Node* currentNode, FrameTreeBuilderContext& ctx)
{
    Frame* parent = nullptr;
    if (isProperTableChild(currentFrame, blockContainer)) {
        parent = blockContainer;
    } else if (lastAnonymousTableObjectParent) {
        parent = findProperAnonymousTableObjectParent(
            currentFrame, lastAnonymousTableObjectParent);
    }

    if (!parent) {
        parent = createAnonymousTableObjectParent(
            blockContainer, lastAnonymousTableObjectParent, currentFrame,
            currentNode, ctx);
    }

    parent->appendChild(currentFrame);
}

static bool isIgnorableWhiteSpace(Frame* parent, Frame* child)
{
    bool containOnlyWhiteSpace =
        child->isFrameText() &&
        child->asFrameText()->text()->containsOnlyWhitespace();
    if (parent) {
        if (parent->isFrameTableObjectBox() && !parent->isFrameTableCellBox() &&
            !parent->isFrameTableCaptionBox()) {
            return containOnlyWhiteSpace;
        } else if (!parent->firstChild()) {
            return containOnlyWhiteSpace && child->shouldIgnoreNewlineChar();
        } else {
            return false;
        }
    }
    return containOnlyWhiteSpace && child->shouldIgnoreNewlineChar();
}

void FrameTreeBuilder::insertChild(FrameBlockBox* blockContainer,
                                   Frame* currentFrame, Node* currentNode,
                                   FrameTreeBuilderContext& ctx)
{
    if (ctx.isInFrameTableFlow()) {
        FrameTableObjectBox* tableParent = nullptr;
        if (blockContainer->isFrameTableObjectBox()) {
            tableParent = blockContainer->asFrameTableObjectBox();
        } else {
            tableParent = ctx.lastAnonymousTableObjectParent();
        }
        if (isIgnorableWhiteSpace(tableParent, currentFrame)) {
            return;
        }
    }

    bool isNormalFlowBlockChild = currentFrame->isBlockLevel() &&
                                  !currentFrame->isFrameLineBreak() &&
                                  currentFrame->isNormalFlow();

    if (ctx.isInFrameInlineFlow() && !isNormalFlowBlockChild) {
        auto iter = ctx.frameInlineItem().find(currentNode->parentNode());
        iter->second->appendChild(currentFrame);
        return;
    } else if (ctx.isInFrameFlexFlow()) {
        insertFlexItemChild(blockContainer, currentFrame, currentNode, ctx);
        return;
    } else if (ctx.isInFrameGridFlow()) {
        insertGridItemChild(blockContainer, currentFrame, currentNode, ctx);
        return;
    } else if (currentFrame->isFrameTableObjectBox() &&
               !currentFrame->isFrameTableBox()) {
        insertTableObjectChild(blockContainer,
                               ctx.lastAnonymousTableObjectParent(),
                               currentFrame, currentNode, ctx);
        return;
    } else {
        if (blockContainer->isFrameTableObjectBox()) {
            if (!blockContainer->isFrameTableCellBox() &&
                !blockContainer->isFrameTableCaptionBox()) {
                if (blockContainer->isFrameTableColBox()) {
                    if (blockContainer->style()->display() ==
                        DisplayValue::TableColumnGroupDisplayValue) {
                        if (!(currentFrame->isFrameTableColBox() &&
                              currentFrame->style()->display() ==
                                  DisplayValue::TableColumnDisplayValue)) {
                            return;
                        }
                    } else {
                        return;
                    }
                }

                insertTableObjectChild(blockContainer,
                                       ctx.lastAnonymousTableObjectParent(),
                                       currentFrame, currentNode, ctx);
                return;
            }
        }
    }

    if (isIgnorableWhiteSpace(blockContainer, currentFrame)) {
        return;
    }

    if (blockContainer->hasBlockFlow()) {
        if (isNormalFlowBlockChild) {
            // Block... + Block case
            blockContainer->appendChild(currentFrame);
        } else {
            // Block... + Inline case
            Frame* last = blockContainer->lastChild();

            STARFISH_ASSERT(last);

            if (!last->isAnonymous() || last->isFrameTableBox()) {
                if (isIgnorableWhiteSpace(nullptr, currentFrame)) {
                    return;
                }

                wrapWithAnonymousBlockBox<FrameBlockBox>(
                    blockContainer, currentNode,
                    DisplayValue::BlockDisplayValue, currentFrame);
            } else {
                last->appendChild(currentFrame);
            }
        }
    } else {
        if (blockContainer->firstChild() && isNormalFlowBlockChild) {
            // Inline... + Block case
            FrameBox* blockBox = createAnonymousBlockBox<FrameBlockBox>(
                blockContainer, currentNode, DisplayValue::BlockDisplayValue);
            Frame* child = blockContainer->firstChild();
            while (child) {
                blockContainer->removeChild(child);
                blockBox->appendChild(child);
                child = blockContainer->firstChild();
            }

            blockContainer->appendChild(blockBox);
            blockContainer->appendChild(currentFrame);
        } else {
            // Inline... + Inline case
            blockContainer->appendChild(currentFrame);
        }
    }
}

ComputedStyle* FrameTreeBuilder::pseudoStyleForElementInternal(
    Node* parent, StyleResolver::PseudoElementType pseudoId,
    ComputedStyle* parentStyle)
{
    STARFISH_ASSERT(pseudoId !=
                    StyleResolver::PseudoElementType::PseudoElementNone);
    STARFISH_ASSERT(parentStyle);

    ComputedStyle* style = new ComputedStyle(parentStyle);
    StyleResolveContext ctx(parent->document());
    parent->document()->styleResolver().matchAllRules(
        ctx, parent->asElement(), style, parentStyle, pseudoId);
    Length fontSize = style->fontSize();
    fontSize.changeToFixedIfNeeded(
        parentStyle->fontSize(),
        parent->document()->rootElement()->style()->fontSize(),
        parentStyle->font(), parent->window()->innerWidth(),
        parent->window()->innerHeight(), style);
    style->setFontSize(fontSize);

    // TODO: Set the proper style according to the type of pseudo-elements
    if (pseudoId ==
        StyleResolver::PseudoElementType::PseudoElementFirstLetter) {
        style->setDisplay(DisplayValue::InlineDisplayValue);
        style->setPosition(PositionValue::StaticPositionValue);
    }
    style->loadResources(parent);
    style->arrangeStyleValues(parentStyle, parent);

    return style;
}

Frame* findPseudoFrameForTable(Frame* frame, bool isBefore = true)
{
    Frame* pseudoFrame = frame;
    while (pseudoFrame) {
        if (pseudoFrame->node() && pseudoFrame->node()->isPseudoElement()) {
            break;
        }
        pseudoFrame =
            isBefore ? pseudoFrame->firstChild() : pseudoFrame->lastChild();
    }
    return pseudoFrame;
}

static ComputedStyle* createStyleForCounter(Node* from)
{
    ComputedStyle* counterStyle = new ComputedStyle(from->style());
    counterStyle->loadResources(from, from->style());
    counterStyle->arrangeStyleValues(from->style(), from);
    counterStyle->setDisplay(DisplayValue::InlineDisplayValue);
    return counterStyle;
}

static FrameBlockBox* findOutsideCounterAttachableFrameBlockBox(Frame* root)
{
    if (!root->isAbsolutePositioned() && root->isFrameBlockBox() &&
        !root->asFrameBlockBox()->hasBlockFlow() && root->hasChildren()) {
        return root->asFrameBlockBox();
    }
    Frame* f = root->firstChild();
    while (f) {
        FrameBlockBox* subresult = findOutsideCounterAttachableFrameBlockBox(f);
        if (subresult) {
            return subresult;
        }
        f = f->next();
    }
    return nullptr;
}

void FrameTreeBuilder::createOutsideCounterIfNeeds(Node* parent,
                                                   FrameTreeBuilderContext& ctx)
{
    DisplayValue display = parent->style()->display();
    if (display != DisplayValue::ListItemDisplayValue) {
        return;
    }
    if (!parent->style()->hasVisibleListCounter()) {
        return;
    }
    if (parent->style()->listStylePosition() !=
        ListStylePositionValue::ListStylePositionOutside) {
        return;
    }

    // Create counter element and frame
    Frame* parentFrame = parent->frame();
    if (!parentFrame) {
        return;
    }
    int32_t index = ctx.getAndIncreaseListCounterIndex();
    STARFISH_ASSERT(parent->style()->listStyleData().typeData()->valid());
    String* label =
        parent->style()->listStyleData().typeData()->generateLabel(index);
    LayoutUnit indent = parent->style()->font()->measureText(label);

    // Generate style for PseudoElement
    ComputedStyle* pseudoStyle = createStyleForCounter(parent);
    pseudoStyle->setWidth(Length(Length::Fixed, 0));
    pseudoStyle->setTextIndent(Length(Length::Fixed, -indent.toFloat()));
    pseudoStyle->setDisplay(DisplayValue::InlineBlockDisplayValue);

    // Generate pseudo element
    PseudoElement* pseudoElement = new PseudoElement(
        parent->document(),
        StyleResolver::PseudoElementType::PseudoElementCounter);
    pseudoElement->setStyle(pseudoStyle);
    pseudoElement->setParentNode(parent);
    pseudoElement->setFrame(new FrameBlockBox(pseudoElement, nullptr));

    // Generate style for Text
    ComputedStyle* textStyle = createStyleForCounter(pseudoElement);
    textStyle->setWhiteSpace(WhiteSpaceValue::PreWhiteSpaceValue);

    // Generate text node
    Text* textNode = new Text(parent->document(), label);
    textNode->setParentNode(pseudoElement);
    textNode->setStyle(textStyle);
    textNode->setFrame(new FrameText(textNode, nullptr));
    pseudoElement->frame()->appendChild(textNode->frame());

    // Append frame
    FrameBlockBox* t = findOutsideCounterAttachableFrameBlockBox(parentFrame);
    if (t) {
        t->prependChild(pseudoElement->frame());
    } else {
        STARFISH_ASSERT(parentFrame->isFrameBlockBox());
        FrameBlockBox* wrapper = createAnonymousBlockBox<FrameBlockBox>(
            parentFrame->asFrameBlockBox(), parent,
            DisplayValue::BlockDisplayValue);
        wrapper->appendChild(pseudoElement->frame());
        parentFrame->prependChild(wrapper);
    }
}

void FrameTreeBuilder::createInsideCounterIfNeeds(Node* parent,
                                                  FrameTreeBuilderContext& ctx)
{
    DisplayValue display = parent->style()->display();
    if (display != DisplayValue::ListItemDisplayValue) {
        return;
    }
    if (!parent->style()->hasVisibleListCounter()) {
        return;
    }
    if (parent->style()->listStylePosition() !=
        ListStylePositionValue::ListStylePositionInside) {
        return;
    }
    Frame* parentFrame = parent->frame();
    if (!parentFrame) {
        return;
    }

    int32_t index = ctx.getAndIncreaseListCounterIndex();
    STARFISH_ASSERT(parent->style()->listStyleData().typeData()->valid());
    String* label =
        parent->style()->listStyleData().typeData()->generateLabel(index);

    // Generate text style
    ComputedStyle* textStyle = createStyleForCounter(parent);
    textStyle->setWhiteSpace(WhiteSpaceValue::PreWhiteSpaceValue);

    // Generate text node
    // TODO Remove newline in label
    Text* textNode = new Text(parent->document(), label);
    textNode->setParentNode(parent);
    textNode->setStyle(textStyle);

    // Generate frame and append
    textNode->setFrame(new FrameText(textNode, textNode->style()));
    parentFrame->appendChild(textNode->frame());
}

void FrameTreeBuilder::createPseudoElement(
    Node* parent, StyleResolver::PseudoElementType pseudoId,
    FrameTreeBuilderContext& ctx)
{
    if (!parent->isElement() || parent->isPseudoElement()) {
        return;
    }
    if (!parent->style()->seenPseudoElement(pseudoId)) {
        return;
    }

    PseudoElement* pseudoElement =
        new PseudoElement(parent->document(), pseudoId);
    pseudoElement->setParentNode(parent);

    Frame* pseudoParentFrame = nullptr;
    ComputedStyle* parentStyle = parent->style();
    if (pseudoId ==
        StyleResolver::PseudoElementType::PseudoElementFirstLetter) {
        if (Frame* nextFrame =
                FirstLetterPseudoElement::firstLetterFrameText(pseudoElement)) {
            pseudoParentFrame = nextFrame->parent();
            parentStyle = nextFrame->style();
        }
    } else if (pseudoElement->parentNode()) {
        pseudoParentFrame = pseudoElement->parentNode()->frame();
    }

    if (!pseudoParentFrame) {
        return;
    }

    ComputedStyle* pseudoStyle = parent->style()->pseudoStyle(
        parent->asElement(), pseudoId, parentStyle);
    if (!pseudoElementFrameIsNeeded(pseudoStyle)) {
        return;
    }
    if ((pseudoId == StyleResolver::PseudoElementBefore ||
         pseudoId == StyleResolver::PseudoElementAfter) &&
        !pseudoStyle->content()) {
        return;
    }
    pseudoElement->setStyle(pseudoStyle);

    if (pseudoElement->isFirstLetterPseudoElement()) {
        Frame* pseudoFrame;
        if (pseudoStyle->floating() != FloatValue::NoneFloatValue) {
            pseudoFrame = new FrameBlockBox(pseudoElement, nullptr);
        } else {
            pseudoFrame = new FrameInline(pseudoElement);
        }
        Frame* originalFrameText =
            FirstLetterPseudoElement::firstLetterFrameText(pseudoElement);
        pseudoParentFrame->insertBefore(originalFrameText, pseudoFrame);

        STARFISH_ASSERT(originalFrameText->isFrameText());
        String* originalText = originalFrameText->asFrameText()->text();
        size_t length =
            FirstLetterPseudoElement::firstLetterLength(originalText);

        Text* letter = new Text(originalFrameText->node()->document(),
                                originalText->substring(0, length));
        ComputedStyle* letterStyle = new ComputedStyle(pseudoStyle);
        letterStyle->loadResources(pseudoElement);
        letterStyle->arrangeStyleValues(pseudoStyle, pseudoElement);
        letter->setStyle(letterStyle);
        letter->setParentNode(pseudoElement);
        letter->clearNeedsStyleRecalc();
        FrameText* letterFrameText = new FrameText(letter, letterStyle);
        letter->setFrame(letterFrameText);
        pseudoFrame->appendChild(letterFrameText);

        Text* remainingText = new Text(
            originalFrameText->node()->document(),
            originalText->substring(length, originalText->length() - length));
        remainingText->setStyle(originalFrameText->style());
        remainingText->setParentNode(parent);
        FrameText* remainingFrameText =
            new FrameText(remainingText, originalFrameText->style());
        remainingText->setFrame(remainingFrameText);

        pseudoParentFrame->insertBefore(originalFrameText, remainingFrameText);
        pseudoParentFrame->removeChild(originalFrameText);
        originalFrameText->setParent(pseudoParentFrame);
    } else if (pseudoElement->isBeforePseudoElement() ||
               pseudoElement->isAfterPseudoElement()) {
        if (!pseudoElement->style()->hasRareComputeStyleData()) {
            return;
        }

        buildTree(pseudoElement, ctx, true);
    }

    STARFISH_ASSERT(parent->isElement());
}

Frame* FrameTreeBuilder::createFrame(Node* current,
                                     FrameTreeBuilderContext& ctx, bool force)
{
    DisplayValue display = current->style()->display();
    bool isVisible = display != DisplayValue::NoneDisplayValue;
    if (!isVisible) {
        current->clearNeedsFrameTreeBuild();
        FrameTreeBuilder::clearTree(current);
        return nullptr;
    }

    if (ComputedStyle::isDisplayTableValueType(display) &&
        current->style()->originalVisibility() ==
            VisibilityValue::CollapseVisibilityValue) {
        return nullptr;
    }
    if (current->isHTMLImageElement()) {
        return new FrameReplacedImage(current);
    }
#ifdef STARFISH_ENABLE_MULTIMEDIA
    else if (current->isHTMLVideoElement()) {
        return new FrameReplacedVideo(current);
    }
#endif
    else if (current->isHTMLIFrameElement()) {
        return new FrameReplacedIFrame(current);
    } else if (current->isHTMLBRElement()) {
        return new FrameLineBreak(current);
    } else if (current->isHTMLObjectElement()) {
        return new FrameReplacedObject(current);
    } else if (current->isSVGSVGElement()) {
        return FrameTreeBuilder::buildSVGFrameTree(current->asSVGSVGElement());
    } else if (current->isHTMLButtonElement()) {
        return new FrameButtonBox(current, nullptr);
    } else if (current->isHTMLInputElement() ||
               current->isHTMLTextAreaElement()) {
        return FrameInputBox::buildFrameTree(current, ctx, force);
    } else if (current->isHTMLSelectElement()) {
        return new FrameSelectBox(current, nullptr);
    } else if (current->isHTMLOptGroupElement()) {
        return new FrameOptGroupBox(current, nullptr);
    } else if (current->isHTMLOptionElement()) {
        return new FrameOptionBox(current, nullptr);
    } else if (display == DisplayValue::FlexDisplayValue ||
               display == DisplayValue::InlineFlexDisplayValue) {
        return new FrameFlexibleBox(current, nullptr);
    } else if (display == DisplayValue::GridDisplayValue ||
               display == DisplayValue::InlineGridDisplayValue) {
        return new FrameGridBox(current, nullptr);
    } else if (display == DisplayValue::TableDisplayValue ||
               display == DisplayValue::InlineTableDisplayValue) {
        return new FrameTableBox(current, nullptr);
    } else if (display == DisplayValue::TableCaptionDisplayValue) {
        return new FrameTableCaptionBox(current, nullptr);
    } else if (display == DisplayValue::TableHeaderGroupDisplayValue ||
               display == DisplayValue::TableRowGroupDisplayValue ||
               display == DisplayValue::TableFooterGroupDisplayValue) {
        return new FrameTableSectionBox(current, nullptr);
    } else if (display == DisplayValue::TableRowDisplayValue) {
        return new FrameTableRowBox(current, nullptr);
    } else if (display == DisplayValue::TableColumnGroupDisplayValue ||
               display == DisplayValue::TableColumnDisplayValue) {
        return new FrameTableColBox(current, nullptr);
    } else if (display == DisplayValue::TableCellDisplayValue) {
        return new FrameTableCellBox(current, nullptr);
    } else {
        if (display == DisplayValue::BlockDisplayValue ||
            display == DisplayValue::InlineBlockDisplayValue ||
            display == DisplayValue::ListItemDisplayValue) {
            return new FrameBlockBox(current, nullptr);
        } else if (display == DisplayValue::InlineDisplayValue ||
                   display == DisplayValue::InlineListItemDisplayValue) {
            if (current->isCharacterData() &&
                current->asCharacterData()->isText()) {
                return new FrameText(current, current->style());
            } else if (current->isComment()) {
                FrameTreeBuilder::clearTree(current);
                return nullptr;
            } else {
                return new FrameInline(current);
            }
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }
}

void FrameTreeBuilderContext::resetPseudoCounter(Node* container,
                                                 AtomicString& counterName,
                                                 int32_t resetValue)
{
    if (!m_pseudoCounters.size() ||
        m_pseudoCounters.back().first != container) {
        m_pseudoCounters.emplace_back(container,
                                      std::unordered_set<AtomicString>());
    }
    std::unordered_set<AtomicString>& currentSet =
        m_pseudoCounters.back().second;
    if (currentSet.find(counterName) != currentSet.end()) {
        STARFISH_ASSERT(m_pseudoCounterIndice[counterName].size());
        m_pseudoCounterIndice[counterName].back() = resetValue;
    } else {
        currentSet.insert(counterName);
        m_pseudoCounterIndice[counterName].push_back(resetValue);
    }
}

void FrameTreeBuilderContext::openCountingContextIfNeeds(Node* from)
{
    // List counter (list-style-type, list-style-position, list-style-image)
    if (from->isHTMLListContainer()) {
        m_listCounterIndice.push_back(from->asHTMLListContainer()->start());
    }
    if (from->style()->display() == DisplayValue::NoneDisplayValue) {
        return;
    }
    // Pseudo counter (counter-reset, counter-increment)
    if (from->style()->counterReset()) {
        CounterBaseList* counterData = from->style()->counterReset();
        size_t dataSize = counterData->size();
        Node* parent = from->parentNode();
        for (size_t i = 0; i < dataSize; i++) {
            auto& item = (*counterData)[i];
            resetPseudoCounter(parent, item.first, item.second);
        }
    }
    if (from->style()->counterIncrement()) {
        CounterBaseList* counterData = from->style()->counterIncrement();
        size_t dataSize = counterData->size();
        Node* parent = from->parentNode();
        for (size_t i = 0; i < dataSize; i++) {
            auto& item = (*counterData)[i];
            AtomicString& counterName = item.first;
            int32_t incrementValue = item.second;
            auto matchResult = m_pseudoCounterIndice.find(counterName);
            if (matchResult != m_pseudoCounterIndice.end()) {
                matchResult->second.back() += incrementValue;
            } else {
                // If 'counter-increment' or 'content' on an element or
                // pseudo-element refers to a counter that is not in the scope
                // of any 'counter-reset', implementations should behave as
                // though a 'counter-reset' had reset the counter to 0 on that
                // element or pseudo-element.
                resetPseudoCounter(parent, counterName, incrementValue);
            }
        }
    }
}

void FrameTreeBuilderContext::closeCountingContextIfNeeds(Node* from)
{
    // List counter (list-style-type, list-style-position, list-style-image)
    if (from->isHTMLListContainer()) {
        m_listCounterIndice.pop_back();
    }
    // Pseudo counter (counter-reset, counter-increment)
    if (m_pseudoCounters.size() && m_pseudoCounters.back().first == from) {
        std::unordered_set<AtomicString>& currentSet =
            m_pseudoCounters.back().second;
        auto iter = currentSet.begin();
        while (iter != currentSet.end()) {
            const AtomicString& counterName = *iter;
            std::vector<int32_t>& counter = m_pseudoCounterIndice[counterName];
            STARFISH_ASSERT(counter.size());
            counter.pop_back();
            if (!counter.size()) {
                m_pseudoCounterIndice.erase(counterName);
            }
            iter++;
        }
        m_pseudoCounters.pop_back();
    }
}

Nullable<String*> FrameTreeBuilderContext::getStringFromContentData(
    ContentData* from)
{
    if (from->isText()) {
        return from->text()->text();
    }
    if (from->isCounter()) {
        CounterContentData* counterData = from->counter();
        const AtomicString& counterName = counterData->id();
        auto matchResult = m_pseudoCounterIndice.find(counterName);
        if (matchResult == m_pseudoCounterIndice.end()) {
            return counterData->counterStyle()
                ->generateLabelForCSSContentProperty(0);
        }
        std::vector<int32_t>& indice = matchResult->second;
        Nullable<String*> sp = counterData->separator();
        if (!sp.hasValue()) {
            STARFISH_ASSERT(indice.size());
            return counterData->counterStyle()
                ->generateLabelForCSSContentProperty(indice.back());
        }
        StringBuilder sb;
        size_t indiceSize = indice.size();
        for (size_t i = 0; i < indiceSize; i++) {
            sb.appendString(
                counterData->counterStyle()->generateLabelForCSSContentProperty(
                    indice[i]));
            if (i + 1 != indiceSize) {
                sb.appendString(sp.getValue());
            }
        }
        return sb.finalize();
    }
    return Nullable<String*>();
}

Frame* FrameTreeBuilder::buildTree(Node* current, FrameTreeBuilderContext& ctx,
                                   bool force = false)
{
    bool prevIsInFrameInlineFlow = ctx.isInFrameInlineFlow();
    bool didSplitBlock = false;
    bool needsCreatePseudoElement = false;
    GCVector<FrameInline*> stackedFrameInline;
    Frame* currentFrame;

    ctx.openCountingContextIfNeeds(current);

    if (ctx.isInFrameFlexFlow() || ctx.isInFrameGridFlow()) {
        if (!current->isCharacterData() && current->style()) {
            current->style()->blockify(current, true);
        }
    }

    if ((current->needsFrameTreeBuild() || force)) {
        needsCreatePseudoElement = true;

        if (current->needsFrameTreeBuild()) {
            force = true;
        }

        currentFrame = createFrame(current, ctx, force);
        if (!currentFrame) {
            return nullptr;
        }

        current->setFrame(currentFrame);
        current->clearNeedsFrameTreeBuild();

        if (currentFrame->isNormalFlow()) {
            if ((currentFrame->isBlockLevel() &&
                 (!currentFrame->isFrameTableObjectBox() ||
                  currentFrame->isFrameTableBox())) &&
                ctx.isInFrameInlineFlow()) {
                // divide block. when comes Inline.. + Block(normal flow)
                didSplitBlock = true;

                STARFISH_ASSERT(current->parentNode());
                Frame* parent = current->parentNode()->frame();
                while (parent) {
                    if (!parent->isAnonymous() && parent->isFrameBlockBox()) {
                        break;
                    }
                    parent = parent->parent();
                }

                Node* nd = current->parentNode();
                while (nd) {
                    if (nd->frame()->isFrameBlockBox()) {
                        break;
                    }
                    auto iter = ctx.frameInlineItem().find(nd);
                    STARFISH_ASSERT(iter != ctx.frameInlineItem().end());
                    FrameInline* in = new FrameInline(nd);
                    if (iter->second->isLeftMBPCleared()) {
                        in->setLeftMBPCleared(true);
                    }
                    if (iter->second->isRightMBPCleared()) {
                        in->setRightMBPCleared(true);
                    }

                    if (in->style()->direction() ==
                        DirectionValue::LtrDirectionValue) {
                        in->setLeftMBPCleared(true);
                        iter->second->setRightMBPCleared(true);
                    } else {
                        iter->second->setLeftMBPCleared(true);
                        in->setRightMBPCleared(true);
                    }

                    stackedFrameInline.push_back(in);
                    iter->second = in;
                    STARFISH_ASSERT(ctx.frameInlineItem().find(nd)->second ==
                                    in);
                    nd = nd->parentNode();
                }

                STARFISH_ASSERT(parent);
                ctx.setCurrentBlockContainer(parent->asFrameBlockBox());
                ctx.setIsInFrameInlineFlow(false);
            }
        }

        if (!currentFrame->parent()) {
            FrameBlockBox* oldBlockContainer = ctx.currentBlockContainer();
            FrameTreeBuilder::insertChild(oldBlockContainer, currentFrame,
                                          current, ctx);
            STARFISH_ASSERT(oldBlockContainer == ctx.currentBlockContainer());

            if (currentFrame->isFrameText() &&
                currentFrame->style()->textTransform() !=
                    NoneTextTransformValue) {
                currentFrame->asFrameText()->transformText(
                    currentFrame->asFrameText()->text());
            }
        }
    } else {
        currentFrame = current->frame();
    }

    // display is none or current that can't be inserted
    if (!currentFrame || !currentFrame->parent()) {
        return nullptr;
    }

    Frame* parent = currentFrame->parent();
    bool shouldSkipChildren =
        (currentFrame->isFrameReplaced() || currentFrame->isFrameLineBreak() ||
         currentFrame->isFrameInputBox() || currentFrame->isFrameText() ||
         (currentFrame->isFrameTableColBox() &&
          current->style()->display() ==
              DisplayValue::TableColumnDisplayValue) ||
         current->isBeforePseudoElement() || current->isAfterPseudoElement());

    FrameBlockBox* back = ctx.currentBlockContainer();
    FrameTableObjectBox* lastAnonymousTableObject = nullptr;
    if (parent->isFrameTableObjectBox() && parent->isAnonymous()) {
        lastAnonymousTableObject = parent->asFrameTableObjectBox();
    }
    ctx.setLastAnonymousTableObjectParent(nullptr);

    if (currentFrame->isFrameBlockBox()) {
        ctx.setCurrentBlockContainer(currentFrame->asFrameBlockBox());
    }

    if (currentFrame->isFrameInline()) {
        ctx.setIsInFrameInlineFlow(true);
        ctx.frameInlineItem().insert(
            std::make_pair(current, currentFrame->asFrameInline()));
    } else {
        ctx.setIsInFrameInlineFlow(false);
    }

    if (needsCreatePseudoElement) {
        createInsideCounterIfNeeds(current, ctx);
        createPseudoElement(
            current, StyleResolver::PseudoElementType::PseudoElementBefore,
            ctx);
    }

    if (!shouldSkipChildren && (current->childNeedsFrameTreeBuild() || force)) {
        if (currentFrame->isFrameDocument() ||
            currentFrame->isEstablishesBlockFormattingContext()) {
            currentFrame->markNeedsLayout();
            if (currentFrame->isFrameTableCellBox() ||
                currentFrame->isFrameTableCaptionBox()) {
                Frame* p = currentFrame->parent();
                while (!p->isFrameTableBox()) {
                    p = p->parent();
                }
                p->markNeedsLayout();
            }
        }

        Node* n = current->firstChild();

        while (n) {
            buildTree(n, ctx, force);
            n = n->nextSibling();
        }

        current->clearChildNeedsFrameTreeBuild();
    } else if (current->isBeforePseudoElement() ||
               current->isAfterPseudoElement()) {
        ContentDataGroup* content = current->style()->content();
        if (content) {
            auto iter = content->begin();
            while (iter != content->end()) {
                Nullable<String*> text = ctx.getStringFromContentData(&(*iter));
                if (text.hasValue()) {
                    ComputedStyle* contentTextStyle =
                        new ComputedStyle(current->style());
                    contentTextStyle->setDisplay(
                        DisplayValue::InlineDisplayValue);
                    contentTextStyle->loadResources(current);
                    contentTextStyle->arrangeStyleValues(contentTextStyle,
                                                         current);

                    Text* contentText =
                        new Text(current->document(), text.getValue());
                    contentText->setStyle(contentTextStyle);
                    contentText->setParentNode(current);
                    contentText->clearNeedsStyleRecalc();
                    buildTree(contentText, ctx, force);
                }
                iter++;
            }
        }
    }

    if (needsCreatePseudoElement) {
        createPseudoElement(
            current, StyleResolver::PseudoElementType::PseudoElementAfter, ctx);
    }

    if (lastAnonymousTableObject) {
        ctx.setLastAnonymousTableObjectParent(lastAnonymousTableObject);
    }

    if (currentFrame->isFrameBlockBox()) {
        ctx.setCurrentBlockContainer(back);
    }

    if (didSplitBlock) {
        FrameInline* prev = nullptr;
        for (size_t i = stackedFrameInline.size(); i > 0; i--) {
            FrameInline* in = stackedFrameInline[i - 1];
            if (i == stackedFrameInline.size()) {
                STARFISH_ASSERT(ctx.currentBlockContainer()->hasBlockFlow());

                FrameBlockBox* blockBox =
                    createAnonymousBlockBox<FrameBlockBox>(
                        ctx.currentBlockContainer(), current,
                        DisplayValue::BlockDisplayValue);

                ctx.currentBlockContainer()->appendChild(blockBox);
                blockBox->appendChild(in);
            } else {
                prev->appendChild(in);
            }
            prev = in;
        }
    }

    ctx.setIsInFrameInlineFlow(prevIsInFrameInlineFlow);

    if (needsCreatePseudoElement) {
        createPseudoElement(
            current, StyleResolver::PseudoElementType::PseudoElementFirstLetter,
            ctx);
        createOutsideCounterIfNeeds(current, ctx);
    }

    ctx.closeCountingContextIfNeeds(current);

    return currentFrame;
}

void FrameTreeBuilder::buildFrameTree(Document* document)
{
    STARFISH_ASSERT(document->frame());

    Node* n = document->rootElement();

    if (n) {
        if (n->style()->display() != DisplayValue::NoneDisplayValue) {
            FrameTreeBuilderContext ctx(document->frame()->asFrameBlockBox());
            buildTree(n, ctx);
        }
        n->clearNeedsFrameTreeBuild();
        n->clearChildNeedsFrameTreeBuild();
    }

    document->clearNeedsFrameTreeBuild();
    document->clearChildNeedsFrameTreeBuild();
}
#ifdef STARFISH_ENABLE_TEST
void dump(Frame* frm, unsigned depth)
{
    for (unsigned i = 0; i < depth; i++) {
        printf("  ");
    }
    if (frm->isFlexItem()) {
        printf("%s(FlexItem)", frm->name());
    } else if (frm->isGridItem()) {
        printf("%s(GridItem)", frm->name());
    } else {
        printf("%s", frm->name());
    }
    printf("[%p]", frm);
    if (frm->isAnonymous()) {
        printf("[anonymous block box] ");
    } else {
        frm->node()->dump();
    }

    frm->dump(depth);

    printf("\n");

    Frame* f = frm->firstChild();
    while (f) {
        dump(f, depth + 1);
        f = f->next();
    }
}

bool isNeedsTabBeforeNode(Node* node)
{
    Frame* f = node->frame();
    if (f && (!f->isFrameTableCellBox() ||
              f->asFrameTableCellBox()->absoluteColumnIndex() <= 0))
        return false;
    return true;
}

bool isNeedsNewlinesBeforeNode(Node* node)
{
    if (node->isHTMLParagraphElement() || node->isHTMLDivElement() ||
        node->isHTMLOListElement() || node->isHTMLLIElement() ||
        node->isHTMLUListElement() || node->isHTMLBRElement()) {
        return true;
    }

    Frame* f = node->frame();
    if (node->isHTMLOptionElement() || node->isHTMLOptGroupElement())
        return false;

    if (!f || f->isFrameTableCellBox())
        return false;

    return !f->isFrameInline() && !node->isHTMLBodyElement() &&
           f->isFrameBlockBox() && f->asFrameBlockBox()->hasBlockFlow();
}

String* dumpText(Node* node, bool* lastTextNode)
{
    String* result = String::emptyString;
    if (isNeedsTabBeforeNode(node) && *lastTextNode) {
        result = result->concat('\t');
    } else if (isNeedsNewlinesBeforeNode(node) && *lastTextNode) {
        result = result->concat('\n');
    }

    if (node->isText() &&
        node->parentNode()->style()->visibility() ==
            VisibilityValue::VisibleVisibilityValue &&
        node->parentNode()->parentNode()->style()->display() !=
            DisplayValue::NoneDisplayValue) {
        result = result->concat(
            node->asText()->wholeText()->stripAndCollapseASCIIwhitespace());
        *lastTextNode = true;
    }
    Node* child = node->firstChild();
    while (child) {
        result = result->concat(dumpText(child, lastTextNode));
        child = child->nextSibling();
    }
    return result;
}

void FrameTreeBuilder::dumpFrameTree(Document* document, unsigned depth)
{
    dump(document->frame(), depth);
}

String* FrameTreeBuilder::dumpFrameTreeAsText(Document* document,
                                              unsigned depth)
{
    bool lastTextNode = false;
    return dumpText(document, &lastTextNode);
}

#endif
}
