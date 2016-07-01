/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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
