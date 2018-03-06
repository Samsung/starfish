/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
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
