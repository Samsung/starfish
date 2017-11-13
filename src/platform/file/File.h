/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
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
        m_path = filePath;
        auto utf8Data = m_path->toUTF8NonGCString();
        return open(utf8Data.data(), mode);
    }

    bool open(ResourceURL* url, FileMode mode)
    {
        if (!url->isFileURL()) {
            return false;
        }

        String* path = url->getUrlPathString();
        String* filePath = path->substring(7, path->length() - 7);

        return this->open(filePath, mode);
    }

    int removeFile()
    {
        close();
        auto utf8Data = m_path->toUTF8NonGCString();
        return remove(utf8Data.data());
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
        if ((out.capacity()) != expected) {
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

    virtual bool open(const char* filePath, FileMode mode) = 0;
    virtual long int size() = 0;
    virtual size_t read(void* buf, size_t size, size_t count) = 0;
    virtual size_t write(void* buf, size_t size, size_t count) = 0;
    virtual ssize_t readLine(char** out, size_t* len) = 0;

    virtual int seek(long offset, int whence) = 0;
    virtual int flush() = 0;
    virtual int close() = 0;
    virtual int eof() = 0;

protected:
    File()
        : m_path(String::emptyString)
        , m_isOpen(false)
    {
    }

    const char* fileModeToString(const FileMode var)
    {
        STARFISH_ASSERT(FileMode::Read <= var && var <= FileMode::ReadWrite);
        return kFileModeStrList[var];
    }

    String* m_path;
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
