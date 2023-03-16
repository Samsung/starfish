/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WEBRTC)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "core/modules/mediastream/RTCStatsReport.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

class RTCStatsReportIterationSource final
    : public IterationSource<Nullable<String*>, Nullable<ScriptValue>> {
public:
    RTCStatsReportIterationSource(
        GCVector<std::pair<String*, ScriptValue>>& data)
        : m_data(data)
    {
        m_iterator = m_data.begin();
    }

    virtual bool next(Escargot::ExecutionStateRef* state,
                      Nullable<String*>& key,
                      Nullable<ScriptValue>& value) override
    {
        if (m_iterator == m_data.end()) {
            return false;
        }
        key = m_iterator->first;
        value = m_iterator->second;

        m_iterator++;
        return true;
    }

private:
    GCVector<std::pair<String*, ScriptValue>> m_data;
    GCVector<std::pair<String*, ScriptValue>>::iterator m_iterator;
};

RTCStatsReport::RTCStatsReport(ExecutionContext* executionContext)
    : ScriptWrappable(this)
    , m_executionContext(executionContext)

{
}

RTCStatsReport::~RTCStatsReport()
{
}

ScriptBindingInstance* RTCStatsReport::scriptBindingInstance()
{
    return m_executionContext->scriptBindingInstance();
}

IterationSource<Nullable<String*>, Nullable<ScriptValue>>*
RTCStatsReport::startIteration(Escargot::ExecutionStateRef* state)
{
    return new RTCStatsReportIterationSource(m_data);
}

Nullable<ScriptValue> RTCStatsReport::get(String* key)
{
    for (auto pair : m_data) {
        if (key->equals(pair.first)) {
            return pair.second;
        }
    }
    return nullptr;
}

bool RTCStatsReport::has(String* key)
{
    for (auto pair : m_data) {
        if (key->equals(pair.first)) {
            return true;
        }
    }
    return false;
}

} // namespace Starfish

#endif
