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

#ifndef __StarfishWebStorageNamespaceProvider__
#define __StarfishWebStorageNamespaceProvider__

#include "core/storage/StorageNamespaceProvider.h"

namespace Starfish {

class StorageNamespaceImpl;

class WebStorageNamespaceProvider : public StorageNamespaceProvider {
public:
    static WebStorageNamespaceProvider* create(String* localStoragePath);

    virtual ~WebStorageNamespaceProvider();

    virtual StorageNamespace* createLocalStorageNamespace() override;
    virtual StorageNamespace* createSessionStorageNamespace() override;

private:
    WebStorageNamespaceProvider(String* localStoragePath);
    String* m_localStoragePath;
    GCUnorderedMap<String*, StorageNamespaceImpl*>
        m_localStoragePathToStorageNamespace;
};
} // namespace Starfish

#endif
