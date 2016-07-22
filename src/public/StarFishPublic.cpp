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

sfopen_cb open_cb = nullptr;
sflength_cb length_cb = nullptr;
sfread_cb read_cb = nullptr;
sfclose_cb close_cb = nullptr;
sfmatchLocation_cb matchLocation_cb = nullptr;
}

#define TO_STARFISH(instance) ((StarFish::StarFish*)instance->m_starfish)

extern "C" STARFISH_EXPORT StarFishInstance* starfishInit(void* window, const char* locale, const char* timezoneID)
{
    StarFishInstance* instance = new(NoGC) StarFishInstance;
    instance->m_starfish = new StarFish::StarFish((StarFish::StarFishStartUpFlag)0, locale, timezoneID, window, 360, 360, 1);
    return instance;
}

extern "C" STARFISH_EXPORT void starfishRemove(StarFishInstance* instance)
{
    delete TO_STARFISH(instance);
    GC_FREE(instance);

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

