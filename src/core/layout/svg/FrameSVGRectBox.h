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

#ifndef __StarFishFrameSVGRectBox__
#define __StarFishFrameSVGRectBox__

#include "core/layout/svg/FrameSVGBox.h"

namespace StarFish {

class FrameSVGRectBox : public FrameSVGBox {
public:
    FrameSVGRectBox(Node* node)
        : FrameSVGBox(node)
    {
    }

    virtual bool isFrameSVGRectBox()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameSVGRectBox";
    }

    virtual void paintSVG(PaintingContext& ctx)
    {
        ctx.m_canvas->beginPath();
        ctx.m_canvas->moveTo(0, 0);
        ctx.m_canvas->lineTo((float)width(), 0);
        ctx.m_canvas->lineTo((float)width(), (float)height());
        ctx.m_canvas->lineTo(0, (float)height());
        ctx.m_canvas->lineTo(0, 0);

        ctx.m_canvas->setColor(style()->fill().color());
        ctx.m_canvas->fillPreserve();

        ctx.m_canvas->setColor(style()->stroke().color());
        ctx.m_canvas->stroke();
    }

protected:
};
}

#endif
