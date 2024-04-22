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
    uint32_t additionalPixelRatio;
};

typedef GCUnorderedMap<Node*, PrevDrawnStackingContextInfo>
    PrevDrawnStackingContextInfoMap;

typedef std::unordered_map<Node*, LayoutRect> RepaintRegion;

class RepaintRegionTrackerContext {
    friend class RepaintRegionTracker;

public:
    void clear()
    {
        std::unordered_map<FrameBox*, LayoutRect>().swap(
            m_visibleRectOfFrameRectIsOverflowedBoxes);
    }

protected:
    std::unordered_map<FrameBox*, LayoutRect>
        m_visibleRectOfFrameRectIsOverflowedBoxes;
};
} // namespace Starfish

#endif
