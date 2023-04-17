/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ENABLE_WEBRTC)

#ifndef __StarfishRTCRtpParameters__
#define __StarfishRTCRtpParameters__

namespace Starfish {

struct RTCRtcpParameters {
    RTCRtcpParameters();

    // Define getter/setters
    DEFINE_GETTER_SETTER_WITH_HASFLAG(String*, cname, Cname);
    DEFINE_GETTER_SETTER_WITH_HASFLAG(bool, reducedSize, ReducedSize);

    // Define memebers
    DEFINE_MEMBER_WITH_HASFLAG(String*, cname, Cname);
    DEFINE_MEMBER_WITH_HASFLAG(bool, reducedSize, ReducedSize);
};

} // namespace Starfish

#endif
#endif
