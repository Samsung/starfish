/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "FrameRateCounter.h"
#include "core/modules/profiling/Profiling.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/canvas/font/Font.h"
#include "core/page/WebView.h"
#include "core/page/BrowsingContext.h"
#include "core/dom/Document.h"

namespace Starfish {

FrameRateCounter::FrameRateCounter(WebView* webView)
    : m_startTime(timestamp())
    , m_webView(webView)
{
}

void FrameRateCounter::update()
{
    double now = timestamp();
    double dt = now - m_startTime;
    if (dt > 1000) {
        m_fps = m_frames * 1000 / dt;
        m_frames = 0;
        m_startTime = now;
        if (m_observer) {
            m_observer(m_fps);
        }
    }
    m_frames++;
}

void FrameRateCounter::drawFps(Canvas* canvas)
{
    if (!m_font) {
        loadFont();
    }

    canvas->save();
    canvas->setFont(m_font);
    canvas->setFillColor(Unit::Color(255, 0, 0, 255));

    std::string fps = std::to_string(static_cast<int>(round(m_fps)));
    String* fpsString = String::createASCIIString(fps.c_str(), fps.length());
    LayoutUnit width = canvas->font()->measureText(StringView(fpsString));
    canvas->drawText(0, 0, width, StringView(fpsString), false);
    m_updateArea =
        LayoutRect(0, 0, width, canvas->font()->metrics().m_fontHeight);
    canvas->restore();
}

void FrameRateCounter::drawFps(Compositor* compositor)
{
    compositor->save();
    compositor->resetMatrixAndClip();
    if (!m_surface && !m_canvas) {
        // To hold internal buffer even after unmap, set
        // CanvasSurface::PreferEGLImage.
        m_surface = CanvasSurface::create(m_webView->renderer(), 50, 50, 1,
                                          CanvasSurface::PreferEGLImage);
        m_canvas = Canvas::create(m_webView, m_surface);
    }
    LayoutRect updateArea = m_updateArea;
    m_canvas->clearColor(Unit::Color(0, 0, 0, 0));
    drawFps(m_canvas);
    m_canvas->flush();
    updateArea.unite(m_updateArea);
    m_surface->unmapBufferAndNotifyUpdatedRegion(
        0, 0, updateArea.width().toUnsigned(),
        updateArea.height().toUnsigned());
    compositor->drawSurface(
        m_surface,
        Unit::Rect(0, 0, m_surface->bufferWidth(), m_surface->bufferHeight()));
    compositor->restore();
}

void FrameRateCounter::loadFont()
{
    String* fontFamilyStr = String::createASCIIString("SamsungOne");
    m_font =
        m_webView->mainBrowsingContext()->document()->fontSelector()->loadFont(
            &fontFamilyStr, 1, 30, 0, 7);
}

} // namespace Starfish
