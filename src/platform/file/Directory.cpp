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
#if OS(WINDOWS)
#include <direct.h>
#define mkdir(a, b) _mkdir(a)
#define rmdir _rmdir
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode)&S_IFMT) == S_IFDIR)
#endif
/*

    Implementation of POSIX directory browsing functions and types for Win32.

    Author:  Kevlin Henney (kevlin@acm.org, kevlin@curbralan.com)
    History: Created March 1997. Updated June 2003 and July 2012.
    Rights:  See end of file.

*/

#include <errno.h>
#include <io.h> /* _findfirst and _findnext set errno iff they return -1 */
#include <stdlib.h>
#include <string.h>

typedef struct DIR DIR;

struct dirent {
    char* d_name;
};

DIR* opendir(const char*);
int closedir(DIR*);
struct dirent* readdir(DIR*);
void rewinddir(DIR*);

typedef ptrdiff_t handle_type; /* C99's intptr_t not sufficiently portable */

struct DIR {
    handle_type handle; /* -1 for failed rewind */
    struct _finddata_t info;
    struct dirent result; /* d_name null iff first time */
    char* name;           /* null-terminated char string */
};

DIR* opendir(const char* name)
{
    DIR* dir = 0;

    if (name && name[0]) {
        size_t base_length = strlen(name);
        const char* all = /* search pattern must end with suitable wildcard */
            strchr("/\\", name[base_length - 1]) ? "*" : "/*";

        if ((dir = (DIR*)malloc(sizeof *dir)) != 0 &&
            (dir->name = (char*)malloc(base_length + strlen(all) + 1)) != 0) {
            strcat(strcpy(dir->name, name), all);

            if ((dir->handle =
                     (handle_type)_findfirst(dir->name, &dir->info)) != -1) {
                dir->result.d_name = 0;
            } else /* rollback */
            {
                free(dir->name);
                free(dir);
                dir = 0;
            }
        } else /* rollback */
        {
            free(dir);
            dir = 0;
            errno = ENOMEM;
        }
    } else {
        errno = EINVAL;
    }

    return dir;
}

int closedir(DIR* dir)
{
    int result = -1;

    if (dir) {
        if (dir->handle != -1) {
            result = _findclose(dir->handle);
        }

        free(dir->name);
        free(dir);
    }

    if (result == -1) /* map all errors to EBADF */
    {
        errno = EBADF;
    }

    return result;
}

struct dirent* readdir(DIR* dir)
{
    struct dirent* result = 0;

    if (dir && dir->handle != -1) {
        if (!dir->result.d_name || _findnext(dir->handle, &dir->info) != -1) {
            result = &dir->result;
            result->d_name = dir->info.name;
        }
    } else {
        errno = EBADF;
    }

    return result;
}

void rewinddir(DIR* dir)
{
    if (dir && dir->handle != -1) {
        _findclose(dir->handle);
        dir->handle = (handle_type)_findfirst(dir->name, &dir->info);
        dir->result.d_name = 0;
    } else {
        errno = EBADF;
    }
}

/*

    Copyright Kevlin Henney, 1997, 2003, 2012. All rights reserved.

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose is hereby granted without fee, provided
    that this copyright and permissions notice appear in all copies and
    derivatives.

    This software is supplied "as is" without express or implied warranty.

    But that said, if there are any problems please get in touch.

*/
#else
#include <dirent.h>
#endif
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
            return;
        }
        if ((dir = opendir(path)) == nullptr) {
            STARFISH_LOG_ERROR("Can`t open directory : %s\n", path);
            return;
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
                continue;
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
