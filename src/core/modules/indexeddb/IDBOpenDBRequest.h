/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_IDB)

#ifndef __StarfishIDBOpenDBRequest__
#define __StarfishIDBOpenDBRequest__

#include "core/modules/indexeddb/IDBTaskQueue.h"
#include "core/modules/indexeddb/IDBDatabaseIdentifier.h"

namespace Starfish {

class IDBConnection;

struct OpenDBRequestData : public IDBTaskQueueItemData {
    String* name{ nullptr };
    Optional<unsigned long long> version;
    bool upgradeNeeded{ false };
    IDBConnection* connection{ nullptr };
    WebOrigin* webOrigin{ nullptr };
    IDBDatabaseIdentifier identifier;
};

class IDBOpenDBRequest : public IDBRequest {
public:
    IDBOpenDBRequest(ExecutionContext* executionContext);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isIDBOpenDBRequest() const;

    bool isOpenDBRequest() override
    {
        return true;
    }

    void successOpenRequest();
    void failOpenRequest(IDBRequestErrorType error);

    void upgradeNeeded();

    DEFINE_GETTER_SETTER(IDBDatabase*, database, Database);

#define VIRTUAL
#define OVERRIDE
    DECLARE_EVENT_LISTENER(upgradeneeded);
#undef VIRTUAL
#undef OVERRIDE

private:
    IDBDatabase* m_database;
};

} // namespace Starfish

#endif
#endif
