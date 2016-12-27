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

#ifndef __StarFishFrameTable__
#define __StarFishFrameTable__

#include "layout/FrameBlockBox.h"

namespace StarFish {

class FrameTableCaption;
class FrameTreeBuilderContext;

class FrameTable : public FrameBlockBox {
public:
    FrameTable(Node* node, ComputedStyle* style);

    static FrameTable* buildFrameTable(Node* tableNode,
                                       FrameTreeBuilderContext& ctx,
                                       bool force);

    virtual void layout(LayoutContext& ctx, Frame::LayoutWantToResolve resolveWhat);

    virtual bool isFrameTable()
    {
        return true;
    }

    virtual const char* name()
    {
        return "FrameTable";
    }

    virtual bool hasBlockFlow()
    {
        Frame* child = firstChild();

        if (!child) {
            return true;
        } else {
            DisplayValue display = child->style()->originalDisplay();
            return (display == BlockDisplayValue ||
                    display == TableCaptionDisplayValue) && child->isNormalFlow();
        }
    }

private:
    std::vector<FrameTableCaption*,
                gc_allocator_ignore_off_page<FrameTableCaption*>> m_captions;

};

}

#endif
