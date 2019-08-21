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

#if defined(STARFISH_ENABLE_WEBRTC)

#include "StarfishConfig.h"
#include "Starfish.h"

#include "core/modules/mediastream/RTCIceServer.h"

#include "core/dom/DOMException.h"
#include "core/dom/ExecutionContext.h"

namespace Starfish {

RTCIceServer::RTCIceServer()
{
}

RTCIceServer::RTCIceServer(webrtc::PeerConnectionInterface::IceServer& server)
    : m_backend(server)
{
}

GCVector<String*> RTCIceServer::urls()
{
    GCVector<String*> urls;
    for (auto& url : m_backend.urls) {
        urls.push_back(String::fromUTF8(url.data(), url.size()));
    }

    return std::move(urls);
}

void RTCIceServer::setUrls(GCVector<String*>& urls)
{
    m_backend.urls.clear();
    for (auto url : urls) {
        m_backend.urls.push_back(std::string(url->toUTF8NonGCString().data()));
    }
}
}
#endif
