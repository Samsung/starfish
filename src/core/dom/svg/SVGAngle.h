/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishSVGAngle__
#define __StarfishSVGAngle__

#include "binding/DocumentHoldable.h"
#include "binding/ScriptWrappable.h"

namespace Starfish {

class SVGElement;

class SVGAngle : public ScriptWrappable {
public:
    enum UnitType {
        SVG_ANGLETYPE_UNKNOWN = 0,
        SVG_ANGLETYPE_UNSPECIFIED,
        SVG_ANGLETYPE_DEG,
        SVG_ANGLETYPE_RAD,
        SVG_ANGLETYPE_GRAD
    };

    SVGAngle(SVGElement* sourceElement, QualifiedName targetAttribute);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGAngle() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    unsigned short unitType();
    void setUnitType(unsigned short unitType);
    float value();
    void setValue(float v);
    float valueInSpecifiedUnits();
    void setValueInSpecifiedUnits(float v);
    String* valueAsString();
    void setValueAsString(String* valueAsString);

    void newValueSpecifiedUnits(unsigned short unitType,
                                float valueInSpecifiedUnits);
    void convertToSpecifiedUnits(unsigned short unitType);

    bool isUpdated();
    void unsetIsUpdated();

    void newValueSpecifiedUnitsWithoutUpdateAttribute(
        unsigned short unitType, float valueInSpecifiedUnits);
    void updateAttribute();

protected:
    SVGElement* m_sourceElement;
    QualifiedName m_targetAttribute;

    unsigned short m_unitType;
    float m_valueInSpecifiedUnits;

    bool m_isUpdated;
};
} // namespace Starfish

#endif
