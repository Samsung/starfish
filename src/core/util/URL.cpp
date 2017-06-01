/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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
#include "core/dom/Document.h"
#include "core/fileapi/Blob.h"
#include "core/modules/mediasource/MediaSource.h"
#include "core/util/URL.h"
#include "core/modules/window/Window.h"

namespace StarFish {

URL::URL(Window* window, String* url)
    : ScriptWrappable(this)
    , ResourceURL(url)
    , m_scriptBindingInstance(window->scriptBindingInstance())
{
}

URL::URL(Window* window, String* url, String* baseURL)
    : ScriptWrappable(this)
    , ResourceURL(url, baseURL)
    , m_scriptBindingInstance(window->scriptBindingInstance())
{
}

URL::URL(ScriptBindingInstance* ins, String* url, String* baseURL)
    : ScriptWrappable(this)
    , ResourceURL(url, baseURL)
    , m_scriptBindingInstance(ins)
{
}

ScriptBindingInstance* URL::scriptBindingInstance()
{
    return m_scriptBindingInstance;
}

String* URL::createObjectURL(Blob* blob)
{
    BlobURLStore store;
    if (blob->starFish()->isValidBlobURL(blob)) {
        store = blob->starFish()->findBlobURL(blob);
    } else {
        store = blob->starFish()->addBlobInBlobURLStore(blob);
    }
    return StarFish::blobURLStoreToString(
        store, blob->starFish()->window()->document()->urlString());
}

void URL::revokeObjectURL(StarFish* sf, String* blobURLRef)
{
    BlobURLStore store;
    if (StarFish::stringToBlobURLString(blobURLRef, store)) {
        if (sf->isValidBlobURL(store)) {
            sf->removeBlobFromBlobURLStore((Blob*)store.m_blob);
        }
#ifdef STARFISH_ENABLE_MULTIMEDIA
        else if (sf->isValidMediaSourceBlobURL(store)) {
            sf->removeMediaSourceFromBlobURLStore((MediaSource*)store.m_blob);
        }
#endif
    }
}

#ifdef STARFISH_ENABLE_MULTIMEDIA
String* URL::createObjectURL(MediaSource* mediaSource)
{
    BlobURLStore store;
    if (mediaSource->starFish()->isValidMediaSourceBlobURL(mediaSource)) {
        store = mediaSource->starFish()->findMediaSourceBlobURL(mediaSource);
    } else {
        store =
            mediaSource->starFish()->addMediaSourceInBlobURLStore(mediaSource);
    }
    return StarFish::blobURLStoreToString(store,
                                          mediaSource->document()->urlString());
}
#endif
}
