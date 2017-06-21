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

#include "core/storage/Storage.h"
#include "core/storage/StorageType.h"

namespace StarFish {

class SecurityOriginData;
class StorageManager;

class StorageImpl : public Storage {
public:
    StorageImpl(Window* window, StorageType storageType,
                SecurityOriginData* securityOriginData,
                StorageManager* storageManager);
    virtual ~StorageImpl();

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isStorageImpl() const override;

    unsigned long length() override;
    Nullable<String*> key(unsigned long index) override;
    Nullable<String*> getItem(String* key) override;
    void setItem(String* key, String* value) override;
    void removeItem(String* key) override;
    void clear() override;

    void copyDataFrom(StorageImpl* storage);

private:
    StorageImpl(Window* window);

    StorageType m_storageType;
    SecurityOriginData* m_securityOriginData;
    StorageManager* m_storageManager;

    GCUnorderedMap<String*, String*>* m_map;
};
}

#endif
