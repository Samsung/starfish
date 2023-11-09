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

#if defined(STARFISH_ENABLE_WORKER) && !defined(__StarfishWorkerOptions__)
#define __StarfishWorkerOptions__

#include "core/modules/worker/WorkerType.h"
#include "core/fetch/RequestData.h"

namespace Starfish {

struct WorkerOptions : public gc {
public:
    static String* workerTypeString(WorkerType type);
    static WorkerType workerTypeFromString(String* typeString);

    String* type() const;
    void setType(String* type);

    String* credentials() const;
    void setCredentials(String* string);

    DEFINE_GETTER_SETTER(String*, name, Name);

private:
    WorkerType m_type{ WorkerType::Classic };
    RequestCredentials m_credentials{ RequestCredentials::SameOrigin };
    String* m_name{ String::emptyString };
};

} // namespace Starfish

#endif
