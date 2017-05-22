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

#ifndef __StarFishProgressEvent__
#define __StarFishProgressEvent__

#include "Event.h"

namespace StarFish {

struct ProgressEventInit : public EventInit {
public:
    ProgressEventInit();

    // For internal use
    ProgressEventInit(bool bubbles, bool cancelable, bool lengthComputable,
                      uint64_t loaded, uint64_t total);

    bool lengthComputable() const;
    void setLengthComputable(bool lengthComputable);

    uint64_t loaded() const;
    void setLoaded(uint64_t loaded);

    uint64_t total() const;
    void setTotal(uint64_t total);

private:
    bool m_lengthComputable;
    uint64_t m_loaded;
    uint64_t m_total;
};

class ProgressEvent : public Event {
public:
    ProgressEvent(String* eventType,
                  const ProgressEventInit& init = ProgressEventInit());

    bool lengthComputable() const
    {
        return m_lengthComputable;
    }
    unsigned long long loaded() const
    {
        return m_loaded;
    }
    unsigned long long total() const
    {
        return m_total;
    }

    /* Other methods (not in ProgressEvent interface) */

    virtual void init(ScriptBindingInstance* instance) override;
    virtual bool isProgressEvent() const override;

private:
    bool m_lengthComputable;
    unsigned long long m_loaded;
    unsigned long long m_total;
};
}

#endif
