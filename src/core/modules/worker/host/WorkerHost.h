/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WORKER) && !defined(__StarfishWorkerHost__)
#define __StarfishWorkerHost__

namespace Starfish {

class RunLoop;
class WebWorker;
class WorkerGlobalScope;
class WorkerThread;

class WorkerHost : public gc {
public:
    WorkerHost(WorkerThread* workerThread, RunLoop* runLoop);

    static void run(void* data);

    DEFINE_GETTER(WebWorker*, webWorker);
    DEFINE_GETTER(WorkerGlobalScope*, globalScope);

protected:
    WebWorker* m_webWorker;
    WorkerGlobalScope* m_globalScope;
    bool m_wasDisposed;

    void dispose();
};

} // namespace Starfish

#endif
