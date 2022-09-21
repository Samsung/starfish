/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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
#include "Internal.h"
#include "core/modules/serviceworker/util/ParallelTask.h"
#include "core/modules/serviceworker/util/Trace.h"

namespace Starfish {

Internal::Internal(ScriptBindingInstance* scriptBindingInstance)
    : ScriptWrappable(this)
    , m_scriptBindingInstance(scriptBindingInstance)
{
}

Promise* Internal::open(String* name)
{
    class CacheOpenTask : public ParallelTask {
    public:
        CacheOpenTask(Promise* p)
            : promise_(p)
        {
        }

        void run() override
        {
            TRACE_SCOPE(INTERNAL);
            // TODO: Call openning a cache entry
            // TODO: Set the result
            result = true;
        }

        void end() override
        {
            TRACE_SCOPE(INTERNAL, "result", result);
            auto sucessResult = scriptUndefined();
            promise_->fulfill(sucessResult);
        }

    private:
        bool result{ false };
        Promise* promise_;
    };

    TRACE_SCOPE(INTERNAL);

    auto promise = new Promise(m_scriptBindingInstance);
    auto task = new CacheOpenTask(promise);

    task->start();

    return promise;
}

} // namespace Starfish
