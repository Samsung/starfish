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

#ifndef __StarFishFrameFlexibleBox__
#define __StarFishFrameFlexibleBox__

#include "core/layout/FrameBlockBox.h"

namespace StarFish {

class ComputedStyle;
class FrameBox;
class FrameFlexibleBox;
class LineBox;

struct FlexLine {
    std::vector<FrameBox*> m_flexItems;
    LayoutUnit m_lineWidth;
    LayoutUnit m_lineHeight;
    LayoutUnit m_maxAscender;

    FlexLine()
        : m_lineWidth(0)
        , m_lineHeight(0)
        , m_maxAscender(0)
    {
    }
};

class FlexFormattingContext {
public:
    FlexFormattingContext(LayoutContext& ctx, FrameFlexibleBox* container,
                          LayoutUnit availableWidth);

    void computeAvailableSpace(LayoutUnit availableWidth);

    LayoutUnit basisSize(FrameBox* flexItem);
    LayoutUnit sumOfUsedupMainSize(std::vector<FrameBox*>& flexItems,
                                   std::vector<bool> isFrozens);
    void computeMainSize();
    bool isMainSizeFlexible(FrameBox* flexItem, bool usingGrowFactor);
    void applyFlexFactor();
    void resolveMainMargin();
    void applyJustifyContent();
    void layoutMain();

    void computeCrossSize();
    void resolveCrossMargin();
    void applyAlignSelf();
    void applyAlignContent();
    void layoutCross();

    void addNewLine()
    {
        m_currentLineIdx++;
        m_flexLines.emplace_back(FlexLine());
    }

    bool isMainAxisInInlineAxis() const
    {
        return m_isMainAxisInInlineAxis;
    }

    bool isLtrDirection() const
    {
        return m_isLtrDirection;
    }

    bool isSingleLine() const
    {
        return m_isSingleLine;
    }

    static bool doesParticipateInFlexFormattingContext(Frame* flexItem);
    Nullable<LayoutUnit> firstLineBoxYPosition(FrameBox* flexItem) const;

private:
    LayoutContext& m_layoutContext;
    FrameFlexibleBox* m_container;
    bool m_isMainAxisInInlineAxis;
    bool m_isLtrDirection;
    bool m_isTtbDirection;
    bool m_isSingleLine;
    LayoutUnit m_availableMainSize;
    LayoutUnit m_availableCrossSize;
    size_t m_currentLineIdx;

    std::vector<FlexLine> m_flexLines;
    std::unordered_map<FrameBox*, LayoutUnit> m_basisSizes;
    std::unordered_map<FrameBox*, LayoutUnit> m_firstLineBoxYPositions;
};

class FrameFlexibleBox : public FrameBlockBox {
public:
    FrameFlexibleBox(Node* node, ComputedStyle* style);

    virtual const char* name() override
    {
        return "FrameFlexibleBox";
    }

    virtual bool isFrameFlexibleBox() override
    {
        return true;
    }

    virtual bool hasBlockFlow() override
    {
        return true;
    }

    LayoutUnit basisSize(LayoutContext& ctx, LayoutUnit availableMainSize,
                         LayoutUnit availableCrossSize, FrameBox* flexItem);
    bool isMainAxisInInlineAxis();
    bool isSingleLine();
    bool isLtrDirection();
    bool isTtbDirection();

    virtual void computePreferredWidth(PreferredWidthContext& ctx) override;
    void layoutFlex(LayoutContext& ctx);
};
}
#endif
