/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishRepaintRegionTracker__
#define __StarfishRepaintRegionTracker__

#include "core/page/RenderResult.h"

namespace Starfish {

class FrameBox;
class StackingContext;

class RepaintRegionTracker {
public:
    class ComputeOverflow {
    public:
        ComputeOverflow(RepaintRegionTracker& tracker, FrameBox* frame,
                        const SkMatrix& matrix);
        ~ComputeOverflow();

    private:
        RepaintRegionTracker& tracker;
        FrameBox* frame;
    };

    class ClearNeedsPainting {
    public:
        ClearNeedsPainting(FrameBox* frame);
        ~ClearNeedsPainting();

    private:
        FrameBox* frame;
    };

    RepaintRegionTracker(
        const RepaintRegionTrackerContext& oldContext,
        RepaintRegionTrackerContext& newContext, FrameBox* rootFrame,
        bool needsFullPainting,
        PrevDrawnStackingContextInfoMap& prevDrawnStackingContextInfoMap,
        LayoutUnit sx, LayoutUnit sy, bool wc);

    const RepaintRegion& repaintRegion();
    ~RepaintRegionTracker();
    void notifyDirty(FrameBox* frame, StackingContext* sc,
                     const SkMatrix& currentMatrix, LayoutRect r);
    FrameBox* findNearestStackingContextOwner(FrameBox* frame);

protected:
    bool m_willCompositing;
    bool m_needsFullPainting;
    const RepaintRegionTrackerContext& m_oldContext;
    RepaintRegionTrackerContext& m_newContext;
    std::unordered_map<Node*, LayoutRect> m_repaintRegionPerGraphicsLayer;
    std::vector<std::tuple<LayoutRect, StackingContext*>>
        m_boundMaxExtentDueToOverflow;
    LayoutRect m_screenRect;
    PrevDrawnStackingContextInfoMap& m_prevDrawnStackingContextInfoMap;

    void trackRepaintRegion(FrameBox* frame, SkMatrix currentMatrix);
};
}

#endif
