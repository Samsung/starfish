/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishInternalMatrixTransform__
#define __StarFishInternalMatrixTransform__

#include "core/style/Style.h"

namespace StarFish {

class InternalMatrixTransform : public gc {
public:
    InternalMatrixTransform(const SkMatrix& matrix)
        : m_matrix(matrix)
    {
    }

    bool operator==(const InternalMatrixTransform& o)
    {
        return this->m_matrix == o.m_matrix;
    }

    bool operator!=(const InternalMatrixTransform& o)
    {
        return !operator==(o);
    }

    void setMatrix(const SkMatrix& m)
    {
        m_matrix = m;
    }

    const SkMatrix& matrix()
    {
        return m_matrix;
    }

private:
    SkMatrix m_matrix;
};
}

#endif
