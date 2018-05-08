/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#ifndef __StarFishVTTCue__
#define __StarFishVTTCue__

#include "core/dom/TextTrackCue.h"

namespace StarFish {

// VTTCue
// : Only support "text"
// : Wrapper class of TextTrackCue
class VTTCue : public TextTrackCue {
public:
    VTTCue(Document* document, double start, double end, String* payload)
        : TextTrackCue(document, start, end, payload)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isVTTCue() const override;

    String* text()
    {
        return getPayload();
    }

    void setText(String* text)
    {
        setPayload(text);
    }
};
}

#endif
#endif // STARFISH_ENABLE_MULTIMEDIA
