#if defined(STARFISH_WINDOWS)
/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#ifndef __StarfishShellRendererANGLE__
#define __StarfishShellRendererANGLE__

#include <windows.h>

#include <EGL/egl.h>

#include <atomic>
#include <cstdint>
#include <string>

namespace StarfishShell {

class RendererANGLE {
public:
    RendererANGLE() = default;
    ~RendererANGLE() = default;

    // offscreenWidth/offscreenHeight, when nonzero, render into an EGL
    // Pbuffer surface of that size instead of a window surface tied to
    // `window`. A window surface needs a real DXGI swapchain, which needs an
    // active/interactive desktop session -- unavailable on a CI runner with
    // no one logged in, where it fails with DXGI_ERROR_NOT_CURRENTLY_AVAILABLE
    // for hardware and WARP devices alike. A Pbuffer is a plain D3D11
    // texture with no swapchain and no session requirement, so screenshot
    // mode (which only ever reads the framebuffer back, never presents it)
    // uses this instead.
    bool initialize(HWND window, unsigned offscreenWidth = 0,
                    unsigned offscreenHeight = 0);
    void deinitialize();

    bool makeCurrent();
    bool swapBuffers();
    uintptr_t createSharedContext();
    bool destroyContext(uintptr_t context);
    bool clearCurrentContext();
    bool makeCurrentWithContext(uintptr_t context);
    void* getProcAddress(const char* name);
    bool isSupportedExtension(const char* extension);

    void requestScreenshot(const std::string& path, unsigned skipFrames,
                           HWND notifyWindow);
    bool screenshotSucceeded() const
    {
        return m_screenshotSucceeded;
    }
    bool screenshotPending() const
    {
        return m_screenshotPending.load(std::memory_order_acquire);
    }
    unsigned long swapCount() const
    {
        return m_swapCount;
    }

private:
    EGLContext createContext(EGLContext shareContext);
    void loadExtensionString();
    bool captureBackBuffer(const std::string& path);
    bool bringUpDisplay(EGLDisplay display, const char* label);
    void teardownDisplay();
    bool recreateSurface();

    HWND m_window{ nullptr };
    HDC m_dc{ nullptr };
    bool m_offscreen{ false };
    EGLDisplay m_display{ EGL_NO_DISPLAY };
    EGLConfig m_config{ nullptr };
    EGLSurface m_surface{ EGL_NO_SURFACE };
    EGLContext m_context{ EGL_NO_CONTEXT };
    HMODULE m_egl{ nullptr };
    HMODULE m_gles{ nullptr };
    std::string m_extensions;
    unsigned m_width{ 0 };
    unsigned m_height{ 0 };
    unsigned m_missingProcCount{ 0 };
    bool m_loggedMakeCurrent{ false };
    std::atomic<bool> m_screenshotPending{ false };
    std::string m_screenshotPath;
    HWND m_screenshotNotify{ nullptr };
    unsigned m_screenshotSkip{ 0 };
    bool m_screenshotSucceeded{ false };
    unsigned long m_swapCount{ 0 };
};

} // namespace StarfishShell

#endif
#endif
