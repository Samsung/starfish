/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
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

#ifndef __LWEA11yAtspiBridge__
#define __LWEA11yAtspiBridge__

#include "StarfishConfig.h"

#if defined(STARFISH_SHELL_EFL) && defined(STARFISH_ENABLE_A11Y_ATSPI)

#include <Evas.h>

namespace LWEDelegate {

// Registers this process as an AT-SPI2 (ATK) accessibility provider.
//
// When the platform screen reader is enabled, the Tizen compositor grabs raw
// touch input and drives applications through AT-SPI2 D-Bus calls instead, so
// the app must expose an ATK plug on the a11y bus to receive anything at all.
// The plug exposes the engine's hierarchical accessibility tree
// (A11yAtspiTreeSource) and keeps the daemon in sync with event-driven
// children-changed / state-change emissions.
class A11yAtspiBridge {
public:
    // Call once when the webview window is created. Listens to the platform
    // accessibility (TTS) vconf key and connects/disconnects the ATK plug
    // accordingly. accessWidget is the elm widget representing the webview
    // in the host's widget tree (the plug id is published on it so the
    // host-side elm_atspi_bridge can embed our tree as a socket child, the
    // way chromium-efl integrates into elm apps).
    // webviewObject is the evas object covering the web content area; the
    // elm-side accessibility wrapper tracks its geometry and reads the plug
    // id off it.
    static void registerWindow(Evas_Object* window, Evas_Object* accessWidget,
                               Evas_Object* webviewObject);
    static void unregisterWindow(Evas_Object* window);
};

} // namespace LWEDelegate

#endif

#endif
