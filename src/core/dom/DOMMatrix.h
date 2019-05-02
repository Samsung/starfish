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

#ifndef __StarfishDOMMatrix__
#define __StarfishDOMMatrix__

#include <SkMatrix44.h>
#include "core/dom/DOMMatrixReadOnly.h"
#include "binding/DOMStringOrSequenceUnion.h"

namespace Starfish {
class DOMMatrix : public DOMMatrixReadOnly {
public:
    DOMMatrix(ExecutionContext* executionContext);
    DOMMatrix(ExecutionContext* executionContext, DOMStringOrSequence value);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(DOMMatrix)

    void setA(double value);
    void setB(double value);
    void setC(double value);
    void setD(double value);
    void setE(double value);
    void setF(double value);
    void setM11(double value);
    void setM12(double value);
    void setM13(double value);
    void setM14(double value);
    void setM21(double value);
    void setM22(double value);
    void setM23(double value);
    void setM24(double value);
    void setM31(double value);
    void setM32(double value);
    void setM33(double value);
    void setM34(double value);
    void setM41(double value);
    void setM42(double value);
    void setM43(double value);
    void setM44(double value);

    ExecutionContext* executionContext()
    {
        return DOMMatrixReadOnly::executionContext();
    }

    SkMatrix44& matrix()
    {
        return DOMMatrixReadOnly::matrix();
    }

    virtual bool isSerializable() const override;
    virtual Serializable* toSerializable() const override;
    virtual SerializedData* serialize(SerializingMap& memory) override;
    virtual void deserialize(SerializedData* serialized,
                             DeserializingMap& memory) const override;
};
}
#endif
