#if defined(STARFISH_WINDOWS)
/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#ifndef __StarfishShellRendererWGL__
#define __StarfishShellRendererWGL__

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <string>

namespace StarfishShell {

// WGL side of LWE::WebContainer::RendererGLConfiguration. The engine calls
// every one of these from its own thread (LWE::Initialize is asked for
// PreferSeparateThread), so the window-owning UI thread must leave the
// context uncurrent after initialize().
class RendererWGL {
public:
    RendererWGL() = default;
    ~RendererWGL() = default;

    bool initialize(HWND window);
    void deinitialize();

    bool makeCurrent();
    bool swapBuffers();
    uintptr_t createSharedContext();
    bool destroyContext(uintptr_t context);
    bool clearCurrentContext();
    bool makeCurrentWithContext(uintptr_t context);
    void* getProcAddress(const char* name);
    bool isSupportedExtension(const char* extension);

    // Capture one frame to a BMP, then ask notifyWindow to close. Used by CI
    // to prove the port still renders. The capture has to run on the thread
    // that owns the GL context and with a finished frame in the back buffer,
    // so it happens inside swapBuffers() rather than on the UI thread.
    // skipFrames swaps are let through first: the frame right after page load
    // is not necessarily the one with the finished paint in it.
    void requestScreenshot(const std::string& path, unsigned skipFrames,
                           HWND notifyWindow);
    // False until a capture has been attempted; then whether it succeeded.
    bool screenshotSucceeded() const
    {
        return m_screenshotSucceeded;
    }
    bool screenshotPending() const
    {
        return m_screenshotPending.load(std::memory_order_acquire);
    }
    // Diagnostic: a screenshot that never happens is almost always this
    // sitting still, because the engine stops presenting once a static page
    // has finished painting.
    unsigned long swapCount() const
    {
        return m_swapCount;
    }

private:
    HGLRC createContext(HGLRC shareContext);
    void loadExtensionString();
    bool captureBackBuffer(const std::string& path);

    HWND m_window{ nullptr };
    HDC m_dc{ nullptr };
    HGLRC m_context{ nullptr };
    HMODULE m_opengl32{ nullptr };
    // wglCreateContextAttribsARB, resolved while the bootstrap context is
    // current; the pointer stays valid for this pixel format afterwards.
    void* m_createContextAttribs{ nullptr };
    std::string m_extensions;
    // Bring-up diagnostics: report a missing GL entry point (a NULL there
    // silently disables whatever the engine wanted to use it for) and the
    // first context bind, both only a handful of times.
    unsigned m_missingProcCount{ 0 };
    bool m_loggedMakeCurrent{ false };
    // Armed from whichever thread runs the page-loaded callback and consumed
    // in swapBuffers(); the atomic publishes the plain members before it.
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
