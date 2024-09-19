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

#ifndef __StarfishSVGMarkerElement__
#define __StarfishSVGMarkerElement__

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/svg/SVGElement.h"
#include "core/dom/svg/SVGAngle.h"
#include "core/dom/svg/SVGAnimatedAngle.h"
#include "core/dom/svg/SVGAnimatedEnumeration.h"

namespace Starfish {

class SVGMarkerElement : public SVGElement {
public:
    SVGMarkerElement(Document* document, const QualifiedName& qname);

    enum UNIT {
        SVG_MARKERUNTIS_UNKNOWN = 0,
        SVG_MARKERUNITS_USERSPACEONUSE,
        SVG_MARKERUNITS_STROKEWIDTH
    };

    enum ORIENT {
        SVG_MARKER_ORIENT_UNKNOWN = 0,
        SVG_MARKER_ORIENT_AUTO,
        SVG_MARKER_ORIENT_ANGLE
    };

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isSVGMarkerElement() const override;

    virtual void didAttributeChanged(QualifiedName name, Nullable<String*> old,
                                     String* value, bool attributeCreated,
                                     bool attributeRemoved) override;

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    SVGAnimatedEnumeration* markerUnits();
    SVGAnimatedAngle* orientAngle();
    SVGAnimatedEnumeration* orientType();

    void setOrientToAuto();
    void setOrientToAngle(SVGAngle* angle);

private:
    SVGAnimatedEnumeration* m_markerUnits;
    SVGAnimatedAngle* m_orientAngle;
    SVGAnimatedEnumeration* m_orientType;
};
} // namespace Starfish

#endif
