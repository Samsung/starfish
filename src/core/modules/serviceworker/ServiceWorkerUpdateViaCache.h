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

#if !defined(__StarfishServiceWorkerUpdateViaCache__)
#define __StarfishServiceWorkerUpdateViaCache__

namespace Starfish {

class String;

enum class ServiceWorkerUpdateViaCache : unsigned {
    Imports,
    All,
    None,
};

class UpdateViaCacheUtils {
public:
    static Optional<ServiceWorkerUpdateViaCache> stringToUpdateViaCache(
        String* upateViaCache)
    {
        if (upateViaCache->equals("imports")) {
            return ServiceWorkerUpdateViaCache::Imports;
        } else if (upateViaCache->equals("all")) {
            return ServiceWorkerUpdateViaCache::All;
        } else if (upateViaCache->equals("none")) {
            return ServiceWorkerUpdateViaCache::None;
        } else {
            STARFISH_LOG_WARN("Invalid UpdateViaCache value");
        }

        return Optional<ServiceWorkerUpdateViaCache>();
    }

    static String* updateViaCacheToString(
        ServiceWorkerUpdateViaCache updateViaCache)
    {
        switch (updateViaCache) {
        case ServiceWorkerUpdateViaCache::Imports:
            return String::createASCIIString("imports");
        case ServiceWorkerUpdateViaCache::All:
            return String::createASCIIString("all");
        case ServiceWorkerUpdateViaCache::None:
            return String::createASCIIString("none");
        }
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
};

} // namespace Starfish

#endif
