/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/dom/Element.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/PseudoElement.h"
#include "core/dom/Text.h"
#include "core/dom/svg/SVGSVGElement.h"
#include "core/layout/Frame.h"
#include "core/layout/FrameText.h"
#include "core/layout/FrameInline.h"
#include "core/layout/FrameBlockBox.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameFlexibleBox.h"
#include "core/layout/FrameReplaced.h"
#include "core/layout/FrameReplacedImage.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableRowBox.h"
#include "core/layout/FrameTableSectionBox.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/layout/FrameInputBox.h"
#include "core/layout/FrameSelectBox.h"
#include "core/layout/FrameOptGroupBox.h"
#include "core/layout/FrameOptionBox.h"
#ifdef STARFISH_ENABLE_MULTIMEDIA
#include "core/layout/FrameReplacedVideo.h"
#endif
#include "core/layout/FrameReplacedIFrame.h"
#include "core/layout/FrameReplacedObject.h"
#include "core/layout/FrameLineBreak.h"
#include "core/layout/FrameTableTreeBuilder.h"
#include "core/layout/svg/FrameSVGSVGBox.h"

namespace StarFish {
void dump(Frame* frm, unsigned depth);

FrameTreeBuilderContext::FrameTreeBuilderContext(
    FrameBlockBox* currentBlockContainer)
{
    m_isInFrameInlineFlow = false;
    m_isInFrameFlexFlow = false;
    setCurrentBlockContainer(currentBlockContainer);
}

void FrameTreeBuilderContext::setCurrentBlockContainer(
    FrameBlockBox* blockContainer)
{
    m_currentBlockContainer = blockContainer;
    m_isInFrameFlexFlow = blockContainer->isFrameFlexibleBox();
}

FrameBlockBox* FrameTreeBuilderContext::currentBlockContainer()
{
    return m_currentBlockContainer;
}

std::unordered_map<Node*, FrameInline*>&
FrameTreeBuilderContext::frameInlineItem()
{
    return m_frameInlineItem;
}

bool FrameTreeBuilderContext::isInFrameInlineFlow()
{
    return m_isInFrameInlineFlow;
}

void FrameTreeBuilderContext::setIsInFrameInlineFlow(bool b)
{
    m_isInFrameInlineFlow = b;
}

bool FrameTreeBuilderContext::isInFrameFlexFlow()
{
    return m_isInFrameFlexFlow;
}

void FrameTreeBuilderContext::setIsInFrameFlexFlow(bool b)
{
    m_isInFrameFlexFlow = b;
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

static FrameBlockBox* createAnonymouseBlockBox(FrameBlockBox* blockContainer,
                                               Node* node)
{
    ComputedStyle* style = new ComputedStyle(blockContainer->style());
    style->setDisplay(DisplayValue::BlockDisplayValue);
    style->loadResources(node);
    ComputedStyle* rootStyle = node->document()->rootElement()->style();
    style->arrangeStyleValues(blockContainer->style(), rootStyle, node);

    return new FrameBlockBox(nullptr, style);
}

static FrameBlockBox* wrapWithAnonymouseBlockBox(FrameBlockBox* blockContainer,
                                                 Node* node, Frame* frame)
{
    FrameBlockBox* blockBox = createAnonymouseBlockBox(blockContainer, node);
    blockBox->appendChild(frame);
    blockContainer->appendChild(blockBox);
    return blockBox;
}

void FrameTreeBuilder::insertChild(FrameBlockBox* blockContainer,
                                   Frame* currentFrame, Node* currentNode,
                                   FrameTreeBuilderContext& ctx)
{
    bool isBlockChild = currentFrame->isBlockLevel();
    if (!blockContainer->firstChild()) {
        blockContainer->appendChild(currentFrame);
        return;
    }

    if (!isBlockChild || (!currentFrame->isNormalFlow())) {
        if (currentNode->parentNode()->style()->display() ==
            InlineDisplayValue) {
            auto iter = ctx.frameInlineItem().find(currentNode->parentNode());
            iter->second->appendChild(currentFrame);
            return;
        }
    }

    if (blockContainer->hasBlockFlow()) {
        if (isBlockChild) {
            // Block... + Block case
            if (currentFrame->isNormalFlow() || currentFrame->isFlexItem()) {
                blockContainer->appendChild(currentFrame);
            } else {
                wrapWithAnonymouseBlockBox(blockContainer, currentNode,
                                           currentFrame);
            }
        } else {
            // Block... + Inline case
            Frame* last = blockContainer->lastChild();

            STARFISH_ASSERT(last);

            if (!last->isAnonymous() || last->isFrameTableBox()) {
                wrapWithAnonymouseBlockBox(blockContainer, currentNode,
                                           currentFrame);
            } else {
                last->appendChild(currentFrame);
            }
        }
    } else {
        if (isBlockChild) {
            if (!currentFrame->isNormalFlow()) {
                blockContainer->appendChild(currentFrame);
                return;
            }

            // Inline... + Block case
            GCVector<Frame*> backup;
            while (blockContainer->firstChild()) {
                backup.push_back(blockContainer->firstChild());
                blockContainer->removeChild(blockContainer->firstChild());
            }

            FrameBox* blockBox =
                createAnonymouseBlockBox(blockContainer, currentNode);
            for (unsigned i = 0; i < backup.size(); i++) {
                blockBox->appendChild(backup[i]);
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
    parent->document()->styleResolver().matchAllRules(
        parent->asElement(), style, parentStyle, pseudoId);

    // TODO: Set the proper style according to the type of pseudo-elements
    if (pseudoId ==
        StyleResolver::PseudoElementType::PseudoElementFirstLetter) {
        style->setDisplay(DisplayValue::InlineDisplayValue);
        style->setPosition(PositionValue::StaticPositionValue);
    }
    style->loadResources(parent);
    ComputedStyle* rootStyle = parent->document()->rootElement()->style();
    style->arrangeStyleValues(parentStyle, rootStyle, parent);

    ComputedStyleDamage damage = ComputedStyleDamage::ComputedStyleDamageNone;
    damage = compareStyle(parentStyle, style);

    if (style->pseudoType() !=
            StyleResolver::PseudoElementType::PseudoElementNone &&
        damage != ComputedStyleDamage::ComputedStyleDamageNone &&
        damage != ComputedStyleDamage::ComputedStyleDamageInherited) {
        return style;
    }

    return nullptr;
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

void FrameTreeBuilder::createPseudoElementIfNeeded(
    Node* parent, StyleResolver::PseudoElementType pseudoId,
    FrameTreeBuilderContext& ctx)
{
    if (!parent->isElement() ||
        !parent->asElement()->hasPseudoElement(pseudoId)) {
        return;
    }

    if (pseudoId ==
            StyleResolver::PseudoElementType::PseudoElementFirstLetter &&
        !FirstLetterPseudoElement::firstLetterFrameText(parent)) {
        return;
    }

    PseudoElement* pseudoElement =
        new PseudoElement(parent->document(), pseudoId);
    pseudoElement->setParentNode(parent);

    Frame* pseudoParentFrame = nullptr;
    ComputedStyle* parentStyle = parent->style();
    if (pseudoElement->isFirstLetterPseudoElement()) {
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

    ComputedStyle* pseudoStyle =
        pseudoStyleForElementInternal(parent, pseudoId, parentStyle);
    if (!pseudoElementFrameIsNeeded(pseudoStyle)) {
        return;
    }
    pseudoElement->setStyle(pseudoStyle);

    ComputedStyle* rootStyle = parent->document()->rootElement()->style();

    Frame* pseudoFrame;
    if (pseudoStyle->floating() != FloatValue::NoneFloatValue) {
        pseudoFrame = new FrameBlockBox(pseudoElement, nullptr);
    } else {
        pseudoFrame = new FrameInline(pseudoElement);
    }

    if (pseudoElement->isFirstLetterPseudoElement()) {
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
        letterStyle->arrangeStyleValues(pseudoStyle, rootStyle);
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
    } else if (pseudoElement->isBeforePseudoElement() ||
               pseudoElement->isAfterPseudoElement()) {
        if (!pseudoElement->style()->hasRareComputeStyleData()) {
            return;
        }

        DisplayValue contentDisplay = pseudoElement->style()->display();
        DisplayValue parentDisplay = parent->style()->display();

        // Create pseudo-element's frame.
        Frame* pseudoFrame = nullptr;
        if (ctx.currentBlockContainer()->isFrameTableBox()) {
            FrameTableBox* tableFrame =
                ctx.currentBlockContainer()->asFrameTableBox();
            tableFrame->addChild(pseudoElement, ctx, true);
            pseudoFrame = findPseudoFrameForTable(
                tableFrame, pseudoElement->isBeforePseudoElement());
        } else if (ctx.currentBlockContainer()->isFrameTableRowBox()) {
            FrameTableRowBox* tableRowFrame =
                ctx.currentBlockContainer()->asFrameTableRowBox();
            tableRowFrame->addChild(pseudoElement, ctx, true);
            pseudoFrame = findPseudoFrameForTable(
                tableRowFrame, pseudoElement->isBeforePseudoElement());
        } else if (ComputedStyle::isDisplayTableValueType(parentDisplay) &&
                   !(ctx.currentBlockContainer()->isFrameTableCaptionBox() ||
                     ctx.currentBlockContainer()->isFrameTableCellBox())) {
            // Table has its own frame tree builder. FrameTreeBuilder::buildTree
            // is called to generate frames that is not related to the table in
            // FrameTableCellBox and FrameTableCaptionBox. Thus, duplicated
            // frames for the pseudo-element can be created.
            return;
        } else if (parent->frame()->isFrameBlockBox()) {
            FrameBlockBox* pre = ctx.currentBlockContainer();
            ctx.setCurrentBlockContainer(parent->frame()->asFrameBlockBox());
            pseudoFrame = buildTree(pseudoElement, ctx, true);
            ctx.setCurrentBlockContainer(pre);
        } else {
            pseudoFrame = buildTree(pseudoElement, ctx, true);
        }

        // If the content's display is a table-related value, we have to find
        // the frame for the pseudo-element. Because table has its own frame
        // tree builder and it returns the entire table frames.
        FrameBlockBox* pre = ctx.currentBlockContainer();
        if (contentDisplay == DisplayValue::TableDisplayValue ||
            contentDisplay == DisplayValue::InlineTableDisplayValue ||
            contentDisplay == DisplayValue::TableRowGroupDisplayValue ||
            contentDisplay == DisplayValue::TableHeaderGroupDisplayValue ||
            contentDisplay == DisplayValue::TableFooterGroupDisplayValue ||
            contentDisplay == DisplayValue::TableRowDisplayValue) {
            pseudoFrame = findPseudoFrameForTable(pseudoFrame);
            ctx.setCurrentBlockContainer(pseudoFrame->asFrameBlockBox());
        } else if (contentDisplay == DisplayValue::TableCaptionDisplayValue ||
                   contentDisplay == DisplayValue::TableCellDisplayValue) {
            pseudoFrame = findPseudoFrameForTable(pseudoFrame);
        } else if (contentDisplay == DisplayValue::TableColumnDisplayValue ||
                   contentDisplay ==
                       DisplayValue::TableColumnGroupDisplayValue) {
            return;
        }

        // Add content's frame to the pseudo-element.
        ContentDataGroup& content = pseudoElement->style()->content();
        auto iter = content.begin();
        while (iter != content.end()) {
            if (iter->isText()) {
                ComputedStyle* contentTextStyle =
                    new ComputedStyle(pseudoStyle);
                contentTextStyle->setDisplay(DisplayValue::InlineDisplayValue);
                contentTextStyle->loadResources(pseudoElement);
                contentTextStyle->arrangeStyleValues(contentTextStyle,
                                                     rootStyle);

                Text* contentText =
                    new Text(parent->document(), iter->text()->text());
                contentText->setStyle(contentTextStyle);
                contentText->setParentNode(pseudoElement);
                contentText->clearNeedsStyleRecalc();

                if (contentDisplay == DisplayValue::TableDisplayValue ||
                    contentDisplay == DisplayValue::InlineTableDisplayValue) {
                    STARFISH_ASSERT(pseudoFrame->isFrameTableBox());
                    pseudoFrame->asFrameTableBox()->addChild(contentText, ctx,
                                                             true);
                } else if (contentDisplay ==
                               DisplayValue::TableRowGroupDisplayValue ||
                           contentDisplay ==
                               DisplayValue::TableHeaderGroupDisplayValue ||
                           contentDisplay ==
                               DisplayValue::TableFooterGroupDisplayValue) {
                    STARFISH_ASSERT(pseudoFrame->isFrameTableSectionBox());
                    pseudoFrame->asFrameTableSectionBox()->addChild(contentText,
                                                                    ctx, true);
                } else if (contentDisplay ==
                           DisplayValue::TableRowDisplayValue) {
                    STARFISH_ASSERT(pseudoFrame->isFrameTableRowBox());
                    pseudoFrame->asFrameTableRowBox()->addChild(contentText,
                                                                ctx, true);
                } else {
                    FrameText* contentTextFrame =
                        new FrameText(contentText, contentTextStyle);
                    contentText->setFrame(contentTextFrame);
                    pseudoFrame->appendChild(contentTextFrame);
                }
            }
            iter++;
        }

        ctx.setCurrentBlockContainer(pre);
    }

    STARFISH_ASSERT(parent->isElement());
    parent->asElement()->setPseudoElement(pseudoId, pseudoElement);
}

Frame* FrameTreeBuilder::buildTree(Node* current, FrameTreeBuilderContext& ctx,
                                   bool force = false)
{
    bool prevIsInFrameInlineFlow = ctx.isInFrameInlineFlow();
    bool didSplitBlock = false;
    bool shouldSkipChildren = false;
    bool isTableType =
        current->style() &&
        ComputedStyle::isDisplayTableValueType(current->style()->display());
    FrameBlockBox* originalFrameBlockBox = nullptr;
    GCVector<FrameInline*> stackedFrameInline;

    if (ctx.isInFrameFlexFlow()) {
        if (!current->isCharacterData() && current->style()) {
            current->style()->blockify(current, true);
        }
    }

    if ((current->needsFrameTreeBuild() || force) || isTableType) {
        if (current->needsFrameTreeBuild()) {
            force = true;
        }

        Frame* currentFrame;
        DisplayValue display = current->style()->display();
        bool isVisible = display != DisplayValue::NoneDisplayValue;
        if (!isVisible) {
            current->clearNeedsFrameTreeBuild();
            FrameTreeBuilder::clearTree(current);
            return nullptr;
        }
        if (current->isHTMLImageElement()) {
            currentFrame = new FrameReplacedImage(current);
            shouldSkipChildren = true;
        }
#ifdef STARFISH_ENABLE_MULTIMEDIA
        else if (current->isHTMLVideoElement()) {
            currentFrame = new FrameReplacedVideo(current);
            shouldSkipChildren = true;
        }
#endif
        else if (current->isHTMLIFrameElement()) {
            currentFrame = new FrameReplacedIFrame(current);
            shouldSkipChildren = true;
        } else if (current->isHTMLBRElement()) {
            currentFrame = new FrameLineBreak(current);
            shouldSkipChildren = true;
        } else if (current->isHTMLObjectElement()) {
            currentFrame = new FrameReplacedObject(current);
            shouldSkipChildren = true;
        } else if (current->isSVGSVGElement()) {
            return FrameTreeBuilder::buildSVGFrameTree(
                current->asSVGSVGElement());
        } else if (isTableType) {
            // table has its own frametree builder
            // return nullptr, if buildFrameTable reuse before anonymous table
            // wrapper
            if (!(current->needsFrameTreeBuild() ||
                  current->childNeedsFrameTreeBuild())) {
                return nullptr;
            }
            currentFrame =
                FrameTableTreeBuilder::buildFrameTableTree(current, ctx, force);
            STARFISH_ASSERT(FrameTableTreeBuilder::isTableWrapperDisplayValue(
                currentFrame->style()->display()));
            if (!currentFrame->parent()) {
                FrameTreeBuilder::insertChild(ctx.currentBlockContainer(),
                                              currentFrame, current, ctx);
            }
            if (display != DisplayValue::InlineTableDisplayValue) {
                ctx.setIsInFrameInlineFlow(false);
            }
            shouldSkipChildren = true;
        } else if (current->isHTMLInputElement()) {
            currentFrame = FrameInputBox::buildFrameTree(current, ctx, force);
            if (!currentFrame) {
                return nullptr;
            }
            shouldSkipChildren = true;
        } else if (current->isHTMLSelectElement()) {
            currentFrame = new FrameSelectBox(current, nullptr);
        } else if (current->isHTMLOptGroupElement()) {
            currentFrame = new FrameOptGroupBox(current, nullptr);
        } else if (current->isHTMLOptionElement()) {
            currentFrame = new FrameOptionBox(current, nullptr);
        } else if (display == DisplayValue::FlexDisplayValue ||
                   display == DisplayValue::InlineFlexDisplayValue) {
            currentFrame = new FrameFlexibleBox(current, nullptr);
        } else {
            if (display == DisplayValue::BlockDisplayValue ||
                display == DisplayValue::InlineBlockDisplayValue) {
                currentFrame = new FrameBlockBox(current, nullptr);
            } else if (display == DisplayValue::InlineDisplayValue) {
                if (current->isCharacterData() &&
                    current->asCharacterData()->isText()) {
                    currentFrame = new FrameText(current, current->style());
                } else if (current->isComment()) {
                    FrameTreeBuilder::clearTree(current);
                    return nullptr;
                } else {
                    currentFrame = new FrameInline(current);
                    ctx.setIsInFrameInlineFlow(true);
                    ctx.frameInlineItem().insert(
                        std::make_pair(current, currentFrame->asFrameInline()));
                }
            } else {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        }

        current->setFrame(currentFrame);
        current->clearNeedsFrameTreeBuild();

        if (ctx.isInFrameFlexFlow()) {
            if (currentFrame->isFrameText()) {
                currentFrame = wrapWithAnonymouseBlockBox(
                    ctx.currentBlockContainer(), current, currentFrame);
            }
            currentFrame->markFlexItem();
        }

        if (currentFrame->isNormalFlow()) {
            if (currentFrame->isBlockLevel() && ctx.isInFrameInlineFlow()) {
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
                        in->setLeftMBPCleared();
                    }
                    if (iter->second->isRightMBPCleared()) {
                        in->setRightMBPCleared();
                    }

                    if (in->style()->direction() ==
                        DirectionValue::LtrDirectionValue) {
                        in->setLeftMBPCleared();
                        iter->second->setRightMBPCleared();
                    } else {
                        iter->second->setLeftMBPCleared();
                        in->setRightMBPCleared();
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
        } else {
            // To prevent inline contents from splitting, add not-normal flowed
            // block to inline-box, inline-boxes + Block(Not normal flow)
            if (!ctx.currentBlockContainer()->isFrameFlexibleBox() &&
                ctx.currentBlockContainer()->hasBlockFlow()) {
                Frame* last = ctx.currentBlockContainer()->lastChild();
                if (last) {
                    if (last->isAnonymous() && last->isFrameBlockBox() &&
                        !last->asFrameBlockBox()->hasBlockFlow()) {
                        originalFrameBlockBox = ctx.currentBlockContainer();
                        ctx.setCurrentBlockContainer(last->asFrameBlockBox());
                    }
                }
            }
        }

        if (!currentFrame->parent()) {
            FrameTreeBuilder::insertChild(ctx.currentBlockContainer(),
                                          currentFrame, current, ctx);
#ifndef NDEBUG
            if (ctx.currentBlockContainer()->isFrameFlexibleBox()) {
                STARFISH_ASSERT(
                    ctx.currentBlockContainer()->lastChild()->isFlexItem());
            }
#endif
        }

        STARFISH_ASSERT(currentFrame->parent());
    } else {
        shouldSkipChildren =
            current->frame() && (current->frame()->isFrameReplaced() ||
                                 current->frame()->isFrameLineBreak() ||
                                 current->frame()->isFrameInputBox());
    }

    Frame* currentFrame = current->frame();

    // display == none
    if (!currentFrame) {
        return nullptr;
    }

    createPseudoElementIfNeeded(
        current, StyleResolver::PseudoElementType::PseudoElementBefore, ctx);

    if (!shouldSkipChildren && (current->childNeedsFrameTreeBuild() || force)) {
        Frame* currentFrame = current->frame();
        FrameBlockBox* back = ctx.currentBlockContainer();

        bool isBlockContainer = currentFrame->isFrameBlockBox();
        if (isBlockContainer) {
            ctx.setCurrentBlockContainer(currentFrame->asFrameBlockBox());
        }

        Node* n = current->firstChild();

        while (n) {
            buildTree(n, ctx, force);
            n = n->nextSibling();
        }

        if (isBlockContainer) {
            ctx.setCurrentBlockContainer(back);
        }

        current->clearChildNeedsFrameTreeBuild();
    }

    createPseudoElementIfNeeded(
        current, StyleResolver::PseudoElementType::PseudoElementAfter, ctx);

    if (didSplitBlock) {
        FrameInline* prev = nullptr;
        for (size_t i = stackedFrameInline.size(); i > 0; i--) {
            FrameInline* in = stackedFrameInline[i - 1];
            if (i == stackedFrameInline.size()) {
                STARFISH_ASSERT(ctx.currentBlockContainer()->hasBlockFlow());

                FrameBlockBox* blockBox = createAnonymouseBlockBox(
                    ctx.currentBlockContainer(), current);

                ctx.currentBlockContainer()->appendChild(blockBox);
                blockBox->appendChild(in);
                ctx.setCurrentBlockContainer(blockBox);
            } else {
                prev->appendChild(in);
            }
            prev = in;
        }
    }
    if (originalFrameBlockBox) {
        ctx.setCurrentBlockContainer(originalFrameBlockBox);
    }
    ctx.setIsInFrameInlineFlow(prevIsInFrameInlineFlow);

    createPseudoElementIfNeeded(
        current, StyleResolver::PseudoElementType::PseudoElementFirstLetter,
        ctx);

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

void FrameTreeBuilder::dumpFrameTree(Document* document)
{
    dump(document->frame(), 0);
}
#endif
}
