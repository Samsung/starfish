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

#ifdef STARFISH_ENABLE_CAST_SERVICE

#include "StarfishConfig.h"
#include "StarfishInfo.h"

#include "core/modules/cast/CastConfig.h"

#define DEVICE_VENDOR_NAME VENDOR_NAME
#define DEVICE_MODEL_NAME STARFISH_NAME
#define DEVICE_PRODUCT_NAME STARFISH_NAME
#define DEVICE_PRODUCT_VERSION VERSION
#define DEVICE_TYPE "urn:dial-multiscreen-org:device:dialreceiver:1"
#define DEVICE_UUID "9ad8fd1a-e0f5-44e9-8322-61dd08533c04"
#define DEVICE_FRIENDLY_NAME "LWE:StarFish"
#define DEVICE_PRODUCT_NAME_AND_VERSION \
    DEVICE_PRODUCT_NAME "/" DEVICE_PRODUCT_VERSION

namespace Starfish {

// clang-format off
const char* templateDeviceDescription =
    "<?xml version=\"1.0\"?>\r\n"
    "<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\r\n"
    "   <specVersion>\r\n"
    "       <major>1</major>\r\n"
    "       <minor>0</minor>\r\n"
    "   </specVersion>\r\n"
    "   <device>\r\n"
    "       <deviceType>" DEVICE_TYPE "</deviceType>\r\n"
    "       <friendlyName>" DEVICE_FRIENDLY_NAME "</friendlyName>\r\n"
    "       <manufacturer>" DEVICE_VENDOR_NAME "</manufacturer>\r\n"
    "       <modelName>" DEVICE_MODEL_NAME "</modelName>\r\n"
    "       <UDN>uuid:" DEVICE_UUID "</UDN>\r\n"
    "   </device>\r\n"
    "</root>\r\n"
    "\r\n";

const char* templateMSearchResponse =
    "HTTP/1.1 200 OK\r\n"
    "LOCATION: http://%s:%d" LOCATION_DESC "\r\n"
    "CACHE-CONTROL: max-age=1800\r\n"
    "EXT:\r\n"
    "BOOTID.UPNP.ORG: 1\r\n"
    "SERVER: " DEVICE_PRODUCT_NAME_AND_VERSION " UPnP/1.1\r\n"
    "USN: uuid:" DEVICE_UUID "\r\n"
    "ST: urn:dial-multiscreen-org:service:dial:1\r\n"
    "%s" // should be filled if `WAKEUP` is supported.
    "\r\n";
// clang-format on

} // namespace Starfish

#endif
