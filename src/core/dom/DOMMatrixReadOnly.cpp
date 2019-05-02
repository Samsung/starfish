/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "core/dom/DOMMatrixReadOnly.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

DOMMatrixReadOnly::DOMMatrixReadOnly(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    STARFISH_ASSERT(m_executionContext != nullptr);
    m_matrix.reset();
}

DOMMatrixReadOnly::DOMMatrixReadOnly(ExecutionContext* executionContext,
                                     DOMStringOrSequence value)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)
{
    STARFISH_ASSERT(m_executionContext != nullptr);
}

double DOMMatrixReadOnly::a() const
{
    return m_matrix.getDouble(0, 0);
}

double DOMMatrixReadOnly::b() const
{
    return m_matrix.getDouble(1, 0);
}

double DOMMatrixReadOnly::c() const
{
    return m_matrix.getDouble(0, 1);
}

double DOMMatrixReadOnly::d() const
{
    return m_matrix.getDouble(1, 1);
}

double DOMMatrixReadOnly::e() const
{
    return m_matrix.getDouble(0, 3);
}

double DOMMatrixReadOnly::f() const
{
    return m_matrix.getDouble(1, 3);
}

double DOMMatrixReadOnly::m11() const
{
    return m_matrix.getDouble(0, 0);
}

double DOMMatrixReadOnly::m12() const
{
    return m_matrix.getDouble(1, 0);
}

double DOMMatrixReadOnly::m13() const
{
    return m_matrix.getDouble(2, 0);
}

double DOMMatrixReadOnly::m14() const
{
    return m_matrix.getDouble(3, 0);
}

double DOMMatrixReadOnly::m21() const
{
    return m_matrix.getDouble(0, 1);
}

double DOMMatrixReadOnly::m22() const
{
    return m_matrix.getDouble(1, 1);
}

double DOMMatrixReadOnly::m23() const
{
    return m_matrix.getDouble(2, 1);
}

double DOMMatrixReadOnly::m24() const
{
    return m_matrix.getDouble(3, 1);
}

double DOMMatrixReadOnly::m31() const
{
    return m_matrix.getDouble(0, 2);
}

double DOMMatrixReadOnly::m32() const
{
    return m_matrix.getDouble(1, 2);
}

double DOMMatrixReadOnly::m33() const
{
    return m_matrix.getDouble(2, 2);
}

double DOMMatrixReadOnly::m34() const
{
    return m_matrix.getDouble(3, 2);
}

double DOMMatrixReadOnly::m41() const
{
    return m_matrix.getDouble(0, 3);
}

double DOMMatrixReadOnly::m42() const
{
    return m_matrix.getDouble(1, 3);
}

double DOMMatrixReadOnly::m43() const
{
    return m_matrix.getDouble(2, 3);
}

double DOMMatrixReadOnly::m44() const
{
    return m_matrix.getDouble(3, 3);
}

ScriptBindingInstance* DOMMatrixReadOnly::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

SerializedData* DOMMatrixReadOnly::serialize(SerializingMap& memory)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}
void DOMMatrixReadOnly::deserialize(SerializedData* serialized,
                                    DeserializingMap& memory) const
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}
}
