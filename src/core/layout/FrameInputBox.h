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

#ifndef __StarFishFrameInputBox__
#define __StarFishFrameInputBox__

#include "core/layout/FrameBlockBox.h"

namespace StarFish {

class FrameTreeBuilderContext;
class ComputedStyle;

class FrameInputBox : public FrameBlockBox {
public:
    FrameInputBox(Node* node, ComputedStyle* style);
    static FrameInputBox* buildFrameTree(Node* current,
                                         FrameTreeBuilderContext& ctx,
                                         bool force);

    virtual const char* name()
    {
        return "FrameInputBox";
    }

    virtual bool isFrameInputBox()
    {
        return true;
    }

private:
    static ComputedStyle* createInputElementStyleFrom(Node* parent);
};
}

#endif
