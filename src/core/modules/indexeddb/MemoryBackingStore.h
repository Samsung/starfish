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

#ifndef __StarfishMemoryBackingStore__
#define __StarfishMemoryBackingStore__

#include "core/modules/indexeddb/IDBBackingStore.h"

namespace Starfish {

class ExecutionContext;
class IDBOpenDBRequest;
class IDBKey;

// NOTE: The MemoryBackingStore class is called only on the main work thread of
// IDBTaskQueue.

class MemoryBackingStore final : public IDBBackingStore {
public:
    void open(String* name, unsigned long long version) override;

    IDBRequestErrorType addOrPut(String* name, const char* data,
                                 size_t dataSize, IDBKey* key,
                                 bool noOverwrite) override;

    bool get(String* name, IDBKey* key, char*& data, size_t& dataSize) override;

private:
    std::string m_openPath;
};
} // namespace Starfish

#endif
#endif
