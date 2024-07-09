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

#pragma once

#ifdef STARFISH_ENABLE_WORKER

#include "core/util/GlobalOptions.h"
#include "core/util/debug/Trace.h"

// PATHS
#define PATH_TMP_DIR "/tmp"
#define PATH_IPC_DIR "/.ipc"
#define PATH_WORKER_DATA_DIR "/starfish-worker-data"
#define PATH_SERVICE_WORKER_IPC_DIR "/.ipc-service-worker"
#define PATH_SHARED_WORKER_IPC_DIR "/.ipc-shared-worker"

// NAMES
#define WORKER_IPC_PROCESS_NAME "ipc"

#endif
