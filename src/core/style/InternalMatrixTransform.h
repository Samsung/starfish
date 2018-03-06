/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
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
