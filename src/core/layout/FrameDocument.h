/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishFrameDocument__
#define __StarFishFrameDocument__

#include "core/layout/FrameBlockBox.h"

namespace StarFish {

class FrameDocument : public FrameBlockBox {
    friend class FrameBlockBox;

public:
    FrameDocument(Node* node)
        : FrameBlockBox(node, nullptr)
        , m_countingOutdatedFlag(false)
        , m_quoteOutdatedFlag(false)
    {
    }

    virtual const char* name()
    {
        return "FrameDocument";
    }

    virtual bool isFrameDocument()
    {
        return true;
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;
    virtual void paintContent(PaintingContext& ctx) override;
    virtual Frame* hitTest(LayoutUnit x, LayoutUnit y, HitTestStage stage);

    virtual LayoutUnit scrollLeft() override
    {
        return m_scrollLeft;
    }

    virtual LayoutUnit scrollTop() override
    {
        return m_scrollTop;
    }

    bool scrollTo(LayoutUnit left, LayoutUnit top);

    void setCountingOutdatedFlag()
    {
        m_countingOutdatedFlag = true;
    }

    bool popCountingOutdatedFlag()
    {
        bool result = m_countingOutdatedFlag;
        m_countingOutdatedFlag = false;
        return result;
    }

    void setQuoteOutdatedFlag()
    {
        m_quoteOutdatedFlag = true;
    }

    bool popQuoteOutdatedFlag()
    {
        bool result = m_quoteOutdatedFlag;
        m_quoteOutdatedFlag = false;
        return result;
    }

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    LayoutUnit m_scrollLeft;
    LayoutUnit m_scrollTop;
    bool m_countingOutdatedFlag;
    bool m_quoteOutdatedFlag;
};
}

#endif
