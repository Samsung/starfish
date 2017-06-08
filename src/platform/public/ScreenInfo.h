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

#ifndef __StarFishScreenInfo__
#define __StarFishScreenInfo__

#include "platform/public/ScreenOrientationType.h"
namespace StarFish {

struct ScreenInfo {
    // Device scale factor. Specifies the ratio between physical and logical
    // pixels.
    float deviceScaleFactor;

    // The screen depth in bits per pixel
    int depth;

    // The bits per colour component. This assumes that the colours are balanced
    // equally.
    int depthPerComponent;

    // This can be true for black and white printers
    bool isMonochrome;

    // This is set from the rcMonitor member of MONITORINFOEX, to whit:
    //   "A RECT structure that specifies the display monitor rectangle,
    //   expressed in virtual-screen coordinates. Note that if the monitor
    //   is not the primary display monitor, some of the rectangle's
    //   coordinates may be negative values."
    LayoutRect rect;

    // This is set from the rcWork member of MONITORINFOEX, to whit:
    //   "A RECT structure that specifies the work area rectangle of the
    //   display monitor that can be used by applications, expressed in
    //   virtual-screen coordinates. Windows uses this rectangle to
    //   maximize an application on the monitor. The rest of the area in
    //   rcMonitor contains system windows such as the task bar and side
    //   bars. Note that if the monitor is not the primary display monitor,
    //   some of the rectangle's coordinates may be negative values".
    LayoutRect availableRect;

    // This is the orientation 'type' or 'name', as in landscape-primary or
    // portrait-secondary for examples.
    // See WebScreenOrientationType.h for the full list.
    ScreenOrientationType orientationType;

    // This is the orientation angle of the displayed content in degrees.
    // It is the opposite of the physical rotation.
    int32_t orientationAngle;

    ScreenInfo()
        : deviceScaleFactor(1)
        , depth(24)
        , depthPerComponent(8)
        , isMonochrome(false)
        , rect(LayoutLocation(), LayoutSize())
        , availableRect(LayoutLocation(), LayoutSize())
        , orientationType(ScreenOrientationUndefined)
        , orientationAngle(0)
    {
    }

    bool operator==(const ScreenInfo& other) const
    {
        return this->deviceScaleFactor == other.deviceScaleFactor &&
               this->depth == other.depth &&
               this->depthPerComponent == other.depthPerComponent &&
               this->isMonochrome == other.isMonochrome &&
               this->rect.location().x() == other.rect.location().x() &&
               this->rect.location().y() == other.rect.location().y() &&
               this->rect.size() == other.rect.size() &&
               this->availableRect.location().x() ==
                   other.availableRect.location().x() &&
               this->availableRect.location().y() ==
                   other.availableRect.location().y() &&
               this->availableRect.size() == other.availableRect.size() &&
               this->orientationType == other.orientationType &&
               this->orientationAngle == other.orientationAngle;
    }

    bool operator!=(const ScreenInfo& other) const
    {
        return !this->operator==(other);
    }
};
}

#endif
