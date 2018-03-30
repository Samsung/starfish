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

#ifndef __StarFishMediaValues__
#define __StarFishMediaValues__

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
    bool hasScriptEngineInstance() const;

private:
    Frame* m_frame;
};

} /* namespace StarFish */

#endif /* __StarFishMediaValues__ */
