/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishAbortController__
#define __StarfishAbortController__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class AbortSignal;

class AbortController : public ScriptWrappable {
public:
    AbortController(ExecutionContext* executionContext);
    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(AbortController)

    AbortSignal* signal() const
    {
        return m_signal;
    }
    void abort(ScriptValue reason = scriptUndefined());

private:
    AbortSignal* m_signal;
};

} // namespace Starfish
#endif
