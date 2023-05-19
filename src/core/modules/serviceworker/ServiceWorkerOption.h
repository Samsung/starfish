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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishServiceWorkerOption__)
#define __StarfishServiceWorkerOption__

namespace Starfish {

class ServiceWorkerOption : public gc {
public:
    using ProcessExecutorCallback = std::function<bool()>;
    using OnChangeDataDirectoryPathCallback =
        std::function<void(const std::string&)>;

    static std::string getDefaultDataDirectoryPath();

    ServiceWorkerOption(const std::string& dataDirectoryPath);

    void setDataDirectoryPath(const std::string& path);
    const std::string dataDirectoryPath()
    {
        return m_dataDirectoryPath;
    }

    void setServiceWorkerProcessExecutor(
        const ProcessExecutorCallback& swExecutor)
    {
        m_swExecutor = swExecutor;
    }
    ProcessExecutorCallback serviceWorkerProcessExecutor()
    {
        return m_swExecutor;
    }

    void addOnChangeDataDirectoryPathCallback(
        OnChangeDataDirectoryPathCallback callback);

private:
    std::string m_dataDirectoryPath;
    ProcessExecutorCallback m_swExecutor{ nullptr };

    std::vector<OnChangeDataDirectoryPathCallback>
        m_onChangeDataDirectoryPathCallbacks;
};

} // namespace Starfish

#endif
