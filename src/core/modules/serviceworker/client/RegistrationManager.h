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
    !defined(__StarfishRegistrationManager__)
#define __StarfishRegistrationManager__

namespace Starfish {

class ServiceWorkerOption;
class RegistrationStore;

class RegistrationManager : public gc {
public:
    RegistrationManager(ServiceWorkerOption* option);

    void refreshRegistrationList(const std::string path = "");

    bool isActivatedRegistration(String* scope);
    void startRegisteredServiceWorkerContext(
        ServiceWorkerClientConnection* connection, Id<GlobalScope> id,
        String* scope);

private:
    RegistrationStore* m_registrationStore;
};

} // namespace Starfish

#endif
