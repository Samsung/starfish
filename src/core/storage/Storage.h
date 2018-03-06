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

#ifndef __StarFishStorage__
#define __StarFishStorage__

#include "binding/ScriptWrappable.h"
#include "binding/WindowHoldable.h"
#include "core/storage/StorageType.h"

namespace StarFish {

class SecurityOriginData;
class StorageManager;
class StorageImpl;

class Storage : public ScriptWrappable, public WindowHoldable {
public:
    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isStorage() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override
    {
        return WindowHoldable::scriptBindingInstance();
    }

    Storage(Window* window, StorageImpl* storageImpl);

    // 4.1 Storage interface in IDL
    unsigned long length();
    Nullable<String*> key(unsigned long index);
    Nullable<String*> getItem(String* key);
    bool setItem(String* key, String* value);
    bool removeItem(String* key);
    void clear();
    void defaultNamedEnumerator(GCVector<String*>& enums);
    bool defaultNamedDeleter(String* key);

private:
    Storage();

    StorageImpl* m_storageImpl;
};
}

#endif
