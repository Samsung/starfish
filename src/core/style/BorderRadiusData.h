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

#ifndef __StarFishBorderRadiusData__
#define __StarFishBorderRadiusData__

namespace StarFish {

class BorderRadiusData : public gc {
public:
    Length m_topLeftHorizontal;
    Length m_topLeftVertical;
    Length m_topRightHorizontal;
    Length m_topRightVertical;
    Length m_bottomRightHorizontal;
    Length m_bottomRightVertical;
    Length m_bottomLeftHorizontal;
    Length m_bottomLeftVertical;

    BorderRadiusData()
        : m_topLeftHorizontal(Length::Fixed, 0)
        , m_topLeftVertical(Length::Fixed, 0)
        , m_topRightHorizontal(Length::Fixed, 0)
        , m_topRightVertical(Length::Fixed, 0)
        , m_bottomRightHorizontal(Length::Fixed, 0)
        , m_bottomRightVertical(Length::Fixed, 0)
        , m_bottomLeftHorizontal(Length::Fixed, 0)
        , m_bottomLeftVertical(Length::Fixed, 0)
    {
    }

    bool operator==(const BorderRadiusData& o)
    {
        return this->m_topLeftHorizontal == o.m_topLeftHorizontal &&
               this->m_topLeftVertical == o.m_topLeftVertical &&
               this->m_topRightHorizontal == o.m_topRightHorizontal &&
               this->m_topRightVertical == o.m_topRightVertical &&
               this->m_bottomRightHorizontal == o.m_bottomRightHorizontal &&
               this->m_bottomRightVertical == o.m_bottomRightVertical &&
               this->m_bottomLeftHorizontal == o.m_bottomLeftHorizontal &&
               this->m_bottomLeftVertical == o.m_bottomLeftVertical;
    }

    bool operator!=(const BorderRadiusData& o)
    {
        return !operator==(o);
    }
};

} /* namespace StarFish */

#endif /* __StarFishBorderRadiusData__ */
