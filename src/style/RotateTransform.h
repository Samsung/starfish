#ifndef __StarFishRotateTransformData__
#define __StarFishRotateTransformData__

#include "style/Style.h"

namespace StarFish {

class RotateTransform: public gc {
public:
    RotateTransform(double angle)
        : m_angle(angle)
    {
    }

    ~RotateTransform()
    {
    }

    void setData(double angle)
    {
        m_angle = angle;
    }

    double angle() { return m_angle; }

private:
    double m_angle;
};

}

#endif
