/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "platform/canvas/Canvas.h"

namespace StarFish {
class Node;
class FrameBox;
class StackingContext;
class Canvas;
class CanvasSurface;
class Frame;

class StackingContextChild : public std::vector<StackingContext*, gc_allocator<StackingContext*> > , public gc {
};

class StackingContext : public gc {
public:
    StackingContext(FrameBox* owner, StackingContext* parent);
    ~StackingContext();

    void clearChildContexts()
    {
        m_childContexts.clear();
    }

    const std::map<int32_t, StackingContextChild*, std::less<int32_t>, gc_allocator<std::pair<uint32_t, StackingContextChild*> > >& childContexts()
    {
        return m_childContexts;
    }

    FrameBox* owner()
    {
        return m_owner;
    }

    bool needsOwnBuffer()
    {
        return m_needsOwnBuffer;
    }

    void clearOwnBuffer()
    {
        delete m_buffer;
        m_buffer = nullptr;
    }

    const LayoutRect& visibleRect()
    {
        return m_visibleRect;
    }

    bool computeStackingContextProperties(bool forceNeedsBuffer = false);

    void paintStackingContext(Canvas* canvas);
    void compositeStackingContext(Canvas* canvas);
    Frame* hitTestStackingContext(LayoutUnit x, LayoutUnit y);
protected:
    bool m_needsOwnBuffer: 1;
    bool m_ownerHasBuffer: 1;

    FrameBox* m_owner;
    LayoutRect m_visibleRect;
    StackingContext* m_parent;
    CanvasSurface* m_buffer;

    SkMatrix m_matrix;

    std::map<int32_t, StackingContextChild*, std::less<int32_t>, gc_allocator<std::pair<uint32_t, StackingContextChild*> > > m_childContexts;
};
}

#endif
