/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#if defined(STARFISH_ENABLE_CAST_SERVICE) && !defined(__StarfishCastConfig__)
#define __StarfishCastConfig__

namespace Starfish {

#define SSDP_GROUP "239.255.255.250"
#define SSDP_PORT 1900
#define SSDP_ST "urn:dial-multiscreen-org:service:dial:1"

#define LOCATION_PORT 5696
#define LOCATION_DESC "/deviceDescription.xml"

extern const char* templateDeviceDescription;
extern const char* templateMSearchResponse;

class CastConfig : public gc {
public:
    CastConfig() = default;

    DEFINE_GETTER_SETTER(String*, localAddress, LocalAddress);

private:
    String* m_localAddress{ String::emptyString };
};

} // namespace Starfish

#endif
