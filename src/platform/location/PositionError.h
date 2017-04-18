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

namespace StarFish {

class StarFish;

class PositionError : public ScriptWrappable {
public:
    enum Error { PERMISSION_DENIED = 1, POSITION_UNAVAILABLE = 2, TIMEOUT = 3 };

    PositionError(StarFish* starFish, Error code)
        : ScriptWrappable(this)
        , m_starFish(starFish)
        , m_code(code)
    {
    }

    StarFish* starFish()
    {
        return m_starFish;
    }

    Error code()
    {
        return m_code;
    }

    const char* message()
    {
        if (m_code == 1) {
            return "Permission denied";
        } else if (m_code == 2) {
            return "Position unavailable";
        } else if (m_code == 3) {
            return "Timeout expired";
        } else {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void initScriptObject(ScriptBindingInstance* instance)
    {
        initScriptWrappable(this);
    }

    virtual bool isPositionError() const override
    {
        return true;
    }

protected:
    StarFish* m_starFish;
    Error m_code;
};
}

#endif
