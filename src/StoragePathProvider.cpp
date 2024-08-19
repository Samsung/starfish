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

#include "StarfishConfig.h"
#include "platform/file/PlatformDirectory.h"
#include "platform/file/PlatformFile.h"
#include "StoragePathProvider.h"

namespace Starfish {

#define STARFISH_LOCAL_STORAGE_FILE_NAME "localStorage.txt"
#define STARFISH_COOKIES_FILE_NAME "cookies.txt"
#define STARFISH_CACHE_DIR_NAME "cache"
#define STARFISH_SHARED_WORKER_DIR_NAME "shared_worker"
#define STARFISH_SERVICE_WORKER_DIR_NAME "service_worker"

StoragePathProvider::StoragePathProvider(const char* storageDirectoryPath)
    : m_storageDirectoryPath(storageDirectoryPath)
{
    STARFISH_RELEASE_ASSERT(storageDirectoryPath != nullptr);

    PlatformDirectoryUtil::createDirectory(String::fromUTF8(
        m_storageDirectoryPath.data(), m_storageDirectoryPath.size()));
}

std::string StoragePathProvider::getLocalStorageDataFilePath() const
{
    return PlatformFileUtil::joinPath(m_storageDirectoryPath,
                                      STARFISH_LOCAL_STORAGE_FILE_NAME);
}

std::string StoragePathProvider::getCookieStoreDataFilePath() const
{
    return PlatformFileUtil::joinPath(m_storageDirectoryPath,
                                      STARFISH_COOKIES_FILE_NAME);
}

std::string StoragePathProvider::getHttpCacheDataDirectoryPath() const
{
    return PlatformFileUtil::joinPath(m_storageDirectoryPath,
                                      STARFISH_CACHE_DIR_NAME);
}

std::string StoragePathProvider::getSharedWorkerDataDirectoryPath() const
{
    return PlatformFileUtil::joinPath(m_storageDirectoryPath,
                                      STARFISH_SHARED_WORKER_DIR_NAME);
}

std::string StoragePathProvider::getServiceWorkerDataDirectoryPath() const
{
    return PlatformFileUtil::joinPath(m_storageDirectoryPath,
                                      STARFISH_SERVICE_WORKER_DIR_NAME);
}

} // namespace Starfish
