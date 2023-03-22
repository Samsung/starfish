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

#ifndef __StarfishRTCStatsReport__
#define __StarfishRTCStatsReport__

#include "core/dom/EventTarget.h"
#include "binding/ScriptWrappable.h"
#include "binding/Maplike.h"
#include <EscargotPublic.h>

namespace Starfish {

class RTCStatsReport : public ScriptWrappable,
                       public Maplike<String*, ScriptValue> {
public:
    RTCStatsReport(ExecutionContext* executionContext);
    virtual ~RTCStatsReport();

    DECLARE_SCRIPT_BINDING_REQUIRED_FUNCTIONS(RTCStatsReport)

    IterationSource<Nullable<String*>, Nullable<ScriptValue>>* startIteration(
        Escargot::ExecutionStateRef* state) override;

    virtual Nullable<ScriptValue> get(String* key) override;

    virtual void set(String* key, ScriptValue value) override;

    virtual bool has(String* key) override;

    virtual bool deleteItem(String* key) override;

    virtual void clear() override
    {
        // readonly
    }

private:
    ExecutionContext* m_executionContext = nullptr;
    GCVector<std::pair<String*, ScriptValue>> m_data;
};

} // namespace Starfish
#endif
#endif
