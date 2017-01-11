/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
#include "FrameTable.h"

#include "FrameTreeBuilder.h"
#include "FrameTableCaption.h"
#include "FrameTableSection.h"

namespace StarFish {

class TableFormattingContextBlock {
public:
    TableFormattingContextBlock(Frame* frm, LayoutContext& ctx)
        : m_ctx(ctx)
        , m_needs(false)
    {
        if (frm->isEstablishesBlockFormattingContext()) {
            m_needs = true;
            m_ctx.establishBlockFormattingContext(frm->isNormalFlow());
        }
    }

    ~TableFormattingContextBlock()
    {
        if (m_needs) {
            m_ctx.removeBlockFormattingContext();
        }
    }

    LayoutContext& m_ctx;
    bool m_needs;
};

FrameTable::FrameTable(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
    STARFISH_ASSERT((node == nullptr && style != nullptr)
        || (node != nullptr && style == nullptr));
}

FrameTable* FrameTable::buildFrameTable(Node* current, FrameTreeBuilderContext& ctx, bool force)
{
    FrameTable* tableWrapper;
    if (current->isTable()) {
        // if current node is table then make wrapper and current node owns this wrapper
        tableWrapper = new FrameTable(current, nullptr);
        current->setFrame(tableWrapper);

        // Table establishes a new block context
        FrameBlockBox* lastContext = ctx.currentBlockContainer();
        ctx.setCurrentBlockContainer(tableWrapper);
        ctx.mergeTextDecorationData(tableWrapper->style());

        for (Node* c = current->firstChild(); c; c = c->nextSibling()) {
            tableWrapper->addChild(c, ctx, force);
        }

        ctx.setCurrentBlockContainer(lastContext);
        return tableWrapper;
    } else if (current->isTable() == false) {
        // if current node is not table wrapper node then make anonymous wrapper or
        // reuse before anonymous wrapper
        FrameBlockBox* parent = ctx.currentBlockContainer();
        Frame* before = parent->lastChild();

        // NEED TO DISCUSSION : StarFish generate anonymous block box which has only wihtespace,
        // below code treat above situation
        while (before->isAnonymous() && before->firstChild()->isFrameText()
            && before->firstChild()->asFrameText()->text()->containsOnlyWhitespace()) {
            before = before->previous();
        }

        if (before && before->isAnonymous() && before->isFrameTable()) {
            tableWrapper = before->asFrameTable();
        } else {
            tableWrapper = FrameTable::createAnonymousWithParent(parent, current);
        }
        ctx.setCurrentBlockContainer(tableWrapper);
        ctx.mergeTextDecorationData(tableWrapper->style());

        if (current->isTableCaption()) {
            tableWrapper->addChild(current, ctx, force);
        } else {
            // TODO : Treat of two or more sections
            // return nullptr, if buildFrameTableSection reuse before anonymouse section
            FrameTableSection* section = FrameTableSection::buildFrameTableSection(current, ctx, force);
            if (section != nullptr) {
                tableWrapper->appendChild(section);
            }
        }
        ctx.setCurrentBlockContainer(parent);
        return tableWrapper->parent()? nullptr : tableWrapper;
    } else {
        STARFISH_ASSERT_NOT_REACHED();
    }
}

FrameTable* FrameTable::createAnonymousWithParent(FrameBlockBox* parent, Node* node)
{
    ComputedStyle* style = new ComputedStyle(parent->style());
    style->setDisplay(DisplayValue::TableRowDisplayValue);
    style->loadResources(node);
    style->arrangeStyleValues(parent->style(), node);

    return new FrameTable(nullptr, style);
}

void FrameTable::addChild(Node* child, FrameTreeBuilderContext& ctx, bool force)
{
    bool wrapInAnnoymousSection = false;
    Frame* childFrame;

    if (child->isTableCaption()) {
        childFrame = FrameTableCaption::buildFrameTableCaption(child, ctx, force);
        m_captions.push_back(childFrame->asFrameTableCaption());
    } else if (child->isTableCol()) {
        // TODO
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    } else if (child->isTableSection()) {
        // Only [TableHeaderGroup | TableFooterGroup | TableRowGroup] should appear
        switch (child->style()->display()) {
        case DisplayValue::TableHeaderGroupDisplayValue:
        case DisplayValue::TableFooterGroupDisplayValue:
        case DisplayValue::TableRowGroupDisplayValue:
            childFrame = FrameTableSection::buildFrameTableSection(child, ctx, force);
            break;
        default:
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else {
        wrapInAnnoymousSection = true;
    }

    if (!wrapInAnnoymousSection) {
        FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(), childFrame, child, ctx);
        STARFISH_ASSERT(childFrame->parent());
        return;
    }

    if (child->isCharacterData() || child->isComment()) {
        // TODO
        return;
    } else {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
}

void FrameTable::layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat)
{
    // This method is called by FrameBlockBox::layout() to do table layout.
    // Table starts its own layout algorithm that has minimum interaction with
    // the existing layout algorithm.
    //
    // In brief,
    // after establishes a table context, we calculate the width of the table,
    // and place cells in rows and columns. To do so, we calculate x positions
    // of cells first, and then calculate the y positions of cells.
    TableFormattingContextBlock context(this, ctx);

    if (resolveWhat & Frame::LayoutWantToResolve::ResolveWidth) {
        calContentWidth(ctx);
        layoutWidth(ctx);
    }
    if (resolveWhat & Frame::LayoutWantToResolve::ResolveHeight) {
        layoutHeight(ctx);
    }
}

void FrameTable::calContentWidth(LayoutContext& ctx)
{
    // We traverse the table to calculate min/max content width of the table
    // before we perform table layout.
    m_columnWidths.clear();
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSection()) {
            c->asFrameTableSection()->calContentWidth(ctx);
            collectColumnWidths(m_columnWidths, c->asFrameTableSection()->columnWidths());
        }
    }
}

void FrameTable::layoutWidth(LayoutContext& ctx)
{
    // The width of the caption is limited by the max width of the
    // FrameTableSection. Hence, captions can only be placed after calculating
    // the width of the table, which has already been done by calContentWidth()
    LayoutUnit maxWidth = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSection()) {
            c->asFrameTableSection()->layoutWidth(ctx);
            maxWidth = std::max(maxWidth, c->asFrameBox()->width());
        } else if (c->isFrameTableCaption()) {
            c->asFrameTableCaption()->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    // The width of all table sections should be the same,
    // so getting the max width should be the same as the width of
    // any table sections.
    setWidth(maxWidth);
    computeBorderMarginPadding(width());
}

void FrameTable::layoutHeight(LayoutContext& ctx)
{
    // Table is placed in the follow order:
    // 1. Captions that has property "caption-side: top"
    //    If there are multiple captions, place them in document order
    // 2. Table sections in document order
    // 3. Captions that has property "caption-side: bottom"
    //    If there are multiple captions, place them in document order

    LayoutUnit ySoFar = LayoutUnit::fromPixel(style()->borderTopWidth().fixed());
    LayoutUnit topCaptionHeightsSoFar = 0;

    // 1. place captions with caption-side: top
    for (auto& caption : m_captions) {
        if (caption->style()->captionSide() == CaptionSideValue::TopCaptionSideValue) {
            caption->setWidth(width());
            caption->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
            caption->setY(ySoFar);
            ySoFar += caption->height();
            topCaptionHeightsSoFar += caption->height();
        }
    }

    // 2. place table sections
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSection()) {
            c->asFrameTableSection()->layoutHeight(ctx);
            c->asFrameBox()->setY(ySoFar);
            ySoFar += c->asFrameBox()->height();
        }
    }

    // 3. place captions with caption-side: bottom
    for (auto& caption : m_captions) {
        if (caption->style()->captionSide() == CaptionSideValue::BottomCaptionSideValue) {
            caption->setWidth(width());
            caption->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
            caption->setY(ySoFar);
            ySoFar += caption->height();
        }
    }

    ySoFar += LayoutUnit::fromPixel(style()->borderBottomWidth().fixed());
    setHeight(ySoFar);
}

void FrameTable::collectColumnWidths(GCVector<ColStruct>& columnWidthsSoFar, GCVector<ColStruct>& columnWidths)
{
    // FIXME: absolute at this stage
    // Need to consider absolute and logical columns
    if (columnWidthsSoFar.empty()) {
        columnWidthsSoFar = columnWidths;
    } else {
        // Update to support colspans
        STARFISH_ASSERT(columnWidthsSoFar.size() == columnWidths.size());
        for (unsigned i = 0; i < columnWidths.size(); i++) {
            ColStruct& colSoFar = columnWidthsSoFar[i];
            ColStruct& col = columnWidths[i];
            colSoFar.maxContentWidth = std::max(colSoFar.maxContentWidth, col.maxContentWidth);
            colSoFar.minContentWidth = std::max(colSoFar.minContentWidth, col.minContentWidth);
        }
    }
}

}
