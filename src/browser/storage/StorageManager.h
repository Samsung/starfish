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

#ifndef __StarFishStorageManager__
#define __StarFishStorageManager__

namespace StarFish {

struct JSONDocumentHolder {
    void* m_ptr;
};

class FileIO;
class SecurityOriginData;

class StorageManager : public gc {
public:
    StorageManager(String* localStoragePath);
    ~StorageManager()
    {
    }
    Nullable<String*> key(SecurityOriginData* securityOriginData,
                          unsigned long index);
    Nullable<String*> getItem(SecurityOriginData* securityOriginData,
                              String* key);
    GCUnorderedMap<String*, String*>* getItems(
        SecurityOriginData* securityOriginData);
    void setItem(SecurityOriginData* securityOriginData, String* key,
                 String* value);
    void removeItem(SecurityOriginData* securityOriginData, String* key);
    void clear(SecurityOriginData* securityOriginData);
    unsigned long length(SecurityOriginData* securityOriginData);

private:
    void jsonDocumentRead();
    void jsonDocumentWrite();
    String* m_localStoragePath;
    JSONDocumentHolder m_jsonHolder;
};
}

#endif
