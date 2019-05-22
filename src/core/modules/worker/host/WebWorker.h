/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && !defined(__StarfishWebWorker__)
#define __StarfishWebWorker__

#include "core/page/WebBase.h"

namespace Starfish {

class WorkerGlobalScope;
class ScriptEngineInstance;
class ServiceWorkerServer;
class ServiceWorkerContextManager;

class WebWorker : public WebBase {
public:
    static WebWorker* create(Starfish* starfish, const char* locale,
                             const char* timezoneID,
                             String* customUserAgentString);

    virtual ~WebWorker();

    void destory();

    void setNeedsRendering() override
    {
    }

    uint64_t lastRenderingTick() override
    {
        return 0;
    }

#if defined(STARFISH_ENABLE_INSPECTOR)
    Inspector* inspector() const override
    {
        return nullptr;
    }
#endif

    ScriptEngineInstance* scriptEngineInstance()
    {
        return m_scriptEngineInstance;
    }

    void loadJavaScript(const std::string& scriptURL,
                        const std::string& baseURL);

private:
    WebWorker(Starfish* starfish, const char* locale, const char* timezoneID,
              String* customUserAgentString);

    WorkerGlobalScope* m_workerGlobalScope{ nullptr };
    ScriptEngineInstance* m_scriptEngineInstance{ nullptr };
    ServiceWorkerServer* m_SWServer{ nullptr };
    ServiceWorkerContextManager* m_SWContextManager{ nullptr };

    void createScriptEngineInstance();
    void removeScriptEngineInstance();
};

} // namespace Starfish

#endif
