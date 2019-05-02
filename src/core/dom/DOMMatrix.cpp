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
#include "core/dom/DOMMatrix.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

DOMMatrix::DOMMatrix(ExecutionContext* executionContext)
    : DOMMatrixReadOnly(executionContext)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

DOMMatrix::DOMMatrix(ExecutionContext* executionContext,
                     DOMStringOrSequence value)
    : DOMMatrixReadOnly(executionContext, value)
{
    STARFISH_ASSERT(executionContext != nullptr);
}

void DOMMatrix::setA(double value)
{
    matrix().setDouble(0, 0, value);
}

void DOMMatrix::setB(double value)
{
    matrix().setDouble(1, 0, value);
}

void DOMMatrix::setC(double value)
{
    matrix().setDouble(0, 1, value);
}

void DOMMatrix::setD(double value)
{
    matrix().setDouble(1, 1, value);
}

void DOMMatrix::setE(double value)
{
    matrix().setDouble(0, 3, value);
}

void DOMMatrix::setF(double value)
{
    matrix().setDouble(1, 3, value);
}

void DOMMatrix::setM11(double value)
{
    matrix().setDouble(0, 0, value);
}

void DOMMatrix::setM12(double value)
{
    matrix().setDouble(1, 0, value);
}

void DOMMatrix::setM13(double value)
{
    matrix().setDouble(2, 0, value);
}

void DOMMatrix::setM14(double value)
{
    matrix().setDouble(3, 0, value);
}

void DOMMatrix::setM21(double value)
{
    matrix().setDouble(0, 1, value);
}

void DOMMatrix::setM22(double value)
{
    matrix().setDouble(1, 1, value);
}

void DOMMatrix::setM23(double value)
{
    matrix().setDouble(2, 1, value);
}

void DOMMatrix::setM24(double value)
{
    matrix().setDouble(3, 1, value);
}

void DOMMatrix::setM31(double value)
{
    matrix().setDouble(0, 2, value);
}

void DOMMatrix::setM32(double value)
{
    matrix().setDouble(1, 2, value);
}

void DOMMatrix::setM33(double value)
{
    matrix().setDouble(2, 2, value);
}

void DOMMatrix::setM34(double value)
{
    matrix().setDouble(3, 2, value);
}

void DOMMatrix::setM41(double value)
{
    matrix().setDouble(0, 3, value);
}

void DOMMatrix::setM42(double value)
{
    matrix().setDouble(1, 3, value);
}

void DOMMatrix::setM43(double value)
{
    matrix().setDouble(2, 3, value);
}

void DOMMatrix::setM44(double value)
{
    matrix().setDouble(3, 3, value);
}

ScriptBindingInstance* DOMMatrix::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

SerializedData* DOMMatrix::serialize(SerializingMap& memory)
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
    return nullptr;
}
void DOMMatrix::deserialize(SerializedData* serialized,
                            DeserializingMap& memory) const
{
    STARFISH_RELEASE_ASSERT_UNIMPLEMENTED();
}
}
