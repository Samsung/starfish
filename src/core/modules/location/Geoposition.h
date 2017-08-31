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

#ifndef __StarFishGeoposition__
#define __StarFishGeoposition__

#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"

namespace StarFish {

class Coordinates;

class Geoposition : public ScriptWrappable, public DocumentHoldable {
public:
    Geoposition(Document* document, Coordinates* c, DOMTimeStamp timestamp)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
        , m_coords(c)
        , m_timestamp(timestamp)
    {
    }

    Coordinates* coords()
    {
        return m_coords;
    }

    const DOMTimeStamp& timestamp()
    {
        return m_timestamp;
    }

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isGeoposition() const override;

    virtual ScriptBindingInstance* scriptBindingInstance()
    {
        return DocumentHoldable::scriptBindingInstance();
    }

protected:
    Coordinates* m_coords;
    DOMTimeStamp m_timestamp;
};
}

#endif
