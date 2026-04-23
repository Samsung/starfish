/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "Starfish.h"
#include "binding/ScriptWrappable.h"
#include "core/modules/profiling/Profiling.h"
#include "core/dom/ExecutionContext.h"
#include "core/extra/Performance.h"

namespace Starfish {

Performance::Performance(ExecutionContext* executionContext)
    : m_executionContext(executionContext)
    , m_performanceResourceTiming(
          new PerformanceResourceTiming(executionContext))
{
}

double Performance::now()
{
    return (longTickCount() - m_executionContext->createdTick()) / 1000.0;
}

double Performance::timeOrigin()
{
    return m_executionContext->createdTick() / 1000.0;
}

// Performance Timeline API implementation
// https://w3c.github.io/performance-timeline/

GCVector<PerformanceEntry*> Performance::getEntries()
{
    return m_entries;
}

GCVector<PerformanceEntry*> Performance::getEntriesByType(String* entryType)
{
    GCVector<PerformanceEntry*> result;
    for (size_t i = 0; i < m_entries.size(); i++) {
        if (m_entries[i]->entryType()->equals(entryType)) {
            result.push_back(m_entries[i]);
        }
    }
    return result;
}

GCVector<PerformanceEntry*> Performance::getEntriesByName(
    String* name, Optional<String*> entryType)
{
    GCVector<PerformanceEntry*> result;
    for (size_t i = 0; i < m_entries.size(); i++) {
        if (m_entries[i]->name()->equals(name)) {
            if (!entryType.hasValue() ||
                m_entries[i]->entryType()->equals(entryType.value())) {
                result.push_back(m_entries[i]);
            }
        }
    }
    return result;
}

// User Timing API implementation
// https://w3c.github.io/user-timing/

void Performance::mark(String* markName)
{
    // Create a PerformanceMark entry
    String* markString =
        executionContext()->starfish()->staticStrings()->m_mark.localName();
    double currentTime = now();
    PerformanceEntry* entry = PerformanceEntry::create(
        m_executionContext, markName, markString, currentTime, 0.0);
    addEntry(entry);
}

void Performance::measure(String* measureName, Optional<String*> startMark,
                          Optional<String*> endMark)
{
    double startTime = 0.0;
    double endTime = now();

    String* markString =
        executionContext()->starfish()->staticStrings()->m_mark.localName();

    // Find start mark if specified
    if (startMark.hasValue()) {
        GCVector<PerformanceEntry*> startEntries =
            getEntriesByName(startMark.value(), markString);
        if (startEntries.size() > 0) {
            startTime = startEntries.back()->startTime();
        }
    }

    // Find end mark if specified
    if (endMark.hasValue()) {
        GCVector<PerformanceEntry*> endEntries =
            getEntriesByName(endMark.value(), markString);
        if (endEntries.size() > 0) {
            endTime = endEntries.back()->startTime();
        }
    }

    double duration = endTime - startTime;

    PerformanceEntry* entry = PerformanceEntry::create(
        m_executionContext, measureName,
        String::createASCIIStringWithNoCopy("measure"), startTime, duration);
    addEntry(entry);
}

void Performance::clearMarks(Optional<String*> markName)
{
    if (!markName.hasValue()) {
        // Clear all marks
        GCVector<PerformanceEntry*> newEntries;
        for (size_t i = 0; i < m_entries.size(); i++) {
            if (!m_entries[i]->entryType()->equals("mark")) {
                newEntries.push_back(m_entries[i]);
            }
        }
        m_entries = newEntries;
    } else {
        // Clear specific mark
        GCVector<PerformanceEntry*> newEntries;
        for (size_t i = 0; i < m_entries.size(); i++) {
            if (!(m_entries[i]->entryType()->equals("mark") &&
                  m_entries[i]->name()->equals(markName.value()))) {
                newEntries.push_back(m_entries[i]);
            }
        }
        m_entries = newEntries;
    }
}

void Performance::clearMeasures(Optional<String*> measureName)
{
    if (!measureName.hasValue()) {
        // Clear all measures
        GCVector<PerformanceEntry*> newEntries;
        for (size_t i = 0; i < m_entries.size(); i++) {
            if (!m_entries[i]->entryType()->equals("measure")) {
                newEntries.push_back(m_entries[i]);
            }
        }
        m_entries = newEntries;
    } else {
        // Clear specific measure
        GCVector<PerformanceEntry*> newEntries;
        for (size_t i = 0; i < m_entries.size(); i++) {
            if (!(m_entries[i]->entryType()->equals("measure") &&
                  m_entries[i]->name()->equals(measureName.value()))) {
                newEntries.push_back(m_entries[i]);
            }
        }
        m_entries = newEntries;
    }
}

void Performance::clearResourceTimings()
{
    // Clear all resource timing entries
    GCVector<PerformanceEntry*> newEntries;
    for (size_t i = 0; i < m_entries.size(); i++) {
        if (!m_entries[i]->entryType()->equals("resource")) {
            newEntries.push_back(m_entries[i]);
        }
    }
    m_entries = newEntries;
}

void Performance::addEntry(PerformanceEntry* entry)
{
    m_entries.push_back(entry);
}

ScriptObject Performance::toJSON()
{
    ScriptBindingInstance* instance =
        m_executionContext->scriptBindingInstance();
    ScriptObject result = createEmptyScriptObject(instance);

    setScriptObjectProperty(
        instance, result,
        createScriptValue(String::createASCIIStringWithNoCopy("timeOrigin")),
        createScriptValue(timeOrigin()));

    return result;
}

} // namespace Starfish
