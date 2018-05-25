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
#include "core/layout/FrameFlexibleBox.h"

namespace StarFish {

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

FlexFormattingContext::FlexFormattingContext(LayoutContext& ctx,
                                             FrameFlexibleBox* container,
                                             LayoutUnit availableWidth)
    : m_layoutContext(ctx)
    , m_container(container)
    , m_isMainAxisInInlineAxis(m_container->isMainAxisInInlineAxis())
    , m_isLtrDirection(m_container->isLtrDirection())
    , m_isTtbDirection(m_container->isTtbDirection())
    , m_isSingleLine(m_container->isSingleLine())
    , m_currentLineIdx(SIZE_MAX)
{
    addNewLine();
    computeAvailableSpace(availableWidth);
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
            m_container->contentHeightApplyingBoxSizing(contentHeight);
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
    auto iter = m_basisSizes.find(flexItem);
    if (iter != m_basisSizes.end()) {
        return iter->second;
    }

    LayoutUnit basisSize = m_container->basisSize(
        m_layoutContext, m_availableMainSize, m_availableCrossSize, flexItem);
    m_basisSizes[flexItem] = basisSize;
    return basisSize;
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
        } else {
            // to register absolute positioned box
            child->layout(m_layoutContext,
                          Frame::LayoutWantToResolve::ResolveAll);
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
        STARFISH_ASSERT(mainSize != intMaxForLayoutUnit);

        if (m_isMainAxisInInlineAxis) {
            flexItem->applyMinMaxWidthIfNeeds(m_layoutContext, mainSize,
                                              m_availableMainSize);
        } else {
            bool parentHasFixedHeight =
                m_layoutContext.parentHasFixedHeight(flexItem);
            flexItem->applyMinMaxHeightIfNeeds(m_layoutContext, mainSize,
                                               m_availableMainSize,
                                               parentHasFixedHeight);
        }

        if (m_isSingleLine || (lineMainSize == 0) ||
            ((m_isMainAxisInInlineAxis &&
              lineMainSize + flexItem->outerWidth() <= m_availableMainSize) ||
             (!m_isMainAxisInInlineAxis &&
              lineMainSize + flexItem->outerHeight() <= m_availableMainSize))) {
            flexItems.push_back(flexItem);
            if (m_isMainAxisInInlineAxis) {
                lineMainSize += flexItem->outerWidth();
            } else {
                lineMainSize += flexItem->outerHeight();
            }
        } else {
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

            if (usingGrowFactor) {
                if (!isMainSizeFlexible(flexItem, usingGrowFactor)) {
                    isFrozens[j] = true;
                }
            } else {
                if (!isMainSizeFlexible(flexItem, usingGrowFactor)) {
                    isFrozens[j] = true;
                }
            }

            isAllFrozen &= isFrozens[j];
        }

        LayoutUnit initialFreeSpace =
            m_availableMainSize - sumOfUsedupMainSize(flexItems, isFrozens);
        LayoutUnit remainingFreeSpace = initialFreeSpace;

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
                    if (usingGrowFactor) {
                        float factor = flexItem->style()->flexGrow();
                        if (factor != 0) {
                            targetMainSize =
                                basisSize(flexItem) +
                                (factor / sumOfFactor) * remainingFreeSpace;
                        }
                    } else {
                        float factor = flexItem->style()->flexShrink();
                        if (factor != 0) {
                            targetMainSize = basisSize(flexItem) +
                                             (factor * basisSize(flexItem) /
                                              scaledFlexShrinkFactor) *
                                                 remainingFreeSpace;
                        }
                    }
                    unclampedSize += targetMainSize;

                    if (m_isMainAxisInInlineAxis) {
                        flexItem->applyMinMaxWidthIfNeeds(m_layoutContext,
                                                          targetMainSize,
                                                          m_availableMainSize);
                        mainSize = flexItem->contentWidth();
                    } else {
                        flexItem->applyMinMaxHeightIfNeeds(m_layoutContext,
                                                           targetMainSize,
                                                           m_availableMainSize);
                        mainSize = flexItem->contentHeight();
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

        if (m_availableMainSize > sumOfMainSize && autoMarginCnt > 0) {
            LayoutUnit margin =
                (m_availableMainSize - sumOfMainSize) / autoMarginCnt;
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

        LayoutUnit offset;
        LayoutUnit separator;
        JustifyContentValue justifyContent =
            m_container->style()->justifyContent();
        FlexDirectionValue flexDirection =
            m_container->style()->flexDirection();
        switch (justifyContent) {
        case JustifyContentValue::FlexStartJustifyContentValue:
            break;
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
                    x += flexItem->outerWidth() + separator;
                }
            } else {
                LayoutUnit x = m_availableMainSize - offset +
                               m_container->borderLeft() +
                               m_container->paddingLeft();
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* flexItem = flexItems[j];
                    flexItem->setX(x - flexItem->outerWidth() +
                                   flexItem->marginLeft());
                    x -= flexItem->outerWidth() + separator;
                }
            }
        } else {
            if (m_isTtbDirection) {
                LayoutUnit y = offset + m_container->borderTop() +
                               m_container->paddingTop();
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* flexItem = flexItems[j];
                    flexItem->setY(y + flexItem->marginTop());
                    y += flexItem->outerHeight() + separator;
                }
            } else {
                LayoutUnit y = m_availableMainSize - offset +
                               m_container->borderTop() +
                               m_container->paddingTop();
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* flexItem = flexItems[j];
                    flexItem->setY(y - flexItem->outerHeight() +
                                   flexItem->marginTop());
                    y -= flexItem->outerHeight() + separator;
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

Nullable<LayoutUnit> FlexFormattingContext::firstLineBoxYPosition(
    FrameBox* flexItem) const
{
    auto it = m_firstLineBoxYPositions.find(flexItem);
    if (it == m_firstLineBoxYPositions.end()) {
        return Nullable<LayoutUnit>();
    }

    return it->second;
}

struct MainSizeFixer {
    MainSizeFixer(FrameBox* flexItem, bool isMainAxisInInlineAxis)
        : m_flexItem(flexItem)
        , m_isMainAxisInInlineAxis(isMainAxisInInlineAxis)
    {
        if (m_isMainAxisInInlineAxis) {
            m_oldMainSizeLength = flexItem->style()->width();
            m_oldMainSize = flexItem->width();
            flexItem->style()->setWidth(Length(Length::Fixed, m_oldMainSize));
        } else {
            m_oldMainSizeLength = flexItem->style()->height();
            m_oldMainSize = flexItem->height();
            flexItem->style()->setHeight(Length(Length::Fixed, m_oldMainSize));
        }
    }

    ~MainSizeFixer()
    {
        if (m_isMainAxisInInlineAxis) {
            m_flexItem->style()->setWidth(m_oldMainSizeLength);
            m_flexItem->setWidth(m_oldMainSize);
        } else {
            m_flexItem->style()->setHeight(m_oldMainSizeLength);
            m_flexItem->setHeight(m_oldMainSize);
        }
    }

    FrameBox* m_flexItem;
    Length m_oldMainSizeLength;
    LayoutUnit m_oldMainSize;
    bool m_isMainAxisInInlineAxis;
};

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
            if (m_isMainAxisInInlineAxis) {
                MainSizeFixer fixer(flexItem, m_isMainAxisInInlineAxis);
                auto resolveWhat = Frame::LayoutWantToResolve::ResolveHeight;
                if (flexItem->isFrameReplaced()) {
                    // height of FrameReplaced is computed at ResolveWidth
                    resolveWhat = Frame::LayoutWantToResolve::ResolveAll;
                }
                flexItem->layout(m_layoutContext, resolveWhat);
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

            if (flexItem->style()->alignSelf() == StretchAlignItemValue) {
                if ((m_isMainAxisInInlineAxis &&
                     flexItem->style()->height().isAuto() &&
                     !margin.top().isAuto() && !margin.bottom().isAuto()) ||
                    (!m_isMainAxisInInlineAxis &&
                     flexItem->style()->width().isAuto() &&
                     !margin.left().isAuto() && !margin.right().isAuto())) {
                    flexItemsToStretchInfos.emplace_back(i, flexItem);
                }
            }
        }

        flexLine.m_lineHeight = maxHypotheticalCrossSize;
        flexLine.m_maxAscender = maxAscender;
        sumOfCrossSize += maxHypotheticalCrossSize;
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
            MainSizeFixer fixer(flexItem, true);
            if (style->boxSizing() ==
                BoxSizingValue::ContentBoxBoxSizingValue) {
                style->setHeight(
                    Length(Length::Fixed,
                           flexLine.m_lineHeight - flexItem->mbpHeight()));
            } else {
                style->setHeight(
                    Length(Length::Fixed,
                           flexLine.m_lineHeight - flexItem->marginHeight()));
            }
            flexItem->markNeedsLayout();
            flexItem->layout(m_layoutContext,
                             Frame::LayoutWantToResolve::ResolveHeight);
            style->setHeight(Length());
        } else {
            MainSizeFixer fixer(flexItem, false);
            if (style->boxSizing() ==
                BoxSizingValue::ContentBoxBoxSizingValue) {
                style->setWidth(
                    Length(Length::Fixed,
                           flexLine.m_lineHeight - flexItem->mbpWidth()));
            } else {
                style->setWidth(
                    Length(Length::Fixed,
                           flexLine.m_lineHeight - flexItem->marginWidth()));
            }
            flexItem->markNeedsLayout();
            flexItem->layout(m_layoutContext,
                             Frame::LayoutWantToResolve::ResolveAll);
            style->setWidth(Length());
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
            LayoutUnit lineCrossSize = flexLine.m_lineHeight;

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
                    flexItem->setY(lineCrossSize - offset -
                                   flexItem->outerHeight());
                }

                flexItem->moveY(flexItem->marginTop());
            } else {
                if (m_isLtrDirection) {
                    flexItem->setX(offset);
                } else {
                    flexItem->setX(lineCrossSize - offset -
                                   flexItem->outerWidth());
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
                y += flexLine.m_lineHeight + separator;
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
                y -= flexLine.m_lineHeight + separator;
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
                x += flexLine.m_lineHeight + separator;
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
                x -= flexLine.m_lineHeight + separator;
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

struct MinMaxWidthHeightRestorer {
    MinMaxWidthHeightRestorer(FrameBox* b)
        : m_box(b)
    {
        ComputedStyle* style = b->style();
        m_minWidth = style->minWidth();
        m_maxWidth = style->maxWidth();
        m_minHeight = style->minHeight();
        m_maxHeight = style->maxHeight();
    }

    void initValues()
    {
        ComputedStyle* style = m_box->style();
        style->setMinWidth(Length(Length::Fixed, 0));
        style->setMaxWidth(Length());
        style->setMinHeight(Length(Length::Fixed, 0));
        style->setMaxHeight(Length());
    }

    ~MinMaxWidthHeightRestorer()
    {
        ComputedStyle* style = m_box->style();
        style->setMinWidth(m_minWidth);
        style->setMaxWidth(m_maxWidth);
        style->setMinHeight(m_minHeight);
        style->setMaxHeight(m_maxHeight);
    }

    FrameBox* m_box;
    Length m_minWidth;
    Length m_maxWidth;
    Length m_minHeight;
    Length m_maxHeight;
};

LayoutUnit FrameFlexibleBox::basisSize(LayoutContext& ctx,
                                       LayoutUnit availableMainSize,
                                       LayoutUnit availableCrossSize,
                                       FrameBox* flexItem)
{
    bool isMainAxisInInlineAxis = this->isMainAxisInInlineAxis();
    LayoutUnit basisSize = intMaxForLayoutUnit;
    FlexBasisData flexBasis = flexItem->style()->flexBasis();
    MinMaxWidthHeightRestorer restorer(flexItem);
    restorer.initValues();
    MBPRestorer restorer2(flexItem);
    if (isMainAxisInInlineAxis) {
        flexItem->computeBorderMarginPadding(ctx, availableMainSize);
    } else {
        flexItem->computeBorderMarginPadding(ctx, availableCrossSize);
    }

    // A. If the item has a definite used flex basis, that’s the flex
    // base size.
    if (flexBasis.isWidth()) {
        Length basisWidth = flexBasis.width();
        if (basisWidth.isDefinite(availableMainSize != intMaxForLayoutUnit)) {
            basisSize = basisWidth.specifiedValue(availableMainSize, this);
            if (isMainAxisInInlineAxis) {
                basisSize = flexItem->contentWidthApplyingBoxSizing(basisSize);
            } else {
                basisSize = flexItem->contentHeightApplyingBoxSizing(basisSize);
            }
            return basisSize;
        }
    } else if (flexBasis.isContent()) {
        if (flexItem->isFrameReplaced()) {
            // B. If the flex item has an intrinsic aspect ratio, a used
            // flex basis of 'content', and a definite cross size.
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

            return basisSize;
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

    if (isMainAxisInInlineAxis) {
        containingBlockOfFlexItem->setContentWidth(availableMainSize);
        Length oldWidth = flexItem->style()->width(), width;
        if (flexBasis.isWidth()) {
            if (flexBasis.width().isAuto()) {
                width = oldWidth;
            } else {
                width = flexBasis.width();
            }
        }
        flexItem->style()->setWidth(width);
        flexItem->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);
        flexItem->style()->setWidth(oldWidth);
        basisSize = flexItem->contentWidth();
    } else {
        containingBlockOfFlexItem->setContentWidth(availableCrossSize);
        flexItem->layout(ctx, Frame::LayoutWantToResolve::ResolveWidth);
        Length oldHeight = flexItem->style()->height(), height;
        if (flexBasis.isWidth()) {
            if (flexBasis.width().isAuto()) {
                height = oldHeight;
            } else {
                height = flexBasis.width();
            }
        }
        flexItem->style()->setHeight(height);
        flexItem->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
        flexItem->style()->setHeight(oldHeight);
        basisSize = flexItem->contentHeight();
    }

    containingBlockOfFlexItem->setContentWidth(oldContainingBlockWidth);

    return basisSize;
}

bool FrameFlexibleBox::isMainAxisInInlineAxis()
{
    FlexDirectionValue flexDirection = style()->flexDirection();
    return (flexDirection == RowFlexDirectionValue) ||
           (flexDirection == RowReverseFlexDirectionValue);
}

bool FrameFlexibleBox::isSingleLine()
{
    FlexWrapValue flexWrap = style()->flexWrap();
    return flexWrap == NoWrapFlexWrapValue;
}

bool FrameFlexibleBox::isLtrDirection()
{
    DirectionValue direction = style()->direction();
    FlexDirectionValue flexDirection = style()->flexDirection();

    if (isMainAxisInInlineAxis()) {
        if (direction == LtrDirectionValue) {
            return flexDirection == RowFlexDirectionValue;
        } else {
            return flexDirection == RowReverseFlexDirectionValue;
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
        FlexDirectionValue flexDirection = style()->flexDirection();
        return flexDirection == ColumnFlexDirectionValue;
    }
}

void FrameFlexibleBox::layoutFlex(LayoutContext& ctx)
{
    FlexFormattingContext flexFormattingContext(ctx, this, contentWidth());

    flexFormattingContext.layoutMain();
    flexFormattingContext.layoutCross();
}
}
