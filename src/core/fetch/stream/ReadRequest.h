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

#ifndef __StarfishReadRequest__
#define __StarfishReadRequest__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class ScriptBindingInstance;

class ReadRequest : public gc {
public:
    virtual void chunkSteps(ScriptBindingInstance* instance,
                            ScriptValue chunk) = 0;
    virtual void closeSteps(ScriptBindingInstance* instance,
                            ScriptValue value) = 0;
    virtual void errorSteps(ScriptBindingInstance* instance,
                            ScriptValue error) = 0;
};

} // namespace Starfish

#endif
