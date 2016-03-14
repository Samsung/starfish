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

    double m_angleX;
    double m_angleY;
};

}

#endif
