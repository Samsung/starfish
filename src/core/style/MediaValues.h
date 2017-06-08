/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
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

#ifndef __StarFishMediaValues__
#define __StarFishMediaValues__

#include "StarFishConfig.h"
#include "core/style/Style.h"

namespace StarFish {

class Frame;
class MediaValues : public gc {
public:
    MediaValues(Frame* frame)
        : m_frame(frame)
    {
    }

    bool computeLength(double value, UnitType type, double& result);
    bool computeLength(double value, UnitType type, float defaultFontSize,
                       double viewportWidth, double viewportHeight,
                       double& reslt);

    int32_t viewportWidth() const;
    int32_t viewportHeight() const;
    int32_t screenWidth() const;
    int32_t screenHeight() const;
    float devicePixelRatio() const;
    int32_t colorBitsPerComponent() const;
    bool isMonochrome() const;

private:
    Frame* m_frame;
};

} /* namespace StarFish */

#endif /* __StarFishMediaValues__ */
