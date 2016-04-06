#ifndef __FileIO__
#define __FileIO__

#include "StarFishConfig.h"

namespace StarFish {

class Window;
class FileIO : public gc_cleanup {
protected:
    FileIO() { }
public:
    static FileIO* create();
    virtual ~FileIO()
    {

    }

    virtual bool open(const char* fileName) = 0;
    virtual int seek(long int offset, int origin) = 0;
    virtual long int tell() = 0;
    virtual void rewind() = 0;
    virtual size_t read(void* buf, size_t size, size_t count) = 0;
    virtual int close() = 0;
};

}

#endif
