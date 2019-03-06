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

#ifndef __StarfishProcessHost__
#define __StarfishProcessHost__

#include "platform/process/base/ProcessType.h"
#include <mutex>

namespace Starfish {

enum class ProcessState {
    INITIALIZED = 0,
    ERROR,
    CHILD_PROCESS_STARTED,
    CHILD_PROCESS_STOPPED
};

class ProcessHost {
public:
    ProcessHost();
    virtual ~ProcessHost();

    bool launch(std::vector<std::string>& args);
    bool terminate();
    void setState(const ProcessState state);

private:
    ProcessState m_processState;
    PID m_childPid;
    std::mutex m_stateMutex;
};

} // namespace Starfish

#endif
