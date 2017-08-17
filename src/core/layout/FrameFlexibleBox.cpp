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
                                             FrameFlexibleBox* container)
    : m_layoutContext(ctx)
    , m_container(container)
    , m_currentLineIdx(SIZE_MAX)
{
    FlexDirectionValue flexDirection = container->style()->flexDirection();
    m_mainDirectionIsInlineAxis =
        (flexDirection == RowFlexDirectionValue) ||
        (flexDirection == RowReverseFlexDirectionValue);
    addNewLine(0);
    computeAvailableSpace();
}

bool FlexFormattingContext::ltrDirection()
{
    DirectionValue direction = m_container->style()->direction();
    FlexDirectionValue flexDirection = m_container->style()->flexDirection();
    FlexWrapValue flexWrap = m_container->style()->flexWrap();

    if (m_mainDirectionIsInlineAxis) {
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

void FlexFormattingContext::computeAvailableSpace()
{
    Length width = m_container->style()->width();
    Length height = m_container->style()->height();
    FlexDirectionValue flexDirection = m_container->style()->flexDirection();
    LayoutUnit parentContentWidth =
        m_layoutContext.parentContentWidth(m_container);
    bool parentHasFixedHeight =
        m_layoutContext.parentHasFixedHeight(m_container);

    if (m_mainDirectionIsInlineAxis) {
        m_availableMainSize = m_container->contentWidth();

        if (height.isFixed()) {
            m_availableCrossSize = height.fixed();
            m_availableCrossSize = m_container->contentHeightApplyingBoxSizing(
                m_availableCrossSize);
        } else if (height.isPercent() && parentHasFixedHeight) {
            LayoutUnit parentContentHeight =
                m_layoutContext.parentFixedHeight(m_container);
            m_availableCrossSize = height.percentValue(parentContentHeight);
            m_availableCrossSize = m_container->contentHeightApplyingBoxSizing(
                m_availableCrossSize);
        } else if (height.isViewportPercent()) {
            m_availableCrossSize =
                height.viewportPercentValue(m_layoutContext.viewportHeight());
            m_availableCrossSize = m_container->contentHeightApplyingBoxSizing(
                m_availableCrossSize);
        } else {
            m_availableCrossSize = intMaxForLayoutUnit;
        }
    } else {
        if (height.isFixed()) {
            m_availableMainSize = height.fixed();
            m_availableMainSize = m_container->contentHeightApplyingBoxSizing(
                m_availableMainSize);
        } else if (height.isPercent() && parentHasFixedHeight) {
            LayoutUnit parentContentHeight =
                m_layoutContext.parentFixedHeight(m_container);
            m_availableMainSize = height.percentValue(parentContentHeight);
            m_availableMainSize = m_container->contentHeightApplyingBoxSizing(
                m_availableMainSize);
        } else if (height.isViewportPercent()) {
            m_availableMainSize =
                height.viewportPercentValue(m_layoutContext.viewportHeight());
            m_availableMainSize = m_container->contentHeightApplyingBoxSizing(
                m_availableMainSize);
        } else {
            m_availableMainSize = intMaxForLayoutUnit;
        }

        m_availableCrossSize = m_container->contentWidth();
    }
}

void FlexFormattingContext::computeHypotheticalMainSize()
{
    // https://www.w3.org/TR/css-flexbox-1/#algo-main-item
    // 3. Determine the flex base size and hypothetical main size of each item:
    Frame* child = m_container->firstChild();
    LayoutUnit sumOfMainSize;
    while (child) {
        FrameBox* flexItemBox = child->asFrameBox();
        if (m_mainDirectionIsInlineAxis) {
            flexItemBox->computeBorderMarginPadding(m_layoutContext,
                                                    m_availableMainSize);
        } else {
            flexItemBox->computeBorderMarginPadding(m_layoutContext,
                                                    m_availableCrossSize);
        }

        FlexBasisData flexBasis = child->style()->flexBasis();
        LayoutUnit mainSize = intMaxForLayoutUnit;
        if (flexItemBox->isAbsolutePositioned() || flexBasis.isContent()) {
            if (flexItemBox->isFrameReplaced()) {
                // B. If the flex item has an intrinsic aspect ratio, a used
                // flex basis fo 'content', and a definite cross size.
                LayoutUnit intrinsicWidth, intrinsicHeight;
                LayoutUnit parentContentWidth;
                bool hasAspectRatio;
                Length parentHeightLength;

                if (m_mainDirectionIsInlineAxis &&
                    m_availableCrossSize != intMaxForLayoutUnit) {
                    parentHeightLength =
                        Length(Length::Fixed, m_availableCrossSize);
                } else if (!m_mainDirectionIsInlineAxis &&
                           m_availableMainSize != intMaxForLayoutUnit) {
                    parentHeightLength =
                        Length(Length::Fixed, m_availableMainSize);
                } else {
                    parentHeightLength = Length(Length::Auto);
                }

                flexItemBox->asFrameReplaced()->computeIntrinsicSize(
                    m_layoutContext, intrinsicWidth, intrinsicHeight,
                    hasAspectRatio, parentContentWidth, parentHeightLength);

                if (m_availableCrossSize != intMaxForLayoutUnit &&
                    hasAspectRatio) {
                    if (m_mainDirectionIsInlineAxis) {
                        mainSize = m_availableCrossSize *
                                   (intrinsicWidth / intrinsicHeight);
                    } else {
                        mainSize = m_availableCrossSize *
                                   (intrinsicHeight / intrinsicWidth);
                    }
                }
            }
        } else {
            // A. If the item has a definite used flex basis, that’s the
            // flex base size.
            Length basisWidth = flexBasis.width();
            if (basisWidth.isAuto() ||
                (m_availableMainSize == intMaxForLayoutUnit &&
                 basisWidth.isPercent())) {
            } else {
                if (m_mainDirectionIsInlineAxis) {
                    mainSize = basisWidth.specifiedValue(
                        m_availableMainSize, m_layoutContext.viewportWidth());
                } else {
                    mainSize = basisWidth.specifiedValue(
                        m_availableMainSize, m_layoutContext.viewportHeight());
                }
            }
        }

        if (mainSize == intMaxForLayoutUnit) {
            // E. Otherwise, size the item into the available space using
            // its used flex basis in place of its main size, treating a
            // value of content as max-content. If a cross size is needed
            // to determine the main size (e.g. when the flex item’s main
            // size is in its block axis) and the flex item’s cross size is
            // auto and not definite, in this calculation use fit-content
            // as the flex item’s cross size.
            if (m_mainDirectionIsInlineAxis) {
                PreferredWidthContext p(m_layoutContext, m_availableMainSize);
                flexItemBox->computePreferredWidth(p);
                mainSize = p.preferredWidth();
            } else {
                PreferredWidthContext p(m_layoutContext, m_availableCrossSize);
                flexItemBox->computePreferredWidth(p);
                mainSize = p.preferredWidth();
            }
        }

        if (!m_mainDirectionIsInlineAxis) {
            flexItemBox->setContentWidth(mainSize);
            flexItemBox->layout(m_layoutContext,
                                Frame::LayoutWantToResolve::ResolveHeight);
            mainSize = flexItemBox->contentHeight();
        }

        registerBasisSize(flexItemBox, mainSize);
        STARFISH_ASSERT(mainSize != intMaxForLayoutUnit);

        if (m_mainDirectionIsInlineAxis) {
            flexItemBox->applyMinMaxWidthIfNeeds(
                mainSize, m_availableMainSize, m_layoutContext.viewportWidth());
            sumOfMainSize += flexItemBox->boxWidth();
        } else {
            bool parentHasFixedHeight =
                m_layoutContext.parentHasFixedHeight(flexItemBox);
            flexItemBox->applyMinMaxHeightIfNeeds(
                mainSize, m_availableMainSize, m_layoutContext.viewportHeight(),
                parentHasFixedHeight);
            sumOfMainSize += flexItemBox->boxHeight();
        }

        child = child->next();
    }

    if (!m_mainDirectionIsInlineAxis) {
        m_container->computeContentHeight(m_layoutContext, sumOfMainSize);
        m_availableMainSize = m_container->contentHeight();
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
            if (m_mainDirectionIsInlineAxis) {
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
            if (m_mainDirectionIsInlineAxis) {
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

void FlexFormattingContext::applyFlexFactor(LayoutUnit lineWidth)
{
    bool usingGrowFactor = lineWidth < m_availableMainSize;

    FlexLine& flexLine = m_flexLines[m_currentLineIdx];
    std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
    LayoutUnit initialFreeSpace = m_availableMainSize - lineWidth;
    float sumOfFactor = 0;
    bool allFrozen = true;
    LayoutUnit scaledFlexShrinkFactor;

    for (size_t i = 0; i < flexItems.size(); i++) {
        FrameBox* flexItem = flexItems[i];
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
        return;
    }

    LayoutUnit remainingFreeSpace = initialFreeSpace;
    if (sumOfFactor < 1) {
        remainingFreeSpace = initialFreeSpace * sumOfFactor;
    }

    if (remainingFreeSpace == 0) {
        return;
    }

    for (size_t i = 0; i < flexItems.size(); i++) {
        FrameBox* flexItem = flexItems[i];
        if (flexItem->isAbsolutePositioned()) {
            continue;
        }

        if (usingGrowFactor) {
            float factor = flexItem->style()->flexGrow();
            if (factor != 0) {
                LayoutUnit mainSize =
                    basisSize(flexItem) +
                    (factor / sumOfFactor) * remainingFreeSpace;
                if (m_mainDirectionIsInlineAxis) {
                    flexItem->setContentWidth(mainSize);
                    flexItem->layout(m_layoutContext,
                                     Frame::LayoutWantToResolve::ResolveHeight);
                } else {
                    flexItem->setContentHeight(mainSize);
                }
            }
        } else {
            float factor = flexItem->style()->flexShrink();
            if (factor != 0) {
                LayoutUnit mainSize =
                    basisSize(flexItem) +
                    (factor * basisSize(flexItem) / scaledFlexShrinkFactor) *
                        remainingFreeSpace;
                if (m_mainDirectionIsInlineAxis) {
                    flexItem->setContentWidth(mainSize);
                    flexItem->layout(m_layoutContext,
                                     Frame::LayoutWantToResolve::ResolveHeight);
                } else {
                    flexItem->setContentHeight(mainSize);
                }
            }
        }
    }
}

void FlexFormattingContext::resolveMainMargin()
{
    FlexLine& flexLine = m_flexLines[m_currentLineIdx];
    std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
    LayoutUnit sumOfMainSize;
    size_t autoMarginCnt = 0;

    for (size_t i = 0; i < flexItems.size(); i++) {
        FrameBox* flexItem = flexItems[i];

        if (flexItem->isAbsolutePositioned()) {
            continue;
        }

        if (m_mainDirectionIsInlineAxis) {
            if (sumOfMainSize <= m_availableMainSize &&
                flexItem->style()->marginLeft().isAuto()) {
                autoMarginCnt++;
            }
            sumOfMainSize += flexItem->boxWidth();
            if (sumOfMainSize <= m_availableMainSize &&
                flexItem->style()->marginRight().isAuto()) {
                autoMarginCnt++;
            }
        } else {
            if (sumOfMainSize <= m_availableMainSize &&
                flexItem->style()->marginTop().isAuto()) {
                autoMarginCnt++;
            }
            sumOfMainSize += flexItem->boxHeight();
            if (sumOfMainSize <= m_availableMainSize &&
                flexItem->style()->marginBottom().isAuto()) {
                autoMarginCnt++;
            }
        }
    }

    if (m_availableMainSize > sumOfMainSize && autoMarginCnt > 0) {
        LayoutUnit margin =
            (m_availableMainSize - sumOfMainSize) / autoMarginCnt;
        for (size_t i = 0; i < flexItems.size(); i++) {
            FrameBox* flexItem = flexItems[i];

            if (flexItem->isAbsolutePositioned()) {
                continue;
            }

            if (m_mainDirectionIsInlineAxis) {
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

void FlexFormattingContext::applyJustifyContent()
{
    FlexLine& flexLine = m_flexLines[m_currentLineIdx];
    std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
    LayoutUnit sumOfMainSize;

    for (size_t i = 0; i < flexItems.size(); i++) {
        FrameBox* flexItem = flexItems[i];
        if (m_mainDirectionIsInlineAxis) {
            sumOfMainSize += flexItem->boxWidth();
        } else {
            sumOfMainSize += flexItem->boxHeight();
        }
    }

    LayoutUnit offset;
    LayoutUnit separator;
    JustifyContentValue justifyContent = m_container->style()->justifyContent();
    FlexDirectionValue flexDirection = m_container->style()->flexDirection();
    switch (justifyContent) {
    case JustifyContentValue::FlexStartJustifyContentValue:
        offset = 0;
        break;
    case JustifyContentValue::FlexEndJustifyContentValue:
        offset = m_availableMainSize - sumOfMainSize;
        break;
    case JustifyContentValue::CenterJustifyContentValue:
        offset = (m_availableMainSize - sumOfMainSize) / 2;
        break;
    case JustifyContentValue::SpaceAroundJustifyContentValue:
        separator =
            (m_availableMainSize - sumOfMainSize) / (flexItems.size() + 1);
        offset = separator;
        break;
    case JustifyContentValue::SpaceBetweenJustifyContentValue:
        if (flexItems.size() > 1) {
            separator =
                (m_availableMainSize - sumOfMainSize) / (flexItems.size() - 1);
        }
        break;
    }

    if (m_mainDirectionIsInlineAxis) {
        if (ltrDirection()) {
            LayoutUnit x =
                offset + m_container->borderLeft() + m_container->paddingLeft();
            for (size_t i = 0; i < flexItems.size(); i++) {
                FrameBox* flexItem = flexItems[i];
                if (flexItem->isAbsolutePositioned()) {
                    flexItem->setX(x);
                    STARFISH_ASSERT(flexItems.size() == 1);
                } else {
                    flexItem->setX(x + flexItem->marginLeft());
                }
                x += flexItem->boxWidth() + separator;
            }
        } else {
            LayoutUnit x = m_availableMainSize - offset +
                           m_container->borderLeft() +
                           m_container->paddingLeft();
            for (size_t i = 0; i < flexItems.size(); i++) {
                FrameBox* flexItem = flexItems[i];
                if (flexItem->isAbsolutePositioned()) {
                    flexItem->setX(x - flexItem->boxWidth());
                } else {
                    flexItem->setX(x - flexItem->boxWidth() +
                                   flexItem->marginLeft());
                }
                x -= flexItem->boxWidth() + separator;
            }
        }
    } else {
        LayoutUnit y =
            offset + m_container->borderTop() + m_container->paddingTop();
        for (size_t i = 0; i < flexItems.size(); i++) {
            FrameBox* flexItem = flexItems[i];
            if (flexItem->isAbsolutePositioned()) {
                flexItem->setY(y);
                STARFISH_ASSERT(flexItems.size() == 1);
            } else {
                flexItem->setY(y + flexItem->marginTop());
            }
            y += flexItem->boxHeight() + separator;
        }
    }
}

void FlexFormattingContext::layoutMain(LayoutUnit lineWidth)
{
    if (m_currentLineIdx == SIZE_MAX) {
        return;
    }

    applyFlexFactor(lineWidth);
    resolveMainMargin();
    applyJustifyContent();
}

void FlexFormattingContext::resolveMainSize()
{
    std::vector<FrameBox*> orderedFlexItems;
    LayoutUnit lineMainSize;
    Frame* child = m_container->firstChild();
    FlexWrapValue flexWrap = m_container->style()->flexWrap();
    bool isWrap = (flexWrap == WrapFlexWrapValue) ||
                  (flexWrap == WrapReverseFlexWrapValue);

    while (child) {
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

        if (flexItem->isFrameBlockBox()) {
            FrameBlockBox* blockBox = flexItem->asFrameBlockBox();
            if (blockBox->firstChild() &&
                blockBox->firstChild() == blockBox->lastChild() &&
                blockBox->firstChild()->isFrameText() &&
                blockBox->firstChild()
                    ->asFrameText()
                    ->text()
                    ->containsOnlyWhitespace()) {
                iter++;
                continue;
            }
        }

        if (flexItem->isAbsolutePositioned()) {
            m_layoutContext.registerAbsolutePositionedBox(flexItem);
            addNewLine(lineMainSize);
            FlexLine& newFlexLine = m_flexLines[m_currentLineIdx];
            std::vector<FrameBox*>& newFlexItems = newFlexLine.m_flexItems;
            newFlexItems.push_back(flexItem);
            newFlexLine.m_hasAbsolutePositionedBox = true;
            addNewLine(0);
            lineMainSize = 0;
            iter++;
            continue;
        }

        if (!isWrap || (lineMainSize == 0) ||
            ((m_mainDirectionIsInlineAxis &&
              lineMainSize + flexItem->boxWidth() <= m_availableMainSize) ||
             (!m_mainDirectionIsInlineAxis &&
              lineMainSize + flexItem->boxHeight() <= m_availableMainSize))) {
            flexItems.push_back(flexItem);
            if (m_mainDirectionIsInlineAxis) {
                lineMainSize += flexItem->boxWidth();
            } else {
                lineMainSize += flexItem->boxHeight();
            }
        } else {
            addNewLine(lineMainSize);
            lineMainSize = 0;
            continue;
        }

        iter++;
    }

    layoutMain(lineMainSize);
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
        LayoutUnit ascender = 0;
        LayoutUnit descender = 0;
        LayoutUnit maxHypotheticalCrossSize = 0;

        for (size_t j = 0; j < flexItems.size(); j++) {
            FrameBox* flexItem = flexItems[j];

            if (m_mainDirectionIsInlineAxis) {
                flexItem->layout(m_layoutContext,
                                 Frame::LayoutWantToResolve::ResolveHeight);
            }

            if (flexItem->isAbsolutePositioned()) {
                STARFISH_ASSERT(flexLine.m_hasAbsolutePositionedBox);
                flexLine.m_lineHeight = m_availableCrossSize;
                continue;
            }

            if (flexItem->style()->alignSelf() == BaselineAlignItemValue &&
                ((m_mainDirectionIsInlineAxis && flexItem->marginTop() != 0 &&
                  flexItem->marginBottom() != 0) ||
                 (!m_mainDirectionIsInlineAxis && flexItem->marginLeft() != 0 &&
                  flexItem->marginRight() != 0))) {
                // TODO
            } else {
                if (m_mainDirectionIsInlineAxis) {
                    maxHypotheticalCrossSize = std::max(
                        maxHypotheticalCrossSize, flexItem->boxHeight());
                } else {
                    maxHypotheticalCrossSize = std::max(
                        maxHypotheticalCrossSize, flexItem->boxWidth());
                }
            }

            if (flexItem->style()->alignSelf() == StretchAlignItemValue) {
                if ((m_mainDirectionIsInlineAxis &&
                     flexItem->style()->height().isAuto() &&
                     !flexItem->style()->marginTop().isAuto() &&
                     !flexItem->style()->marginBottom().isAuto()) ||
                    (!m_mainDirectionIsInlineAxis &&
                     flexItem->style()->width().isAuto() &&
                     !flexItem->style()->marginLeft().isAuto() &&
                     !flexItem->style()->marginRight().isAuto())) {
                    flexItemsToStretchInfos.emplace_back(i, flexItem);
                }
            }
        }

        if (!flexLine.m_hasAbsolutePositionedBox) {
            flexLine.m_lineHeight = maxHypotheticalCrossSize;
            sumOfCrossSize += maxHypotheticalCrossSize;
        }
    }

    if (m_container->style()->alignContent() == StretchAlignContentValue &&
        m_availableCrossSize != intMaxForLayoutUnit &&
        sumOfCrossSize < m_availableCrossSize) {
        LayoutUnit amountToStretchByLine =
            (m_availableCrossSize - sumOfCrossSize) / lines;
        for (size_t i = 0; i < lines; i++) {
            FlexLine& flexLine = m_flexLines[i];
            if (!flexLine.m_hasAbsolutePositionedBox) {
                flexLine.m_lineHeight += amountToStretchByLine;
            }
        }
    }

    for (size_t i = 0; i < flexItemsToStretchInfos.size(); i++) {
        auto& flexItemsToStretchInfo = flexItemsToStretchInfos[i];
        size_t lineIdx = flexItemsToStretchInfo.first;
        FrameBox* flexItem = flexItemsToStretchInfo.second;

        if (m_mainDirectionIsInlineAxis) {
            if (m_flexLines[lineIdx].m_lineHeight < flexItem->boxHeight()) {
                continue;
            }
            Length old = flexItem->style()->height();
            LayoutUnit oldContentWidth = flexItem->contentWidth();
            flexItem->style()->setHeight(
                Length(Length::Fixed, m_flexLines[lineIdx].m_lineHeight -
                                          flexItem->marginHeight()));
            flexItem->layout(m_layoutContext,
                             Frame::LayoutWantToResolve::ResolveHeight);
            flexItem->style()->setHeight(old);
            flexItem->setContentWidth(oldContentWidth);
        } else {
            if (m_flexLines[lineIdx].m_lineHeight < flexItem->boxWidth()) {
                continue;
            }
            Length old = flexItem->style()->width();
            LayoutUnit oldContentHeight = flexItem->contentHeight();
            flexItem->style()->setWidth(
                Length(Length::Fixed, m_flexLines[lineIdx].m_lineHeight -
                                          flexItem->marginWidth()));
            flexItem->layout(m_layoutContext,
                             Frame::LayoutWantToResolve::ResolveAll);
            flexItem->style()->setWidth(old);
            flexItem->setContentHeight(oldContentHeight);
        }
    }

    layoutCross();
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

            if (m_mainDirectionIsInlineAxis) {
                if (lineCrossSize > item->boxHeight()) {
                    LayoutUnit margin = lineCrossSize - item->boxHeight();
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
                if (lineCrossSize > item->boxWidth()) {
                    LayoutUnit margin = lineCrossSize - item->boxWidth();
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
            case FlexStartAlignItemValue:
                offset = 0;
                break;
            case FlexEndAlignItemValue:
                if (m_mainDirectionIsInlineAxis) {
                    offset = crossSize - flexItem->boxHeight();
                } else {
                    offset = crossSize - flexItem->boxWidth();
                }
                break;
            case CenterAlignItemValue:
                if (m_mainDirectionIsInlineAxis) {
                    offset = (crossSize - flexItem->boxHeight()) / 2;
                } else {
                    offset = (crossSize - flexItem->boxWidth()) / 2;
                }
                break;
            case BaselineAlignItemValue:
                // TODO
                break;
            case StretchAlignItemValue:
                offset = 0;
                break;
            }

            if (m_mainDirectionIsInlineAxis) {
                if (flexItem->isAbsolutePositioned()) {
                    flexItem->setY(offset);
                    STARFISH_ASSERT(flexItems.size() == 1);
                } else {
                    flexItem->setY(offset + flexItem->marginTop());
                }
            } else {
                if (ltrDirection()) {
                    if (flexItem->isAbsolutePositioned()) {
                        flexItem->setX(offset);
                        STARFISH_ASSERT(flexItems.size() == 1);
                    } else {
                        flexItem->setX(offset + flexItem->marginLeft());
                    }
                } else {
                    if (flexItem->isAbsolutePositioned()) {
                        flexItem->setX(lineCrossSize - offset -
                                       flexItem->boxWidth());
                        STARFISH_ASSERT(flexItems.size() == 1);
                    } else {
                        flexItem->setX(lineCrossSize - offset -
                                       flexItem->boxWidth() +
                                       flexItem->marginLeft());
                    }
                }
            }
        }
    }
}

void FlexFormattingContext::applyAlignContent(LayoutUnit sumOfCrossSize)
{
    size_t lines = m_currentLineIdx + 1;
    LayoutUnit offset;
    LayoutUnit separator;
    AlignContentValue alignContent = m_container->style()->alignContent();
    FlexDirectionValue flexDirection = m_container->style()->flexDirection();
    FlexWrapValue flexWrap = m_container->style()->flexWrap();
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
        separator = (m_availableCrossSize - sumOfCrossSize) / (lines + 1);
        offset = separator;
        break;
    case SpaceBetweenAlignContentValue:
        if (lines > 1) {
            separator = (m_availableCrossSize - sumOfCrossSize) / (lines - 1);
        }
        break;
    case StretchAlignContentValue:
        offset = 0;
        break;
    }

    if (m_mainDirectionIsInlineAxis) {
        if (flexWrap != WrapReverseFlexWrapValue) {
            LayoutUnit y =
                offset + m_container->borderTop() + m_container->paddingTop();
            for (size_t i = 0; i < lines; i++) {
                FlexLine& flexLine = m_flexLines[i];
                std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* item = flexItems[j];
                    if (item->isNormalFlow()) {
                        item->moveY(y);
                    }
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
                    if (item->isNormalFlow()) {
                        item->moveY(y - item->boxHeight());
                    }
                }
                y -= flexLine.m_lineHeight + separator;
            }
        }
    } else {
        if (ltrDirection()) {
            LayoutUnit x =
                offset + m_container->paddingLeft() + m_container->borderLeft();
            for (size_t i = 0; i < lines; i++) {
                FlexLine& flexLine = m_flexLines[i];
                std::vector<FrameBox*>& flexItems = flexLine.m_flexItems;
                for (size_t j = 0; j < flexItems.size(); j++) {
                    FrameBox* item = flexItems[j];
                    if (item->isNormalFlow()) {
                        item->moveX(x);
                    }
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
                    if (item->isNormalFlow()) {
                        item->moveX(x - flexLine.m_lineHeight);
                    }
                }
                x -= flexLine.m_lineHeight + separator;
            }
        }
    }
}

void FlexFormattingContext::layoutCross()
{
    resolveCrossMargin();
    applyAlignSelf();

    size_t lines = m_currentLineIdx + 1;
    LayoutUnit sumOfCrossSize;

    for (size_t i = 0; i < lines; i++) {
        FlexLine& flexLine = m_flexLines[i];
        sumOfCrossSize += flexLine.m_lineHeight;
    }

    if (m_mainDirectionIsInlineAxis) {
        m_container->computeContentHeight(m_layoutContext, sumOfCrossSize);
    }

    applyAlignContent(sumOfCrossSize);
}

FrameFlexibleBox::FrameFlexibleBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}

LayoutUnit FrameFlexibleBox::layoutFlex(LayoutContext& ctx)
{
    if (!isNecessaryBlockBox()) {
        return LayoutUnit(0);
    }

    FlexFormattingContext flexFormattingContext(ctx, this);

    flexFormattingContext.computeHypotheticalMainSize();
    flexFormattingContext.resolveMainSize();
    flexFormattingContext.computeCrossSize();

    return contentHeight();
}
}
