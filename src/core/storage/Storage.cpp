/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#include "Storage.h"

#include "browser/storage/StorageImpl.h"
#include "core/page/SecurityOriginData.h"

namespace StarFish {

Storage::Storage(Window* window, StorageImpl* storageImpl)
    : ScriptWrappable(this)
    , WindowHoldable(window)
    , m_storageImpl(storageImpl)
{
}

unsigned long Storage::length()
{
    return m_storageImpl->length();
}

Nullable<String*> Storage::key(unsigned long index)
{
    return m_storageImpl->key(index);
}

Nullable<String*> Storage::getItem(String* key)
{
    return m_storageImpl->getItem(key);
}

void Storage::setItem(String* key, String* value)
{
    m_storageImpl->setItem(key, value);
}

void Storage::removeItem(String* key)
{
    m_storageImpl->removeItem(key);
}

void Storage::clear()
{
    m_storageImpl->clear();
}
}
