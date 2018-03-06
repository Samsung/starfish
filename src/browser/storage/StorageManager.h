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

#ifndef __StarFishStorageManager__
#define __StarFishStorageManager__

namespace StarFish {

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
    void* m_jsonHolder;
};
}

#endif
