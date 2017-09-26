/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __ImageData__
#define __ImageData__

namespace StarFish {

class ImageData : public gc {
public:
    enum PreserveAspectRatioValue {
        None,
        xMinYMin,
        xMidYMin,
        xMaxYMin,
        xMinYMid,
        xMidYMid,
        xMaxYMid,
        xMinYMax,
        xMidYMax,
        xMaxYMax,
    };

    static ImageData* create(String* localImageSrc);
    static ImageData* create(const char* buf, size_t len);
    static ImageData* create(size_t width, size_t height);

    virtual size_t bufferSize() = 0;
    virtual uint8_t* data() = 0;
    virtual void clear() = 0;
    virtual void* unwrap() = 0;
    virtual size_t width() = 0;
    virtual size_t height() = 0;
    virtual size_t stride() = 0;
    virtual ~ImageData()
    {
    }

    PreserveAspectRatioValue preserveAspectRatioValue()
    {
        return m_preserveAspectRatioValue;
    }

    void setPreserveAspectRatioValue(PreserveAspectRatioValue v)
    {
        m_preserveAspectRatioValue = v;
    }

protected:
    ImageData()
    {
        m_preserveAspectRatioValue = None;
    }

    PreserveAspectRatioValue m_preserveAspectRatioValue;
};
}

#endif
