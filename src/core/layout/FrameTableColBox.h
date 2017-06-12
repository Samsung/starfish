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

#ifndef __StarFishFrameTableColBox__
#define __StarFishFrameTableColBox__

#include "core/layout/FrameTableObjectBox.h"

namespace StarFish {

class FrameTreeBuilderContext;

// The FrameTableColBox is for <col> and <colgroup>
// In the specification, <col> and <colgroup> are very similar
// The only difference is that <col> should not have children
// So we will use A appropriately for <col> and <colgroup>
class FrameTableColBox : public FrameTableObjectBox {
public:
    FrameTableColBox(Node* node, ComputedStyle* style);

    virtual const char* name()
    {
        return "FrameTableColBox";
    }

    virtual bool isFrameTableColBox()
    {
        return true;
    }
    virtual void addChild(Node* child, FrameTreeBuilderContext& ctx,
                          bool force);
    unsigned span();

private:
    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat);
    virtual void paint(PaintingContext& ctx);
};
}

#endif
