#ifndef STYLETRANSFORMORIGIN_H_
#define STYLETRANSFORMORIGIN_H_

#include "style/Style.h"

namespace StarFish {

class TransformOriginData : public gc {
public:
    TransformOriginData()
        : m_xaxis(Length(Length::Percent, 0.5f))
        , m_yaxis(Length(Length::Percent, 0.5f))
        , m_zaxis(Length())
    {
    }

    TransformOriginData(Length x, Length y)
        : m_xaxis(x)
        , m_yaxis(y)
        , m_zaxis(Length())
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

    void setData(Length a, Length b)
    {
        m_xaxis = a;
        m_yaxis = b;
        m_zaxis = Length();
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

    void setOriginValue(Length x, Length y)
    {
        if (!m_originValue)
            m_originValue = new TransformOriginData(x, y);
        m_originValue->setData(x, y);
    }

    TransformOriginData* originValue()
    {
        return m_originValue;
    }

    String* dumpString()
    {
        char temp[100];
        snprintf(temp, sizeof temp, "(%.3f %.3f)", m_originValue->getXAxis().isFixed() ? m_originValue->getXAxis().fixed() : m_originValue->getXAxis().percent()
            , m_originValue->getYAxis().isFixed() ? m_originValue->getYAxis().fixed() : m_originValue->getYAxis().percent());

        return String::fromUTF8(temp);
    }

    bool operator==(const StyleTransformOrigin& origin)
    {
        return m_originValue->getXAxis() == origin.m_originValue->getXAxis()
            && m_originValue->getYAxis() == origin.m_originValue->getYAxis()
            && m_originValue->getZAxis() == origin.m_originValue->getZAxis();
    }


private:
    TransformOriginData* m_originValue;
};

}
#endif /* STYLETRANSFORMORIGIN_H_ */
