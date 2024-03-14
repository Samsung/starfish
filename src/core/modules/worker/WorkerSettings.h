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

#if defined(STARFISH_USE_WORKER_PROCESS) && !defined(__StarfishWorkerSettings__)
#define __StarfishWorkerSettings__

namespace Starfish {

class WorkerSettings : public gc {
public:
    using ProcessExecutorCallback = std::function<bool()>;
    using OnChangeDataDirectoryPathCallback = std::function<void(
        const std::string& curPath, const std::string& newPath)>;

    static std::string getDefaultDataDirectoryPath();

    WorkerSettings();
    WorkerSettings(const std::string& dataDirectoryPath);

    void setDataDirectoryPath(const std::string& path);
    const std::string dataDirectoryPath()
    {
        return m_dataDirectoryPath;
    }

    void setServiceWorkerProcessExecutor(
        const ProcessExecutorCallback& executor)
    {
        m_serviceWorkerProcessExecutor = executor;
    }
    ProcessExecutorCallback serviceWorkerProcessExecutor()
    {
        return m_serviceWorkerProcessExecutor;
    }

    void addOnChangeDataDirectoryPathCallback(
        OnChangeDataDirectoryPathCallback callback);

    void setProcessName(const std::string& name)
    {
        m_processName = name;
    }
    std::string processName()
    {
        return m_processName;
    }

    DEFINE_GETTER_SETTER(size_t, threadPoolSize, ThreadPoolSize);

private:
    std::string m_dataDirectoryPath;
    std::string m_processName;
    ProcessExecutorCallback m_serviceWorkerProcessExecutor{ nullptr };
    size_t m_threadPoolSize = 1;

    std::vector<OnChangeDataDirectoryPathCallback>
        m_onChangeDataDirectoryPathCallbacks;
};

} // namespace Starfish

#endif
