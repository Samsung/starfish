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
#include "core/modules/indexeddb/IDBRequest.h"
#include "core/modules/indexeddb/IDBStorageManager.h"
#include "core/modules/indexeddb/MemoryBackingStore.h"

namespace Starfish {

// The strings built here live on the IDB thread's own heap and only serve
// the file-system calls below.
static String* pathString(const std::string& s)
{
    return String::fromUTF8(s.data(), s.length());
}

static String* joinPath(String* first)
{
    return first;
}

template <typename... Types>
static String* joinPath(String* first, Types... args)
{
    return first->concat("/")->concat(joinPath(args...));
}

void MemoryBackingStore::open(const std::string& name,
                              unsigned long long version)
{
    String* openPath =
        joinPath(IDBStorageManager::instance().getIDBLocalStoragePath(),
                 pathString(name));

    m_openPath = openPath->toUTF8NonGCString();

    PlatformDirectoryUtil::createDirectory(openPath);

    TRACE(IDB, m_openPath);
}

IDBRequestErrorType MemoryBackingStore::addOrPut(const std::string& storeName,
                                                 const char* data,
                                                 size_t dataSize,
                                                 int64_t keyHash,
                                                 bool noOverwrite)
{
    String* path = joinPath(pathString(m_openPath), pathString(storeName));

    PlatformDirectoryUtil::createDirectory(path);

    path = joinPath(path, String::fromInt64(keyHash));
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

bool MemoryBackingStore::get(const std::string& storeName, int64_t keyHash,
                             char*& data, size_t& dataSize)
{
    String* path = joinPath(pathString(m_openPath), pathString(storeName),
                            String::fromInt64(keyHash));
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
