#ifndef __StarFishPublic__
#define __StarFishPublic__

#include "StarFishExport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StarFishInstance {
    void* m_starfish;
} StarFishInstance;

STARFISH_EXPORT StarFishInstance* starfishInit(void* window, const char* workingDirectory, const char* locale);
STARFISH_EXPORT void starfishRemove(StarFishInstance* instance);

STARFISH_EXPORT void starfishLoadHTMLDocument(StarFishInstance* instance, const char* path);
STARFISH_EXPORT void starfishLoadXMLDocument(StarFishInstance* instance, const char* xmlPath);

STARFISH_EXPORT void starfishNotifyPause(StarFishInstance* instance);
STARFISH_EXPORT void starfishNotifyResume(StarFishInstance* instance);

STARFISH_EXPORT void starfishGCAddRoots(void* start, void* end);
STARFISH_EXPORT void starfishGCRemoveRoots(void* start, void* end);

STARFISH_EXPORT void registerFileOpenCB(FILE * (*cb) (const char* fileName));
STARFISH_EXPORT void registerFileLengthCB(long int (*cb) (FILE* stream));
STARFISH_EXPORT void registerFileReadCB(size_t (*cb) (void * buf, size_t size, size_t count, FILE * fp));
STARFISH_EXPORT void registerFileCloseCB(int (*cb) (FILE * fp));
STARFISH_EXPORT void registerFileMatchLocationCB(const char* (*cb) (const char* fileName));


#ifdef __cplusplus
}
#endif

#endif
