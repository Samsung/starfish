#ifndef __StarFishPublic__
#define __StarFishPublic__

#include "StarFishExport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StarFishInstance {
    void* m_starfish;
} StarFishInstance;

STARFISH_EXPORT StarFishInstance* starfishInit(void* window, const char* workingDirectory);
STARFISH_EXPORT void starfishRemove(StarFishInstance* instance);

STARFISH_EXPORT void starfishLoadXMLDocument(StarFishInstance* instance, const char* xmlPath);

STARFISH_EXPORT void starfishNotifyPause(StarFishInstance* instance);
STARFISH_EXPORT void starfishNotifyResume(StarFishInstance* instance);

STARFISH_EXPORT void starfishGCAddRoots(void* start, void* end);
STARFISH_EXPORT void starfishGCRemoveRoots(void* start, void* end);


#ifdef __cplusplus
}
#endif

#endif
