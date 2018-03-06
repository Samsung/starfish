/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __StarFishPublic__
#define __StarFishPublic__

#include "StarFishExport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StarFishInstance {
    void* m_starfish;
#ifdef STARFISH_DALI
    void* m_data;
#endif
} StarFishInstance;

#ifdef STARFISH_TIZEN_TV
STARFISH_EXPORT StarFishInstance* starfishCreate(
    void* window, int windowWidth, int windowHeight, int windowX, int windowY,
    const char* locale, const char* timezoneID,
    float defaultFontSizeMultiplier);
#else
STARFISH_EXPORT StarFishInstance* starfishCreate(
    void* window, int windowWidth, int windowHeight, const char* locale,
    const char* timezoneID, const char* defaultFont,
    float defaultFontSizeMultiplier);
#endif

STARFISH_EXPORT void starfishRemove(StarFishInstance* instance);
STARFISH_EXPORT void starfishRemoveForUpdate(StarFishInstance* instance);
STARFISH_EXPORT void starfishLoadHTMLDocument(StarFishInstance* instance,
                                              const char* path);

STARFISH_EXPORT void starfishNotifyPause(StarFishInstance* instance);
STARFISH_EXPORT void starfishNotifyResume(StarFishInstance* instance);

STARFISH_EXPORT void registerFileOpenCB(FILE* (*cb)(const char* fileName));
STARFISH_EXPORT void registerFileLengthCB(long int (*cb)(FILE* stream));
STARFISH_EXPORT void registerFileReadCB(size_t (*cb)(void* buf, size_t size,
                                                     size_t count, FILE* fp));
STARFISH_EXPORT void registerFileCloseCB(int (*cb)(FILE* fp));
STARFISH_EXPORT void registerFileMatchLocationCB(
    const char* (*cb)(const char* fileName));

#if defined(STARFISH_TIZEN_WEARABLE) && defined(TIZEN_DEVICE_API)
STARFISH_EXPORT void starfishSetWidgetContext(StarFishInstance* instance,
                                              const void* widgetContext);
STARFISH_EXPORT void registerWebWidgetAPISetContentInfoOfContextCB(
    int (*cb)(const void* ctx, const void* data));
STARFISH_EXPORT void registerWebWidgetAPIGetContentInfoOfContextCB(
    int (*cb)(const void* ctx, void** out));
STARFISH_EXPORT void starfishWebWidgetAPINotifyReceiveContent(
    StarFishInstance* instance, const void* data);
#endif

#ifdef __cplusplus
}
#endif

#endif
