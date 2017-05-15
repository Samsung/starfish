/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_TIZEN_TV) && defined(STARFISH_ENABLE_AVPLAY)

#ifndef __StarFishWebApis__
#define __StarFishWebApis__

#include "binding/ScriptWrappable.h"
#include "binding/StarFishHoldable.h"

namespace StarFish {

class Avplay;
class StarFish;

class WebApis : public ScriptWrappable, public StarFishHoldable {
public:
    WebApis(StarFish* starFish);

    virtual void init(ScriptBindingInstance* instance) override
    {
        scriptObject()->set__proto__(
            fetchData(instance)->fnWebApis()->protoType());
    }

    virtual bool isWebApis() const override
    {
        return true;
    }

    Avplay* avplay()
    {
        return m_avplay;
    }

protected:
    Avplay* m_avplay;
};
}

#endif
#endif
