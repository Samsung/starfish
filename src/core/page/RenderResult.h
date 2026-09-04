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

#ifndef __StarfishRenderResult__
#define __StarfishRenderResult__

#include <SkMatrix.h>
#include "core/layout/LayoutUtil.h"

namespace Starfish {

class CanvasSurface;
class GraphicsBufferHolder;
class FrameBox;
class Node;

struct RenderInfo {
    void* updatedBufferAddress;
    size_t bufferStride;
    RenderInfo()
        : updatedBufferAddress(nullptr)
        , bufferStride(0)
    {
    }
};

struct RenderResult {
    bool didPaintingOrCompositing;
    LayoutRect updateRect;
    LayoutRect computedRepaintRect;
};

struct PrevDrawnStackingContextInfo {
    PrevDrawnStackingContextInfo()
    {
        needsGraphicsBuffer = hasThisLayerThisTime = isEqualsWithPrevDrawing =
            isVisibleBefore = false;
        graphicsBufferHolder = nullptr;
        graphicsLayerOwner = nullptr;
        opacity = 1;
        transformMatrix = SkMatrix::I();
        additionalPixelRatio = 1;
    }

    // flags for RepaintRegionTracker
    bool isEqualsWithPrevDrawing;
    bool hasThisLayerThisTime;
    bool needsGraphicsBuffer;
    bool isVisibleBefore;

    LayoutRect screenExtent;
    Node* graphicsLayerOwner;
    LayoutRect extentOnGraphicsLayer;

    SkMatrix transformMatrix;
    float opacity;
    GraphicsBufferHolder* graphicsBufferHolder;
    LayoutRect graphicsBufferVisibleRect;
    float additionalPixelRatio;
};

typedef GCUnorderedMap<Node*, PrevDrawnStackingContextInfo>
    PrevDrawnStackingContextInfoMap;

// Carries a stacking context's composed visibleRect across a full
// stacking-context re-establish (a frame-tree rebuild replaces every
// FrameBox, so the whole SC tree is recreated). Collected per node while
// the old tree is torn down, restored onto the freshly created context in
// the following properties pass when the owner's geometry is unchanged and
// the old context was not marked dirty by the mutation. Lives only within
// that one rendering pass.
struct PrevStackingContextVisibleRect {
    LayoutRect ownerFrameRect;
    LayoutRect visibleRect;
    LayoutRect visibleRectContentOnly;
    bool wasGraphicsBuffer{ false };
};

// Plain std map: entries live only inside one rendering pass (collected
// during the SC tree teardown, consumed by the following properties pass),
// and the Node keys are only compared, never dereferenced through the map.
typedef std::unordered_map<Node*, PrevStackingContextVisibleRect>
    PrevStackingContextVisibleRectMap;

typedef std::unordered_map<Node*, LayoutRect> RepaintRegion;

class RepaintRegionTrackerContext {
    friend class RepaintRegionTracker;

public:
    void clear()
    {
        std::unordered_map<const void*, LayoutRect>().swap(
            m_visibleRectOfFrameRectIsOverflowedBoxes);
    }

protected:
    // Keyed by the FrameBox, and also by its node (element or pseudo-element)
    // for a box the node owns outright -- see overflowedBoxNodeKey() in
    // RepaintRegionTracker.cpp. A style change can rebuild a subtree's
    // frames, and the next frame must still find what the old box painted
    // outside its rect (a box-shadow, an outline) to erase it; the FrameBox*
    // key dies with the old box, the node survives the rebuild.
    std::unordered_map<const void*, LayoutRect>
        m_visibleRectOfFrameRectIsOverflowedBoxes;
};
} // namespace Starfish

#endif
