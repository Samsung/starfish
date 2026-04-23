/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#ifndef __StarfishPerformanceEntry__
#define __StarfishPerformanceEntry__

#include "binding/ScriptWrappable.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

// https://w3c.github.io/performance-timeline/#dom-performanceentry
class PerformanceEntry final : public ScriptWrappable {
public:
    // Factory method to create a PerformanceEntry
    static PerformanceEntry* create(ExecutionContext* executionContext,
                                    String* name, String* entryType,
                                    double startTime, double duration);

    PerformanceEntry(ExecutionContext* executionContext);

    virtual ScriptBindingInstance* scriptBindingInstance() override;

    void init(ScriptBindingInstance*, void*) override;
    bool isPerformanceEntry() const override;

    // PerformanceEntry interface properties
    String* name() const
    {
        return m_name;
    }
    String* entryType() const
    {
        return m_entryType;
    }
    double startTime() const
    {
        return m_startTime;
    }
    double duration() const
    {
        return m_duration;
    }

    // Setters for internal use
    void setName(String* name)
    {
        m_name = name;
    }
    void setEntryType(String* entryType)
    {
        m_entryType = entryType;
    }
    void setStartTime(double startTime)
    {
        m_startTime = startTime;
    }
    void setDuration(double duration)
    {
        m_duration = duration;
    }

    // toJSON
    ScriptObject toJSON();

private:
    ExecutionContext* m_executionContext;
    String* m_name;
    String* m_entryType;
    double m_startTime;
    double m_duration;
};
} // namespace Starfish

#endif
