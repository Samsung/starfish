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
    void setItem(String* key, String* value);
    void removeItem(String* key);
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
