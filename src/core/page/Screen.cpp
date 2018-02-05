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
#include "StarFish.h"
#include "core/page/Screen.h"
#include "platform/public/ScreenInfo.h"

namespace StarFish {

int32_t Screen::availWidth() const
{
    return starFish()->screenInfo().availableRect.size().width();
}

int32_t Screen::availHeight() const
{
    return starFish()->screenInfo().availableRect.size().height();
}

int32_t Screen::width() const
{
    return starFish()->screenInfo().rect.size().width();
}

int32_t Screen::height() const
{
    return starFish()->screenInfo().rect.size().height();
}

uint32_t Screen::colorDepth() const
{
    return starFish()->screenInfo().depth;
}

uint32_t Screen::pixelDepth() const
{
    return starFish()->screenInfo().depthPerComponent;
}

float Screen::devicePixelRatio() const
{
    // TODO: consider page zoom factor.
    return starFish()->screenInfo().deviceScaleFactor;
}

bool Screen::isMonochrome() const
{
    return starFish()->screenInfo().isMonochrome;
}
}
