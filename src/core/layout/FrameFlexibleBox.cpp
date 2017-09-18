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
#include "core/layout/FrameFlexibleBox.h"

namespace StarFish {

FlexFormattingContext::FlexFormattingContext(LayoutContext& ctx,
                                             FrameFlexibleBox* container,
                                             LayoutUnit availableWidth)
    : m_layoutContext(ctx)
    , m_container(container)
    , m_isMainAxisInInlineAxis(m_container->isMainAxisInInlineAxis())
    , m_isLtrDirection(m_container->isLtrDirection())
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
    LayoutUnit contentHeight;
    if (height.isFixed()) {
        contentHeight = height.fixed();
        contentHeight =
            m_container->contentHeightApplyingBoxSizing(contentHeight);
    } else if (height.isPercent() && parentHasFixedHeight) {
        LayoutUnit parentContentHeight =
            m_layoutContext.parentFixedHeight(m_container);
        contentHeight = height.percentValue(parentContentHeight);
        contentHeight =
            m_container->contentHeightApplyingBoxSizing(contentHeight);
    } else if (height.isViewportPercent()) {
        contentHeight =
            height.viewportPercentValue(m_layoutContext.viewportHeight());
        contentHeight =
            m_container->contentHeightApplyingBoxSizing(contentHeight);
    } else {
        contentHeight = intMaxForLayoutUnit;
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
        if (isAnonymousFlexItemContainingOnlyWhitespace(child)) {
            child = child->next();
            continue;
        }

        orderedFlexItems.push_back(child->asFrameBox());
        child = child->next();
    }

    std::sort(orderedFlexItems.begin(), orderedFlexItems.end(),
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

        if (flexItem->isAbsolutePositioned()) {
            m_layoutContext.registerAbsolutePositionedBox(flexItem);
            if (flexItems.size() > 0) {
                maxMainSize = std::max(maxMainSize, lineMainSize);
                flexLine.m_lineWidth = lineMainSize;
                addNewLine();
            }
            FlexLine& newFlexLine = m_flexLines[m_currentLineIdx];
            std::vector<FrameBox*>& newFlexItems = newFlexLine.m_flexItems;
            newFlexItems.push_back(flexItem);
            newFlexLine.m_hasAbsolutePositionedBox = true;
            addNewLine();
            lineMainSize = 0;
            iter++;
            continue;
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

void FlexFormattingContext::applyFlexFactor()
{
    size_t lines = m_currentLineIdx + 1;

    for (size_t i = 0; i < lines; i++) {
        FlexLine& flexLine = m_flexLines[i];
        std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
        LayoutUnit lineWidth = flexLine.m_lineWidth;
        if (lineWidth == m_availableMainSize) {
            continue;
        }

        bool usingGrowFactor = lineWidth < m_availableMainSize;
        LayoutUnit initialFreeSpace = m_availableMainSize - lineWidth;
        float sumOfFactor = 0;
        bool allFrozen = true;
        LayoutUnit scaledFlexShrinkFactor;

        for (size_t j = 0; j < flexItems.size(); j++) {
            FrameBox* flexItem = flexItems[j];
            if (flexItem->isAbsolutePositioned()) {
                continue;
            }

            if (usingGrowFactor) {
                if (isMainSizeFlexible(flexItem, usingGrowFactor)) {
                    sumOfFactor += flexItem->style()->flexGrow();
                    allFrozen = false;
                }
            } else {
                if (isMainSizeFlexible(flexItem, usingGrowFactor)) {
                    sumOfFactor += flexItem->style()->flexShrink();
                    scaledFlexShrinkFactor +=
                        flexItem->style()->flexShrink() * basisSize(flexItem);
                    allFrozen = false;
                }
            }
        }

        if (allFrozen) {
            continue;
        }

        LayoutUnit remainingFreeSpace = initialFreeSpace;
        if (sumOfFactor < 1) {
            remainingFreeSpace = initialFreeSpace * sumOfFactor;
        }

        if (remainingFreeSpace == 0) {
            continue;
        }

        for (size_t j = 0; j < flexItems.size(); j++) {
            FrameBox* flexItem = flexItems[j];
            if (flexItem->isAbsolutePositioned()) {
                continue;
            }

            if (usingGrowFactor) {
                float factor = flexItem->style()->flexGrow();
                if (factor != 0) {
                    LayoutUnit mainSize =
                        basisSize(flexItem) +
                        (factor / sumOfFactor) * remainingFreeSpace;
                    if (m_isMainAxisInInlineAxis) {
                        flexItem->applyMinMaxWidthIfNeeds(
                            m_layoutContext, mainSize, m_availableMainSize);
                    } else {
                        flexItem->applyMinMaxHeightIfNeeds(
                            m_layoutContext, mainSize, m_availableMainSize);
                    }
                }
            } else {
                float factor = flexItem->style()->flexShrink();
                if (factor != 0) {
                    LayoutUnit mainSize = basisSize(flexItem) +
                                          (factor * basisSize(flexItem) /
                                           scaledFlexShrinkFactor) *
                                              remainingFreeSpace;
                    if (m_isMainAxisInInlineAxis) {
                        flexItem->applyMinMaxWidthIfNeeds(
                            m_layoutContext, mainSize, m_availableMainSize);
                    } else {
                        flexItem->applyMinMaxHeightIfNeeds(
                            m_layoutContext, mainSize, m_availableMainSize);
                    }
                }
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

            if (flexItem->isAbsolutePositioned()) {
                continue;
            }

            if (m_isMainAxisInInlineAxis) {
                if (sumOfMainSize <= m_availableMainSize &&
                    flexItem->style()->marginLeft().isAuto()) {
                    autoMarginCnt++;
                }
                sumOfMainSize += flexItem->outerWidth();
                if (sumOfMainSize <= m_availableMainSize &&
                    flexItem->style()->marginRight().isAuto()) {
                    autoMarginCnt++;
                }
            } else {
                if (sumOfMainSize <= m_availableMainSize &&
                    flexItem->style()->marginTop().isAuto()) {
                    autoMarginCnt++;
                }
                sumOfMainSize += flexItem->outerHeight();
                if (sumOfMainSize <= m_availableMainSize &&
                    flexItem->style()->marginBottom().isAuto()) {
                    autoMarginCnt++;
                }
            }
        }

        if (m_availableMainSize > sumOfMainSize && autoMarginCnt > 0) {
            LayoutUnit margin =
                (m_availableMainSize - sumOfMainSize) / autoMarginCnt;
            for (size_t j = 0; j < flexItems.size(); j++) {
                FrameBox* flexItem = flexItems[j];

                if (flexItem->isAbsolutePositioned()) {
                    continue;
                }

                if (m_isMainAxisInInlineAxis) {
                    if (flexItem->style()->marginLeft().isAuto()) {
                        flexItem->setMarginLeft(margin);
                        sumOfMainSize += margin;
                        autoMarginCnt--;
                    }

                    if (autoMarginCnt > 0 &&
                        flexItem->style()->marginRight().isAuto()) {
                        flexItem->setMarginRight(margin);
                        sumOfMainSize += margin;
                        autoMarginCnt--;
                    }
                } else {
                    if (flexItem->style()->marginTop().isAuto()) {
                        flexItem->setMarginTop(margin);
                        sumOfMainSize += margin;
                        autoMarginCnt--;
                    }

                    if (autoMarginCnt > 0 &&
                        flexItem->style()->marginBottom().isAuto()) {
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
                         (flexItems.size() + 1);
                separator = offset;
            } else {
                offset = (m_availableMainSize - sumOfMainSize) /
                         (flexItems.size() + 1);
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
                    flexItem->setX(x);
                    if (flexItem->isAbsolutePositioned()) {
                        STARFISH_ASSERT(flexItems.size() == 1);
                    } else {
                        flexItem->moveX(flexItem->marginLeft());
                        x += flexItem->outerWidth() + separator;
                    }
                }
            } else {
                LayoutUnit x = m_availableMainSize - offset +
                               m_container->borderLeft() +
                               m_container->paddingLeft();
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* flexItem = flexItems[j];
                    flexItem->setX(x - flexItem->outerWidth());
                    if (flexItem->isAbsolutePositioned()) {
                        STARFISH_ASSERT(flexItems.size() == 1);
                    } else {
                        flexItem->moveX(flexItem->marginLeft());
                        x -= flexItem->outerWidth() + separator;
                    }
                }
            }
        } else {
            LayoutUnit y =
                offset + m_container->borderTop() + m_container->paddingTop();
            for (size_t j = 0; j < flexItems.size(); j++) {
                FrameBox* flexItem = flexItems[j];
                flexItem->setY(y);
                if (flexItem->isAbsolutePositioned()) {
                    STARFISH_ASSERT(flexItems.size() == 1);
                } else {
                    flexItem->moveY(flexItem->marginTop());
                    y += flexItem->outerHeight() + separator;
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

bool FlexFormattingContext::isAnonymousFlexItemContainingOnlyWhitespace(
    Frame* flexItem)
{
    if (flexItem->isFrameBlockBox()) {
        FrameBlockBox* blockBox = flexItem->asFrameBlockBox();
        if (blockBox->isAnonymous() && blockBox->firstChild() &&
            blockBox->firstChild() == blockBox->lastChild() &&
            blockBox->firstChild()->isFrameText() &&
            blockBox->firstChild()
                ->asFrameText()
                ->text()
                ->containsOnlyWhitespace()) {
            return true;
        }
    }

    return false;
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

void FlexFormattingContext::computeCrossSize()
{
    size_t lines = m_currentLineIdx + 1;
    size_t normalLines = 0;
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

            if (!flexItem->isAbsolutePositioned() &&
                flexItem->isFrameBlockBox() &&
                flexItem->style()->alignSelf() == BaselineAlignItemValue &&
                m_isMainAxisInInlineAxis &&
                !flexItem->style()->marginTop().isAuto() &&
                !flexItem->style()->marginBottom().isAuto()) {
                shouldAlignAtFirstBaseline = true;
            }

            if (m_isMainAxisInInlineAxis) {
                if (shouldAlignAtFirstBaseline) {
                    m_layoutContext.pushBlockBoxAligningAtFirstBaseline(
                        flexItem->asFrameBlockBox());
                }
                flexItem->layout(m_layoutContext,
                                 Frame::LayoutWantToResolve::ResolveHeight);
                if (shouldAlignAtFirstBaseline) {
                    auto it = m_layoutContext.firstLineAscender(
                        flexItem->asFrameBlockBox());
                    if (it.hasValue()) {
                        LineBox* flb = it.getValue().first;
                        LayoutUnit ascender = flb->absolutePoint(flexItem).y() +
                                              it.getValue().second;
                        m_firstLineBoxYPositions[flexItem] = ascender;
                        maxAscender = std::max(maxAscender, ascender);
                    }
                    m_layoutContext.popBlockBoxAligningAtFirstBaseline();
                }
            }

            if (flexItem->isAbsolutePositioned()) {
                continue;
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
                     !flexItem->style()->marginTop().isAuto() &&
                     !flexItem->style()->marginBottom().isAuto()) ||
                    (!m_isMainAxisInInlineAxis &&
                     flexItem->style()->width().isAuto() &&
                     !flexItem->style()->marginLeft().isAuto() &&
                     !flexItem->style()->marginRight().isAuto())) {
                    flexItemsToStretchInfos.emplace_back(i, flexItem);
                }
            }
        }

        if (flexLine.m_hasAbsolutePositionedBox) {
            flexLine.m_lineHeight = m_availableCrossSize;
        } else {
            normalLines++;
            flexLine.m_lineHeight = maxHypotheticalCrossSize;
            flexLine.m_maxAscender = maxAscender;
            sumOfCrossSize += maxHypotheticalCrossSize;
        }
    }

    if (m_container->style()->alignContent() == StretchAlignContentValue &&
        m_availableCrossSize != intMaxForLayoutUnit &&
        sumOfCrossSize < m_availableCrossSize && normalLines > 0) {
        LayoutUnit amountToStretchByLine =
            (m_availableCrossSize - sumOfCrossSize) / normalLines;
        for (size_t i = 0; i < lines; i++) {
            FlexLine& flexLine = m_flexLines[i];
            if (!flexLine.m_hasAbsolutePositionedBox) {
                flexLine.m_lineHeight += amountToStretchByLine;
            }
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

    if (m_isSingleLine) {
        for (size_t i = 0; i < lines; i++) {
            FlexLine& flexLine = m_flexLines[i];
            if (!flexLine.m_hasAbsolutePositionedBox) {
                if (m_isMainAxisInInlineAxis) {
                    flexLine.m_lineHeight = m_container->contentHeight();
                } else {
                    flexLine.m_lineHeight = m_container->contentWidth();
                }
                break;
            }
        }
    }

    for (size_t i = 0; i < flexItemsToStretchInfos.size(); i++) {
        auto& flexItemsToStretchInfo = flexItemsToStretchInfos[i];
        size_t lineIdx = flexItemsToStretchInfo.first;
        FrameBox* flexItem = flexItemsToStretchInfo.second;

        if (m_isMainAxisInInlineAxis) {
            Length old = flexItem->style()->height();
            LayoutUnit oldContentWidth = flexItem->contentWidth();
            flexItem->style()->setHeight(
                Length(Length::Fixed, m_flexLines[lineIdx].m_lineHeight -
                                          flexItem->marginHeight()));
            flexItem->layout(m_layoutContext,
                             Frame::LayoutWantToResolve::ResolveHeight);
            flexItem->style()->setHeight(old);
            if (oldContentWidth > flexItem->contentWidth()) {
                flexItem->setContentWidth(oldContentWidth);
            } else if (flexItem->contentWidth() > oldContentWidth) {
                m_container->computeContentWidth(m_layoutContext,
                                                 m_container->contentWidth() +
                                                     flexItem->contentWidth() -
                                                     oldContentWidth);
            }
        } else {
            Length old = flexItem->style()->width();
            LayoutUnit oldContentHeight = flexItem->contentHeight();
            flexItem->style()->setWidth(
                Length(Length::Fixed, m_flexLines[lineIdx].m_lineHeight -
                                          flexItem->marginWidth()));
            flexItem->layout(m_layoutContext,
                             Frame::LayoutWantToResolve::ResolveAll);
            flexItem->style()->setWidth(old);
            if (oldContentHeight > flexItem->contentHeight()) {
                flexItem->setContentHeight(oldContentHeight);
            } else if (flexItem->contentHeight() > oldContentHeight) {
                m_container->computeContentHeight(
                    m_layoutContext, m_container->contentHeight() +
                                         flexItem->contentHeight() -
                                         oldContentHeight);
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

            if (item->isAbsolutePositioned()) {
                continue;
            }

            if (m_isMainAxisInInlineAxis) {
                if (lineCrossSize > item->outerHeight()) {
                    LayoutUnit margin = lineCrossSize - item->outerHeight();
                    if (item->style()->marginTop().isAuto() &&
                        item->style()->marginBottom().isAuto()) {
                        item->setMarginTop(margin / 2);
                        item->setMarginBottom(margin / 2);
                    } else if (item->style()->marginTop().isAuto()) {
                        item->setMarginTop(margin);
                    } else if (item->style()->marginBottom().isAuto()) {
                        item->setMarginBottom(margin);
                    }
                }
            } else {
                if (lineCrossSize > item->outerWidth()) {
                    LayoutUnit margin = lineCrossSize - item->outerWidth();
                    if (item->style()->marginLeft().isAuto() &&
                        item->style()->marginRight().isAuto()) {
                        item->setMarginLeft(margin / 2);
                        item->setMarginRight(margin / 2);
                    } else if (item->style()->marginLeft().isAuto()) {
                        item->setMarginLeft(margin);
                    } else if (item->style()->marginRight().isAuto()) {
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

            LayoutUnit lineCrossSize = flexLine.m_lineHeight;
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
                        offset = flexLine.m_maxAscender - it.getValue() -
                                 flexItem->marginTop();
                    } else {
                        offset = flexLine.m_maxAscender - flexItem->height() -
                                 flexItem->marginTop();
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
                flexItem->setY(offset);
                if (flexItem->isAbsolutePositioned()) {
                    flexItem->moveY(m_container->borderTop() +
                                    m_container->paddingTop());
                    STARFISH_ASSERT(flexItems.size() == 1);
                } else {
                    flexItem->moveY(flexItem->marginTop());
                }
            } else {
                if (m_isLtrDirection) {
                    flexItem->setX(offset);
                } else {
                    flexItem->setX(lineCrossSize - offset -
                                   flexItem->outerWidth());
                }
                if (flexItem->isAbsolutePositioned()) {
                    flexItem->moveX(m_container->borderLeft() +
                                    m_container->paddingLeft());
                    STARFISH_ASSERT(flexItems.size() == 1);
                } else {
                    flexItem->moveX(flexItem->marginLeft());
                }
            }
        }
    }
}

void FlexFormattingContext::applyAlignContent()
{
    size_t lines = m_currentLineIdx + 1;
    size_t normalLines = 0;
    LayoutUnit offset;
    LayoutUnit separator;
    AlignContentValue alignContent = m_container->style()->alignContent();
    FlexDirectionValue flexDirection = m_container->style()->flexDirection();
    FlexWrapValue flexWrap = m_container->style()->flexWrap();

    LayoutUnit sumOfCrossSize;

    for (size_t i = 0; i < lines; i++) {
        FlexLine& line = m_flexLines[i];
        if (!line.m_hasAbsolutePositionedBox) {
            sumOfCrossSize += line.m_lineHeight;
            normalLines++;
        }
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
            offset =
                (m_availableCrossSize - sumOfCrossSize) / (normalLines + 1);
            separator = offset;
        } else {
            offset =
                (m_availableCrossSize - sumOfCrossSize) / (normalLines + 1);
        }
        break;
    case SpaceBetweenAlignContentValue:
        if (normalLines > 1 && m_availableCrossSize > sumOfCrossSize) {
            separator =
                (m_availableCrossSize - sumOfCrossSize) / (normalLines - 1);
        }
        break;
    case StretchAlignContentValue:
        offset = 0;
        break;
    }

    if (m_isMainAxisInInlineAxis) {
        if (flexWrap != WrapReverseFlexWrapValue) {
            LayoutUnit y =
                offset + m_container->borderTop() + m_container->paddingTop();
            for (size_t i = 0; i < lines; i++) {
                FlexLine& flexLine = m_flexLines[i];
                if (flexLine.m_hasAbsolutePositionedBox) {
                    continue;
                }
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
                if (flexLine.m_hasAbsolutePositionedBox) {
                    continue;
                }
                std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* item = flexItems[j];
                    item->moveY(y - item->outerHeight());
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
                if (flexLine.m_hasAbsolutePositionedBox) {
                    continue;
                }
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
                if (flexLine.m_hasAbsolutePositionedBox) {
                    continue;
                }
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

LayoutUnit FrameFlexibleBox::basisSize(LayoutContext& ctx,
                                       LayoutUnit availableMainSize,
                                       LayoutUnit availableCrossSize,
                                       FrameBox* flexItem)
{
    bool isMainAxisInInlineAxis = this->isMainAxisInInlineAxis();
    if (isMainAxisInInlineAxis) {
        flexItem->computeBorderMarginPadding(ctx, availableMainSize);
    } else {
        flexItem->computeBorderMarginPadding(ctx, availableCrossSize);
    }

    LayoutUnit basisSize = intMaxForLayoutUnit;

    if (!flexItem->isAbsolutePositioned()) {
        FlexBasisData flexBasis = flexItem->style()->flexBasis();
        if (flexBasis.isContent()) {
            if (flexItem->isFrameReplaced()) {
                // B. If the flex item has an intrinsic aspect ratio, a used
                // flex basis fo 'content', and a definite cross size.
                LayoutUnit intrinsicWidth, intrinsicHeight;
                LayoutUnit parentContentWidth;
                bool hasAspectRatio;
                Length parentHeightLength;

                if (isMainAxisInInlineAxis &&
                    availableCrossSize != intMaxForLayoutUnit) {
                    parentHeightLength =
                        Length(Length::Fixed, availableCrossSize);
                } else if (!isMainAxisInInlineAxis &&
                           availableMainSize != intMaxForLayoutUnit) {
                    parentHeightLength =
                        Length(Length::Fixed, availableMainSize);
                } else {
                    parentHeightLength = Length(Length::Auto);
                }

                flexItem->asFrameReplaced()->computeIntrinsicSize(
                    ctx, intrinsicWidth, intrinsicHeight, hasAspectRatio,
                    parentContentWidth, parentHeightLength);

                if (availableCrossSize != intMaxForLayoutUnit &&
                    hasAspectRatio) {
                    if (isMainAxisInInlineAxis) {
                        basisSize = availableCrossSize *
                                    (intrinsicWidth / intrinsicHeight);
                    } else {
                        basisSize = availableCrossSize *
                                    (intrinsicHeight / intrinsicWidth);
                    }
                }
            }
        } else {
            // A. If the item has a definite used flex basis, that’s the flex
            // base size.
            Length basisWidth = flexBasis.width();
            if (basisWidth.isAuto() ||
                (availableMainSize == intMaxForLayoutUnit &&
                 basisWidth.isPercent())) {
            } else {
                if (isMainAxisInInlineAxis) {
                    basisSize = basisWidth.specifiedValue(availableMainSize,
                                                          ctx.viewportWidth());
                    basisSize =
                        flexItem->contentWidthApplyingBoxSizing(basisSize);
                } else {
                    basisSize = basisWidth.specifiedValue(availableMainSize,
                                                          ctx.viewportHeight());
                    basisSize =
                        flexItem->contentHeightApplyingBoxSizing(basisSize);
                }
            }
        }
    }

    // E. Otherwise, size the item into the available space using its used flex
    // basis in place of its main size, treating a value of content as
    // max-content. If a cross size is needed to determine the main size (e.g.
    // when the flex item’s main size is in its block axis) and the flex item’s
    // cross size is auto and not definite, in this calculation use fit-content
    // as the flex item’s cross size.
    Length width = flexItem->style()->width();

    if (isMainAxisInInlineAxis) {
        if (basisSize == intMaxForLayoutUnit) {
            if (width.isAuto() || (availableMainSize == intMaxForLayoutUnit &&
                                   width.isPercent())) {
                PreferredWidthContext p(
                    ctx, flexItem, availableMainSize - flexItem->mbpWidth());
                p.computePreferredWidth();
                basisSize = p.preferredWidth();
            } else {
                basisSize = width.specifiedValue(availableMainSize,
                                                 ctx.viewportWidth());
                basisSize = flexItem->contentWidthApplyingBoxSizing(basisSize);
            }
        }
    } else {
        if (width.isAuto() ||
            (availableCrossSize == intMaxForLayoutUnit && width.isPercent())) {
            PreferredWidthContext p(ctx, flexItem,
                                    availableCrossSize - flexItem->mbpWidth());
            p.computePreferredWidth();
            flexItem->setContentWidth(p.preferredWidth());
        } else {
            LayoutUnit w =
                width.specifiedValue(availableCrossSize, ctx.viewportWidth());
            w = flexItem->contentWidthApplyingBoxSizing(w);
            flexItem->setContentWidth(w);
        }
        flexItem->layout(ctx, Frame::LayoutWantToResolve::ResolveHeight);
        if (basisSize == intMaxForLayoutUnit) {
            basisSize = flexItem->contentHeight();
        }
    }

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
    FlexWrapValue flexWrap = style()->flexWrap();

    if (isMainAxisInInlineAxis()) {
        if (direction == LtrDirectionValue) {
            return flexDirection == RowFlexDirectionValue;
        } else {
            return flexDirection == RowReverseFlexDirectionValue;
        }
    } else {
        if (direction == LtrDirectionValue) {
            if (flexDirection == ColumnFlexDirectionValue) {
                return flexWrap != WrapReverseFlexWrapValue;
            } else {
                return flexWrap == WrapReverseFlexWrapValue;
            }
        } else {
            if (flexDirection == ColumnFlexDirectionValue) {
                return flexWrap == WrapReverseFlexWrapValue;
            } else {
                return flexWrap != WrapReverseFlexWrapValue;
            }
        }
    }
}

void FrameFlexibleBox::layoutFlex(LayoutContext& ctx)
{
    if (!isNecessaryBlockBox()) {
        return;
    }

    FlexFormattingContext flexFormattingContext(ctx, this, contentWidth());

    flexFormattingContext.layoutMain();
    flexFormattingContext.layoutCross();
}
}
