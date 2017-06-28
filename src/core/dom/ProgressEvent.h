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

#ifndef __StarFishProgressEvent__
#define __StarFishProgressEvent__

#include "Event.h"

namespace StarFish {

struct ProgressEventInit : public EventInit {
    STARFISH_MAKE_STACK_ALLOCATED()
public:
    ProgressEventInit()
        : EventInit()
        , m_lengthComputable(false)
        , m_loaded(0)
        , m_total(0)
    {
    }

    bool lengthComputable() const
    {
        return m_lengthComputable;
    }

    void setLengthComputable(bool lengthComputable)
    {
        m_lengthComputable = lengthComputable;
    }

    uint64_t loaded() const
    {
        return m_loaded;
    }

    void setLoaded(uint64_t loaded)
    {
        m_loaded = loaded;
    }

    uint64_t total() const
    {
        return m_total;
    }

    void setTotal(uint64_t total)
    {
        m_total = total;
    }

private:
    bool m_lengthComputable;
    uint64_t m_loaded;
    uint64_t m_total;
};

class ProgressEvent : public Event {
public:
    ProgressEvent(Document* document, String* eventType)
        : Event(document, eventType)
        , m_lengthComputable(false)
        , m_loaded(0)
        , m_total(0)
    {
    }

    ProgressEvent(Document* document, String* eventType,
                  ProgressEventInit& init)
        : Event(document, eventType, init)
        , m_lengthComputable(init.lengthComputable())
        , m_loaded(init.loaded())
        , m_total(init.total())
    {
    }

    bool lengthComputable() const
    {
        return m_lengthComputable;
    }

    void setLengthComputable(bool lengthComputable)
    {
        m_lengthComputable = lengthComputable;
    }

    uint64_t loaded() const
    {
        return m_loaded;
    }

    void setLoaded(uint64_t loaded)
    {
        m_loaded = loaded;
    }

    uint64_t total() const
    {
        return m_total;
    }

    void setTotal(uint64_t total)
    {
        m_total = total;
    }

    /* Other methods (not in ProgressEvent interface) */

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isProgressEvent() const override;

private:
    bool m_lengthComputable;
    uint64_t m_loaded;
    uint64_t m_total;
};
}

#endif
