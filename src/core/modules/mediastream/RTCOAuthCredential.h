/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

// FIXME: The binding generator uses this class to genereate
// a union type of DOMString and RTCOAuthCredential even if
// the union is used only in WebRTC. To work around, this
// class is always enabled.

// #if defined(STARFISH_ENABLE_WEBRTC)

#ifndef __StarfishRTCOAuthCredential__
#define __StarfishRTCOAuthCredential__

namespace Escargot {

class ExecutionStateRef;
class ValueRef;

} // namespace Escargot

namespace Starfish {
class String;
class RTCOAuthCredential;

extern RTCOAuthCredential toRTCOAuthCredentialFromValueRef(
    Escargot::ExecutionStateRef* state, Escargot::ValueRef* from);
extern Escargot::ValueRef* toValueRefFromRTCOAuthCredential(
    Escargot::ExecutionStateRef* state, const RTCOAuthCredential& from);

struct RTCOAuthCredential : public gc {
    DEFINE_GETTER_SETTER(String*, macKey, MacKey)
    DEFINE_GETTER_SETTER(String*, accessToken, AccessToken)

    String* m_macKey{ String::emptyString };
    String* m_accessToken{ String::emptyString };
};
} // namespace Starfish
#endif
// #endif
