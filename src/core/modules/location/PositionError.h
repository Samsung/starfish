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

#ifndef __StarFishPositionError__
#define __StarFishPositionError__

#include "binding/ScriptWrappable.h"
#include "binding/DocumentHoldable.h"

namespace StarFish {

class PositionError : public ScriptWrappable, public DocumentHoldable {
public:
    enum Error { PERMISSION_DENIED = 1, POSITION_UNAVAILABLE = 2, TIMEOUT = 3 };

    PositionError(Document* document, Error code)
        : ScriptWrappable(this)
        , DocumentHoldable(document)
        , m_code(code)
    {
    }

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

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isPositionError() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return DocumentHoldable::scriptBindingInstance();
    }

protected:
    Error m_code;
};
}

#endif
