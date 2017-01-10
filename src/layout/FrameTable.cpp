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
#include "FrameTableRow.h"

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

FrameTable* FrameTable::buildFrameTable(Node* tableNode, FrameTreeBuilderContext& ctx, bool force)
{

    FrameTable* tableWrapper = new FrameTable(tableNode, nullptr);
    FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(), tableWrapper, tableNode, ctx);
    tableNode->setFrame(tableWrapper);

    // Table establishes a new block context
    FrameBlockBox* lastContext = ctx.currentBlockContainer();
    ctx.setCurrentBlockContainer(tableWrapper);
    ctx.mergeTextDecorationData(tableWrapper->style());

    for (Node* c = tableNode->firstChild(); c; c = c->nextSibling()) {
        tableWrapper->addChild(c, ctx, force);
    }

    ctx.setCurrentBlockContainer(lastContext);

    return tableWrapper;
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
        switch (child->style()->display()) {
        case DisplayValue::TableHeaderGroupDisplayValue:
            // TODO
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            break;
        case DisplayValue::TableFooterGroupDisplayValue:
            // TODO
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
            break;
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
        // simple case to generate anonymous table object
        if (wrapInAnnoymousSection && child->isTableRow()) {
            Frame* last = ctx.currentBlockContainer()->lastChild();
            FrameTableSection* anonymous;
            if (last == nullptr || last->node() != nullptr) {
                anonymous = FrameTableSection::createAnonymousWithParent(ctx.currentBlockContainer(), ctx.currentBlockContainer()->node());
                ctx.currentBlockContainer()->appendChild(anonymous);
            } else if (last && last->isFrameTableSection() && last->node() == nullptr) {
                // last node was placed at anonymous table section
                // and current node that is tableRow must be placed at same anonymous table section
                anonymous = last->asFrameTableSection();
            }
            FrameBlockBox* lastContext = ctx.currentBlockContainer();
            ctx.setCurrentBlockContainer(anonymous);
            ctx.mergeTextDecorationData(anonymous->style());

            FrameTableRow* childFrameRow = FrameTableRow::buildFrameTableRow(child, ctx, force);
            FrameTreeBuilder::frameBlockBoxChildInserter(ctx.currentBlockContainer(), childFrameRow, child, ctx);
            ctx.setCurrentBlockContainer(lastContext);
            STARFISH_ASSERT(childFrameRow->parent());
            return;
        } else {
            // TODO
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
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
    LayoutUnit ySoFar = 0;
    for (Frame* c = firstChild(); c; c = c->next()) {
        if (c->isFrameTableSection()) {
            c->asFrameTableSection()->layoutHeight(ctx);
        } else if (c->isFrameTableCaption()) {
            c->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
        }
        c->asFrameBox()->setY(ySoFar);
        ySoFar += c->asFrameBox()->height();
    }

    setHeight(ySoFar);
}

void FrameTable::collectColumnWidths(GCVector<ColStruct>& columnWidthsSoFar, GCVector<ColStruct>& columnWidths)
{
    // FIXME: absolute at this stage
    // Need to consider absolute and logical columns
    if (columnWidthsSoFar.empty()) {
        for (auto &col : columnWidths) {
            columnWidthsSoFar.push_back(col);
        }
    } else {
        // TODO: need to consider multiple table sections here
    }
}

}
