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

#if defined(STARFISH_ENABLE_SERVICE_WORKER) && \
    !defined(__StarfishServiceWorkerRegistrationStore__)
#define __StarfishServiceWorkerRegistrationStore__

#include "core/modules/serviceworker/ServiceWorkerTypes.h"
#include "core/modules/serviceworker/host/ServiceWorkerHostJobHandler.h"

namespace Starfish {

class ServiceWorkerRegistrationData;
class FetchCacheStream;
class JsonWriter;
class JsonReader;

struct RegistrationStoreData : public gc {
    size_t scopeHash{ 0 };
    std::string registrationDataPath;
    String* scriptURL;
    std::string scriptPath;

    bool writeJsonData(JsonWriter& writer);
    bool readJsonData(JsonReader& reader);
};

class RegistrationStore : public gc {
public:
    virtual ~RegistrationStore(){};

    virtual void load(ServiceWorkerRegistrationMap& map) = 0;
    virtual void add(ServiceWorkerRegistrationData* data) = 0;
    virtual void remove(ServiceWorkerRegistrationData* data) = 0;
    virtual void loadRegistrationList() = 0;
    virtual void saveWorkerScripts(String* scope, String* urlString,
                                   String* scriptText) = 0;
    virtual bool hasRegistraionSW(String* scope) = 0;

protected:
    RegistrationStore(){};
};

class RegistrationStoreLocalStorage final : public RegistrationStore {
public:
    RegistrationStoreLocalStorage(const std::string& rootPath);
    virtual ~RegistrationStoreLocalStorage(){};

    void load(ServiceWorkerRegistrationMap& map) override;
    void add(ServiceWorkerRegistrationData* data) override;
    void remove(ServiceWorkerRegistrationData* data) override;
    void loadRegistrationList() override;
    void saveWorkerScripts(String* scope, String* urlString,
                           String* scriptText) override;
    bool hasRegistraionSW(String* scope) override;

private:
    static std::string s_storeName;
    static std::string s_scriptName;

    std::string m_rootPath;
    std::string m_listPath;
    GCUnorderedMap<size_t, RegistrationStoreData*> m_registrationSW;

    std::string getInstalledSWDirPath(size_t scopeHash);
    void saveRegistrationList();
    RegistrationStoreData* getRegistraionStoreData(size_t scopeHash);
};

} // namespace Starfish

#endif
