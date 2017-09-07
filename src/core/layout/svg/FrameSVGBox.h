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

#ifndef __StarFishFrameSVGBox__
#define __StarFishFrameSVGBox__

#include "core/layout/FrameBox.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/ImageData.h"

namespace StarFish {

class FrameSVGBox : public FrameBox {
public:
    FrameSVGBox(Node* node)
        : FrameBox(node, nullptr)
    {
    }

    virtual bool isFrameSVGBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameSVGBox";
    }

    void resolvePosition(LayoutContext& ctx);
    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override;
    virtual void paint(PaintingContext& ctx);
    virtual void paintSVG(PaintingContext& ctx) = 0;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    virtual bool hasFrameTreeItemModel()
    {
        return true;
    }

    virtual FrameTreeItemModel* frameTreeItemModel()
    {
        return &m_treeItemModel;
    }

    FrameTreeItemModel m_treeItemModel;
};
}

#endif
