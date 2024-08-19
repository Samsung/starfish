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

#if defined(STARFISH_USE_WORKER_PROCESS)
#ifndef __StarfishWorkerIPCAddress__
#define __StarfishWorkerIPCAddress__

namespace Starfish {

class WorkerIPCAddress : public gc {
public:
    WorkerIPCAddress(const std::string &resourceDirPath);

    virtual ~WorkerIPCAddress();

    virtual const std::string getIPCHandlePath(const std::string &last = "");

    virtual const std::string createIPCAddress(const std::string &last = "");

    void acquire();

    virtual void release();

protected:
    const std::string m_resourceDirPath;
};

} // namespace Starfish

#endif
#endif
