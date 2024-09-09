/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_IDB)

#ifndef __StarfishIDBKeyRange__
#define __StarfishIDBKeyRange__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class IDBKey;

class IDBKeyRange : public ScriptWrappable {
public:
    static IDBKeyRange* convertValueToKeyRange(
        ExecutionContext* executionContext, ScriptValue value,
        bool nullDisallowed);

    IDBKeyRange(ExecutionContext* executionContext, IDBKey* lower,
                IDBKey* upper, bool isLowerOpen, bool isUpperOpen);

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(IDBKeyRange)

    DEFINE_GETTER(IDBKey*, lower);
    DEFINE_GETTER(bool, isOnly);

private:
    ExecutionContext* m_executionContext;
    IDBKey* m_lower;
    IDBKey* m_upper;
    bool m_isLowerOpen;
    bool m_isUpperOpen;
    bool m_isOnly;
};

} // namespace Starfish

#endif
#endif
