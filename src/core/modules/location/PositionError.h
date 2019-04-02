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

#ifndef __StarfishPositionError__
#define __StarfishPositionError__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class PositionError : public ScriptWrappable {
public:
    enum Error { PERMISSION_DENIED = 1, POSITION_UNAVAILABLE = 2, TIMEOUT = 3 };

    PositionError(ExecutionContext* executionContext, Error code);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(PositionError)

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

protected:
    ScriptBindingInstance* m_scriptBindingInstance;
    Error m_code;
};
}

#endif
