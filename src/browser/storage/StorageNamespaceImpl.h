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

#ifndef __StarFishStorageNamespaceImpl__
#define __StarFishStorageNamespaceImpl__

#include "core/page/SecurityOriginData.h"
#include "core/storage/StorageNamespace.h"
#include "core/storage/StorageType.h"

namespace StarFish {

class StorageImpl;
class StorageManager;
class Window;

class StorageNamespaceImpl : public StorageNamespace {
public:
    StorageNamespaceImpl(StorageType storageType, String* localStoragePath);
    virtual ~StorageNamespaceImpl();

    virtual Storage* storage(Window* window,
                             SecurityOriginData* origin) override;

private:
    StorageType m_storageType;
    StorageManager* m_storageManager;

    std::unordered_map<SecurityOriginData*, StorageImpl*,
                       SecurityOriginDataHash, SecurityOriginDataEqual,
                       gc_allocator_ignore_off_page<
                           std::pair<SecurityOriginData* const, StorageImpl*>>>
        m_originToStorage;
};
}

#endif
