/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishStackingContext__
#define __StarFishStackingContext__

namespace StarFish {

class Canvas;
class CanvasSurface;
class Frame;
class FrameBox;
class Node;
class StackingContext;
class BrowsingContext;

class StackingContextChild : public GCVector<StackingContext*>, public gc {
};

struct VisibleRectContext {
    VisibleRectContext(FrameBox* box, LayoutLocation* loc);
    ~VisibleRectContext();

private:
    FrameBox* m_box;
    LayoutLocation* m_loc;
};

struct StackingContextRareData : public gc {
    bool m_needsOwnBuffer;
    LayoutRect m_visibleRect;
    CanvasSurface* m_buffer;
    SkMatrix m_matrix;

    StackingContextRareData();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;
};

class StackingContext : public gc {
public:
    StackingContext(FrameBox* owner, StackingContext* parent);

    const GCVector<StackingContextChild*>& childContexts()
    {
        return m_childContexts;
    }

    FrameBox* owner()
    {
        return m_owner;
    }

    StackingContext* parent()
    {
        return m_parent;
    }

    bool needsOwnBuffer()
    {
        return m_rareData ? m_rareData->m_needsOwnBuffer : false;
    }

    void clearOwnBuffer(bool needsDetachNative = true);

    CanvasSurface* buffer()
    {
        return m_rareData ? m_rareData->m_buffer : nullptr;
    }

    LayoutRect visibleRect()
    {
        return m_rareData ? m_rareData->m_visibleRect : LayoutRect(0, 0, 0, 0);
    }

    SkMatrix transformMatrix()
    {
        return m_rareData ? m_rareData->m_matrix : SkMatrix::I();
    }

    void unite(const LayoutRect& other)
    {
        STARFISH_ASSERT(m_rareData);
        m_rareData->m_visibleRect.unite(other);
    }

    bool computeStackingContextProperties(bool forceNeedsBuffer = false);

    void replaceCanvasState(Canvas* canvas, StackingContext* sCtx);
    void paintStackingContext(Canvas* canvas);
    void compositeStackingContext(Canvas* canvas);
    Frame* hitTestStackingContext(LayoutUnit x, LayoutUnit y,
                                  BrowsingContext* from);
    LayoutLocation relativeLocation(StackingContext* child);

    int32_t zIndex();

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    StackingContextRareData* ensureRareData();
    FrameBox* m_owner;
    StackingContext* m_parent;
    GCVector<StackingContextChild*> m_childContexts;
    StackingContextRareData* m_rareData;
};
}

#endif
