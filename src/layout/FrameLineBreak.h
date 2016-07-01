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

#ifndef __StarFishFrameLineBreak__
#define __StarFishFrameLineBreak__

#include "layout/Frame.h"

namespace StarFish {

class FrameLineBreak : public Frame {
public:
    FrameLineBreak(Node* node)
        : Frame(node, nullptr)
    {

    }

    virtual bool isFrameLineBreak()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameLineBreak";
    }

    virtual void layout(LayoutContext& ctx)
    {
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
#ifdef STARFISH_ENABLE_TEST
    virtual void dump(int depth)
    {
        Frame::dump(depth);
    }
#endif

protected:
};

}

#endif
