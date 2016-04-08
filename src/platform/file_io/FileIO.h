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
    static FileIO* createInNonGCArea();
    virtual ~FileIO()
    {

    }

    bool open(String* filePath)
    {
        return open(filePath->utf8Data());
    }

    virtual bool open(const char* filePath) = 0;
    virtual long int length() = 0;
    virtual size_t read(void* buf, size_t size, size_t count) = 0;
    virtual int close() = 0;
};

class PathResolver {
public:
    PathResolver() = delete;
    static String* matchLocation(String* filePath);
};

}

#endif
