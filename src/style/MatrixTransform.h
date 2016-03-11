#ifndef __StarFishMatrixTransformData__
#define __StarFishMatrixTransformData__

#include "style/Style.h"

namespace StarFish {

class MatrixTransform: public gc {
public:
    MatrixTransform(double a, double b, double c, double d, double e, double f)
        : m_a(a)
        , m_b(b)
        , m_c(c)
        , m_d(d)
        , m_e(e)
        , m_f(f)
    {
    }

    ~MatrixTransform()
    {
    }

    void setData(double a, double b, double c, double d, double e, double f)
    {
        m_a = a;
        m_b = b;
        m_c = c;
        m_d = d;
        m_e = e;
        m_f = f;
    }

    double a() { return m_a; }
    double b() { return m_b; }
    double c() { return m_c; }
    double d() { return m_d; }
    double e() { return m_e; }
    double f() { return m_f; }

    double m_a;
    double m_b;
    double m_c;
    double m_d;
    double m_e;
    double m_f;
};

}

#endif
