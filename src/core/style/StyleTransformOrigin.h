/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishStyleTransformOrigin__
#define __StarFishStyleTransformOrigin__

#include "core/style/Style.h"

namespace StarFish {

class TransformOriginData : public gc {
public:
    TransformOriginData()
        : m_xaxis(Length(Length::Percent, 0.5f))
        , m_yaxis(Length(Length::Percent, 0.5f))
        , m_zaxis(Length())
    {
    }

    TransformOriginData(Length x, Length y, Length z)
        : m_xaxis(x)
        , m_yaxis(y)
        , m_zaxis(z)
    {
    }

    ~TransformOriginData()
    {
    }

    void setData(TransformOriginData* data)
    {
        m_xaxis = data->m_xaxis;
        m_yaxis = data->m_yaxis;
        m_zaxis = data->m_zaxis;
    }

    void setData(Length a, Length b, Length c)
    {
        m_xaxis = a;
        m_yaxis = b;
        m_zaxis = c;
    }

    Length getXAxis()
    {
        return m_xaxis;
    }

    Length getYAxis()
    {
        return m_yaxis;
    }

    Length getZAxis()
    {
        return m_zaxis;
    }

    TransformOriginData* getData()
    {
        return this;
    }

private:
    Length m_xaxis, m_yaxis, m_zaxis;
};

class StyleTransformOrigin : public gc {
public:
    StyleTransformOrigin()
        : m_originValue(NULL)
    {
    }

    ~StyleTransformOrigin()
    {
    }

    void setOriginValue(Length x, Length y, Length z)
    {
        if (!m_originValue) {
            m_originValue = new TransformOriginData(x, y, z);
        }
        m_originValue->setData(x, y, z);
    }

    TransformOriginData* originValue()
    {
        return m_originValue;
    }

    String* dumpString()
    {
        StringBuilder builder;
        builder.appendChar('(');
        builder.appendString(m_originValue->getXAxis().dumpString());
        builder.appendString(String::spaceString);
        builder.appendString(m_originValue->getYAxis().dumpString());
        builder.appendString(String::spaceString);
        builder.appendString(m_originValue->getXAxis().dumpString());
        builder.appendChar(')');

        return builder.finalize();
    }

    bool operator==(const StyleTransformOrigin& origin)
    {
        return m_originValue->getXAxis() == origin.m_originValue->getXAxis() &&
               m_originValue->getYAxis() == origin.m_originValue->getYAxis() &&
               m_originValue->getZAxis() == origin.m_originValue->getZAxis();
    }

private:
    TransformOriginData* m_originValue;
};
}
#endif /* STYLETRANSFORMORIGIN_H_ */
