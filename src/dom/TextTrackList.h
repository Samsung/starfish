/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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

    virtual void init(ScriptBindingInstance* instance) override;
    virtual void postInit(ScriptBindingInstance* instance) override;
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
};
}

#endif
#endif // STARFISH_ENABLE_MULTIMEDIA
