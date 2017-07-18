/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
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

#ifndef __StarFishScriptEngineInstance__
#define __StarFishScriptEngineInstance__

#include "binding/StarFishHoldable.h"

namespace Escargot {
class VMInstanceRef;
}

namespace StarFish {

typedef Escargot::VMInstanceRef* ScriptEngine;

class ScriptEngineInstance : public StarFishHoldable, public gc {
public:
    ScriptEngineInstance(StarFish* starFish);

    ScriptEngine engineInstance()
    {
        return m_engineInstance;
    }

    void close();

protected:
    ScriptEngine m_engineInstance;
};
}

#endif
