#ifndef __StarFishTranslateTransformData__
#define __StarFishTranslateTransformData__

#include "style/Style.h"

namespace StarFish {

class TranslateTransform: public gc {
public:
    TranslateTransform(Length& tx, Length& ty)
        : m_tx(tx)
        , m_ty(ty)
    {
    }

    ~TranslateTransform()
    {
    }

    void setData(Length& a, Length& b)
    {
        m_tx = a;
        m_ty = b;
    }

    Length tx() { return m_tx; }
    Length ty() { return m_ty; }

    bool operator==(const TranslateTransform& o)
    {
        return this->m_tx == o.m_tx && this->m_ty == o.m_ty;
    }

    bool operator!=(const TranslateTransform& o)
    {
        return !operator==(o);
    }

    Length m_tx;
    Length m_ty;
};

}

#endif
