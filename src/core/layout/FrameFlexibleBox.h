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

#ifndef __StarFishFrameFlexibleBox__
#define __StarFishFrameFlexibleBox__

#include "core/layout/FrameBlockBox.h"

namespace StarFish {

class ComputedStyle;
class FrameBox;
class FrameFlexibleBox;

struct FlexLine {
    std::vector<FrameBox*> m_flexItems;
    LayoutUnit m_lineHeight;
    bool m_hasAbsolutePositionedBox;

    FlexLine()
        : m_lineHeight(0)
        , m_hasAbsolutePositionedBox(false)
    {
    }
};

class FlexFormattingContext {
public:
    FlexFormattingContext(LayoutContext& ctx, FrameFlexibleBox* container,
                          LayoutUnit availableWidth);

    void computeAvailableSpace(LayoutUnit availableWidth);

    void computeMainSize();
    void computeHypotheticalMainSize();
    void resolveMainSize();
    LayoutUnit basisSize(FrameBox* flexItem);
    bool isMainSizeFlexible(FrameBox* flexItem, bool usingGrowFactor);
    void applyFlexFactor(LayoutUnit lineWidth);
    void resolveMainMargin();
    void applyJustifyContent();
    void layoutMain(LayoutUnit lineWidth);

    void computeCrossSize();
    void resolveCrossMargin();
    void applyAlignSelf();
    void applyAlignContent();
    void layoutCross();

    void addNewLine(LayoutUnit lineWidth)
    {
        layoutMain(lineWidth);
        m_currentLineIdx++;
        m_flexLines.emplace_back(FlexLine());
    }

    bool isMainAxisInInlineAxis()
    {
        return m_isMainAxisInInlineAxis;
    }

    bool isLtrDirection()
    {
        return m_isLtrDirection;
    }

    bool isSingleLine()
    {
        return m_isSingleLine;
    }

    bool isAnonymousFlexItemContainingOnlyWhitespace(Frame* flexItem) const;

private:
    LayoutContext& m_layoutContext;
    FrameFlexibleBox* m_container;
    bool m_isMainAxisInInlineAxis;
    bool m_isLtrDirection;
    bool m_isSingleLine;
    LayoutUnit m_availableMainSize;
    LayoutUnit m_availableCrossSize;
    size_t m_currentLineIdx;

    std::vector<FlexLine> m_flexLines;
    std::unordered_map<FrameBox*, LayoutUnit> m_basisSizes;
};

class FrameFlexibleBox : public FrameBlockBox {
public:
    FrameFlexibleBox(Node* node, ComputedStyle* style);

    virtual const char* name()
    {
        return "FrameFlexibleBox";
    }

    virtual bool isFrameFlexibleBox()
    {
        return true;
    }

    virtual bool hasBlockFlow()
    {
        return true;
    }

    LayoutUnit basisSize(LayoutContext& ctx, LayoutUnit availableMainSize,
                         LayoutUnit availableCrossSize, FrameBox* flexItem);
    bool isMainAxisInInlineAxis();
    bool isSingleLine();
    bool isLtrDirection();

    LayoutUnit layoutFlex(LayoutContext& ctx);
};
}
#endif
