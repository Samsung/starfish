#ifndef __StarFishPublic__
#define __StarFishPublic__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StarFishInstance {
    void* m_starfish;
} StarFishInstance;

StarFishInstance* starfishInit(void* window, const char* workingDirectory);
void starfishRemove(StarFishInstance* instance);

void starfishLoadXMLDocument(StarFishInstance* instance, const char* xmlPath);

void starfishNotifyPause(StarFishInstance* instance);
void starfishNotifyResume(StarFishInstance* instance);

void starfishGCAddRoots(void* start, void* end);
void starfishGCRemoveRoots(void* start, void* end);


#ifdef __cplusplus
}
#endif

#endif
