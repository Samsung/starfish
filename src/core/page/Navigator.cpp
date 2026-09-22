/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#include "StarfishConfig.h"
#include "Navigator.h"
#include "Starfish.h"
#include "core/dom/Document.h"
#include "core/dom/ExecutionContext.h"
#include "core/modules/location/Geolocation.h"
#include "core/page/BrowsingContext.h"
#include "core/page/WebView.h"
#ifdef STARFISH_ENABLE_SERVICE_WORKER
#include "core/modules/serviceworker/ServiceWorkerContainer.h"
#endif

#if defined(STARFISH_ENABLE_WEBRTC)
#include "core/modules/mediastream/WebRtcManager.h"
#include "core/modules/mediastream/MediaStream.h"
#endif

namespace Starfish {

Navigator::Navigator(Document* document)
    : ScriptWrappable(this)
    , DocumentHoldable(document)
    , NavigatorMixin(document->executionContext())
    , m_geolocation(nullptr)
#ifdef STARFISH_ENABLE_SERVICE_WORKER
    , m_serviceWorker(nullptr)
#endif
{
}

Geolocation* Navigator::geolocation()
{
    if (m_geolocation == nullptr) {
        m_geolocation = Geolocation::create(executionContext()->document());
    }
    return m_geolocation;
}

// A touch panel does not tell the engine how many contacts it tracks, so the
// count reported here is the conventional one for the phone and TV panels
// this engine targets. What content branches on is whether it is above zero:
// that is the feature test for touch input.
// https://w3c.github.io/pointerevents/#dom-navigator-maxtouchpoints
int32_t Navigator::maxTouchPoints()
{
    const int32_t typicalPanelContactCount = 5;
    StarfishDeviceKind deviceKind = executionContext()
                                        ->document()
                                        ->browsingContext()
                                        ->webView()
                                        ->deviceKind();
    return (deviceKind & deviceKindUseTouchScreen) ? typicalPanelContactCount
                                                   : 0;
}

#ifdef STARFISH_ENABLE_MULTIMEDIA
MediaCapabilities* Navigator::mediaCapabilities()
{
    if (!m_mediaCapabilities) {
        m_mediaCapabilities = new MediaCapabilities(executionContext());
    }
    return m_mediaCapabilities.value();
}
#endif
#ifdef STARFISH_ENABLE_SERVICE_WORKER
ServiceWorkerContainer* Navigator::serviceWorker()
{
    if (m_serviceWorker == nullptr) {
        m_serviceWorker = new ServiceWorkerContainer(executionContext());
    }
    return m_serviceWorker;
}
#endif

#if defined(STARFISH_ENABLE_WEBRTC)
WebRtcManager* Navigator::webRtcManager()
{
    if (m_webRtcManager == nullptr) {
        m_webRtcManager = new WebRtcManager();
    }
    return m_webRtcManager;
}
#endif

void Navigator::dispose()
{
    if (m_geolocation) {
        m_geolocation->dispose();
    }

#ifdef STARFISH_ENABLE_SERVICE_WORKER
    if (m_serviceWorker) {
        m_serviceWorker->dispose();
    }
#endif

#if defined(STARFISH_ENABLE_WEBRTC)
    if (m_webRtcManager) {
        m_webRtcManager->dispose();
    }
#endif
}

ScriptBindingInstance* Navigator::scriptBindingInstance()
{
    return executionContext()->scriptBindingInstance();
}

} // namespace Starfish
