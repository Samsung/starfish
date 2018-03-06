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

#ifndef __StarFishFile__
#define __StarFishFile__

#define IDEAL_BUFFER_SIZE 262143
namespace StarFish {

class File : public gc_cleanup {
public:
    enum FileMode {
        Read,
        Write,
        ReadWrite,
    };

    enum Whence {
        Start = SEEK_SET,
        Current = SEEK_CUR,
        End = SEEK_END,
    };

    static File* create();
    static File* createInNonGCArea();
    virtual ~File()
    {
    }

    bool open(String* filePath, FileMode mode)
    {
        m_path = filePath->toUTF8NonGCString();
        return open(m_path.data(), mode);
    }

    bool open(const std::string& filePath, FileMode mode)
    {
        m_path = filePath;
        return open(m_path.data(), mode);
    }

    int removeFile()
    {
        close();
        return remove(m_path.data());
    }

    bool isOpen()
    {
        return m_isOpen;
    }

    bool writeLine(String* str = String::emptyString)
    {
        auto s = str->toUTF8NonGCString();
        size_t ret1 = write((void*)s.data(), sizeof(char), s.size());
        size_t ret2 = write((void*)"\n", sizeof(char), 1);

        if (ret1 == s.size() && ret2 == 1) {
            return true;
        }
        return false;
    }

    Nullable<String*> readLine()
    {
        char* line = nullptr;
        size_t len = 0;
        ssize_t ret = readLine(&line, &len);

        if (ret == -1) {
            return nullptr;
        }

        return String::fromUTF8(line, len);
    }

    Nullable<String*> readAll()
    {
        std::string str;
        bool ret = readAll(str);

        if (ret) {
            return String::fromUTF8(str.data(), str.length());
        } else {
            return nullptr;
        }
    }

    template <class T>
    bool readAll(T& out)
    {
        if (!isOpen()) {
            return false;
        }

        size_t expected = size();

        out.reserve(expected);
        if ((out.capacity()) < expected) {
            return false;
        }

        seek(0, Whence::Start);

        char temp[IDEAL_BUFFER_SIZE];

        while (!eof()) {
            size_t readCount = read(temp, sizeof(char), IDEAL_BUFFER_SIZE);
            if (readCount < IDEAL_BUFFER_SIZE && !eof()) {
                return false;
            }
            if (readCount) {
                out.insert(out.end(), temp, temp + readCount);
            }
        }
        return true;
    }

    virtual size_t size() = 0;
    virtual size_t read(void* buf, size_t size, size_t count) = 0;
    virtual size_t write(void* buf, size_t size, size_t count) = 0;
    virtual ssize_t readLine(char** out, size_t* len) = 0;

    virtual int seek(long offset, int whence) = 0;
    virtual int flush() = 0;
    virtual int close() = 0;
    virtual int eof() = 0;
    virtual int64_t lastAccessTime() = 0;
    virtual int64_t lastModificationTime() = 0;
    virtual int64_t lastChangeTime() = 0;

    static Nullable<String*> absolutePath(String* localPath);

protected:
    File()
        : m_path()
        , m_isOpen(false)
    {
    }

    virtual bool open(const char* filePath, FileMode mode) = 0;

    const char* fileModeToString(const FileMode var)
    {
        STARFISH_ASSERT(FileMode::Read <= var && var <= FileMode::ReadWrite);
        return kFileModeStrList[var];
    }

    std::string m_path;
    bool m_isOpen;

private:
    static const char* kFileModeStrList[];
};

class PathResolver {
public:
    PathResolver() = delete;
    static String* matchLocation(String* filePath);
};
}

#undef IDEAL_BUFFER_SIZE
#endif
