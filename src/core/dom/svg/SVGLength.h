/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishSVGLength__
#define __StarfishSVGLength__

#include "binding/DocumentHoldable.h"
#include "binding/ScriptWrappable.h"

namespace Starfish {

class SVGElement;

class SVGLength : public ScriptWrappable {
public:
    enum UnitType {
        SVG_LENGTHTYPE_UNKNOWN = 0,
        SVG_LENGTHTYPE_NUMBER,
        SVG_LENGTHTYPE_PERCENTAGE,
        SVG_LENGTHTYPE_EMS,
        SVG_LENGTHTYPE_EXS,
        SVG_LENGTHTYPE_PX,
        SVG_LENGTHTYPE_CM,
        SVG_LENGTHTYPE_MM,
        SVG_LENGTHTYPE_IN,
        SVG_LENGTHTYPE_PT,
        SVG_LENGTHTYPE_PC
    };

    SVGLength(SVGElement* sourceElement, QualifiedName targetAttribute);
    SVGLength(SVGElement* sourceElement, QualifiedName targetAttribute,
              unsigned short unitType, float value);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGLength() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    unsigned short unitType();
    void setUnitType(unsigned short unitType);
    float value();
    void setValue(float v);
    float valueInSpecifiedUnits(bool layoutIfNeeded = true);
    String* valueAsString();
    void setValueAsString(String* valueAsString,
                          bool fromElementDidAttributeChanged = false,
                          bool throwDOMExceptionOnFailure = true);
    void setValueInSpecifiedUnits(float v,
                                  bool fromElementDidAttributeChanged = false);

    void newValueSpecifiedUnits(unsigned short unitType,
                                float valueInSpecifiedUnits);
    void convertToSpecifiedUnits(unsigned short unitType);

    bool isReadOnly();
    void setReadOnly();
    void detach();
    void attach(SVGElement* sourceElement, QualifiedName targetAttribute);
    bool isDetached();
    bool hasSpecificValue();

protected:
    SVGElement* m_sourceElement;
    QualifiedName m_targetAttribute;

    unsigned short m_unitType;
    float m_valueInSpecifiedUnits;

    bool m_readOnly;
    bool m_hasSpecificValue;
};
} // namespace Starfish

#endif
