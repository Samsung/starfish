/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "File.h"

#include <sys/types.h>
#include <sys/stat.h>

namespace StarFish {

const char* File::kFileModeStrList[] = { "r", "w", "w+" };

class FilePosix : public File {
public:
    FilePosix()
        : File()
        , m_fp(nullptr)
    {
    }

    ~FilePosix()
    {
        close();
    }

    bool open(const char* filePath, FileMode filemode) override
    {
        close();
        struct stat s;
        memset(&s, 0, sizeof(struct stat));
        stat(filePath, &s);
        if ((s.st_mode & S_IFMT) == S_IFDIR) {
            return false;
        }

        m_fp = fopen(filePath, fileModeToString(filemode));
        if (m_fp) {
            m_isOpen = true;
            return m_isOpen;
        }
        return false;
    }

    size_t size() override
    {
        size_t currentPosition = ftell(m_fp);
        seek(0, Whence::End);
        size_t len = ftell(m_fp);
        seek(currentPosition, Whence::Start);
        return len;
    }

    size_t read(void* buf, size_t size, size_t count) override
    {
        return fread(buf, size, count, m_fp);
    }

    size_t write(void* buf, size_t size, size_t count) override
    {
        return fwrite(buf, size, count, m_fp);
    }

    ssize_t readLine(char** out, size_t* len) override
    {
        if (m_fp) {
            return -1;
        }
        return getline(out, len, m_fp);
    }

    int close() override
    {
        int ret = -1;
        if (m_fp) {
            ret = fclose(m_fp);
            m_fp = nullptr;
            m_isOpen = false;
        }
        return ret;
    }

    int seek(long offset, int whence) override
    {
        return fseek(m_fp, offset, whence);
    }

    int flush() override
    {
        return fflush(m_fp);
    }

    int eof() override
    {
        return feof(m_fp);
    }

    int64_t lastAccessTime() override
    {
        struct stat s;
        memset(&s, 0, sizeof(struct stat));
        stat(m_path.data(), &s);
        return s.st_atime;
    }

    int64_t lastModificationTime() override
    {
        struct stat s;
        memset(&s, 0, sizeof(struct stat));
        stat(m_path.data(), &s);
        return s.st_mtime;
    }

    int64_t lastChangeTime() override
    {
        struct stat s;
        memset(&s, 0, sizeof(struct stat));
        stat(m_path.data(), &s);
        return s.st_ctime;
    }

private:
    FILE* m_fp;
};

#ifndef STARFISH_TIZEN_WEARABLE_WIDGET
String* PathResolver::matchLocation(String* filePath)
{
    return filePath;
}
#endif

#ifdef STARFISH_TIZEN_WEARABLE_WIDGET

typedef FILE* (*sfopen_cb)(const char* fileName);
typedef long int (*sflength_cb)(FILE* fp);
typedef size_t (*sfread_cb)(void* buf, size_t size, size_t count, FILE* fp);
typedef int (*sfclose_cb)(FILE* fp);
typedef const char* (*sfmatchLocation_cb)(const char* fileName);

extern sfopen_cb open_cb;
extern sflength_cb length_cb;
extern sfread_cb read_cb;
extern sfclose_cb close_cb;
extern sfmatchLocation_cb matchLocation_cb;

class FileTizen : public File {
public:
    FileTizen()
        : File()
        , m_fp(nullptr)
    {
    }

    ~FileTizen()
    {
        close();
    }

    bool open(const char* filePath, FileMode filemode) override
    {
        close();
        String* newName =
            PathResolver::matchLocation(String::fromUTF8(filePath));

        if (!newName) {
            return false;
        }

        if (open_cb) {
            auto s = newName->toUTF8NonGCString();
            m_fp = open_cb(s.data());
        } else {
            m_fp = fopen(filePath, fileModeToString(filemode));
        }
        if (m_fp) {
            m_isOpen = true;
            return m_isOpen;
        }
        return false;
    }

    size_t size() override
    {
        if (length_cb) {
            return length_cb(m_fp);
        }
        size_t currentPosition = ftell(m_fp);
        seek(0, Whence::End);
        size_t len = ftell(m_fp);
        seek(currentPosition, Whence::Start);
        return len;
    }

    size_t read(void* buf, size_t size, size_t count) override
    {
        if (read_cb) {
            return read_cb(buf, size, count, m_fp);
        }
        return fread(buf, size, count, m_fp);
    }

    size_t write(void* buf, size_t size, size_t count) override
    {
        // TODO : It will connect to the Tizen file I/O interface.
        return fwrite(buf, size, count, m_fp);
    }

    ssize_t readLine(char** out, size_t* len) override
    {
        if (m_fp) {
            return -1;
        }
        return getline(out, len, m_fp);
    }

    int close() override
    {
        int res = -1;
        if (m_fp) {
            if (close_cb) {
                res = close_cb(m_fp);
            } else {
                res = fclose(m_fp);
            }
            m_fp = nullptr;
            m_isOpen = false;
        }
        return res;
    }

    int seek(long offset, int whence) override
    {
        return fseek(m_fp, offset, whence);
    }

    int flush() override
    {
        // TODO : It will connect to the Tizen file I/O interface.
        return fflush(m_fp);
    }

    int eof() override
    {
        return feof(m_fp);
    }

    int64_t lastAccessTime() override
    {
        struct stat s;
        memset(&s, 0, sizeof(struct stat));
        stat(m_path.data(), &s);
        return s.st_atime;
    }

    int64_t lastModificationTime() override
    {
        struct stat s;
        memset(&s, 0, sizeof(struct stat));
        stat(m_path.data(), &s);
        return s.st_mtime;
    }

    int64_t lastChangeTime() override
    {
        struct stat s;
        memset(&s, 0, sizeof(struct stat));
        stat(m_path.data(), &s);
        return s.st_ctime;
    }

private:
    FILE* m_fp;
};

String* PathResolver::matchLocation(String* filePath)
{
    if (!matchLocation_cb) {
        return filePath;
    }
    auto s = filePath->toUTF8NonGCString();
    const char* ret = matchLocation_cb(s.data());
    if (!ret) {
        return nullptr;
    }
    String* r = String::fromUTF8(ret);
    free((char*)ret);
    return r;
}

#endif

File* File::create()
{
#ifdef STARFISH_TIZEN_WEARABLE_WIDGET
    FileTizen* fio = new FileTizen();
#else
    FilePosix* fio = new FilePosix();
#endif
    return fio;
}

File* File::createInNonGCArea()
{
#ifdef STARFISH_TIZEN_WEARABLE_WIDGET
    FileTizen* fio = new (malloc(sizeof(FileTizen))) FileTizen();
#else
    FilePosix* fio = new (malloc(sizeof(FilePosix))) FilePosix();
#endif
    return fio;
}

Nullable<String*> File::absolutePath(String* localPath)
{
    UTF8StringDataNonGCStd data = localPath->toUTF8NonGCString();
    char* resolved = realpath(data.c_str(), NULL);
    if (resolved) {
        String* result = String::fromUTF8(resolved);
        free(resolved);
        return result;
    }
    return nullptr;
}
} // namespace StarFish
