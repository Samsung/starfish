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

#include "StarFishConfig.h"
#include "ProgressEvent.h"

namespace StarFish {

ProgressEventInit::ProgressEventInit()
    : lengthComputable(false)
    , loaded(0)
    , total(0)
{
}

ProgressEventInit::ProgressEventInit(bool b, bool c, bool lengthComputable,
                                     unsigned long long loaded,
                                     unsigned long long total)
    : EventInit(b, c)
    , lengthComputable(lengthComputable)
    , loaded(loaded)
    , total(total)
{
}

ProgressEvent::ProgressEvent(String* eventType, const ProgressEventInit& init)
    : Event(eventType, init)
    , m_lengthComputable(init.lengthComputable)
    , m_loaded(init.loaded)
    , m_total(init.total)
{
    initScriptWrappable(this);
}
}
