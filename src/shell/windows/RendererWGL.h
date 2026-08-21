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

private:
    HGLRC createContext(HGLRC shareContext);
    void loadExtensionString();

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
};

} // namespace StarfishShell

#endif
#endif
