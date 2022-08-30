/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
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

#include "StarfishConfig.h"
#include "CustomStorage.h"
#include "core/storage/StorageInternal.h"

namespace Starfish {

CustomStorage::CustomStorage(ScriptBindingInstance* scriptBindingInstance,
                             StorageInternal* storageInternal)
    : ScriptWrappable(this)
    , m_storageInternal(storageInternal)
    , m_scriptBindingInstance(scriptBindingInstance)
{
}

unsigned long CustomStorage::length()
{
    return m_storageInternal->length();
}

Nullable<String*> CustomStorage::key(unsigned long index)
{
    return m_storageInternal->key(index);
}

Nullable<String*> CustomStorage::getItem(String* key)
{
    return m_storageInternal->getItem(key);
}

bool CustomStorage::setItem(String* key, String* value)
{
    return m_storageInternal->setItem(key, value);
}

void CustomStorage::defaultNamedEnumerator(GCVector<String*>& enums)
{
    enums = m_storageInternal->getKeyNames();
}

bool CustomStorage::defaultNamedDeleter(String* key)
{
    return m_storageInternal->removeItem(key);
}

bool CustomStorage::removeItem(String* key)
{
    return m_storageInternal->removeItem(key);
}

void CustomStorage::clear()
{
    m_storageInternal->clear();
}
} // namespace Starfish
