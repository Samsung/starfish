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

#include "StarFishConfig.h"
#include "core/dom/HTMLElement.h"
#include "core/dom/HTMLTableElement.h"
#include "core/dom/HTMLTDElement.h"
#include "core/dom/HTMLTHElement.h"
#include "core/layout/FrameTableObjectBox.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/ImageData.h"

namespace StarFish {

FrameTableObjectBox::FrameTableObjectBox(Node* node, ComputedStyle* style)
    : FrameBlockBox(node, style)
{
}
}
