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

String* PathResolver::matchLocation(String* filePath)
{
    return filePath;
}


#ifdef STARFISH_TIZEN_WEARABLE

typedef FILE* (*sfopen_cb)(const char* fileName);
typedef long int (*sflength_cb)(FILE* fp);
typedef size_t (*sfread_cb)(void* buf, size_t size, size_t count, FILE* fp);
typedef int (*sfclose_cb)(FILE* fp);
typedef const char* (*sfmatchLocation_cb) (const char* fileName);

sfopen_cb open_cb = nullptr;
sflength_cb length_cb = nullptr;
sfread_cb read_cb = nullptr;
sfclose_cb close_cb = nullptr;
sfmatchLocation_cb matchLocation_cb = nullptr;

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
        if (open_cb)
            m_fp = open_cb(fileName);
        else
            m_fp = fopen(fileName, "r");
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
        if (close_cb)
            return close_cb(m_fp);
        if (m_fp) {
            int res = fclose(m_fp);
            m_fp = NULL;
            return res;
        }
        return -1;
    }

private:
    FILE* m_fp;
};

String* PathResolver::matchLocation(String* filePath)
{
    if (!sfmatchLocation_cb)
        return filePath;

    // FIXME change type from const char* into char*
    const char* ret = sfmatchLocation_cb(filePath->utf8Data());
    String* r = String::fromUTF8(ret);
    free((char*)ret);
    return r;
}

#endif

FileIO* FileIO::create()
{
#ifdef STARFISH_TIZEN_WEARABLE
    FileIOTizen* fio = new FileIOTizen();
#else
    FileIOPosix* fio = new FileIOPosix();
#endif
    return fio;
}

FileIO* FileIO::createInNonGCArea()
{
#ifdef STARFISH_TIZEN_WEARABLE
    FileIOTizen* fio = new(malloc(sizeof (FileIOTizen))) FileIOTizen();
#else
    FileIOPosix* fio = new(malloc(sizeof (FileIOPosix))) FileIOPosix();
#endif
    return fio;
}

}
