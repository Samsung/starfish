#include "FileIO.h"

namespace StarFish {

#ifndef STARFISH_TIZEN_WEARABLE
class FileIOPosix : public FileIO {
public:
    FileIOPosix() {}
    bool open(const char* fileName) {
        m_fp = fopen(fileName, "r");
        if (m_fp) return true;
        return false;
    }
    int seek(long int offset, int origin) { return fseek(m_fp, offset, origin); }
    long int tell() { return ftell(m_fp); }
    void Rewind() { rewind(m_fp); }
    size_t read(void* buf, size_t size, size_t count) { return fread(buf, size, count, m_fp); }
    int close() { return fclose(m_fp); }
private:
    FILE* m_fp;
};
#endif



#ifdef STARFISH_TIZEN_WEARABLE

typedef FILE * (*sfopen_cb) (const char * filename);
typedef int (*sfseek_cb) (FILE * fp, long int offset, int origin);
typedef long int (*sftell_cb) (FILE* stream);
typedef void (*sfrewind_cb) (FILE* stream);
typedef size_t (*sfread_cb) (void * buf, size_t size, size_t count, FILE * fp);
typedef int (*sfclose_cb) (FILE * fp);

sfopen_cb open_cb = nullptr;
sfseek_cb seek_cb = nullptr;
sftell_cb tell_cb = nullptr;
sfrewind_cb rewind_cb = nullptr;
sfread_cb read_cb = nullptr;
sfclose_cb close_cb = nullptr;

class FileIOTizen : public FileIO {
public:
    FileIOTizen() {}
    bool open(const char* fileName) {
        if (open_cb)
            m_fp = open_cb(fileName);
        else
            m_fp = fopen(fileName, "r");
        if (m_fp) return true;
                return false;
    }

    int seek(long int offset, int origin) {
        if (seek_cb)
            return seek_cb(m_fp, offset, origin);
        else
            return fseek(m_fp, offset, origin);
    }

    long int tell() {
        if (tell_cb)
            return tell_cb(m_fp);
        else
            return ftell(m_fp);
    }

    void Rewind() {
        if (rewind_cb)
            rewind_cb(m_fp);
        else
            rewind(m_fp);
    }

    size_t read(void* buf, size_t size, size_t count) {
        if (read_cb)
            return read_cb(buf, size, count, m_fp);
        else
            return fread(buf, size, count, m_fp);
    }

    int close() {
        if (close_cb)
            return close_cb(m_fp);
        else
            return fclose(m_fp);
    }
    FILE* m_fp;
};
#endif

FileIO* FileIO::create() {
#ifdef STARFISH_TIZEN_WEARABLE
    FileIOTizen* fio = new FileIOTizen();
#else
    FileIOPosix* fio = new FileIOPosix();
#endif
    return fio;
}

}
