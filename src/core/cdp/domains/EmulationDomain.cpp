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

#if defined(STARFISH_ENABLE_CDP)

#include "StarfishConfig.h"
#include "Starfish.h"
#include "EmulationDomain.h"
#include "../CDPDispatcher.h"
#include "../CDPCommand.h"
#include "core/page/WebView.h"
#include "core/modules/renderer/Renderer.h"
#include "core/modules/location/Geolocation.h"
#include "core/style/MediaQueryEvaluator.h"

#include "rapidjson/document.h"

#include <string>
#include <cstring>

namespace Starfish {

static double paramNumber(CDPCommand& cmd, const char* name, double def)
{
    if (cmd.params() && cmd.params()->HasMember(name) &&
        (*cmd.params())[name].IsNumber()) {
        return (*cmd.params())[name].GetDouble();
    }
    return def;
}

static std::string paramString(CDPCommand& cmd, const char* name)
{
    if (cmd.params() && cmd.params()->HasMember(name) &&
        (*cmd.params())[name].IsString()) {
        return (*cmd.params())[name].GetString();
    }
    return std::string();
}

// Apply a CSS-pixel viewport (width/height) and device pixel ratio to the
// WebView. The renderer holds *physical* pixels; WebView::resize divides those
// by the DPR to derive the logical Window size (which backs innerWidth/Height).
// So to land logical width/height at the requested CSS values, set DPR first,
// then resize the renderer to width*dpr / height*dpr.
static void applyMetrics(WebView* wv, uint32_t cssWidth, uint32_t cssHeight,
                         double dpr)
{
    Renderer* r = wv->renderer();
    if (!r) {
        return;
    }
    if (dpr > 0) {
        r->setDevicePixelRatio((float)dpr);
    }
    float effDpr = wv->screenInfo().devicePixelRatio;
    if (effDpr <= 0) {
        effDpr = 1;
    }
    uint32_t physW = (uint32_t)(cssWidth * effDpr + 0.5);
    uint32_t physH = (uint32_t)(cssHeight * effDpr + 0.5);
    r->resizeTo(physW, physH);
    wv->layoutIfNeeded();
}

void EmulationDomain::processMessage(CDPCommand& cmd, const std::string& method)
{
    WebView* wv = m_dispatcher->webView();
    if (!wv) {
        cmd.sendError(-32000, "No WebView");
        return;
    }

    if (method == "setDeviceMetricsOverride") {
        double width = paramNumber(cmd, "width", 0);
        double height = paramNumber(cmd, "height", 0);
        double dpr = paramNumber(cmd, "deviceScaleFactor", 0);
        // Snapshot the original renderer physical size + DPR once, so a later
        // clearDeviceMetricsOverride can restore it.
        if (!m_hasSaved && wv->renderer()) {
            m_savedWidth = wv->renderer()->width();
            m_savedHeight = wv->renderer()->height();
            m_savedDpr = wv->screenInfo().devicePixelRatio;
            m_hasSaved = true;
        }
        // width/height of 0 means "do not override" in CDP. Skip the resize in
        // that case but still honor a standalone DPR change.
        if (width > 0 && height > 0) {
            applyMetrics(wv, (uint32_t)width, (uint32_t)height, dpr);
        } else if (dpr > 0) {
            Renderer* r = wv->renderer();
            if (r) {
                r->setDevicePixelRatio((float)dpr);
                wv->layoutIfNeeded();
            }
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "clearDeviceMetricsOverride") {
        // Restore the renderer physical size + DPR captured before the first
        // override (if any). setDevicePixelRatio first since resizeTo derives
        // the logical Window size from physical pixels / DPR.
        Renderer* r = wv->renderer();
        if (r && m_hasSaved) {
            r->setDevicePixelRatio(m_savedDpr > 0 ? m_savedDpr : 1.0f);
            r->resizeTo(m_savedWidth, m_savedHeight);
            wv->layoutIfNeeded();
            m_hasSaved = false;
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "setUserAgentOverride") {
        std::string ua = paramString(cmd, "userAgent");
        // Empty userAgent clears the override (falls back to the built-in UA).
        wv->setCustomUserAgentString(String::fromUTF8(ua.c_str(), ua.size()));
        cmd.sendResultEmpty();
        return;
    }

    if (method == "setGeolocationOverride") {
        // Per CDP, an empty params object (no latitude/longitude/accuracy)
        // emulates "position unavailable"; otherwise install the override
        // coordinates that navigator.geolocation will return.
        bool hasLat = cmd.params() && cmd.params()->HasMember("latitude") &&
                      (*cmd.params())["latitude"].IsNumber();
        bool hasLng = cmd.params() && cmd.params()->HasMember("longitude") &&
                      (*cmd.params())["longitude"].IsNumber();
        if (hasLat && hasLng) {
            double lat = paramNumber(cmd, "latitude", 0);
            double lng = paramNumber(cmd, "longitude", 0);
            double accuracy = paramNumber(cmd, "accuracy", 0);
            Geolocation::setOverride(lat, lng, accuracy);
        } else {
            // Position-unavailable emulation: drop any active override so the
            // backend falls back to its POSITION_UNAVAILABLE error path.
            Geolocation::clearOverride();
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "clearGeolocationOverride") {
        Geolocation::clearOverride();
        cmd.sendResultEmpty();
        return;
    }

    if (method == "setEmulatedMedia") {
        // Install / clear the CSS media query overrides consulted by
        // MediaQueryEvaluator at matchMedia() evaluation time.
        //
        // params.media   : media type to emulate ("screen" / "print"); empty
        //                  string clears the media type override.
        // params.features: [{name, value}] feature overrides; we support
        //                  prefers-color-scheme (light/dark) and
        //                  prefers-reduced-motion (no-preference/reduce). An
        //                  empty / absent array clears feature overrides.

        // Media type override. CDP omits the field to leave it unchanged, but
        // puppeteer's emulateMediaType(null) sends media:"" to reset, which we
        // treat as "clear".
        std::string media = paramString(cmd, "media");
        MediaQueryEvaluator::setMediaTypeOverride(media);

        // Feature overrides are reset wholesale on every call: start from clear
        // and re-apply whatever the array carries (an empty array thus clears).
        MediaQueryEvaluator::setPrefersColorSchemeOverride(0);
        MediaQueryEvaluator::setPrefersReducedMotionOverride(0);
        if (cmd.params() && cmd.params()->HasMember("features") &&
            (*cmd.params())["features"].IsArray()) {
            const rapidjson::Value& arr = (*cmd.params())["features"];
            for (rapidjson::SizeType i = 0; i < arr.Size(); i++) {
                const rapidjson::Value& f = arr[i];
                if (!f.IsObject() || !f.HasMember("name") ||
                    !f["name"].IsString() || !f.HasMember("value") ||
                    !f["value"].IsString()) {
                    continue;
                }
                std::string name = f["name"].GetString();
                std::string value = f["value"].GetString();
                if (name == "prefers-color-scheme") {
                    if (value == "light") {
                        MediaQueryEvaluator::setPrefersColorSchemeOverride(1);
                    } else if (value == "dark") {
                        MediaQueryEvaluator::setPrefersColorSchemeOverride(2);
                    }
                } else if (name == "prefers-reduced-motion") {
                    if (value == "no-preference") {
                        MediaQueryEvaluator::setPrefersReducedMotionOverride(1);
                    } else if (value == "reduce") {
                        MediaQueryEvaluator::setPrefersReducedMotionOverride(2);
                    }
                }
            }
        }
        cmd.sendResultEmpty();
        return;
    }

    if (method == "setScriptExecutionDisabled") {
        // params.value=true blocks page <script>/event-handler/module execution
        // (consulted live by BrowsingContext::isScriptingEnabled, which the
        // override survives navigation through). Inspector Runtime.evaluate
        // keeps working since it consults isScriptingEnabledIgnoringCDP.
        bool value = false;
        if (cmd.params() && cmd.params()->HasMember("value") &&
            (*cmd.params())["value"].IsBool()) {
            value = (*cmd.params())["value"].GetBool();
        }
        wv->setScriptExecutionDisabledByCDP(value);
        cmd.sendResultEmpty();
        return;
    }

    if (method == "setVisibleSize") {
        // Deprecated in CDP; delegate to the device-metrics path so
        // width/height still resize the viewport (DPR unchanged).
        double width = paramNumber(cmd, "width", 0);
        double height = paramNumber(cmd, "height", 0);
        if (width > 0 && height > 0) {
            if (!m_hasSaved && wv->renderer()) {
                m_savedWidth = wv->renderer()->width();
                m_savedHeight = wv->renderer()->height();
                m_savedDpr = wv->screenInfo().devicePixelRatio;
                m_hasSaved = true;
            }
            applyMetrics(wv, (uint32_t)width, (uint32_t)height, 0);
        }
        cmd.sendResultEmpty();
        return;
    }

    // setTimezoneOverride / setLocaleOverride: the Escargot VMInstance fixes
    // the
    //   timezone and locale at construction time (no runtime setter on its
    //   public API; the ICU calendar / tzname caches are private and
    //   immutable), so these are acked without effect. new Date()/Intl keep the
    //   boot value.
    // setCPUThrottlingRate: throttling is not implemented; acked.
    // setTouchEmulationEnabled / setFocusEmulationEnabled /
    //   setDefaultBackgroundColorOverride / etc. are acked as no-ops so
    //   DevTools/Puppeteer handshakes proceed.
    cmd.sendResultEmpty();
}

} // namespace Starfish

#endif
