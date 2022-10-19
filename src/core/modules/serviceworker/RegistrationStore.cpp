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
std::string RegistrationStoreLocalStorage::s_scriptName("script");

bool RegistrationStoreData::writeJsonData(JsonWriter& writer)
{
    writer.StartObject();
    writer.Member("registrationDataPath") & registrationDataPath;
    writer.Member("scopeURL") & scopeURL;
    writer.Member("scriptURL") & scriptURL;
    writer.Member("scriptPath") & scriptPath;
    writer.EndObject();

    if (writer.HasError()) {
        STARFISH_LOG_WARN("Cannot write RegistrationStoreData");
        return false;
    }

    return true;
}

bool RegistrationStoreData::readJsonData(JsonReader& reader)
{
    reader.StartObject();
    reader.Member("registrationDataPath") & registrationDataPath;
    reader.Member("scopeURL") & scopeURL;
    reader.Member("scriptURL") & scriptURL;
    reader.Member("scriptPath") & scriptPath;
    reader.EndObject();

    if (reader.HasError()) {
        STARFISH_LOG_WARN("Cannot read RegistrationStoreData");
        return false;
    }

    return true;
}

RegistrationStoreLocalStorage::RegistrationStoreLocalStorage(
    const std::string& rootPath)
    : m_rootPath(rootPath)
{
    m_listPath = m_rootPath + "/registrationList";
    LocalStorageHelper::File::mkdirIfNotExists(m_rootPath);
}

void RegistrationStoreLocalStorage::loadRegistrationList()
{
    TRACE(SVCWORKER);

    if (!LocalStorageHelper::File::exists(m_listPath)) {
        return;
    }

    std::string rawJsonString;

    {
        LocalStorageHelper::Reader reader(m_listPath);
        if (!reader.readAll(rawJsonString)) {
            STARFISH_LOG_WARN("Cannot read registration list file");
            return;
        }
    }

    JsonReader jsonReader(rawJsonString.data());

    size_t size = 0;
    jsonReader.StartArray(&size);

    for (size_t i = 0; i < size; i++) {
        auto data = new RegistrationStoreData;
        auto result = data->readJsonData(jsonReader);
        STARFISH_ASSERT(result);

        m_registrationSW.insert(
            std::make_pair(data->scopeURL->hashValue(), data));
    }
    jsonReader.EndArray();

    if (jsonReader.HasError()) {
        STARFISH_LOG_WARN("Cannot read Json data");
    }
}

void RegistrationStoreLocalStorage::saveRegistrationList()
{
    TRACEF(SVCWORKER, "size(%zu)", m_registrationSW.size());

    JsonWriter jsonWriter;
    jsonWriter.StartArray();
    for (const auto& r : m_registrationSW) {
        auto result = r.second->writeJsonData(jsonWriter);
        STARFISH_ASSERT(result);
    }
    jsonWriter.EndArray();

    if (jsonWriter.HasError()) {
        STARFISH_LOG_WARN("Cannot save RegistrationStoreList");
        return;
    }

    {
        LocalStorageHelper::Writer fileWriter(m_listPath);
        fileWriter.write(jsonWriter.GetString(), jsonWriter.GetSize());
    }
}

bool RegistrationStoreLocalStorage::hasRegistraionSW(String* scope)
{
    return m_registrationSW.find(scope->hashValue()) != m_registrationSW.end();
}

void RegistrationStoreLocalStorage::load(ServiceWorkerRegistrationMap& map)
{
    TRACE(SVCWORKER);

    loadRegistrationList();

    for (const auto& registrationSW : m_registrationSW) {
        auto path = registrationSW.second->registrationDataPath;
        if (LocalStorageHelper::File::exists(path)) {
            LocalStorageHelper::Reader fileReader(path);
            std::string buffer;
            if (fileReader.readAll(buffer)) {
                JsonReader reader(buffer.data());
                Message msg;
                msg.archive(reader);

                auto msgname = msg.name();
                auto data =
                    downcast<ServiceWorkerRegistrationData*>(msg.param(0));

                TRACE(SVCWORKER, "load registration", CSTR(data->scope));
                map.insert(std::make_pair(data->scope, data));
            }
        }
    }
}

void RegistrationStoreLocalStorage::add(ServiceWorkerRegistrationData* data)
{
    TRACE(SVCWORKER, CSTR(data->scope));

    auto scopeHash = data->scope->hashValue();
    auto appPath = getInstalledSWDirPath(scopeHash);
    LocalStorageHelper::File::mkdirIfNotExists(appPath);

    auto dataPath = appPath + "/" + s_storeName;

    JsonWriter jsonWriter;
    Message msg("registration");
    msg.addParam(data);
    msg.archive(jsonWriter);

    LocalStorageHelper::Writer fileWriter(dataPath);
    fileWriter.write(jsonWriter.GetString(), jsonWriter.GetSize());

    auto storeData = getRegistraionStoreData(scopeHash);
    storeData->registrationDataPath = dataPath;

    saveRegistrationList();
}

void RegistrationStoreLocalStorage::remove(ServiceWorkerRegistrationData* data)
{
    TRACE(SVCWORKER, CSTR(data->scope));

    auto scopeHash = data->scope->hashValue();
    auto dataPath = getInstalledSWDirPath(scopeHash) + "/" + s_storeName;
    LocalStorageHelper::File::remove(dataPath);

    auto itr = m_registrationSW.find(scopeHash);
    if (itr != m_registrationSW.end()) {
        m_registrationSW.erase(itr);

        saveRegistrationList();
    }
}

void RegistrationStoreLocalStorage::saveWorkerScripts(String* scope,
                                                      String* urlString,
                                                      String* scriptText)
{
    TRACE(SVCWORKER, CSTR(scope), CSTR(urlString));

    auto scopeHash = scope->hashValue();

    auto data = getRegistraionStoreData(scopeHash);
    data->scopeURL = scope;
    data->scriptURL = urlString;

    auto appPath = getInstalledSWDirPath(scopeHash);
    LocalStorageHelper::File::mkdirIfNotExists(appPath);

    data->scriptPath = appPath + "/" + s_scriptName;

    LocalStorageHelper::Writer fileWriter(data->scriptPath);
    auto scriptTextUTF8String = scriptText->toUTF8NonGCString();
    if (!fileWriter.write(scriptTextUTF8String.data(),
                          scriptTextUTF8String.size())) {
        STARFISH_LOG_WARN("Cannot save Worker script");
        return;
    }

    saveRegistrationList();

    return;
}

Nullable<String*> RegistrationStoreLocalStorage::loadWorkerScript(String* scope)
{
    TRACE(HOST);

    auto scopeHash = scope->hashValue();
    auto storeData = findRegistraionStoreData(scopeHash);
    if (!storeData.hasValue()) {
        return nullptr;
    }

    if (!LocalStorageHelper::File::exists(storeData->scriptPath)) {
        return nullptr;
    }

    std::string script;

    {
        LocalStorageHelper::Reader reader(storeData->scriptPath);
        if (!reader.readAll(script)) {
            return nullptr;
        }
    }

    return String::fromUTF8(script.data(), script.size());
}

std::string RegistrationStoreLocalStorage::getInstalledSWDirPath(
    size_t scopeHash)
{
    return m_rootPath + "/" + std::to_string(scopeHash);
}

RegistrationStoreData* RegistrationStoreLocalStorage::getRegistraionStoreData(
    size_t scopeHash)
{
    auto storeData = findRegistraionStoreData(scopeHash);
    if (storeData.hasValue()) {
        return storeData.getValue();
    }

    auto newStoreData = new RegistrationStoreData();
    m_registrationSW.insert(std::make_pair(scopeHash, newStoreData));
    return newStoreData;
}

Nullable<RegistrationStoreData*>
RegistrationStoreLocalStorage::findRegistraionStoreData(size_t scopeHash)
{
    auto itr = m_registrationSW.find(scopeHash);
    if (itr == m_registrationSW.end()) {
        return nullptr;
    }
    return itr->second;
}

} // namespace Starfish
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
