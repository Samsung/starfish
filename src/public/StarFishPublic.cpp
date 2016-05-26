#ifdef STARFISH_TIZEN_WEARABLE_APP

#include "StarFishConfig.h"
#include "StarFish.h"

#include "StarFishPublic.h"

#include <cstdlib>

using namespace StarFish;

namespace StarFish {

typedef FILE * (*sfopen_cb) (const char * filename);
typedef long int (*sflength_cb) (FILE* fp);
typedef size_t (*sfread_cb) (void * buf, size_t size, size_t count, FILE * fp);
typedef int (*sfclose_cb) (FILE * fp);
typedef const char* (*sfmatchLocation_cb) (const char* filename);

extern sfopen_cb open_cb;
extern sflength_cb length_cb;
extern sfread_cb read_cb;
extern sfclose_cb close_cb;
extern sfmatchLocation_cb matchLocation_cb;

}

#define TO_STARFISH(instance) ((StarFish::StarFish*)instance->m_starfish)

extern "C" STARFISH_EXPORT StarFishInstance* starfishInit(void* window, const char* locale, const char* timezoneID)
{
    StarFishInstance* instance = (StarFishInstance*)malloc(sizeof(StarFishInstance));
    instance->m_starfish = new StarFish::StarFish((StarFish::StarFishStartUpFlag)0, locale, timezoneID, window, 360, 360);
    starfishGCAddRoots(instance->m_starfish, (StarFish::StarFish*)instance->m_starfish + 1);
    return instance;
}

extern "C" STARFISH_EXPORT void starfishRemove(StarFishInstance* instance)
{
    starfishGCRemoveRoots(instance->m_starfish, (StarFish::StarFish*)instance->m_starfish + 1);
    delete TO_STARFISH(instance);
    free(instance);

    GC_gcollect_and_unmap();
    GC_gcollect_and_unmap();
}

extern "C" STARFISH_EXPORT void starfishLoadHTMLDocument(StarFishInstance* instance, const char* path)
{
    TO_STARFISH(instance)->loadHTMLDocument(String::fromUTF8(path));
}

extern "C" STARFISH_EXPORT void starfishNotifyPause(StarFishInstance* instance)
{
    TO_STARFISH(instance)->pause();
}

extern "C" STARFISH_EXPORT void starfishNotifyResume(StarFishInstance* instance)
{
    TO_STARFISH(instance)->resume();
}

extern "C" STARFISH_EXPORT void starfishGCAddRoots(void* start, void* end)
{
    GC_add_roots(start, end);
}

extern "C" STARFISH_EXPORT void starfishGCRemoveRoots(void* start, void* end)
{
    GC_remove_roots(start, end);
}

extern "C" STARFISH_EXPORT void registerFileOpenCB(FILE* (*cb)(const char* fileName))
{
    open_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileLengthCB(long int (*cb)(FILE* fp))
{
    length_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileReadCB(size_t (*cb)(void * buf, size_t size, size_t count, FILE * fp))
{
    read_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileCloseCB(int (*cb)(FILE * fp))
{
    close_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileMatchLocationCB(const char* (*cb)(const char* fileName))
{
    matchLocation_cb = cb;
}

#endif
