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

#ifndef __StarfishStorageInternal__
#define __StarfishStorageInternal__

#include "core/storage/StorageType.h"

namespace Starfish {

class WebOrigin;
class StorageManager;

using StorageKey = WebOrigin;

class StorageInternal : public gc {
public:
    StorageInternal(StorageType storageType, WebOrigin* webOrigin);
    virtual ~StorageInternal();

    virtual unsigned long length() = 0;
    virtual Optional<String*> key(unsigned long index) = 0;
    virtual Optional<String*> getItem(String* key) = 0;
    virtual bool setItem(String* key, String* value) = 0;
    virtual bool removeItem(String* key) = 0;
    virtual void clear() = 0;

    virtual GCVector<String*> getKeyNames() = 0;

    static Optional<StorageKey*> getStorageKey(ExecutionContext* context);

protected:
    StorageType m_storageType;
    WebOrigin* m_webOrigin = nullptr;

private:
    StorageInternal();
};

class StorageMemory : public StorageInternal {
public:
    StorageMemory(StorageType storageType, WebOrigin* webOrigin);
    virtual ~StorageMemory(){};

    unsigned long length() override;
    Optional<String*> key(unsigned long index) override;
    Optional<String*> getItem(String* key) override;
    bool setItem(String* key, String* value) override;
    bool removeItem(String* key) override;
    void clear() override;

    GCVector<String*> getKeyNames() override;

private:
    GCUnorderedMap<String*, String*> m_map;
};
} // namespace Starfish

#endif
