#ifndef __StarFishScaleTransformData__
#define __StarFishScaleTransformData__

#include "style/Style.h"

namespace StarFish {

class ScaleTransform: public gc {
public:
    ScaleTransform(double a, double b)
        : m_x(a)
        , m_y(b)
    {
    }

    ~ScaleTransform()
    {
    }

    void setData(double a, double b)
    {
        m_x = a;
        m_y = b;
    }

    double x() { return m_x; }
    double y() { return m_y; }

    double m_x;
    double m_y;
};

}

#endif
