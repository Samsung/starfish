/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#include "SocketPath.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <cstring>

namespace StarfishCLI {

namespace {

    // Splits "/a/b/default.sock" into "/a/b" and "default.sock". A path with
    // no slash is already relative to the current directory.
    void splitPath(const std::string& path, std::string& directory,
                   std::string& name)
    {
        size_t slash = path.rfind('/');
        if (slash == std::string::npos) {
            directory = ".";
            name = path;
            return;
        }
        // Keep the slash for a path directly under the root, so "/x" gives
        // "/" rather than an empty string.
        directory = slash == 0 ? "/" : path.substr(0, slash);
        name = path.substr(slash + 1);
    }

    // Holds the directory the process started in and returns to it.
    class DirectoryGuard {
    public:
        explicit DirectoryGuard(const std::string& directory)
        {
            m_previous = open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
            if (m_previous < 0) {
                return;
            }
            if (chdir(directory.c_str()) != 0) {
                close(m_previous);
                m_previous = -1;
                return;
            }
            m_didEnter = true;
        }

        ~DirectoryGuard()
        {
            if (m_previous < 0) {
                return;
            }
            if (m_didEnter) {
                // Nothing useful is left to do if this fails: the directory
                // the process started in is gone.
                (void)fchdir(m_previous);
            }
            close(m_previous);
        }

        bool didEnter() const
        {
            return m_didEnter;
        }

        DirectoryGuard(const DirectoryGuard&) = delete;
        DirectoryGuard& operator=(const DirectoryGuard&) = delete;

    private:
        int m_previous{ -1 };
        bool m_didEnter{ false };
    };

    // Fills address with name, which is short because it is a file name.
    bool setAddress(sockaddr_un& address, const std::string& name)
    {
        if (name.empty() || name.size() >= sizeof(address.sun_path)) {
            return false;
        }
        address.sun_family = AF_UNIX;
        memcpy(address.sun_path, name.c_str(), name.size() + 1);
        return true;
    }

} // namespace

int bindUnixSocket(const std::string& path, int mode, int backlog)
{
    std::string directory;
    std::string name;
    splitPath(path, directory, name);

    DirectoryGuard guard(directory);
    if (!guard.didEnter()) {
        return -1;
    }

    sockaddr_un address = {};
    if (!setAddress(address, name)) {
        return -1;
    }

    int descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    if (descriptor < 0) {
        return -1;
    }

    // A socket file left by a dead daemon would make bind fail with EADDRINUSE.
    unlink(name.c_str());
    if (bind(descriptor, reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) != 0 ||
        chmod(name.c_str(), static_cast<mode_t>(mode)) != 0 ||
        listen(descriptor, backlog) != 0) {
        close(descriptor);
        unlink(name.c_str());
        return -1;
    }
    return descriptor;
}

int connectUnixSocket(const std::string& path)
{
    std::string directory;
    std::string name;
    splitPath(path, directory, name);

    // A missing directory means no session, same as a missing socket.
    DirectoryGuard guard(directory);
    if (!guard.didEnter()) {
        return -1;
    }

    sockaddr_un address = {};
    if (!setAddress(address, name)) {
        return -1;
    }

    int descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    if (descriptor < 0) {
        return -1;
    }

    if (connect(descriptor, reinterpret_cast<sockaddr*>(&address),
                sizeof(address)) != 0) {
        close(descriptor);
        return -1;
    }
    return descriptor;
}

} // namespace StarfishCLI
