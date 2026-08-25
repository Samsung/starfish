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

#if defined(STARFISH_WEBWORKER_HOST)

#ifndef __LWEWorkerDelegate__
#define __LWEWorkerDelegate__

#include "LWEDelegateContract.h"

namespace LWE {
enum class WorkerProcessState;
}

namespace LWEDelegate {

class EXPORT_UNMANAGED_API LWEWorker {
public:
    static void Initialize(const std::string& storageDirectoryPath);

    static void RegisterOnStatusChangedHandler(
        const std::function<void(::LWE::WorkerProcessState)>& cb);

    static void Finalize();
};

} // namespace LWEDelegate

// C wrappers used for dlopen/dlsym.
extern "C" {
void EXPORT_UNMANAGED_API
LWEWorkerDelegate_LWEWorker_Initialize(const std::string& storageDirectoryPath);

void EXPORT_UNMANAGED_API
LWEWorkerDelegate_LWEWorker_RegisterOnStatusChangedHandler(
    const std::function<void(::LWE::WorkerProcessState)>& cb);

void EXPORT_UNMANAGED_API LWEWorkerDelegate_LWEWorker_Finalize();

typedef struct {
    void (*Initialize)(const std::string&);
    void (*RegisterOnStatusChangedHandler)(
        const std::function<void(::LWE::WorkerProcessState)>& cb);
    void (*Finalize)();
} LWEWorkerProcTable;
}

#endif
#endif
