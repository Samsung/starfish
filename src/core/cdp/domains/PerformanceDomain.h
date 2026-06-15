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

#if defined(STARFISH_ENABLE_CDP) && !defined(__StarfishCDPPerformanceDomain__)
#define __StarfishCDPPerformanceDomain__

#include <string>

namespace Starfish {

class CDPDispatcher;
class CDPCommand;

// Performance domain. Implements enable/disable (state ack) and getMetrics,
// which reports the metric set puppeteer's page.metrics() maps. Values are
// sampled live from the engine (GC heap, DOM node count, frame count) where
// available; metrics this engine does not track are reported as 0 so the
// client receives the full expected key set without throwing.
class PerformanceDomain {
public:
    PerformanceDomain(CDPDispatcher* d)
        : m_dispatcher(d)
    {
    }
    void processMessage(CDPCommand& cmd, const std::string& method);

private:
    CDPDispatcher* m_dispatcher;
};

} // namespace Starfish

#endif
