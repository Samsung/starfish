/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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
#ifndef __StarFishTextTrackCueList__
#define __StarFishTextTrackCueList__

#include "TextTrackCue.h"
#include "binding/DocumentHoldable.h"

namespace StarFish {

class TextTrackCueList : public ScriptWrappable,
                         public DocumentHoldable,
                         public GCVector<TextTrackCue*> {
public:
    TextTrackCueList(Document* document)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
        , GCVector<TextTrackCue*>()
    {
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isTextTrackCueList() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return DocumentHoldable::scriptBindingInstance();
    }

    uint32_t length()
    {
        return size();
    }
};
}

#endif
#endif // STARFISH_ENABLE_MULTIMEDIA
