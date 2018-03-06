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
#ifndef __StarFishTextTrackList__
#define __StarFishTextTrackList__

#include "TextTrack.h"

namespace StarFish {

class TextTrackList : public EventTarget, public GCVector<TextTrack*> {
public:
    TextTrackList(Document* document)
        : EventTarget(document)
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isTextTrackList() const override;

    // JS Bindings
    uint32_t length()
    {
        return size();
    }

    TextTrack* getTrackById(String* id)
    {
        auto iter = std::find_if(begin(), end(), [&id](TextTrack* track) {
            return track->id()->equals(id);
        });
        if (iter != end()) {
            return (*iter);
        }
        return nullptr;
    }

    TextTrack* defaultIndexedGetter(uint32_t idx)
    {
        return (*this)[idx];
    }
};
}

#endif
#endif // STARFISH_ENABLE_MULTIMEDIA
