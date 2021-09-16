/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishPlatformFile__
#define __StarfishPlatformFile__

namespace Starfish {

class PlatformFileUtil {
public:
    static bool removeFile(const std::string& filePath);
    static Nullable<std::string> absolutePath(const std::string& filePath);
};

class PlatformFile {
public:
    enum FileMode {
        Read = 1,
        Write = 1 << 1,
        ReadWrite = Read | Write,
    };

    enum Whence {
        Start = SEEK_SET,
        Current = SEEK_CUR,
        End = SEEK_END,
    };

    static std::unique_ptr<PlatformFile> open(String* filePath, FileMode mode)
    {
        return open(filePath->toUTF8NonGCString().data(), mode);
    }
    static std::unique_ptr<PlatformFile> open(const std::string& filePath,
                                              FileMode mode);

    virtual ~PlatformFile()
    {
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
        const size_t IDEAL_BUFFER_SIZE = 262143;
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
    virtual int eof() = 0;
    virtual int64_t lastAccessTime() = 0;
    virtual int64_t lastModificationTime() = 0;
    virtual int64_t lastChangeTime() = 0;

protected:
    PlatformFile()
    {
    }

private:
};
} // namespace Starfish

#endif
