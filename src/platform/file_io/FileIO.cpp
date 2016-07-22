/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
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

#include "StarFishConfig.h"
#include "FileIO.h"

namespace StarFish {

class FileIOPosix : public FileIO {
public:
    FileIOPosix()
        : m_fp(NULL)
    { }
    ~FileIOPosix()
    {
        close();
    }
    bool open(const char* filePath)
    {
        close();
        m_fp = fopen(filePath, "r");
        if (m_fp)
            return true;
        return false;
    }
    long int length()
    {
        fseek(m_fp, 0, 2);
        long int len = ftell(m_fp);
        rewind(m_fp);
        return len;
    }
    size_t read(void* buf, size_t size, size_t count) { return fread(buf, size, count, m_fp); }
    int close()
    {
        int ret = -1;
        if (m_fp) {
            ret = fclose(m_fp);
            m_fp = NULL;
        }
        return ret;
    }
private:
    FILE* m_fp;
};

#ifndef STARFISH_TIZEN_WEARABLE_LIB
String* PathResolver::matchLocation(String* filePath)
{
    return filePath;
}
#endif


#ifdef STARFISH_TIZEN_WEARABLE_LIB

typedef FILE* (*sfopen_cb)(const char* fileName);
typedef long int (*sflength_cb)(FILE* fp);
typedef size_t (*sfread_cb)(void* buf, size_t size, size_t count, FILE* fp);
typedef int (*sfclose_cb)(FILE* fp);
typedef const char* (*sfmatchLocation_cb) (const char* fileName);

extern sfopen_cb open_cb;
extern sflength_cb length_cb;
extern sfread_cb read_cb;
extern sfclose_cb close_cb;
extern sfmatchLocation_cb matchLocation_cb;

class FileIOTizen : public FileIO {
public:
    FileIOTizen()
    {
        m_fp = NULL;
    }

    ~FileIOTizen()
    {
        close();
    }

    bool open(const char* fileName)
    {
        close();

        String* newName = PathResolver::matchLocation(String::fromUTF8(fileName));

        if (!newName)
            return false;

        if (open_cb)
            m_fp = open_cb(newName->utf8Data());
        else
            m_fp = fopen(newName->utf8Data(), "r");
        if (m_fp)
            return true;
        return false;
    }

    long int length()
    {
        if (length_cb)
            return length_cb(m_fp);
        fseek(m_fp, 0, 2);
        long int len = ftell(m_fp);
        rewind(m_fp);
        return len;
    }

    size_t read(void* buf, size_t size, size_t count)
    {
        if (read_cb)
            return read_cb(buf, size, count, m_fp);
        return fread(buf, size, count, m_fp);
    }

    int close()
    {
        int res = -1;
        if (m_fp) {
            if (close_cb)
                res = close_cb(m_fp);
            else
                res = fclose(m_fp);
            m_fp = NULL;
        }
        return res;
    }

private:
    FILE* m_fp;
};

String* PathResolver::matchLocation(String* filePath)
{
    if (!matchLocation_cb)
        return filePath;

    const char* ret = matchLocation_cb(filePath->utf8Data());
    if (!ret)
        return nullptr;
    String* r = String::fromUTF8(ret);
    free((char*)ret);
    return r;
}

#endif

FileIO* FileIO::create()
{
#ifdef STARFISH_TIZEN_WEARABLE_LIB
    FileIOTizen* fio = new FileIOTizen();
#else
    FileIOPosix* fio = new FileIOPosix();
#endif
    return fio;
}

FileIO* FileIO::createInNonGCArea()
{
#ifdef STARFISH_TIZEN_WEARABLE_LIB
    FileIOTizen* fio = new(malloc(sizeof (FileIOTizen))) FileIOTizen();
#else
    FileIOPosix* fio = new(malloc(sizeof (FileIOPosix))) FileIOPosix();
#endif
    return fio;
}

}
