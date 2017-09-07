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

#ifndef __StarFishFrameSVGSVGBox__
#define __StarFishFrameSVGSVGBox__

#include "core/layout/FrameReplaced.h"
#include "core/layout/svg/FrameSVGBox.h"

namespace StarFish {

class FrameSVGSVGBox : public FrameReplaced {
public:
    FrameSVGSVGBox(Node* node)
        : FrameReplaced(node, nullptr)
        , m_surface(nullptr)
    {
    }

    virtual bool isFrameSVGSVGBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameSVGSVGBox";
    }

    virtual void layout(LayoutContext& ctx,
                        Frame::LayoutWantToResolve resolveWhat) override
    {
        FrameReplaced::layout(ctx, resolveWhat);

        if (resolveWhat & Frame::LayoutWantToResolve::ResolveHeight) {
            Frame* f = firstChild();
            while (f) {
                f->asFrameSVGBox()->resolvePosition(ctx);
                f->asFrameSVGBox()->moveX(borderLeft() + paddingLeft());
                f->asFrameSVGBox()->moveY(borderTop() + paddingTop());
                f->layout(ctx, Frame::LayoutWantToResolve::ResolveAll);

                f = f->next();
            }
        }
    }
    virtual IntrinsicSize intrinsicSize() override;
    virtual void paintReplaced(Canvas* canvas) override;
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

protected:
    ImageData* m_surface;
};
}

#endif
