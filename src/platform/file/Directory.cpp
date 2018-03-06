/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "Directory.h"

#include <dirent.h>
#include <sys/stat.h>

namespace StarFish {

class DirectoryPosix : public Directory {
public:
    DirectoryPosix()
        : Directory()
        , m_dir(nullptr)
    {
    }

    ~DirectoryPosix()
    {
        close();
    }

    bool open(String* path) override
    {
        close();
        m_dir = opendir(path->toUTF8NonGCString().data());
        m_path = path;

        if (m_dir) {
            m_isOpen = true;
            return true;
        } else {
            m_isOpen = false;
            return false;
        }
    }

    bool mkDir() override
    {
        int ret = -1;
        if (m_path) {
            ret = mkdir(m_path->toUTF8NonGCString().data(), 0755);
        }

        if (ret == 0) {
            return true;
        } else {
            return false;
        }
    }

    bool close() override
    {
        int ret = -1;
        if (m_dir) {
            ret = closedir(m_dir);
            m_dir = nullptr;
        }

        if (ret == 0) {
            return true;
        } else {
            return false;
        }
    }

    void removeDir() override
    {
        auto str = m_path->toUTF8NonGCString();
        removeDirectory(str.data());
    }

    void clearDir() override
    {
        auto str = m_path->toUTF8NonGCString();
        clearDirectory(str.data());
    }

    bool isOpen() override
    {
        return m_isOpen;
    }

    size_t fileCount() override
    {
        rewinddir(m_dir);

        size_t ret = 0;
        struct dirent* entry = nullptr;
        while ((entry = readdir(m_dir))) {
            if (!strncmp(entry->d_name, ".", 1) ||
                !strncmp(entry->d_name, "..", 2)) {
                continue;
            }
            ret++;
        }
        return ret;
    }

    GCVector<String*> listOfFileName()
    {
        rewinddir(m_dir);

        GCVector<String*> listFiles;
        struct dirent* entry = nullptr;
        while ((entry = readdir(m_dir))) {
            if (!strncmp(entry->d_name, ".", 1) ||
                !strncmp(entry->d_name, "..", 2)) {
                continue;
            }
            listFiles.push_back(String::fromUTF8(entry->d_name));
        }
        return listFiles;
    }

private:
    void removeDirectory(const char* path)
    {
        clearDirectory(path);

        if (rmdir(path) != 0) {
            STARFISH_LOG_ERROR("Can`t remove a directory: %s\n", path);
            STARFISH_ASSERT_NOT_REACHED();
        }
    }

    void clearDirectory(const char* path)
    {
        DIR* dir;
        struct stat statPath, statEntry;
        struct dirent* entry;

        stat(path, &statPath);
        if (S_ISDIR(statPath.st_mode) == 0) {
            STARFISH_LOG_ERROR("Is not directory : %s\n", path);
            STARFISH_ASSERT_NOT_REACHED();
        }
        if ((dir = opendir(path)) == nullptr) {
            STARFISH_LOG_ERROR("Can`t open directory : %s\n", path);
            STARFISH_ASSERT_NOT_REACHED();
        }

        size_t pathLen = strlen(path);
        while ((entry = readdir(dir)) != NULL) {
            // Skip entries "." and ".."
            if (!strncmp(entry->d_name, ".", 1) ||
                !strncmp(entry->d_name, "..", 2)) {
                continue;
            }

            std::string fullPath(path);
            fullPath += "/";
            fullPath += entry->d_name;

            stat(fullPath.c_str(), &statEntry);

            // recursively remove a nested directorys
            if (S_ISDIR(statEntry.st_mode) != 0) {
                removeDirectory(fullPath.c_str());
                continue;
            }

            // remove a file object
            if (unlink(fullPath.c_str()) != 0) {
                STARFISH_LOG_ERROR("Can`t remove a file: %s\n",
                                   fullPath.c_str());
                STARFISH_ASSERT_NOT_REACHED();
            }
        }
        closedir(dir);
    }
    DIR* m_dir;
};

Directory* Directory::create()
{
    DirectoryPosix* dir = new DirectoryPosix();
    return dir;
}

Directory* Directory::createInNonGCArea()
{
    DirectoryPosix* dir = new (malloc(sizeof(DirectoryPosix))) DirectoryPosix();
    return dir;
}
}; // namespace StarFish
