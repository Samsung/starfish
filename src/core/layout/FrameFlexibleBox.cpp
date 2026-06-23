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
#include "Starfish.h"

#include "core/style/Style.h"
#include "core/style/ComputedStyle.h"
#include "core/dom/Node.h"
#include "core/layout/FrameFlexibleBox.h"

namespace Starfish {

void* FrameFlexibleBox::operator new(size_t size)
{
    STARFISH_ASSERT(size == sizeof(FrameFlexibleBox));
    static bool typeInited = false;
    static GC_descr descr;
    if (!typeInited) {
        GC_word desc[GC_BITMAP_SIZE(FrameFlexibleBox)] = { 0 };
        FrameFlexibleBox::fillGCDescriptor(desc);
        descr = GC_make_descriptor(desc, GC_WORD_LEN(FrameFlexibleBox));
        typeInited = true;
    }
    return GC_MALLOC_EXPLICITLY_TYPED(size, descr);
}

FlexFormattingContext::FlexFormattingContext(
    LayoutContext& ctx, FrameFlexibleBox* container, LayoutUnit availableWidth,
    bool shouldRespectPercentageWidthOnComputingBasisSize)
    : m_layoutContext(ctx)
    , m_container(container)
    , m_isMainAxisInInlineAxis(m_container->isMainAxisInInlineAxis())
    , m_isLtrDirection(m_container->isLtrDirection())
    , m_isTtbDirection(m_container->isTtbDirection())
    , m_isSingleLine(m_container->isSingleLine())
    , m_shouldRespectPercentageWidthOnComputingBasisSize(
          shouldRespectPercentageWidthOnComputingBasisSize)
    , m_currentLineIdx(SIZE_MAX)
    , m_mainGap(0)
    , m_crossGap(0)
{
    addNewLine(); // Add initial line.
    computeAvailableSpace(availableWidth);

    LayoutUnit columnGap, rowGap;
    if (m_container->style()->columnGap().isSpecified()) {
        columnGap = m_container->style()->columnGap().specifiedValue(
            availableWidth, container->node());
    }
    if (m_container->style()->rowGap().isSpecified()) {
        LayoutUnit blockSize = m_isMainAxisInInlineAxis ? m_availableCrossSize
                                                        : m_availableMainSize;
        if (blockSize == intMaxForLayoutUnit) {
            blockSize = 0;
        }
        rowGap = m_container->style()->rowGap().specifiedValue(
            blockSize, container->node());
    }
    if (m_isMainAxisInInlineAxis) {
        m_mainGap = columnGap;
        m_crossGap = rowGap;
    } else {
        m_mainGap = rowGap;
        m_crossGap = columnGap;
    }
}

void FlexFormattingContext::computeAvailableSpace(LayoutUnit availableWidth)
{
    Length height = m_container->style()->height();
    bool parentHasFixedHeight =
        m_layoutContext.parentHasFixedHeight(m_container);
    LayoutUnit contentHeight = intMaxForLayoutUnit;

    if (height.isDefinite(parentHasFixedHeight)) {
        LayoutUnit parentContentHeight;
        if (parentHasFixedHeight) {
            parentContentHeight =
                m_layoutContext.parentFixedHeight(m_container);
        }
        contentHeight = height.specifiedValue(parentContentHeight, m_container);
        contentHeight =
            m_container->contentHeightAfterApplyingBoxSizing(contentHeight);
    } else if (height.isAuto() && m_container->isAbsolutePositioned()) {
        LengthData offset = m_container->style()->offset();
        Length top = offset.top();
        Length bottom = offset.bottom();
        if (top.isSpecified() && bottom.isSpecified()) {
            FrameBox* cb = containingBlock(m_container);
            LayoutUnit parentHeight = cb->contentHeight() + cb->paddingHeight();
            LayoutUnit t = top.specifiedValue(parentHeight, m_container);
            LayoutUnit b = bottom.specifiedValue(parentHeight, m_container);
            contentHeight = parentHeight - t - b -
                            m_container->paddingHeight() -
                            m_container->borderHeight();
        }
    }

    if (m_isMainAxisInInlineAxis) {
        m_availableMainSize = availableWidth;
        m_availableCrossSize = contentHeight;
    } else {
        m_availableMainSize = contentHeight;
        m_availableCrossSize = availableWidth;
    }
}

LayoutUnit FlexFormattingContext::basisSize(FrameBox* flexItem)
{
    auto cache = m_layoutContext.testBasisSizeCache(
        flexItem, m_isMainAxisInInlineAxis,
        m_isMainAxisInInlineAxis ? m_availableMainSize : m_availableCrossSize,
        m_shouldRespectPercentageWidthOnComputingBasisSize);
    if (cache) {
        return cache.value();
    }

    auto basisSize = m_container->basisSize(
        m_layoutContext, m_availableMainSize, m_availableCrossSize, flexItem,
        m_shouldRespectPercentageWidthOnComputingBasisSize);

    m_layoutContext.registerToBasisSizeCache(
        flexItem,
        m_isMainAxisInInlineAxis ? m_availableMainSize : m_availableCrossSize,
        basisSize.second, m_shouldRespectPercentageWidthOnComputingBasisSize,
        basisSize.first);
    return basisSize.first;
}

// CSS Flexbox §4.5: Automatic Minimum Size of Flex Items.
// Returns the content-based size suggestion that should floor the item's
// hypothetical main size when its used min main-size is `auto` and it is not a
// scroll container in the main axis. Returns 0 when the automatic minimum does
// not apply (so callers can safely std::max() it).
//
// Without this floor, a flex-basis:0 item inside a container with an indefinite
// main size (e.g. an auto-height column flex container) collapses to 0 because
// there is no positive free space to grow into, leaving siblings stacked at the
// same position.
LayoutUnit FlexFormattingContext::automaticMinimumMainSize(FrameBox* flexItem)
{
    if (m_isMainAxisInInlineAxis) {
        if (!flexItem->style()->minWidth().isAuto() ||
            flexItem->appliedOverflowX() != OverflowValue::VisibleOverflow) {
            return 0;
        }
    } else {
        if (!flexItem->style()->minHeight().isAuto() ||
            flexItem->appliedOverflowY() != OverflowValue::VisibleOverflow) {
            return 0;
        }
    }

    // The content size suggestion is the min-content size in the main axis. For
    // a column container the main axis is the block axis, so the result depends
    // on the item's cross size (its width). Per CSS Flexbox §9.2, a definite
    // cross size must be used here; only an auto-and-indefinite cross size
    // falls back to fit-content. A stretch-aligned item in a container with a
    // definite cross size has a definite (stretched) cross size, so measure at
    // that width/height. Otherwise the item (e.g. a nested flex container)
    // would be sized to its fit-content cross size, wrapping its text and
    // overstating the main size.
    bool didFixCrossSize = false;
    if (m_availableCrossSize != intMaxForLayoutUnit &&
        isStretchedAlongCrossAxis(flexItem)) {
        flexItem->computeBorderMarginPadding(m_layoutContext,
                                             m_availableCrossSize);
        bool contentBox = flexItem->style()->boxSizing() ==
                          BoxSizingValue::ContentBoxBoxSizingValue;
        LayoutUnit crossSize;
        if (m_isMainAxisInInlineAxis) {
            crossSize =
                m_availableCrossSize -
                (contentBox ? flexItem->mbpHeight() : flexItem->marginHeight());
        } else {
            crossSize =
                m_availableCrossSize -
                (contentBox ? flexItem->mbpWidth() : flexItem->marginWidth());
        }
        if (crossSize < 0) {
            crossSize = 0;
        }
        // The cross-axis size is auto here (required for stretch), so it is
        // safe to restore it to auto afterwards.
        if (m_isMainAxisInInlineAxis) {
            flexItem->style()->setHeight(
                Length(Length::Fixed, crossSize.toInt()));
        } else {
            flexItem->style()->setWidth(
                Length(Length::Fixed, crossSize.toInt()));
        }
        didFixCrossSize = true;
    }

    // Compute the content size suggestion by sizing the item with a content
    // flex basis. We call the base-size computation directly (not the cached
    // basisSize() wrapper) since the cache key does not include flex-basis.
    FlexBasisData savedBasis = flexItem->style()->flexBasis();
    flexItem->style()->setFlexBasis(FlexBasisData(FlexBasisData::Content));
    flexItem->markNeedsLayout();
    LayoutUnit contentSuggestion =
        m_container
            ->basisSize(m_layoutContext, m_availableMainSize,
                        m_availableCrossSize, flexItem,
                        m_shouldRespectPercentageWidthOnComputingBasisSize)
            .first;
    flexItem->style()->setFlexBasis(savedBasis);
    if (didFixCrossSize) {
        if (m_isMainAxisInInlineAxis) {
            flexItem->style()->setHeight(Length());
        } else {
            flexItem->style()->setWidth(Length());
        }
    }
    flexItem->markNeedsLayout();
    clearBasisSizeFromCache(flexItem);

    if (contentSuggestion == intMaxForLayoutUnit) {
        return 0;
    }
    return contentSuggestion;
}

// Mirrors the stretch determination in computeCrossSize(): an item stretches
// along the cross axis when align-self is stretch, its cross-axis size is auto,
// and its cross-axis margins are not auto.
bool FlexFormattingContext::isStretchedAlongCrossAxis(FrameBox* flexItem)
{
    if (flexItem->style()->alignSelf() != StretchAlignItemValue) {
        return false;
    }
    LengthData margin = flexItem->style()->margin();
    if (m_isMainAxisInInlineAxis) {
        return flexItem->style()->height().isAuto() && !margin.top().isAuto() &&
               !margin.bottom().isAuto();
    }
    return flexItem->style()->width().isAuto() && !margin.left().isAuto() &&
           !margin.right().isAuto();
}

void FlexFormattingContext::clearBasisSizeFromCache(FrameBox* flexItem)
{
    m_layoutContext.unregisterToBasisSizeCache(
        flexItem,
        m_isMainAxisInInlineAxis ? m_availableMainSize : m_availableCrossSize);
}

void FlexFormattingContext::computeMainSize()
{
    std::vector<FrameBox*> orderedFlexItems;
    LayoutUnit lineMainSize;
    Frame* child = m_container->firstChild();
    LayoutUnit maxMainSize = 0;

    while (child) {
        if (child->isFlexItem()) {
            orderedFlexItems.push_back(child->asFrameBox());
        }
        child = child->next();
    }

    std::stable_sort(orderedFlexItems.begin(), orderedFlexItems.end(),
                     [](FrameBox* a, FrameBox* b) {
                         return a->style()->order() < b->style()->order();
                     });

    auto iter = orderedFlexItems.begin();

    while (iter != orderedFlexItems.end()) {
        FlexLine& flexLine = m_flexLines[m_currentLineIdx];
        std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
        FrameBox* flexItem = *iter;
        LayoutUnit mainSize = basisSize(flexItem);
        LayoutUnit requiredMainSizeForItemInFlexLine;

        STARFISH_ASSERT(mainSize != intMaxForLayoutUnit);

        // When the container's main size is indefinite, flexible lengths cannot
        // grow into free space, so the hypothetical main size must be floored
        // by the item's automatic minimum size (CSS Flexbox §4.5). Otherwise a
        // flex-basis:0 item collapses to 0 and siblings overlap.
        if (m_availableMainSize == intMaxForLayoutUnit) {
            mainSize = std::max(mainSize, automaticMinimumMainSize(flexItem));
        }

        if (m_isMainAxisInInlineAxis) {
            flexItem->computeBorderMarginPadding(m_layoutContext,
                                                 m_availableMainSize);
            flexItem->setContentWidthConsideringMinMaxWidths(
                m_layoutContext, mainSize, m_availableMainSize);

            requiredMainSizeForItemInFlexLine = flexItem->outerWidth();
        } else {
            flexItem->computeBorderMarginPadding(m_layoutContext,
                                                 m_availableCrossSize);
            bool parentHasFixedHeight =
                m_layoutContext.parentHasFixedHeight(flexItem);

            // CSS Flexbox §4.5: the automatic minimum size (min-height:auto) of
            // a column flex item is derived from its content via its content
            // height and line boxes (heightAfterApplyingMinMaxHeights() ->
            // LayoutContext::lookupFirstLineOrDefiniteHeight). An item with a
            // definite flex-basis is resolved by basisSize() case A without
            // being laid out, so it has neither a content height nor line boxes
            // yet and the content-based minimum would be lost, collapsing the
            // item (and an indefinite container) to zero main size. Lay it out
            // once at its auto main size so the minimum can be measured. Items
            // with a content/auto basis were already laid out by basisSize()
            // (case E), so the lookup succeeds and this extra layout is
            // skipped.
            if (!flexItem->style()->minHeight().isSpecified() &&
                !m_container->shouldApplyLineClamp(flexItem) &&
                flexItem->appliedOverflowY() == VisibleOverflow &&
                !m_layoutContext.lookupFirstLineOrDefiniteHeight(flexItem)) {
                Length heightBackup = flexItem->style()->height();
                flexItem->markNeedsLayout();
                flexItem->style()->setHeight(Length());
                flexItem->layout(m_layoutContext,
                                 Frame::LayoutWantToResolve::ResolveWidth);
                flexItem->layout(m_layoutContext,
                                 Frame::LayoutWantToResolve::ResolveHeight);
                flexItem->style()->setHeight(heightBackup);
            }

            flexItem->setContentHeightConsideringMinMaxHeights(
                m_layoutContext, mainSize, m_availableMainSize,
                parentHasFixedHeight);

            requiredMainSizeForItemInFlexLine = flexItem->outerHeight();
        }

        if (flexItems.size()) {
            requiredMainSizeForItemInFlexLine += m_mainGap;
        }

        bool canPlaceToFlexLine =
            (lineMainSize + requiredMainSizeForItemInFlexLine <=
             m_availableMainSize);
        if (m_isSingleLine || (lineMainSize == 0) || canPlaceToFlexLine) {
            if (flexItems.size() > 0) {
                lineMainSize += m_mainGap;
                flexLine.m_sumOfMainGapInComputeMainSize += m_mainGap;
            }
            flexItems.push_back(flexItem);
            if (m_isMainAxisInInlineAxis) {
                lineMainSize += flexItem->outerWidth();
            } else {
                lineMainSize += flexItem->outerHeight();
            }
        } else {
            // Add new flexLine and Retry to put this flexitem on new flexLine.
            maxMainSize = std::max(maxMainSize, lineMainSize);
            flexLine.m_lineWidth = lineMainSize;
            addNewLine();
            lineMainSize = 0;
            continue;
        }

        iter++;
    }

    FlexLine& flexLine = m_flexLines[m_currentLineIdx];
    std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
    if (flexItems.size() == 0) {
        m_currentLineIdx--;
    } else {
        maxMainSize = std::max(maxMainSize, lineMainSize);
        flexLine.m_lineWidth = lineMainSize;
    }

    if (!m_isMainAxisInInlineAxis) {
        m_container->computeContentHeight(m_layoutContext, maxMainSize);
        if (m_availableMainSize == intMaxForLayoutUnit) {
            m_availableMainSize = m_container->contentHeight();
        }
    }
    STARFISH_ASSERT(m_availableMainSize != intMaxForLayoutUnit);
}

bool FlexFormattingContext::isMainSizeFlexible(FrameBox* flexItem,
                                               bool usingGrowFactor)
{
    if (usingGrowFactor) {
        if (flexItem->style()->flexGrow() == 0) {
            return false;
        } else {
            if (m_isMainAxisInInlineAxis) {
                if (basisSize(flexItem) > flexItem->contentWidth()) {
                    return false;
                }
            } else {
                if (basisSize(flexItem) > flexItem->contentHeight()) {
                    return false;
                }
            }
        }
    } else {
        if (flexItem->style()->flexShrink() == 0) {
            return false;
        } else {
            if (m_isMainAxisInInlineAxis) {
                if (basisSize(flexItem) < flexItem->contentWidth()) {
                    return false;
                }
            } else {
                if (basisSize(flexItem) < flexItem->contentHeight()) {
                    return false;
                }
            }
        }
    }

    return true;
}

LayoutUnit FlexFormattingContext::sumOfUsedupMainSize(
    std::vector<FrameBox*>& flexItems, std::vector<bool> isFrozens)
{
    LayoutUnit usedupMainSize;
    for (size_t i = 0; i < flexItems.size(); i++) {
        FrameBox* flexItem = flexItems[i];
        if (isFrozens[i]) {
            if (m_isMainAxisInInlineAxis) {
                usedupMainSize += flexItem->outerWidth();
            } else {
                usedupMainSize += flexItem->outerHeight();
            }
        } else {
            if (m_isMainAxisInInlineAxis) {
                usedupMainSize += basisSize(flexItem) + flexItem->mbpWidth();
            } else {
                usedupMainSize += basisSize(flexItem) + flexItem->mbpHeight();
            }
        }
    }

    return usedupMainSize;
}

void FlexFormattingContext::applyFlexFactor()
{
    if (m_container->lineClamp()) {
        return;
    }

    size_t lines = m_currentLineIdx + 1;
    enum Violations {
        None,
        Min,
        Max,
    };

    for (size_t i = 0; i < lines; i++) {
        FlexLine& flexLine = m_flexLines[i];
        std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
        std::vector<bool> isFrozens;
        std::vector<Violations> violations;
        LayoutUnit lineWidth = flexLine.m_lineWidth;
        if (lineWidth == m_availableMainSize) {
            continue;
        }

        bool usingGrowFactor = lineWidth < m_availableMainSize;
        bool isAllFrozen = true;
        isFrozens.resize(flexItems.size());
        violations.resize((flexItems.size()));

        for (size_t j = 0; j < flexItems.size(); j++) {
            FrameBox* flexItem = flexItems[j];

            if (!isMainSizeFlexible(flexItem, usingGrowFactor)) {
                isFrozens[j] = true;
            }
            isAllFrozen &= isFrozens[j];
        }

        LayoutUnit initialFreeSpace =
            m_availableMainSize - sumOfUsedupMainSize(flexItems, isFrozens);
        initialFreeSpace -= flexLine.m_sumOfMainGapInComputeMainSize;

        LayoutUnit remainingFreeSpace = initialFreeSpace;
        if (remainingFreeSpace < 0) {
            bool hasNonOverflowVisibleItem = false;
            bool everyItemHaveNonLengthFlexBasis = true;
            for (size_t j = 0; j < flexItems.size(); j++) {
                OverflowValue of;
                if (m_isMainAxisInInlineAxis) {
                    of = flexItems[j]->appliedOverflowX();
                } else {
                    of = flexItems[j]->appliedOverflowY();
                }
                if (of != OverflowValue::VisibleOverflow) {
                    hasNonOverflowVisibleItem = true;
                }
                everyItemHaveNonLengthFlexBasis &=
                    !flexItems[j]->style()->flexBasis().isWidth();
            }
            if (hasNonOverflowVisibleItem && everyItemHaveNonLengthFlexBasis) {
                for (size_t j = 0; j < flexItems.size(); j++) {
                    OverflowValue of;
                    if (m_isMainAxisInInlineAxis) {
                        of = flexItems[j]->appliedOverflowX();
                    } else {
                        of = flexItems[j]->appliedOverflowY();
                    }
                    if (of == OverflowValue::VisibleOverflow) {
                        isFrozens[j] = true;
                        isAllFrozen &= isFrozens[j];
                    }
                }
            }
        }

        while (!isAllFrozen) {
            LayoutUnit unclampedSize;
            LayoutUnit clampedSize;
            float sumOfFactor = 0;
            LayoutUnit scaledFlexShrinkFactor;

            for (size_t j = 0; j < flexItems.size(); j++) {
                FrameBox* flexItem = flexItems[j];
                if (isFrozens[j]) {
                    continue;
                }

                if (usingGrowFactor) {
                    sumOfFactor += flexItem->style()->flexGrow();
                } else {
                    sumOfFactor += flexItem->style()->flexShrink();
                    scaledFlexShrinkFactor +=
                        flexItem->style()->flexShrink() * basisSize(flexItem);
                }
            }

            if (sumOfFactor < 1) {
                if (remainingFreeSpace > 0) {
                    remainingFreeSpace =
                        std::min(LayoutUnit(initialFreeSpace * sumOfFactor),
                                 remainingFreeSpace);
                } else {
                    remainingFreeSpace =
                        std::max(LayoutUnit(initialFreeSpace * sumOfFactor),
                                 remainingFreeSpace);
                }
            }

            if (remainingFreeSpace != 0) {
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* flexItem = flexItems[j];
                    if (isFrozens[j]) {
                        continue;
                    }

                    LayoutUnit targetMainSize, mainSize;
                    LayoutUnit flexItemBasisSize = basisSize(flexItem);
                    if (usingGrowFactor) {
                        float factor = flexItem->style()->flexGrow();
                        if (factor != 0) {
                            targetMainSize =
                                flexItemBasisSize +
                                (factor / sumOfFactor) * remainingFreeSpace;
                        }
                    } else {
                        float factor = flexItem->style()->flexShrink();
                        if (factor != 0) {
                            if (scaledFlexShrinkFactor != 0) {
                                targetMainSize = flexItemBasisSize +
                                                 (factor * flexItemBasisSize /
                                                  scaledFlexShrinkFactor) *
                                                     remainingFreeSpace;
                            } else {
                                targetMainSize =
                                    flexItemBasisSize +
                                    (factor / sumOfFactor) * remainingFreeSpace;
                            }
                        }
                    }

                    targetMainSize = std::ceil(targetMainSize.toDouble());

                    unclampedSize += targetMainSize;

                    if (m_isMainAxisInInlineAxis) {
                        flexItem->setContentWidthConsideringMinMaxWidths(
                            m_layoutContext, targetMainSize,
                            m_availableMainSize);
                        mainSize = flexItem->contentWidth();
                    } else {
                        flexItem->setContentHeightConsideringMinMaxHeights(
                            m_layoutContext, targetMainSize,
                            m_availableMainSize);
                        mainSize = flexItem->contentHeight();
                    }

                    if (mainSize != flexItemBasisSize) {
                        clearBasisSizeFromCache(flexItem);
                    }

                    clampedSize += mainSize;

                    if (targetMainSize > mainSize) {
                        violations[j] = Max;
                    } else if (targetMainSize < mainSize) {
                        violations[j] = Min;
                    } else {
                        violations[j] = None;
                    }
                }
            }

            isAllFrozen = true;
            if (clampedSize != unclampedSize) {
                bool shouldMinFreeze = clampedSize > unclampedSize;
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* flexItem = flexItems[j];
                    if (isFrozens[j]) {
                        continue;
                    }

                    if ((shouldMinFreeze && violations[j] == Min) ||
                        (!shouldMinFreeze && violations[j] == Max)) {
                        isFrozens[j] = true;
                    } else {
                        isAllFrozen = false;
                    }
                }
            }

            if (!isAllFrozen) {
                sumOfFactor = 0;
                scaledFlexShrinkFactor = 0;
                remainingFreeSpace = m_availableMainSize -
                                     sumOfUsedupMainSize(flexItems, isFrozens);
                remainingFreeSpace -= flexLine.m_sumOfMainGapInComputeMainSize;
            }
        }
    }
}

void FlexFormattingContext::resolveMainMargin()
{
    size_t lines = m_currentLineIdx + 1;

    for (size_t i = 0; i < lines; i++) {
        FlexLine& flexLine = m_flexLines[i];
        std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
        LayoutUnit sumOfMainSize;
        size_t autoMarginCnt = 0;

        for (size_t j = 0; j < flexItems.size(); j++) {
            FrameBox* flexItem = flexItems[j];
            if (m_isMainAxisInInlineAxis) {
                LengthData margin = flexItem->style()->margin();
                if (sumOfMainSize <= m_availableMainSize &&
                    margin.left().isAuto()) {
                    autoMarginCnt++;
                }
                sumOfMainSize += flexItem->outerWidth();
                if (sumOfMainSize <= m_availableMainSize &&
                    margin.right().isAuto()) {
                    autoMarginCnt++;
                }
            } else {
                LengthData margin = flexItem->style()->margin();
                if (sumOfMainSize <= m_availableMainSize &&
                    margin.top().isAuto()) {
                    autoMarginCnt++;
                }
                sumOfMainSize += flexItem->outerHeight();
                if (sumOfMainSize <= m_availableMainSize &&
                    margin.bottom().isAuto()) {
                    autoMarginCnt++;
                }
            }
        }

        bool canResolve =
            m_availableMainSize >
            sumOfMainSize + flexLine.m_sumOfMainGapInComputeMainSize;

        if (canResolve && autoMarginCnt > 0) {
            LayoutUnit margin = (m_availableMainSize - sumOfMainSize -
                                 flexLine.m_sumOfMainGapInComputeMainSize) /
                                autoMarginCnt;

            for (size_t j = 0; j < flexItems.size(); j++) {
                FrameBox* flexItem = flexItems[j];
                if (m_isMainAxisInInlineAxis) {
                    LengthData marginL = flexItem->style()->margin();
                    if (marginL.left().isAuto()) {
                        flexItem->setMarginLeft(margin);
                        sumOfMainSize += margin;
                        autoMarginCnt--;
                    }

                    if (autoMarginCnt > 0 && marginL.right().isAuto()) {
                        flexItem->setMarginRight(margin);
                        sumOfMainSize += margin;
                        autoMarginCnt--;
                    }
                } else {
                    LengthData marginL = flexItem->style()->margin();
                    if (marginL.top().isAuto()) {
                        flexItem->setMarginTop(margin);
                        sumOfMainSize += margin;
                        autoMarginCnt--;
                    }

                    if (autoMarginCnt > 0 && marginL.bottom().isAuto()) {
                        flexItem->setMarginBottom(margin);
                        sumOfMainSize += margin;
                        autoMarginCnt--;
                    }
                }

                if (autoMarginCnt == 0) {
                    break;
                }
            }
        }
    }
}

void FlexFormattingContext::applyJustifyContent()
{
    size_t lines = m_currentLineIdx + 1;

    for (size_t i = 0; i < lines; i++) {
        FlexLine& flexLine = m_flexLines[i];
        std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
        LayoutUnit sumOfMainSize;

        for (size_t j = 0; j < flexItems.size(); j++) {
            FrameBox* flexItem = flexItems[j];
            if (m_isMainAxisInInlineAxis) {
                sumOfMainSize += flexItem->outerWidth();
            } else {
                sumOfMainSize += flexItem->outerHeight();
            }
        }

        sumOfMainSize += flexLine.m_sumOfMainGapInComputeMainSize;

        LayoutUnit offset;
        LayoutUnit separator;
        JustifyContentValue justifyContent =
            m_container->style()->justifyContent();
        switch (justifyContent) {
        case JustifyContentValue::NormalJustifyContentValue:
        case JustifyContentValue::StartJustifyContentValue:
        case JustifyContentValue::FlexStartJustifyContentValue:
        case JustifyContentValue::StretchJustifyContentValue:
            break;
        case JustifyContentValue::EndJustifyContentValue:
        case JustifyContentValue::FlexEndJustifyContentValue:
            offset = m_availableMainSize - sumOfMainSize;
            break;
        case JustifyContentValue::CenterJustifyContentValue:
            offset = (m_availableMainSize - sumOfMainSize) / 2;
            break;
        case JustifyContentValue::SpaceAroundJustifyContentValue:
            if (m_availableMainSize > sumOfMainSize) {
                offset = (m_availableMainSize - sumOfMainSize) /
                         (flexItems.size() * 2);
                separator = 2 * offset;
            } else {
                offset = (m_availableMainSize - sumOfMainSize) / 2;
            }
            break;
        case JustifyContentValue::SpaceBetweenJustifyContentValue:
            if (flexItems.size() > 1 && m_availableMainSize > sumOfMainSize) {
                separator = (m_availableMainSize - sumOfMainSize) /
                            (flexItems.size() - 1);
            }
            break;
        }

        if (m_isMainAxisInInlineAxis) {
            if (m_isLtrDirection) {
                LayoutUnit x = offset + m_container->borderLeft() +
                               m_container->paddingLeft();
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* flexItem = flexItems[j];
                    flexItem->setX(x + flexItem->marginLeft());
                    x += flexItem->outerWidth() + separator + m_mainGap;
                }
            } else {
                LayoutUnit x = m_availableMainSize - offset +
                               m_container->borderLeft() +
                               m_container->paddingLeft();
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* flexItem = flexItems[j];
                    flexItem->setX(x - flexItem->outerWidth() +
                                   flexItem->marginLeft());
                    x -= flexItem->outerWidth() + separator + m_mainGap;
                }
            }
        } else {
            if (m_isTtbDirection) {
                LayoutUnit y = offset + m_container->borderTop() +
                               m_container->paddingTop();
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* flexItem = flexItems[j];
                    flexItem->setY(y + flexItem->marginTop());
                    y += flexItem->outerHeight() + separator + m_mainGap;
                }
            } else {
                LayoutUnit y = m_availableMainSize - offset +
                               m_container->borderTop() +
                               m_container->paddingTop();
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* flexItem = flexItems[j];
                    flexItem->setY(y - flexItem->outerHeight() +
                                   flexItem->marginTop());
                    y -= flexItem->outerHeight() + separator + m_mainGap;
                }
            }
        }
    }
}

void FlexFormattingContext::layoutMain()
{
    computeMainSize();
    applyFlexFactor();
    resolveMainMargin();
    applyJustifyContent();
}

bool FlexFormattingContext::doesParticipateInFlexFormattingContext(
    Frame* flexItem)
{
    if (flexItem->isFrameBlockBox() && flexItem->isAnonymous()) {
        FrameBlockBox* blockBox = flexItem->asFrameBlockBox();
        Frame* c = blockBox->firstChild();
        while (c) {
            if (!c->isAbsolutePositioned() &&
                !(c->isFrameText() &&
                  c->asFrameText()->text()->containsOnlyWhitespace())) {
                return true;
            }

            c = c->next();
        }

        return false;
    }

    return true;
}

Optional<LayoutUnit> FlexFormattingContext::firstLineBoxYPosition(
    FrameBox* flexItem) const
{
    auto it = m_firstLineBoxYPositions.find(flexItem);
    if (it == m_firstLineBoxYPositions.end()) {
        return Optional<LayoutUnit>();
    }

    return it->second;
}

struct MainSizeFixer {
    MainSizeFixer(FrameBox* flexItem, bool isMainAxisInInlineAxis)
        : m_flexItem(flexItem)
        , m_isMainAxisInInlineAxis(isMainAxisInInlineAxis)
    {
    }

    ~MainSizeFixer()
    {
    }

    void fix()
    {
        if (m_isMainAxisInInlineAxis) {
            FrameBox* cb = containingBlock(m_flexItem);
            m_maxHeightBackup = m_flexItem->style()->maxHeight();
            if (cb->style()->flexWrap() == FlexWrapValue::NoWrapFlexWrapValue) {
                Length maxHeight = cb->style()->maxHeight();
                if (maxHeight.isFixed()) {
                    if (m_maxHeightBackup.isAuto() ||
                        (m_maxHeightBackup.isFixed() &&
                         maxHeight.fixed() < m_maxHeightBackup.fixed())) {
                        m_flexItem->style()->setMaxHeight(
                            cb->style()->maxHeight());
                    }
                }
            }

            m_mainSizeBackup = m_flexItem->style()->width();
            m_mainSizeLayoutBackup = m_flexItem->width();
            m_flexItem->style()->setWidth(
                Length(Length::Fixed, m_mainSizeLayoutBackup));
        } else {
            m_mainSizeBackup = m_flexItem->style()->height();
            m_mainSizeLayoutBackup = m_flexItem->height();
            m_flexItem->style()->setHeight(
                Length(Length::Fixed, m_mainSizeLayoutBackup));
        }
    }

    void restore()
    {
        if (m_isMainAxisInInlineAxis) {
            m_flexItem->style()->setWidth(m_mainSizeBackup);
            m_flexItem->setWidth(m_mainSizeLayoutBackup);
            m_flexItem->style()->setMaxHeight(m_maxHeightBackup);
        } else {
            m_flexItem->style()->setHeight(m_mainSizeBackup);
            m_flexItem->setHeight(m_mainSizeLayoutBackup);
        }
    }

    FrameBox* m_flexItem;
    Length m_mainSizeBackup;
    LayoutUnit m_mainSizeLayoutBackup;
    Length m_maxHeightBackup;
    bool m_isMainAxisInInlineAxis;
};

struct CrossSizeFixer {
    CrossSizeFixer(FrameBox* flexItem, bool isMainAxisInInlineAxis)
        : m_flexItem(flexItem)
        , m_isMainAxisInInlineAxis(isMainAxisInInlineAxis)

    {
    }
    ~CrossSizeFixer()
    {
    }

    void fix(LayoutUnit crossSize)
    {
        if (m_isMainAxisInInlineAxis) {
            m_flexItem->style()->setHeight(
                Length(Length::Fixed, crossSize.toInt()));
        } else {
            m_flexItem->style()->setWidth(
                Length(Length::Fixed, crossSize.toInt()));
        }
    }

    void restoreToAuto()
    {
        if (m_isMainAxisInInlineAxis) {
            m_flexItem->style()->setHeight(Length());
        } else {
            m_flexItem->style()->setWidth(Length());
        }
    }

    FrameBox* m_flexItem;
    bool m_isMainAxisInInlineAxis;
};

void FlexFormattingContext::layoutFlexItem(
    FrameBox* flexItem, Frame::LayoutWantToResolve resolveWhat,
    Optional<LayoutUnit> crossSize)
{
    STARFISH_ASSERT(flexItem != nullptr);
    flexItem->markNeedsLayout();

    MainSizeFixer mainSizeFixer(flexItem, m_isMainAxisInInlineAxis);
    mainSizeFixer.fix();
    CrossSizeFixer crossSizeFixer(flexItem, m_isMainAxisInInlineAxis);
    if (crossSize.hasValue()) {
        crossSizeFixer.fix(crossSize.value());
    }
    m_layoutContext.registerModifiedStyleFlexItem(flexItem);

    if ((resolveWhat & Frame::ResolveWidth) != 0) {
        flexItem->layout(m_layoutContext, Frame::ResolveWidth);
    }

    if ((resolveWhat & Frame::ResolveHeight) != 0) {
        flexItem->markContentWidthDamaged();
        flexItem->layout(m_layoutContext, Frame::ResolveHeight);
    }

    m_layoutContext.unregisterModifiedStyleFlexItem(flexItem);
    if (crossSize.hasValue()) {
        crossSizeFixer.restoreToAuto();
    }
    mainSizeFixer.restore();

    if ((resolveWhat & Frame::ResolveHeight) != 0) {
        if (flexItem->isFrameBlockBox()) {
            m_layoutContext.layoutRegisteredAbsolutePositionedBoxes(
                flexItem->asFrameBlockBox());
        }
        flexItem->clearContentWidthDamaged();
    }
}

void FlexFormattingContext::computeCrossSize()
{
    size_t lines = m_currentLineIdx + 1;
    LayoutUnit sumOfCrossSize;
    std::vector<std::pair<size_t, FrameBox*>> flexItemsToStretchInfos;
    std::vector<LayoutUnit> lineCrossSizes;
    for (size_t i = 0; i < lines; i++) {
        FlexLine& flexLine = m_flexLines[i];
        std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
        LayoutUnit maxAscender = 0;
        LayoutUnit maxHypotheticalCrossSize = 0;

        for (size_t j = 0; j < flexItems.size(); j++) {
            FrameBox* flexItem = flexItems[j];
            bool shouldAlignAtFirstBaseline = false;
            LengthData margin = flexItem->style()->margin();

            if (flexItem->isFrameBlockBox() &&
                flexItem->style()->alignSelf() == BaselineAlignItemValue &&
                m_isMainAxisInInlineAxis && !margin.top().isAuto() &&
                !margin.bottom().isAuto()) {
                shouldAlignAtFirstBaseline = true;
            }

            if (shouldAlignAtFirstBaseline) {
                m_layoutContext.pushBlockBoxAligningAtFirstBaseline(
                    flexItem->asFrameBlockBox());
            }

            bool isStretchFlexItem = false;
            if (flexItem->style()->alignSelf() == StretchAlignItemValue) {
                if ((m_isMainAxisInInlineAxis &&
                     flexItem->style()->height().isAuto() &&
                     !margin.top().isAuto() && !margin.bottom().isAuto()) ||
                    (!m_isMainAxisInInlineAxis &&
                     flexItem->style()->width().isAuto() &&
                     !margin.left().isAuto() && !margin.right().isAuto())) {
                    flexItemsToStretchInfos.emplace_back(i, flexItem);
                    isStretchFlexItem = true;
                }
            }

            flexItem->markNeedsLayout();
            if (m_isMainAxisInInlineAxis) {
                auto resolveWhat = Frame::LayoutWantToResolve::ResolveHeight;
                if (flexItem->isFrameReplaced()) {
                    // height of FrameReplaced is computed at ResolveWidth
                    resolveWhat = Frame::LayoutWantToResolve::ResolveAll;
                }
                layoutFlexItem(flexItem, resolveWhat);
            } else {
                auto resolveWhat = Frame::LayoutWantToResolve::ResolveWidth;
                if (!isStretchFlexItem) {
                    resolveWhat = Frame::LayoutWantToResolve::ResolveAll;
                }
                layoutFlexItem(flexItem, resolveWhat);
            }

            if (shouldAlignAtFirstBaseline) {
                auto it = m_layoutContext.firstLineAscender(
                    flexItem->asFrameBlockBox());
                if (it.hasValue()) {
                    LineBox* flb = it.getValue().first;
                    LayoutUnit ascender = flb->absolutePoint(flexItem).y() +
                                          it.getValue().second +
                                          flexItem->marginTop();
                    m_firstLineBoxYPositions[flexItem] = ascender;
                    maxAscender = std::max(maxAscender, ascender);
                }
                m_layoutContext.popBlockBoxAligningAtFirstBaseline();
            }

            if (m_isMainAxisInInlineAxis) {
                maxHypotheticalCrossSize =
                    std::max(maxHypotheticalCrossSize, flexItem->outerHeight());
            } else {
                maxHypotheticalCrossSize =
                    std::max(maxHypotheticalCrossSize, flexItem->outerWidth());
            }
        }

        flexLine.m_lineHeight = maxHypotheticalCrossSize;
        flexLine.m_maxAscender = maxAscender;
        sumOfCrossSize += maxHypotheticalCrossSize;
    }

    if (lines > 1) {
        sumOfCrossSize += m_crossGap * (lines - 1);
    }

    if (m_container->style()->alignContent() == StretchAlignContentValue &&
        m_availableCrossSize != intMaxForLayoutUnit &&
        sumOfCrossSize < m_availableCrossSize && lines > 0) {
        LayoutUnit amountToStretchByLine =
            (m_availableCrossSize - sumOfCrossSize) / lines;

        for (size_t i = 0; i < lines; i++) {
            FlexLine& flexLine = m_flexLines[i];
            flexLine.m_lineHeight += amountToStretchByLine;
        }
        sumOfCrossSize = m_availableCrossSize;
    }

    if (m_isMainAxisInInlineAxis) {
        m_container->computeContentHeight(m_layoutContext, sumOfCrossSize);
        if (m_availableCrossSize == intMaxForLayoutUnit) {
            m_availableCrossSize = m_container->contentHeight();
        }
    }
    STARFISH_ASSERT(m_availableCrossSize != intMaxForLayoutUnit);

    if (lines == 1) {
        for (size_t i = 0; i < lines; i++) {
            FlexLine& flexLine = m_flexLines[i];
            if (m_isMainAxisInInlineAxis) {
                flexLine.m_lineHeight = m_container->contentHeight();
            } else {
                flexLine.m_lineHeight = m_container->contentWidth();
            }
        }
    }

    for (size_t i = 0; i < flexItemsToStretchInfos.size(); i++) {
        auto& flexItemsToStretchInfo = flexItemsToStretchInfos[i];
        size_t lineIdx = flexItemsToStretchInfo.first;
        FrameBox* flexItem = flexItemsToStretchInfo.second;
        // Invalid content height cache.
        m_layoutContext.registerContentHeight(flexItem, intMaxForLayoutUnit);
        FlexLine& flexLine = m_flexLines[lineIdx];
        ComputedStyle* style = flexItem->style();
        if (m_isMainAxisInInlineAxis) {
            bool shouldResizeFlexItem = false;
            LayoutUnit newCrossSize;
            if (style->boxSizing() ==
                BoxSizingValue::ContentBoxBoxSizingValue) {
                newCrossSize = flexLine.m_lineHeight - flexItem->mbpHeight();
                shouldResizeFlexItem =
                    flexItem->contentHeight() != newCrossSize;
            } else {
                newCrossSize = flexLine.m_lineHeight - flexItem->marginHeight();
                shouldResizeFlexItem = flexItem->height() != newCrossSize;
            }

            if (shouldResizeFlexItem) {
                layoutFlexItem(flexItem,
                               Frame::LayoutWantToResolve::ResolveHeight,
                               newCrossSize);
            }
        } else {
            bool shouldResizeFlexItem = false;
            LayoutUnit newCrossSize;
            if (style->boxSizing() ==
                BoxSizingValue::ContentBoxBoxSizingValue) {
                newCrossSize = flexLine.m_lineHeight - flexItem->mbpWidth();
                shouldResizeFlexItem = flexItem->contentWidth() != newCrossSize;
            } else {
                newCrossSize = flexLine.m_lineHeight - flexItem->marginWidth();
                shouldResizeFlexItem = flexItem->width() != newCrossSize;
            }

            if (shouldResizeFlexItem) {
                layoutFlexItem(flexItem, Frame::LayoutWantToResolve::ResolveAll,
                               newCrossSize);
            } else {
                layoutFlexItem(flexItem,
                               Frame::LayoutWantToResolve::ResolveAll);
            }
        }
    }
}

void FlexFormattingContext::resolveCrossMargin()
{
    size_t lines = m_currentLineIdx + 1;

    for (size_t i = 0; i < lines; i++) {
        FlexLine& flexLine = m_flexLines[i];
        std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;

        for (size_t j = 0; j < flexItems.size(); j++) {
            LayoutUnit lineCrossSize = flexLine.m_lineHeight;
            FrameBox* item = flexItems[j];

            LengthData marginL = item->style()->margin();
            if (m_isMainAxisInInlineAxis) {
                if (lineCrossSize > item->outerHeight()) {
                    LayoutUnit margin = lineCrossSize - item->outerHeight();
                    if (marginL.top().isAuto() && marginL.bottom().isAuto()) {
                        item->setMarginTop(margin / 2);
                        item->setMarginBottom(margin / 2);
                    } else if (marginL.top().isAuto()) {
                        item->setMarginTop(margin);
                    } else if (marginL.bottom().isAuto()) {
                        item->setMarginBottom(margin);
                    }
                }
            } else {
                if (lineCrossSize > item->outerWidth()) {
                    LayoutUnit margin = lineCrossSize - item->outerWidth();
                    if (marginL.left().isAuto() && marginL.right().isAuto()) {
                        item->setMarginLeft(margin / 2);
                        item->setMarginRight(margin / 2);
                    } else if (marginL.left().isAuto()) {
                        item->setMarginLeft(margin);
                    } else if (marginL.right().isAuto()) {
                        item->setMarginRight(margin);
                    }
                }
            }
        }
    }
}

void FlexFormattingContext::applyAlignSelf()
{
    size_t lines = m_currentLineIdx + 1;
    for (size_t i = 0; i < lines; i++) {
        FlexLine& flexLine = m_flexLines[i];
        std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
        LayoutUnit crossSize = flexLine.m_lineHeight;

        for (size_t j = 0; j < flexItems.size(); j++) {
            FrameBox* flexItem = flexItems[j];

            LayoutUnit offset;
            switch (flexItem->style()->alignSelf()) {
            case FlexEndAlignItemValue:
                if (m_isMainAxisInInlineAxis) {
                    offset = crossSize - flexItem->outerHeight();
                } else {
                    offset = crossSize - flexItem->outerWidth();
                }
                break;
            case CenterAlignItemValue:
                if (m_isMainAxisInInlineAxis) {
                    offset = (crossSize - flexItem->outerHeight()) / 2;
                } else {
                    offset = (crossSize - flexItem->outerWidth()) / 2;
                }
                break;
            case BaselineAlignItemValue:
                if (m_isMainAxisInInlineAxis) {
                    auto it = firstLineBoxYPosition(flexItem);
                    if (it.hasValue()) {
                        offset = flexLine.m_maxAscender - it.getValue();
                    } else {
                        offset = flexLine.m_maxAscender - flexItem->height();
                    }
                } else {
                    offset = 0;
                }
                break;
            default:
                offset = 0;
                break;
            }

            if (m_isMainAxisInInlineAxis) {
                if (m_isTtbDirection) {
                    flexItem->setY(offset);
                } else {
                    flexItem->setY(crossSize - offset -
                                   flexItem->outerHeight());
                }

                flexItem->moveY(flexItem->marginTop());
            } else {
                if (m_isLtrDirection) {
                    flexItem->setX(offset);
                } else {
                    flexItem->setX(crossSize - offset - flexItem->outerWidth());
                }

                flexItem->moveX(flexItem->marginLeft());
            }
        }
    }
}

void FlexFormattingContext::applyAlignContent()
{
    size_t lines = m_currentLineIdx + 1;
    LayoutUnit offset;
    LayoutUnit separator;
    AlignContentValue alignContent = m_container->style()->alignContent();
    LayoutUnit sumOfCrossSize;

    for (size_t i = 0; i < lines; i++) {
        FlexLine& line = m_flexLines[i];
        sumOfCrossSize += line.m_lineHeight;
    }

    if (lines > 1) {
        sumOfCrossSize += m_crossGap * (lines - 1);
    }

    switch (alignContent) {
    case FlexStartAlignContentValue:
        offset = 0;
        break;
    case FlexEndAlignContentValue:
        offset = m_availableCrossSize - sumOfCrossSize;
        break;
    case CenterAlignContentValue:
        offset = (m_availableCrossSize - sumOfCrossSize) / 2;
        break;
    case SpaceAroundAlignContentValue:
        if (m_availableCrossSize > sumOfCrossSize) {
            offset = (m_availableCrossSize - sumOfCrossSize) / (lines * 2);
            separator = 2 * offset;
        } else {
            offset = (m_availableCrossSize - sumOfCrossSize) / 2;
        }
        break;
    case SpaceBetweenAlignContentValue:
        if (lines > 1 && m_availableCrossSize > sumOfCrossSize) {
            separator = (m_availableCrossSize - sumOfCrossSize) / (lines - 1);
        }
        break;
    case StretchAlignContentValue:
        offset = 0;
        break;
    }

    if (m_isMainAxisInInlineAxis) {
        if (m_isTtbDirection) {
            LayoutUnit y =
                offset + m_container->borderTop() + m_container->paddingTop();
            for (size_t i = 0; i < lines; i++) {
                FlexLine& flexLine = m_flexLines[i];
                std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* item = flexItems[j];
                    item->moveY(y);
                }
                y += flexLine.m_lineHeight + separator + m_crossGap;
            }
        } else {
            LayoutUnit y = m_availableCrossSize - offset +
                           m_container->borderTop() + m_container->paddingTop();
            for (size_t i = 0; i < lines; i++) {
                FlexLine& flexLine = m_flexLines[i];
                std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* item = flexItems[j];
                    item->moveY(y - flexLine.m_lineHeight);
                }
                y -= flexLine.m_lineHeight + separator + m_crossGap;
            }
        }
    } else {
        if (m_isLtrDirection) {
            LayoutUnit x =
                offset + m_container->paddingLeft() + m_container->borderLeft();
            for (size_t i = 0; i < lines; i++) {
                FlexLine& flexLine = m_flexLines[i];
                std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* item = flexItems[j];
                    item->moveX(x);
                }
                x += flexLine.m_lineHeight + separator + m_crossGap;
            }
        } else {
            LayoutUnit x = m_availableCrossSize - offset +
                           m_container->paddingLeft() +
                           m_container->borderLeft();
            for (size_t i = 0; i < lines; i++) {
                FlexLine& flexLine = m_flexLines[i];
                std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* item = flexItems[j];
                    item->moveX(x - flexLine.m_lineHeight);
                }
                x -= flexLine.m_lineHeight + separator + m_crossGap;
            }
        }
    }
}

void FlexFormattingContext::layoutCross()
{
    computeCrossSize();
    resolveCrossMargin();
    applyAlignSelf();
    applyAlignContent();
}

FrameFlexibleBox::FrameFlexibleBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

bool FrameFlexibleBox::shouldLayout(LayoutContext& ctx,
                                    LayoutWantToResolve resolveWhat,
                                    FrameBox* containingBox)
{
    if (FrameBlockBox::shouldLayout(ctx, resolveWhat, containingBox)) {
        return true;
    }

    Frame* child = firstChild();
    while (child) {
        if (child->isFlexItem()) {
            if (child->needsLayout()) {
                return true;
            }
        }
        child = child->next();
    }

    return false;
}

struct MinMaxWidthHeightRestorer {
    MinMaxWidthHeightRestorer(FrameBox* b, bool isMainAxisInInlineAxis)
        : m_box(b)
        , m_isMainAxisInInlineAxis(isMainAxisInInlineAxis)
    {
        ComputedStyle* style = b->style();
        if (m_isMainAxisInInlineAxis) {
            m_minWidth = style->minWidth();
            m_maxWidth = style->maxWidth();
        } else {
            m_minHeight = style->minHeight();
            m_maxHeight = style->maxHeight();
        }
    }

    void initValues()
    {
        ComputedStyle* style = m_box->style();
        if (m_isMainAxisInInlineAxis) {
            style->setMinWidth(Length(Length::Fixed, 0));
            style->setMaxWidth(Length());
        } else {
            style->setMinHeight(Length(Length::Fixed, 0));
            style->setMaxHeight(Length());
        }
    }

    ~MinMaxWidthHeightRestorer()
    {
        ComputedStyle* style = m_box->style();
        if (m_isMainAxisInInlineAxis) {
            style->setMinWidth(m_minWidth);
            style->setMaxWidth(m_maxWidth);
        } else {
            style->setMinHeight(m_minHeight);
            style->setMaxHeight(m_maxHeight);
        }
    }

    FrameBox* m_box;
    Length m_minWidth;
    Length m_maxWidth;
    Length m_minHeight;
    Length m_maxHeight;
    bool m_isMainAxisInInlineAxis;
};

static void computeBorderMarginPaddingWithinFlexContext(
    LayoutContext& ctx, FrameBox* flexItem, LayoutUnit availableMainSize,
    LayoutUnit availableCrossSize, bool isMainAxisInInlineAxis)
{
    if (isMainAxisInInlineAxis) {
        flexItem->computeBorderMarginPadding(ctx, availableMainSize);
    } else {
        flexItem->computeBorderMarginPadding(ctx, availableCrossSize);
    }
}

std::pair<LayoutUnit, bool> FrameFlexibleBox::basisSize(
    LayoutContext& ctx, LayoutUnit availableMainSize,
    LayoutUnit availableCrossSize, FrameBox* flexItem,
    bool shouldRespectPercentageWidthOnComputingBasisSize)
{
    LayoutContextComputingBasisSizeStateMaker marker(ctx, true);

    bool isMainAxisInInlineAxis = this->isMainAxisInInlineAxis();
    LayoutUnit basisSize = intMaxForLayoutUnit;
    FlexBasisData flexBasis = flexItem->style()->flexBasis();
    MinMaxWidthHeightRestorer restorer(flexItem, isMainAxisInInlineAxis);
    restorer.initValues();
    MBPRestorer restorer2(flexItem);
    bool applyLineClamp = shouldApplyLineClamp(flexItem);

    // A. If the item has a definite used flex basis, that’s the flex
    // base size.
    if (!applyLineClamp && flexBasis.isWidth()) {
        Length basisWidth = flexBasis.width();
        if (basisWidth.isDefinite(availableMainSize != intMaxForLayoutUnit)) {
            computeBorderMarginPaddingWithinFlexContext(
                ctx, flexItem, availableMainSize, availableCrossSize,
                isMainAxisInInlineAxis);

            basisSize = basisWidth.specifiedValue(availableMainSize, this);
            if (isMainAxisInInlineAxis) {
                basisSize =
                    flexItem->contentWidthAfterApplyingBoxSizing(basisSize);
            } else {
                basisSize =
                    flexItem->contentHeightAfterApplyingBoxSizing(basisSize);
            }
            return std::make_pair(basisSize, false);
        }
    } else if (!applyLineClamp && flexBasis.isContent()) {
        if (flexItem->isFrameReplaced()) {
            // B. If the flex item has an intrinsic aspect ratio, a used
            // flex basis of 'content', and a definite cross size.
            computeBorderMarginPaddingWithinFlexContext(
                ctx, flexItem, availableMainSize, availableCrossSize,
                isMainAxisInInlineAxis);

            LayoutUnit intrinsicWidth, intrinsicHeight;
            LayoutUnit parentContentWidth;
            bool hasAspectRatio;
            Length parentHeightLength;

            if (isMainAxisInInlineAxis &&
                availableCrossSize != intMaxForLayoutUnit) {
                parentHeightLength = Length(Length::Fixed, availableCrossSize);
            } else if (!isMainAxisInInlineAxis &&
                       availableMainSize != intMaxForLayoutUnit) {
                parentHeightLength = Length(Length::Fixed, availableMainSize);
            } else {
                parentHeightLength = Length(Length::Auto);
            }

            flexItem->asFrameReplaced()->computeIntrinsicSize(
                ctx, intrinsicWidth, intrinsicHeight, hasAspectRatio,
                parentContentWidth, parentHeightLength);

            if (availableCrossSize != intMaxForLayoutUnit && hasAspectRatio) {
                if (isMainAxisInInlineAxis) {
                    basisSize =
                        availableCrossSize * (intrinsicWidth / intrinsicHeight);
                } else {
                    basisSize =
                        availableCrossSize * (intrinsicHeight / intrinsicWidth);
                }
            }
            return std::make_pair(basisSize, false);
        }
    }

    // E. Otherwise, size the item into the available space using its used flex
    // basis in place of its main size, treating a value of content as
    // max-content. If a cross size is needed to determine the main size (e.g.
    // when the flex item’s main size is in its block axis) and the flex item’s
    // cross size is auto and not definite, in this calculation use fit-content
    // as the flex item’s cross size.

    FrameBox* containingBlockOfFlexItem = containingBlock(flexItem);
    LayoutUnit oldContainingBlockWidth =
        containingBlockOfFlexItem->contentWidth();
    bool containingBlockOfFlexItemContentWidthDamaged =
        containingBlockOfFlexItem->contentWidthDamaged();
    bool seenPercentageBasisSize = false;

    if (!applyLineClamp && isMainAxisInInlineAxis) {
        containingBlockOfFlexItem->setContentWidth(availableMainSize);
        containingBlockOfFlexItem->markContentWidthDamaged();

        Length oldWidth = flexItem->style()->width(), width;
        if (flexBasis.isAuto() || flexBasis.isWidth()) {
            if (flexBasis.isAuto() || flexBasis.width().isAuto()) {
                width = oldWidth;
                seenPercentageBasisSize |= width.isPercent();

                if (!shouldRespectPercentageWidthOnComputingBasisSize &&
                    width.isPercent()) {
                    width = Length();
                }
            } else if (width.isDefinite(
                           shouldRespectPercentageWidthOnComputingBasisSize)) {
                width = flexBasis.width();
            }
        }

        computeBorderMarginPaddingWithinFlexContext(
            ctx, flexItem, availableMainSize, availableCrossSize,
            isMainAxisInInlineAxis);

        flexItem->markNeedsLayout();
        flexItem->style()->setWidth(width);
        flexItem->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);
        flexItem->style()->setWidth(oldWidth);
        basisSize = flexItem->contentWidth();
    } else {
        Length maxWidth = restorer.m_maxWidth;
        LayoutUnit cbWidth = availableCrossSize;
        if (maxWidth.isSpecified()) {
            LayoutUnit specifiedMaxWidth =
                maxWidth.specifiedValue(availableCrossSize, flexItem);
            if (specifiedMaxWidth < availableCrossSize) {
                cbWidth = specifiedMaxWidth;
            }
        }

        Length oldHeight = flexItem->style()->height(), height;
        if (flexBasis.isAuto() || flexBasis.isWidth()) {
            if (flexBasis.isAuto() || flexBasis.width().isAuto()) {
                height = oldHeight;
                seenPercentageBasisSize |= height.isPercent();
            } else {
                height = flexBasis.width();
            }
        }

        computeBorderMarginPaddingWithinFlexContext(
            ctx, flexItem, availableMainSize, availableCrossSize,
            isMainAxisInInlineAxis);

        flexItem->markNeedsLayout();
        containingBlockOfFlexItem->setContentWidth(cbWidth);
        containingBlockOfFlexItem->markContentWidthDamaged();
        flexItem->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);
        flexItem->style()->setHeight(height);
        flexItem->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
        flexItem->style()->setHeight(oldHeight);

        if (applyLineClamp) {
            LayoutUnit sumofLineBoxHeight;
            auto lc = lineClamp();
            flexItem->iterateChildFrameBoxOnCondition([&](FrameBox* box) {
                if (box->isFrameBlockBox() &&
                    !box->asFrameBlockBox()->style()->isAbsolutePositioned()) {
                    auto& lineBoxes = box->asFrameBlockBox()->lineBoxes();

                    for (size_t i = 0; i < lineBoxes.size() && lc; i++) {
                        sumofLineBoxHeight += lineBoxes[i]->height();
                        lc--;
                    }
                    return lc ? true : false;
                }
                return false;
            });
            flexItem->setContentHeight(sumofLineBoxHeight);
        }
        basisSize = flexItem->contentHeight();
    }

    containingBlockOfFlexItem->setContentWidth(oldContainingBlockWidth);
    if (containingBlockOfFlexItemContentWidthDamaged) {
        containingBlockOfFlexItem->markContentWidthDamaged();
    } else {
        containingBlockOfFlexItem->clearContentWidthDamaged();
    }

    return std::make_pair(basisSize, seenPercentageBasisSize);
}

bool FrameFlexibleBox::isMainAxisInInlineAxis()
{
    if (isStandardMode()) {
        FlexDirectionValue flexDirection = style()->flexDirection();
        return (flexDirection == RowFlexDirectionValue) ||
               (flexDirection == RowReverseFlexDirectionValue);
    } else {
        BoxOrientValue boxOrient = style()->boxOrient();
        return boxOrient == BoxOrientValue::HorizontalBoxOrientValue;
    }
}

bool FrameFlexibleBox::isSingleLine()
{
    FlexWrapValue flexWrap = style()->flexWrap();
    return flexWrap == NoWrapFlexWrapValue;
}

bool FrameFlexibleBox::isLtrDirection()
{
    DirectionValue direction = style()->direction();

    if (isMainAxisInInlineAxis()) {
        if (isStandardMode()) {
            FlexDirectionValue flexDirection = style()->flexDirection();
            if (direction == LtrDirectionValue) {
                return flexDirection == RowFlexDirectionValue;
            } else {
                return flexDirection == RowReverseFlexDirectionValue;
            }
        } else {
            BoxOrientValue boxOrient = style()->boxOrient();
            if (direction == LtrDirectionValue) {
                return boxOrient == BoxOrientValue::HorizontalBoxOrientValue;
            } else {
                STARFISH_UNIMPLEMENTED();
                return true;
            }
        }
    } else {
        FlexWrapValue flexWrap = style()->flexWrap();
        if (direction == LtrDirectionValue) {
            return flexWrap != WrapReverseFlexWrapValue;
        } else {
            return flexWrap == WrapReverseFlexWrapValue;
        }
    }
}

bool FrameFlexibleBox::isTtbDirection()
{
    if (isMainAxisInInlineAxis()) {
        FlexWrapValue flexWrap = style()->flexWrap();
        return flexWrap != WrapReverseFlexWrapValue;
    } else {
        if (isStandardMode()) {
            FlexDirectionValue flexDirection = style()->flexDirection();
            return flexDirection == ColumnFlexDirectionValue;
        } else {
            BoxOrientValue boxOrient = style()->boxOrient();
            return boxOrient == BoxOrientValue::VerticalBoxOrientValue;
        }
    }
}

bool FrameFlexibleBox::isColumnDirection()
{
    if (isStandardMode()) {
        FlexDirectionValue direction = style()->flexDirection();
        return direction == FlexDirectionValue::ColumnFlexDirectionValue ||
               direction == FlexDirectionValue::ColumnReverseFlexDirectionValue;
    } else {
        BoxOrientValue boxOrient = style()->boxOrient();
        return boxOrient == BoxOrientValue::VerticalBoxOrientValue;
    }
}

uint32_t FrameFlexibleBox::lineClamp()
{
    if (isStandardMode()) {
        if (style()->flexDirection() == ColumnFlexDirectionValue) {
            return style()->lineClamp();
        }
    } else {
        if (style()->boxOrient() == BoxOrientValue::VerticalBoxOrientValue) {
            return style()->lineClamp();
        }
    }

    return 0;
}

bool FrameFlexibleBox::shouldApplyLineClamp(FrameBox* flexItem)
{
    return lineClamp() && flexItem->isFlexItem() &&
           !flexItem->isFrameReplaced() &&
           !flexItem->style()->height().isFixed();
}

void FrameFlexibleBox::layoutFlex(LayoutContext& ctx)
{
    FlexFormattingContext flexFormattingContext(ctx, this, contentWidth());

    do {
        flexFormattingContext.layoutMain();
        if (ctx.inComputingBasisSize() && !isMainAxisInInlineAxis()) {
            // FrameFlexibleBox content height is computed on layoutMain when
            // !isMainAxisInInlineAxis()
            break;
        }
        flexFormattingContext.layoutCross();
    } while (0);

    Frame* child = firstChild();

    while (child) {
        if (!child->isFlexItem()) {
            // to register absolute positioned box
            child->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);
        } else if (child->isFrameBox()) {
            if (shouldApplyLineClamp(child->asFrameBox())) {
                auto lc = lineClamp();
                child->asFrameBox()->iterateChildFrameBoxOnCondition(
                    [&](FrameBox* box) {
                        if (box->isFrameBlockBox() &&
                            !box->asFrameBlockBox()
                                 ->style()
                                 ->isAbsolutePositioned() &&
                            box->asFrameBlockBox()->style()->direction() !=
                                DirectionValue::RtlDirectionValue) {
                            auto& lineBoxes =
                                box->asFrameBlockBox()->lineBoxes();

                            if (!lineBoxes.size()) {
                                return lc ? true : false;
                            }

                            for (size_t i = 0; i < lineBoxes.size() - 1 && lc;
                                 i++) {
                                lc--;
                                if (lc == 0) {
                                    auto lineBoxContentWidth =
                                        lineBoxes[i]->contentWidth();
                                    auto& boxes = lineBoxes[i]->boxes();
                                    for (size_t j = boxes.size() - 1;
                                         j != SIZE_MAX; j--) {
                                        if (boxes[j]->isInlineTextBox()) {
                                            auto overflowString =
                                                ctx.starfish()
                                                    ->staticStrings()
                                                    ->m_overflowString;
                                            auto fnt =
                                                boxes[j]->style()->font();
                                            auto inlineTextBoxWidth =
                                                boxes[j]->width();

                                            StringView text =
                                                boxes[j]
                                                    ->asInlineTextBox()
                                                    ->text();

                                            auto overflowStringWidth =
                                                fnt->measureText(
                                                    overflowString);
                                            auto textWidth =
                                                fnt->measureText(text);
                                            auto oldTextWidth = textWidth;
                                            auto sum =
                                                textWidth + overflowStringWidth;

                                            while (text.length() &&
                                                   sum > lineBoxContentWidth) {
                                                text.setEnd(text.end() - 1);
                                                textWidth =
                                                    fnt->measureText(text);
                                                sum = textWidth +
                                                      overflowStringWidth;
                                            }
                                            boxes[j]
                                                ->asInlineTextBox()
                                                ->setText(
                                                    text.substring()->concat(
                                                        overflowString));
                                        }
                                    }
                                }
                            }
                            return lc ? true : false;
                        }
                        return false;
                    });
            }
        }
        child = child->next();
    }
}

bool FrameFlexibleBox::isStandardMode()
{
    DisplayValue display = style()->display();
    return display == DisplayValue::FlexDisplayValue ||
           display == DisplayValue::InlineFlexDisplayValue;
}

} // namespace Starfish
