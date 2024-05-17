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

#ifndef __StarfishFrameRateCounter__
#define __StarfishFrameRateCounter__

namespace Starfish {

class Canvas;
class CanvasSurface;
class Compositor;
class Font;
class WebView;

class FrameRateCounter : public gc {
public:
    FrameRateCounter(WebView* webView);

    using Observer = void (*)(double fps);
    void setObserver(Observer observer)
    {
        m_observer = observer;
    }

    void update();

    double fps() const
    {
        return m_fps;
    }

    LayoutRect updateArea()
    {
        return m_updateArea;
    }

    void drawFps(Canvas* canvas);
    void drawFps(Compositor* canvas);

    void releaseResource()
    {
        m_font = nullptr;
    }

private:
    ~FrameRateCounter() = default;
    FrameRateCounter(const FrameRateCounter&) = delete;
    FrameRateCounter& operator=(const FrameRateCounter&) = delete;

    void loadFont();

    double m_startTime = 0.0;
    double m_frames = 0.0;
    double m_fps = 0.0;
    Font* m_font = nullptr;
    LayoutRect m_updateArea;

    CanvasSurface* m_surface = nullptr;
    WebView* m_webView = nullptr;
    Observer m_observer = nullptr;
};
} // namespace Starfish

#endif
