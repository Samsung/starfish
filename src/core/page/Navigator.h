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

#ifndef __StarfishNavigator__
#define __StarfishNavigator__

#include "binding/ScriptWrappable.h"
#include "core/page/NavigatorMixin.h"
#include "core/modules/battery/Battery.h"
#include "core/page/MediaCapabilities.h"

#ifdef STARFISH_ENABLE_WEBRTC
#include "core/modules/mediastream/MediaDevices.h"
#endif

namespace Starfish {

class Starfish;
class Geolocation;
#ifdef STARFISH_ENABLE_SERVICE_WORKER
class ServiceWorkerContainer;
#endif

#ifdef STARFISH_ENABLE_WEBRTC
class WebRtcManager;
#endif

class Navigator : public ScriptWrappable,
                  public DocumentHoldable,
                  public NavigatorMixin {
public:
    Navigator(Document* document);

    virtual void init(ScriptBindingInstance* instance,
                      void* domObjectPointer) override;
    virtual bool isNavigator() const override;
    virtual ScriptBindingInstance* scriptBindingInstance() override;

    Geolocation* geolocation();

    void dispose();

    bool cookieEnabled()
    {
        return true;
    }

    bool javaEnabled()
    {
        return false;
    }

#ifdef STARFISH_ENABLE_MULTIMEDIA
    MediaCapabilities* mediaCapabilities();
#endif

protected:
    Geolocation* m_geolocation;
#ifdef STARFISH_ENABLE_MULTIMEDIA
    Optional<MediaCapabilities*> m_mediaCapabilities;
#endif

#ifdef STARFISH_ENABLE_SERVICE_WORKER
public:
    ServiceWorkerContainer* serviceWorker();

protected:
    ServiceWorkerContainer* m_serviceWorker;
#endif

#ifdef STARFISH_ENABLE_WEBRTC
public:
    WebRtcManager* webRtcManager();
    MediaDevices* mediaDevices();

protected:
    WebRtcManager* m_webRtcManager{ nullptr };
    MediaDevices* m_mediaDevices{ nullptr };
#endif

#ifdef STARFISH_ENABLE_BATTERY_STATUS
public:
    Promise* getBattery();
#ifdef STARFISH_ENABLE_OBSOLETE_SPEC
    BatteryManager* battery();
#endif

protected:
    Promise* m_batteryPromise;
    BatteryManager* m_batteryManager;
#endif
};
} // namespace Starfish

#endif
