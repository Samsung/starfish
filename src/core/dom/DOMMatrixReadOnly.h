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

#ifndef __StarfishDOMMatrixReadOnly__
#define __StarfishDOMMatrixReadOnly__

#include "binding/ScriptWrappable.h"
#include "core/page/Serializer.h"
#include <SkMatrix44.h>
#include "binding/DOMStringOrSequenceUnion.h"

namespace Starfish {
class DOMMatrixReadOnly : public ScriptWrappable, public Serializable {
public:
    DOMMatrixReadOnly(ExecutionContext* executionContext);
    DOMMatrixReadOnly(ExecutionContext* executionContext,
                      DOMStringOrSequence value);
    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(DOMMatrixReadOnly)

    double a() const;
    double b() const;
    double c() const;
    double d() const;
    double e() const;
    double f() const;
    double m11() const;
    double m12() const;
    double m13() const;
    double m14() const;
    double m21() const;
    double m22() const;
    double m23() const;
    double m24() const;
    double m31() const;
    double m32() const;
    double m33() const;
    double m34() const;
    double m41() const;
    double m42() const;
    double m43() const;
    double m44() const;

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

    SkMatrix44& matrix()
    {
        return m_matrix;
    }

    virtual bool isSerializable() const override;
    virtual Serializable* toSerializable() const override;
    virtual SerializedData* serialize(SerializingMap& memory) override;
    virtual void deserialize(SerializedData* serialized,
                             DeserializingMap& memory) const override;

private:
    ExecutionContext* m_executionContext;
    SkMatrix44 m_matrix;
};
}
#endif
