/*
 * Copyright (c) 2017-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishStorageNamespaceImpl__
#define __StarfishStorageNamespaceImpl__

#include "core/dom/WebOrigin.h"
#include "core/storage/StorageInternal.h"
#include "core/storage/StorageNamespace.h"
#include "core/storage/StorageType.h"

namespace Starfish {

class StorageInternal;
class StorageManager;
class Window;

using GCWebOriginToStorageMap =
    std::unordered_map<WebOrigin*, StorageInternal*, WebOriginHash,
                       WebOriginEqual,
                       gc_allocator_ignore_off_page<
                           std::pair<WebOrigin* const, StorageInternal*>>>;

class StorageNamespaceImpl : public StorageNamespace {
public:
    StorageNamespaceImpl(StorageType storageType,
                         Optional<String*> localStoragePath);
    virtual ~StorageNamespaceImpl(){};

    virtual StorageInternal* storageInternal(WebOrigin* origin) override;

private:
    StorageType m_storageType;
    Optional<String*> m_localStoragePath;

    GCWebOriginToStorageMap m_originToStorage;
};
} // namespace Starfish

#endif
