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

#ifndef __StarFishGeoposition__
#define __StarFishGeoposition__

#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"

namespace StarFish {

class Coordinates;

class Geoposition : public ScriptWrappable, public DocumentHoldable {
public:
    Geoposition(Document* document, Coordinates* c, DOMTimeStamp timestamp)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
        , m_coords(c)
        , m_timestamp(timestamp)
    {
    }

    Coordinates* coords()
    {
        return m_coords;
    }

    const DOMTimeStamp& timestamp()
    {
        return m_timestamp;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isGeoposition() const override;

    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return DocumentHoldable::scriptBindingInstance();
    }

protected:
    Coordinates* m_coords;
    DOMTimeStamp m_timestamp;
};
}

#endif
