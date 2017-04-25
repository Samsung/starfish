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

#include "ProgressEvent.h"

namespace StarFish {

ProgressEventInit::ProgressEventInit()
    : EventInit()
    , m_lengthComputable(false)
    , m_loaded(0)
    , m_total(0)
{
}

ProgressEventInit::ProgressEventInit(bool b, bool c, bool lengthComputable,
                                     uint64_t loaded, uint64_t total)
    : EventInit(b, c)
    , m_lengthComputable(lengthComputable)
    , m_loaded(loaded)
    , m_total(total)
{
}

bool ProgressEventInit::lengthComputable() const
{
    return m_lengthComputable;
}

void ProgressEventInit::setLengthComputable(bool lengthComputable)
{
    m_lengthComputable = lengthComputable;
}

uint64_t ProgressEventInit::loaded() const
{
    return m_loaded;
}

void ProgressEventInit::setLoaded(uint64_t loaded)
{
    m_loaded = loaded;
}

uint64_t ProgressEventInit::total() const
{
    return m_total;
}

void ProgressEventInit::setTotal(uint64_t total)
{
    m_total = total;
}

ProgressEvent::ProgressEvent(String* eventType, const ProgressEventInit& init)
    : Event(eventType, init)
    , m_lengthComputable(init.lengthComputable())
    , m_loaded(init.loaded())
    , m_total(init.total())
{
}
}
