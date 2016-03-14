#ifndef __StarFishSkewTransformData__
#define __StarFishSkewTransformData__

#include "style/Style.h"

namespace StarFish {

class SkewTransform: public gc {
public:
    SkewTransform(double anglex, double angley)
        : m_angleX(anglex)
        , m_angleY(angley)
    {
    }

    ~SkewTransform()
    {
    }

    void setData(double anglex, double angley)
    {
        m_angleX = anglex;
        m_angleY = angley;
    }

    double angleX() { return m_angleX; }
    double angleY() { return m_angleY; }

    bool operator==(const SkewTransform& o)
    {
        return this->m_angleX == o.m_angleX && this->m_angleY == o.m_angleY;
    }

    bool operator!=(const SkewTransform& o)
    {
        return !operator==(o);
    }

private:
    double m_angleX;
    double m_angleY;
};

}

#endif
