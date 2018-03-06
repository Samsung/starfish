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

#ifndef __StarFishPositionError__
#define __StarFishPositionError__

#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"

namespace StarFish {

class PositionError : public ScriptWrappable, public DocumentHoldable {
public:
    enum Error { PERMISSION_DENIED = 1, POSITION_UNAVAILABLE = 2, TIMEOUT = 3 };

    PositionError(Document* document, Error code)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
        , m_code(code)
    {
    }

    Error code() const
    {
        return m_code;
    }

    String* message() const
    {
        switch (m_code) {
        case PERMISSION_DENIED:
            return String::createASCIIString("Permission denied");
        case POSITION_UNAVAILABLE:
            return String::createASCIIString("Position unavailable");
        case TIMEOUT:
            return String::createASCIIString("Timeout expired");
        }

        STARFISH_ASSERT_NOT_REACHED();
        return String::emptyString;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isPositionError() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return DocumentHoldable::scriptBindingInstance();
    }

protected:
    Error m_code;
};
}

#endif
