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

    bool operator==(const RotateTransform& o)
    {
        return this->m_angle == o.m_angle;
    }

    bool operator!=(const RotateTransform& o)
    {
        return !operator==(o);
    }

private:
    double m_angle;
};

}

#endif
