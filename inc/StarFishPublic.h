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

#ifndef __StarFishPublic__
#define __StarFishPublic__

#include "StarFishExport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StarFishInstance {
    void* m_starfish;
} StarFishInstance;

STARFISH_EXPORT StarFishInstance* starfishInit(void* window, int windowWidth, int windowHeight, const char* locale, const char* timezoneID, float defaultFontSizeMultiplier);
STARFISH_EXPORT void starfishRemove(StarFishInstance* instance);

STARFISH_EXPORT void starfishLoadHTMLDocument(StarFishInstance* instance, const char* path);

STARFISH_EXPORT void starfishNotifyPause(StarFishInstance* instance);
STARFISH_EXPORT void starfishNotifyResume(StarFishInstance* instance);

STARFISH_EXPORT void registerFileOpenCB(FILE * (*cb) (const char* fileName));
STARFISH_EXPORT void registerFileLengthCB(long int (*cb) (FILE* stream));
STARFISH_EXPORT void registerFileReadCB(size_t (*cb) (void * buf, size_t size, size_t count, FILE * fp));
STARFISH_EXPORT void registerFileCloseCB(int (*cb) (FILE * fp));
STARFISH_EXPORT void registerFileMatchLocationCB(const char* (*cb) (const char* fileName));


#ifdef __cplusplus
}
#endif

#endif
