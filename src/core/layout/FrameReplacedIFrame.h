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

#ifndef __StarFishFrameReplacedIFrame__
#define __StarFishFrameReplacedIFrame__

#include "core/layout/FrameReplaced.h"

namespace StarFish {

class FrameReplacedIFrame : public FrameReplaced {
public:
    FrameReplacedIFrame(Node* node)
        : FrameReplaced(node, nullptr)
    {
        computeStyleFlags();
    }

    virtual void computeStyleFlags()
    {
        FrameReplaced::computeStyleFlags();
    }

    virtual bool isFrameReplacedIFrame()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameReplacedIFrame";
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat);

    virtual void paintReplaced(Canvas* canvas);

    virtual IntrinsicSize intrinsicSize();

    virtual void willCompsiteStackingContext(Canvas* c)
    {
    }

    virtual void didCompsiteStackingContext(Canvas* c)
    {
    }

    virtual void compsitingStackingContext(Canvas* c)
    {
    }

    virtual void establishesStackingContextIfNeeds();

protected:
};
}

#endif
