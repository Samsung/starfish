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

#ifndef __StarFishFrameTableCaptionBox__
#define __StarFishFrameTableCaptionBox__

#include "core/layout/FrameTableObjectBox.h"

namespace StarFish {

class FrameTreeBuilderContext;

class FrameTableCaptionBox : public FrameTableObjectBox {
public:
    FrameTableCaptionBox(Node* node, ComputedStyle* style);

    virtual void addChild(Node* child, FrameTreeBuilderContext& ctx,
                          bool force);
    virtual const char* name()
    {
        return "FrameTableCaption";
    }

    virtual bool isFrameTableCaptionBox()
    {
        return true;
    }

    void layoutWidth(LayoutContext& ctx);

    LayoutUnit minCaptionWidth()
    {
        return m_minCaptionWidth;
    }

    LayoutUnit maxCaptionWidth()
    {
        return m_maxCaptionWidth;
    }

private:
    LayoutUnit m_minCaptionWidth;
    LayoutUnit m_maxCaptionWidth;
};
}

#endif
