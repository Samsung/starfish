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
#include "core/layout/FrameTableTreeBuilder.h"

#include "core/dom/Document.h"
#include "core/dom/HTMLHtmlElement.h"
#include "core/dom/Node.h"
#include "core/layout/FrameDocument.h"
#include "core/layout/FrameTreeBuilder.h"
#include "core/layout/FrameTableObjectBox.h"
#include "core/layout/FrameTableBox.h"
#include "core/layout/FrameTableCaptionBox.h"
#include "core/layout/FrameTableSectionBox.h"
#include "core/layout/FrameTableRowBox.h"
#include "core/layout/FrameTableColBox.h"
#include "core/layout/FrameTableCellBox.h"

namespace StarFish {

FrameTableObjectBox* FrameTableTreeBuilder::buildFrameTableTree(
    Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    if (current->style()->display() == DisplayValue::NoneDisplayValue) {
        return nullptr;
    }

    FrameTableObjectBox* currentFrame = nullptr;
    FrameBlockBox* parent = ctx.currentBlockContainer();
    bool isProperChild = isProperChildDisplayValueType(
        parent->style()->display(), current->style()->display());

    if (isProperChild) {
        if (force || (current->needsFrameTreeBuild() && !current->frame())) {
            force = true;
            currentFrame = createFrameTableObjectBox(current);
            current->setFrame(currentFrame);
        } else {
            STARFISH_ASSERT(current->frame());
            currentFrame = current->frame()->asFrameTableObjectBox();
        }

        ctx.setCurrentBlockContainer(currentFrame);

        FrameTreeBuilder::createPseudoElementIfNeeded(
            current, StyleResolver::PseudoElementType::PseudoElementBefore,
            ctx);

        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            if (force || c->needsFrameTreeBuild() ||
                c->childNeedsFrameTreeBuild()) {
                if (c->style()->display() != DisplayValue::NoneDisplayValue) {
                    currentFrame->addChild(c, ctx, force);
                }
            }
        }
        current->clearNeedsFrameTreeBuild();
        current->clearChildNeedsFrameTreeBuild();
    } else {
        if (current->needsFrameTreeBuild() || force) {
            force = true;
            // If the current node is not a table wrapper node, make either
            // * an anonymous table wrapper box, or
            // * use the last anonymous wrapper box if it has already been
            // created
            //   by a previous (and continuous) sibling of the current node.
            Frame* consecutiveSibling = parent->lastChild();

            // NEED TO DISCUSSION : StarFish generate anonymous block box which
            // has only whitespace, below code treat above situation
            while (consecutiveSibling && consecutiveSibling->isAnonymous() &&
                   consecutiveSibling->firstChild() &&
                   consecutiveSibling->firstChild()->isFrameText() &&
                   consecutiveSibling->firstChild()
                       ->asFrameText()
                       ->text()
                       ->containsOnlyWhitespace()) {
                consecutiveSibling = consecutiveSibling->previous();
            }

            if (consecutiveSibling && consecutiveSibling->isAnonymous() &&
                consecutiveSibling->isFrameTableObjectBox() &&
                isProperDescendantDisplayValueType(
                    consecutiveSibling->style()->display(),
                    current->style()->display())) {
                currentFrame = consecutiveSibling->asFrameTableObjectBox();
            } else {
                currentFrame = createAnonymousFrameTableObjectBoxWithParent(
                    parent, current);
            }
            ctx.setCurrentBlockContainer(currentFrame);
            currentFrame->addChild(current, ctx, force);
        } else if (current->childNeedsFrameTreeBuild()) {
            for (Frame* f = current->frame(); f; f = f->parent()) {
                if (f->parent() == parent) {
                    STARFISH_ASSERT(f->isAnonymous());
                    STARFISH_ASSERT(f->isFrameTableObjectBox());
                    currentFrame = f->asFrameTableObjectBox();
                    break;
                }
            }
            ctx.setCurrentBlockContainer(currentFrame);
            currentFrame->addChild(current, ctx, force);
        } else {
            // If the frame-tree-building state get here, I think something
            // wrong
            // Therefore, I write the following to recognize the situation.
            STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
        }
    }

    FrameTreeBuilder::createPseudoElementIfNeeded(
        current, StyleResolver::PseudoElementType::PseudoElementAfter, ctx);

    ctx.setCurrentBlockContainer(parent);

    FrameTreeBuilder::createPseudoElementIfNeeded(
        current, StyleResolver::PseudoElementType::PseudoElementFirstLetter,
        ctx);

    STARFISH_ASSERT(currentFrame);
    return currentFrame;
}

bool FrameTableTreeBuilder::isProperChildDisplayValueType(DisplayValue parent,
                                                          DisplayValue child)
{
    if (!ComputedStyle::isDisplayTableValueType(parent)) {
        return isTableWrapperDisplayValue(child);
    }

    if (isTableWrapperDisplayValue(parent)) {
        return (isTableCaptionDisplayValue(child) ||
                isTableRowGroupDisplayValue(child) ||
                isTableColumnGroupDisplayValue(child));
    }

    if (isTableCaptionDisplayValue(parent)) {
        return true;
    }

    if (isTableRowGroupDisplayValue(parent)) {
        return isTableRowDisplayValue(child);
    }

    if (isTableRowDisplayValue(parent)) {
        return isTableCellDisplayValue(child);
    }

    if (isTableColumnGroupDisplayValue(parent)) {
        return isTableColumnDisplayValue(child);
    }

    if (isTableCellDisplayValue(parent)) {
        return (isTableWrapperDisplayValue(child) ||
                !ComputedStyle::isDisplayTableValueType(child));
    }

    STARFISH_ASSERT_NOT_REACHED();
    return false;
}

bool FrameTableTreeBuilder::isProperDescendantDisplayValueType(
    DisplayValue parent, DisplayValue child)
{
    if (isTableCaptionDisplayValue(parent)) {
        return false;
    }
    if (isTableColumnDisplayValue(child)) {
        return isTableColumnGroupDisplayValue(parent);
    }
    if (isTableColumnGroupDisplayValue(parent)) {
        return isTableColumnDisplayValue(child);
    }
    return true;
}

FrameTableObjectBox* FrameTableTreeBuilder::createFrameTableObjectBox(
    Node* current)
{
    STARFISH_ASSERT(current->style());
    STARFISH_ASSERT(current->style()->display());

    DisplayValue display = current->style()->display();
    if (isTableWrapperDisplayValue(display)) {
        return new FrameTableBox(current, nullptr);
    }
    if (isTableCaptionDisplayValue(display)) {
        return new FrameTableCaptionBox(current, nullptr);
    }
    if (isTableRowGroupDisplayValue(display)) {
        return new FrameTableSectionBox(current, nullptr);
    }
    if (isTableRowDisplayValue(display)) {
        return new FrameTableRowBox(current, nullptr);
    }
    if (isTableColumnGroupDisplayValue(display) ||
        isTableColumnDisplayValue(display)) {
        return new FrameTableColBox(current, nullptr);
    }
    if (isTableCellDisplayValue(display)) {
        return new FrameTableCellBox(current, nullptr);
    }

    STARFISH_ASSERT_NOT_REACHED();
    return nullptr;
}

FrameTableObjectBox*
FrameTableTreeBuilder::createAnonymousFrameTableObjectBoxWithParent(
    FrameBlockBox* parent, Node* current)
{
    STARFISH_ASSERT(parent->style());
    STARFISH_ASSERT(current->style());
    DisplayValue parentDisplay = parent->style()->display();
    ComputedStyle* style = new ComputedStyle(parent->style());

    if (!parent->isFrameTableObjectBox() ||
        isTableCellDisplayValue(parentDisplay)) {
        DisplayValue currentDisplay = current->style()->display();
        STARFISH_ASSERT(ComputedStyle::isDisplayTableValueType(currentDisplay));
        style->setDisplay(DisplayValue::TableDisplayValue);
        style->loadResources(current, true);
        style->arrangeStyleValues(parent->style(), true, current);
        return new FrameTableBox(nullptr, style);
    }

    if (isTableWrapperDisplayValue(parentDisplay)) {
        DisplayValue currentDisplay = current->style()->display();
        if (isTableColumnDisplayValue(currentDisplay)) {
            style->setDisplay(DisplayValue::TableColumnGroupDisplayValue);
            style->loadResources(current, true);
            style->arrangeStyleValues(parent->style(), true, current);
            return new FrameTableColBox(nullptr, style);
        } else {
            style->setDisplay(DisplayValue::TableRowGroupDisplayValue);
            style->loadResources(current, true);
            style->arrangeStyleValues(parent->style(), true, current);
            return new FrameTableSectionBox(nullptr, style);
        }
    }

    if (isTableRowGroupDisplayValue(parentDisplay)) {
        style->setDisplay(DisplayValue::TableRowDisplayValue);
        style->loadResources(current, true);
        style->arrangeStyleValues(parent->style(), true, current);
        return new FrameTableRowBox(nullptr, style);
    }

    if (isTableRowDisplayValue(parentDisplay)) {
        style->setDisplay(DisplayValue::TableCellDisplayValue);
        style->loadResources(current, true);
        style->arrangeStyleValues(parent->style(), true, current);
        return new FrameTableCellBox(nullptr, style);
    }

    STARFISH_ASSERT_NOT_REACHED();
    return nullptr;
}

void FrameTableBox::addChild(Node* child, FrameTreeBuilderContext& ctx,
                             bool force)
{
    bool wrapInAnnoymousSection = false;
    Frame* childFrame;
    DisplayValue display = child->style()->display();

    switch (display) {
    case DisplayValue::TableCaptionDisplayValue:
        childFrame =
            FrameTableTreeBuilder::buildFrameTableTree(child, ctx, force);
        STARFISH_ASSERT(childFrame->isFrameTableCaptionBox());
        m_captions.push_back(childFrame->asFrameTableCaptionBox());
        break;
    case DisplayValue::TableColumnGroupDisplayValue:
    case DisplayValue::TableColumnDisplayValue:
        // buildFrameTableColBox always return a pointer of Frame object
        // If childFrame is reused anonymous, it will already have a parent,
        // so only forms a parent-child relationship when there is no parent.
        childFrame =
            FrameTableTreeBuilder::buildFrameTableTree(child, ctx, force);
        STARFISH_ASSERT(childFrame->isFrameTableColBox());
        if (!childFrame->parent()) {
            ctx.currentBlockContainer()->appendChild(childFrame);
            m_colObjects.push_back(childFrame->asFrameTableColBox());
        }
        return;
    case DisplayValue::TableHeaderGroupDisplayValue:
    case DisplayValue::TableFooterGroupDisplayValue:
    case DisplayValue::TableRowGroupDisplayValue:
        childFrame =
            FrameTableTreeBuilder::buildFrameTableTree(child, ctx, force);
        STARFISH_ASSERT(childFrame->isFrameTableSectionBox());
        break;
    default:
        wrapInAnnoymousSection = true;
    }

    if (!wrapInAnnoymousSection && !childFrame->parent()) {
        ctx.currentBlockContainer()->appendChild(childFrame);
        STARFISH_ASSERT(childFrame->parent());
        if (!m_thead &&
            (display == DisplayValue::TableHeaderGroupDisplayValue)) {
            m_thead = childFrame->asFrameTableSectionBox();
        }
        if (!m_tfoot &&
            (display == DisplayValue::TableFooterGroupDisplayValue)) {
            m_tfoot = childFrame->asFrameTableSectionBox();
        }
        return;
    }

    if (child->isComment() ||
        (child->isCharacterData() &&
         (!child->textContent().hasValue() ||
          child->textContent().getValue()->containsOnlyWhitespace()))) {
        // TODO, do not use assert!
        return;
    } else if (wrapInAnnoymousSection) {
        // If there are 2 continuous node which becomes internal table box
        // without any proper parent, we have a problem, because parser doesn't
        // form a group these with one parent.
        // so we handle this at buileFrameTableXXX. The first node's frame box
        // will be returned with a hierarchical anonymous table box,
        // the point is the second one. If the second one does the same with
        // the first one, then we might have duplication processing about
        // reused anonymous box so that currentBlockContainer has 2 children
        // which referencing the same thing.
        // To prevent this situation, we separate two cases with which returned
        // pointer has a parent or not.
        childFrame =
            FrameTableTreeBuilder::buildFrameTableTree(child, ctx, force);
        STARFISH_ASSERT(childFrame->isFrameTableSectionBox());
        if (childFrame && !childFrame->parent()) {
            ctx.currentBlockContainer()->appendChild(childFrame);
            STARFISH_ASSERT(childFrame->parent());
        }
        return;
    }
}

void FrameTableCaptionBox::addChild(Node* child, FrameTreeBuilderContext& ctx,
                                    bool force)
{
    FrameTreeBuilder::buildTree(child, ctx, force);
}

void FrameTableSectionBox::addChild(Node* child, FrameTreeBuilderContext& ctx,
                                    bool force)
{
    STARFISH_ASSERT(ctx.currentBlockContainer()->isFrameTableSectionBox());
    if (child->isComment() ||
        (child->isCharacterData() &&
         (!child->textContent().hasValue() ||
          child->textContent().getValue()->containsOnlyWhitespace()))) {
        // TODO, do not use assert!
        return;
    } else {
        FrameTableSectionBox* parentSection =
            ctx.currentBlockContainer()->asFrameTableSectionBox();
        FrameTableRowBox* childFrame = nullptr;
        Frame* frame =
            FrameTableTreeBuilder::buildFrameTableTree(child, ctx, force);
        STARFISH_ASSERT(frame->isFrameTableRowBox());
        childFrame = frame->asFrameTableRowBox();
        if (!childFrame->parent()) {
            childFrame->setRowIndex(parentSection->grid().size());
            RowStruct row(childFrame);
            parentSection->grid().push_back(row);
            ctx.currentBlockContainer()->appendChild(childFrame);
        }
        STARFISH_ASSERT(childFrame->parent());
        return;
    }
}

void FrameTableRowBox::addChild(Node* child, FrameTreeBuilderContext& ctx,
                                bool force)
{
    STARFISH_ASSERT(ctx.currentBlockContainer()->isFrameTableRowBox());
    if (child->isComment() ||
        (child->isCharacterData() &&
         (!child->textContent().hasValue() ||
          child->textContent().getValue()->containsOnlyWhitespace()))) {
        // TODO, do not use assert!
        return;
    } else {
        FrameTableRowBox* parentRow =
            ctx.currentBlockContainer()->asFrameTableRowBox();
        FrameTableCellBox* childFrame = nullptr;
        Frame* frame =
            FrameTableTreeBuilder::buildFrameTableTree(child, ctx, force);
        STARFISH_ASSERT(frame->isFrameTableCellBox());
        childFrame = frame->asFrameTableCellBox();
        if (!childFrame->parent()) {
            childFrame->setAbsoluteColumnIndex(m_lastAbsoluteColumnIndex);
            m_lastAbsoluteColumnIndex += childFrame->colspan();
            ctx.currentBlockContainer()->appendChild(childFrame);
            // Put the cell manually into the rowstruct If the row is
            // an anonymous row that has already been created
            if (parentRow->isAnonymous() && parentRow->parent()) {
                RowStruct& row = parentRow->parent()
                                     ->asFrameTableSectionBox()
                                     ->grid()[parentRow->rowIndex()];
                row.cells().push_back(
                    CellStruct(childFrame->asFrameTableCellBox()));
            }
        }
        STARFISH_ASSERT(childFrame->parent());
        return;
    }
}

void FrameTableColBox::addChild(Node* child, FrameTreeBuilderContext& ctx,
                                bool force)
{
    if (child->style()->display() == DisplayValue::TableColumnDisplayValue) {
        // column must have no children, so don't build the sub frame-tree
        FrameTableColBox* childFrame = new FrameTableColBox(child, nullptr);
        ctx.currentBlockContainer()->appendChild(childFrame);
        child->setFrame(childFrame);
        child->clearNeedsFrameTreeBuild();
        child->clearChildNeedsFrameTreeBuild();
        STARFISH_ASSERT(childFrame->parent());
        return;
    }
    // Ignore the other child node which display value is not
    // 'TableColumnDisplayValue'.
    return;
}

void FrameTableCellBox::addChild(Node* child, FrameTreeBuilderContext& ctx,
                                 bool force)
{
    FrameTreeBuilder::buildTree(child, ctx, force);
}
}
