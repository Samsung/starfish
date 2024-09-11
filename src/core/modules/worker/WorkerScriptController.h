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

#if defined(STARFISH_ENABLE_WORKER) && \
    !defined(__StarfishWorkerScriptController__)
#define __StarfishWorkerScriptController__

#include "StarfishBase.h"

namespace Starfish {

class ResourceRequest;
class ScriptBindingInstance;
class ScriptEngineInstance;

enum class ScriptLoadResult {
    NotHandled,
    Success,
    NetworkError,
    FileError,
    ScriptError,
};

class WorkerScriptController : public gc {
public:
    WorkerScriptController(ExecutionContext* executionContext);

    virtual ScriptLoadResult loadJavaScript(ResourceURL* resourceURL);

    bool evaluatefromString(String* string);

    ExecutionContext* executionContext()
    {
        return m_executionContext;
    }

    ScriptBindingInstance* scriptBindingInstance();

protected:
    template <typename ClientType, typename ControllerType>
    ScriptLoadResult loadJavaScriptInternal(ResourceURL* resourceURL,
                                            ControllerType* controller);

private:
    ExecutionContext* m_executionContext;
};
} // namespace Starfish

#endif
