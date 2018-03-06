/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

bool Storage::setItem(String* key, String* value)
{
    return m_storageImpl->setItem(key, value);
}

void Storage::defaultNamedEnumerator(GCVector<String*>& enums)
{
    enums = m_storageImpl->getKeyNames();
}

bool Storage::defaultNamedDeleter(String* key)
{
    return m_storageImpl->removeItem(key);
}

bool Storage::removeItem(String* key)
{
    return m_storageImpl->removeItem(key);
}

void Storage::clear()
{
    m_storageImpl->clear();
}
}
