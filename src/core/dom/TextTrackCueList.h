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

#if defined(STARFISH_ENABLE_MULTIMEDIA)
#ifndef __StarfishTextTrackCueList__
#define __StarfishTextTrackCueList__

#include "binding/ScriptWrappable.h"

namespace Starfish {

class TextTrackCue;

class TextTrackCueList : public ScriptWrappable,
                         public GCVector<TextTrackCue*> {
public:
    TextTrackCueList(ExecutionContext* executionContext)
        : ScriptWrappable(this, executionContext)
        , GCVector<TextTrackCue*>()
    {
    }

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(TextTrackCueList)

    uint32_t length()
    {
        return size();
    }

    TextTrackCue* defaultIndexedGetter(uint32_t idx)
    {
        return (*this)[idx];
    }
};
}

#endif
#endif // STARFISH_ENABLE_MULTIMEDIA
