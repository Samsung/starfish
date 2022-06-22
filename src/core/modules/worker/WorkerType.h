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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && !defined(__StarfishWorkerType__)
#define __StarfishWorkerType__

#include "StarfishBase.h"

#include "core/util/String.h"

namespace Starfish {

enum class WorkerType : uint8_t {
    Classic,
    Module,
};

class WorkerTypeUtils {
public:
    static Optional<WorkerType> stringToWorkerType(String* workerType)
    {
        if (workerType->equals("classic")) {
            return WorkerType::Classic;
        } else if (workerType->equals("module")) {
            return WorkerType::Module;
        } else {
            STARFISH_LOG_WARN("Invalid WorkerType value");
        }

        return Optional<WorkerType>();
    }

    static String* workerTypeToString(WorkerType workerType)
    {
        switch (workerType) {
        case WorkerType::Classic:
            return String::createASCIIString("classic");
        case WorkerType::Module:
            return String::createASCIIString("module");
        }
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
};

} // namespace Starfish

#endif
