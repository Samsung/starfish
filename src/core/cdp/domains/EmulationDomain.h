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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPEmulationDomain__)
#define __StarfishCDPEmulationDomain__

#include <string>
#include <cstdint>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;

// Emulation domain (MVP). setDeviceMetricsOverride / clearDeviceMetricsOverride
// apply a real viewport size + device pixel ratio change to the WebView;
// setUserAgentOverride sets the custom user agent; setGeolocationOverride /
// clearGeolocationOverride install the coordinates navigator.geolocation
// returns; setEmulatedMedia overrides the CSS media type and the
// prefers-color-scheme / prefers-reduced-motion media features used by
// matchMedia. setScriptExecutionDisabled blocks page script execution (without
// disabling inspector evaluation); setVisibleSize resizes the viewport via the
// device-metrics path. setTimezoneOverride / setLocaleOverride /
// setCPUThrottlingRate are acked without effect (the engine fixes timezone /
// locale at construction and has no throttling). Other Emulation.* methods are
// acked as no-ops. Handlers run on the main thread.
class EmulationDomain {
public:
    EmulationDomain(CDPDispatcher* d)
        : m_dispatcher(d)
        , m_hasSaved(false)
        , m_savedWidth(0)
        , m_savedHeight(0)
        , m_savedDpr(1)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

private:
    CDPDispatcher* m_dispatcher;
    // Renderer physical size + DPR captured before the first metrics override,
    // used to restore on clearDeviceMetricsOverride.
    bool m_hasSaved;
    uint32_t m_savedWidth;
    uint32_t m_savedHeight;
    float m_savedDpr;
};

} // namespace Starfish

#endif
