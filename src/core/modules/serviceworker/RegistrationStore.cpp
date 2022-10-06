/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#ifdef STARFISH_ENABLE_SERVICE_WORKER

#include "StarfishConfig.h"

#include "platform/loader/ResourceURL.h"
#include "core/util/Id.h"
#include "core/util/Archiver.h"
#include "core/util/Archivable.h"
#include "core/modules/serviceworker/ServiceWorkerOption.h"
#include "core/modules/serviceworker/util/LocalStorageHelper.h"
#include "core/modules/serviceworker/Message.h"
#include "core/modules/serviceworker/ServiceWorkerRegistrationData.h"
#include "core/modules/serviceworker/RegistrationStore.h"

namespace Starfish {

std::string RegistrationStoreLocalStorage::s_storeName("registration");

RegistrationStoreLocalStorage::RegistrationStoreLocalStorage(
    const std::string& rootPath)
    : m_rootPath(rootPath)
{
    m_listPath = m_rootPath + "/registrationList";
    LocalStorageHelper::File::mkdirIfNotExists(m_rootPath);
}

void RegistrationStoreLocalStorage::saveRegistrationList()
{
    TRACEF(SVCWORKER, "size(%zu)", m_registrationSW.size());

    LocalStorageHelper::Writer fileWriter(m_listPath);
    fileWriter.write(m_registrationSW.size(), " ");
    for (const auto& r : m_registrationSW) {
        fileWriter.write(r.first, " ");
        fileWriter.writeString(r.second);
    }
}

void RegistrationStoreLocalStorage::load(ServiceWorkerRegistrationMap& map)
{
    TRACE(SVCWORKER);

    if (!LocalStorageHelper::File::exists(m_listPath)) {
        return;
    }

    LocalStorageHelper::Reader reader(m_listPath);
    size_t size = 0;
    reader.read(size);

    for (size_t i = 0; i < size; i++) {
        size_t hash;
        reader.read(hash);
        std::string path;
        reader.readString(path);

        LocalStorageHelper::Reader fileReader(path);
        std::string buffer;
        if (fileReader.readAll(buffer)) {
            JsonReader reader(buffer.data());
            Message msg;
            msg.archive(reader);

            auto msgname = msg.name();
            auto data = downcast<ServiceWorkerRegistrationData*>(msg.param(0));
            map.insert(std::make_pair(data->scope, data));
        }
    }
}

void RegistrationStoreLocalStorage::add(ServiceWorkerRegistrationData* data)
{
    TRACE(SVCWORKER, CSTR(data->scope));

    auto appPath = getInstalledSWDirPath(data->scope);
    LocalStorageHelper::File::mkdirIfNotExists(appPath);

    auto dataPath = appPath + "/" + s_storeName;

    JsonWriter jsonWriter;
    Message msg("registration");
    msg.addParam(data);
    msg.archive(jsonWriter);

    LocalStorageHelper::Writer fileWriter(dataPath);
    fileWriter.write(jsonWriter.GetString(), jsonWriter.GetSize());

    m_registrationSW.insert(std::make_pair(data->scope->hashValue(), dataPath));

    saveRegistrationList();
}

void RegistrationStoreLocalStorage::remove(ServiceWorkerRegistrationData* data)
{
    TRACE(SVCWORKER, CSTR(data->scope));

    auto dataPath = getInstalledSWDirPath(data->scope) + "/" + s_storeName;
    LocalStorageHelper::File::remove(dataPath);

    auto itr = m_registrationSW.find(data->scope->hashValue());
    if (itr != m_registrationSW.end()) {
        m_registrationSW.erase(itr);

        saveRegistrationList();
    }
}

void RegistrationStoreLocalStorage::loadRegistrationList(
    std::unordered_map<size_t, std::string>& list)
{
    TRACE(SVCWORKER);

    if (!LocalStorageHelper::File::exists(m_listPath)) {
        return;
    }

    LocalStorageHelper::Reader reader(m_listPath);
    size_t size = 0;
    reader.read(size);

    for (size_t i = 0; i < size; i++) {
        size_t hash;
        reader.read(hash);

        std::string path;
        reader.readString(path);

        list.insert(std::make_pair(hash, std::move(path)));
    }
}

std::string RegistrationStoreLocalStorage::getInstalledSWDirPath(
    String* scopeURL)
{
    return m_rootPath + "/" + std::to_string(scopeURL->hashValue());
}

} // namespace Starfish
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
