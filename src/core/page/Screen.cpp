/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
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
    return starFish()->screenInfo().depth;
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
