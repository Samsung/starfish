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
#include "core/dom/Node.h"
#include "core/dom/HTMLImageElement.h"
#include "core/layout/FrameReplacedImage.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/image/ImageData.h"

namespace StarFish {
void FrameReplacedImage::paintReplaced(Canvas* canvas)
{
    ImageData* id = node()->asHTMLImageElement()->imageData();
    if (id) {
        canvas->drawImage(
            id,
            Unit::Rect(borderLeft() + paddingLeft(), borderTop() + paddingTop(),
                       width() - borderWidth() - paddingWidth(),
                       height() - borderHeight() - paddingHeight()));
    }
}

IntrinsicSize FrameReplacedImage::intrinsicSize()
{
    IntrinsicSize result;
    ImageData* id = node()->asHTMLImageElement()->imageData();
    if (id) {
        result.m_hasAspectRatio = true;
        result.m_isContentExists = true;
        result.m_intrinsicContentSize = LayoutSize(id->width(), id->height());
    } else {
        result.m_hasAspectRatio = false;
        result.m_isContentExists = false;
        result.m_intrinsicContentSize = LayoutSize(15, 15);
    }
    return result;
}
}
