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

#ifndef __StarFishStorageImpl__
#define __StarFishStorageImpl__

#include "core/storage/StorageType.h"

namespace StarFish {

class SecurityOriginData;
class StorageManager;

class StorageImpl : public gc {
public:
    StorageImpl(StorageType storageType, SecurityOriginData* securityOriginData,
                StorageManager* storageManager);
    virtual ~StorageImpl();

    unsigned long length();
    Nullable<String*> key(unsigned long index);
    Nullable<String*> getItem(String* key);
    GCVector<String*> getKeyNames();
    bool setItem(String* key, String* value);
    bool removeItem(String* key);
    void clear();

private:
    StorageImpl();

    StorageType m_storageType;
    SecurityOriginData* m_securityOriginData;
    StorageManager* m_storageManager;

    GCUnorderedMap<String*, String*>* m_map;
};
}

#endif
