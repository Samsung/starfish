#ifdef NDEBUG
#ifdef STARFISH_TIZEN_WEARABLE

#include "StarFishConfig.h"
#include "StarFish.h"

#include "StarFishPublic.h"

#include <cstdlib>

using namespace StarFish;

#define TO_STARFISH(instance) ((StarFish::StarFish*)instance->m_starfish)

extern "C" STARFISH_EXPORT StarFishInstance* starfishInit(void* window, const char* workingDirectory)
{
    starfishGCAddRoots(String::emptyString, String::emptyString + sizeof(String*));
    starfishGCAddRoots(String::spaceString, String::spaceString + sizeof(String*));
    StarFishInstance* instance = (StarFishInstance*)malloc(sizeof(StarFishInstance));
    instance->m_starfish = new StarFish::StarFish((StarFish::StarFishStartUpFlag)0, String::fromUTF8(workingDirectory), window);
    starfishGCAddRoots(instance->m_starfish, (StarFish::StarFish*)instance->m_starfish + sizeof(StarFish::StarFish*));
    return instance;
}

extern "C" STARFISH_EXPORT void starfishRemove(StarFishInstance* instance)
{
    TO_STARFISH(instance)->close();
    starfishGCRemoveRoots(instance->m_starfish, (StarFish::StarFish*)instance->m_starfish + sizeof(StarFish::StarFish*));
    delete TO_STARFISH(instance);
    free(instance);
}

extern "C" STARFISH_EXPORT void starfishLoadXMLDocument(StarFishInstance* instance, const char* xmlPath)
{
    TO_STARFISH(instance)->loadXMLDocument(String::fromUTF8(xmlPath));
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

extern "C" STARFISH_EXPORT void registerFileOpenCB(FILE * (*cb) ( const char * filename))
{
    open_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileSeekCB(int (*cb) (FILE * fp, long int offset, int origin))
{
    seek_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileTellCB(long int (*cb) (FILE* stream))
{
    tell_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileRewindCB(void (*cb) (FILE* stream))
{
    rewind_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileReadCB(size_t (*cb) (void * buf, size_t size, size_t count, FILE * fp))
{
    read_cb = cb;
}

extern "C" STARFISH_EXPORT void registerFileCloseCB(int (*cb) (FILE * fp))
{
    close_cb = cb;
}

#endif
#endif
