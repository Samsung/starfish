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

#include "StarfishConfig.h"
#include "platform/file/PlatformDirectory.h"
#include "platform/file/PlatformFile.h"
#include "core/util/debug/Trace.h"
#include "core/modules/indexeddb/IDBKey.h"
#include "core/modules/indexeddb/IDBRequest.h"
#include "core/modules/indexeddb/IDBStorageManager.h"
#include "core/modules/indexeddb/MemoryBackingStore.h"

namespace Starfish {

static String* joinPath(String* path)
{
    return path;
}

template <typename... Types>
static String* joinPath(String* first, Types... args)
{
    return first->concat("/")->concat(joinPath(args...));
}

void MemoryBackingStore::open(String* name, unsigned long long version)
{
    String* openPath =
        joinPath(IDBStorageManager::instance().getIDBLocalStoragePath(), name);

    m_openPath = openPath->toUTF8NonGCString();

    PlatformDirectoryUtil::createDirectory(openPath);

    TRACE(IDB, m_openPath);
}

IDBRequestErrorType MemoryBackingStore::addOrPut(String* name, const char* data,
                                                 size_t dataSize, IDBKey* key,
                                                 bool noOverwrite)
{
    Nullable<String*> keyString = key->toString();
    if (!keyString.hasValue()) {
        return IDBRequestErrorType::Unknown;
    }

    String* hashString = String::fromInt64(keyString->hashValue());
    String* path = joinPath(
        String::fromUTF8(m_openPath.c_str(), m_openPath.length()), name);

    PlatformDirectoryUtil::createDirectory(path);

    path = joinPath(path, hashString);
    auto out = PlatformFile::open(path, PlatformFile::FileMode::Write);

    if (!out) {
        return IDBRequestErrorType::Unknown;
    }

    if (noOverwrite && out->size() != 0) {
        return IDBRequestErrorType::OverWriteError;
    }

    TRACE(CSTR(path));
    size_t size = out->write((void*)data, sizeof(char), dataSize);

    return size == dataSize ? IDBRequestErrorType::None
                            : IDBRequestErrorType::Unknown;
}

bool MemoryBackingStore::get(String* name, IDBKey* key, char*& data,
                             size_t& dataSize)
{
    Nullable<String*> keyString = key->toString();
    if (!keyString.hasValue()) {
        return false;
    }

    String* hashString = String::fromInt64(keyString->hashValue());
    String* path =
        joinPath(String::fromUTF8(m_openPath.c_str(), m_openPath.length()),
                 name, hashString);
    TRACE(IDB, CSTR(path));

    auto in = PlatformFile::open(path, PlatformFile::FileMode::Read);
    if (!in) {
        return true;
    }

    dataSize = in->size();
    data = static_cast<char*>(malloc(dataSize));
    if (in->read(data, sizeof(char), dataSize) != dataSize) {
        free(data);
        TRACE(IDB, "read error");
        return false;
    }

    TRACE(IDB, "get data: size(", dataSize, ")");

    return true;
}

} // namespace Starfish

#endif
