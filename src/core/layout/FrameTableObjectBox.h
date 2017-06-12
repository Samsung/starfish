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

#ifndef __StarFishFrameTableObjectBox__
#define __StarFishFrameTableObjectBox__

#include "core/layout/FrameBlockBox.h"

namespace StarFish {

class FrameTreeBuilderContext;

// FrameTableObjectBox is an abstract class where
// common table-related methods are implemented

class FrameTableObjectBox : public FrameBlockBox {
public:
    FrameTableObjectBox(Node* node, ComputedStyle* style);
    virtual const char* name() = 0;
    virtual bool isFrameTableObjectBox()
    {
        return true;
    }
    virtual void addChild(Node* child, FrameTreeBuilderContext& ctx,
                          bool force) = 0;
    bool bgColorFromAttribute(Unit::Color* ret);

protected:
    void paintBorders(Canvas* canvas, LayoutRect& rect);
};
}

#endif
