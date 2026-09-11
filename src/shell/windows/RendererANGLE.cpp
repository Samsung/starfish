#if defined(STARFISH_WINDOWS)
/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include "RendererANGLE.h"

#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

// vcpkg installs the plain Khronos egl-registry headers for this port (the
// ANGLE build excludes its own EGL/ headers from install), so the ANGLE
// platform extension used below to request a D3D11 WARP (software) device
// isn't declared anywhere we get to include. Values are from ANGLE's own
// include/EGL/eglext_angle.h at the pinned commit
// (aa292a59f9f222535c2ff34d8eecbe3cce039664).
#ifndef EGL_PLATFORM_ANGLE_ANGLE
#define EGL_PLATFORM_ANGLE_ANGLE 0x3202
#endif
#ifndef EGL_PLATFORM_ANGLE_TYPE_ANGLE
#define EGL_PLATFORM_ANGLE_TYPE_ANGLE 0x3203
#endif
#ifndef EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE
#define EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE 0x3208
#endif
#ifndef EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE 0x3209
#endif
#ifndef EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE 0x320A
#endif
#ifndef EGL_PLATFORM_ANGLE_DEVICE_TYPE_D3D_WARP_ANGLE
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_D3D_WARP_ANGLE 0x320B
#endif
// ANGLE defaults a D3D11 window surface to a flip-model swapchain, which
// DXGI only accepts when DWM composition is running on the session that
// owns the HWND -- true on an interactive desktop, not guaranteed on a CI
// runner. Without it, SwapChain11::reset fails with DXGI_ERROR_UNSUPPORTED,
// identically for the hardware and WARP device types (this isn't a GPU
// limitation, so falling back to WARP alone doesn't help). This attribute
// requests ANGLE's old BitBlt-style ("copy") present path instead, which
// works with no DWM.
#ifndef EGL_EXPERIMENTAL_PRESENT_PATH_ANGLE
#define EGL_EXPERIMENTAL_PRESENT_PATH_ANGLE 0x33A4
#endif
#ifndef EGL_EXPERIMENTAL_PRESENT_PATH_COPY_ANGLE
#define EGL_EXPERIMENTAL_PRESENT_PATH_COPY_ANGLE 0x33AA
#endif

namespace StarfishShell {

namespace {

    void logEGL(const char* what, bool ok)
    {
        std::fprintf(stderr, "[StarfishShell] EGL %s: %s (0x%04x)\n", what,
                     ok ? "ok" : "failed", eglGetError());
        std::fflush(stderr);
    }

    const char* describeGL(GLenum name)
    {
        const GLubyte* value = glGetString(name);
        return value ? reinterpret_cast<const char*>(value) : "(null)";
    }

} // namespace

bool RendererANGLE::initialize(HWND window, unsigned offscreenWidth,
                               unsigned offscreenHeight)
{
    m_window = window;
    m_offscreen = offscreenWidth > 0 && offscreenHeight > 0;
    if (m_offscreen) {
        m_width = offscreenWidth;
        m_height = offscreenHeight;
    }
    m_dc = GetDC(window);
    if (!m_dc) {
        logEGL("GetDC", false);
        return false;
    }

    // Both attempts below need the ANGLE platform extension (to request the
    // "copy" present path, and for the WARP retry) -- eglGetDisplay() alone
    // can't take an attribute list. The installed eglext.h only declares
    // this entry point's prototype under EGL_EGLEXT_PROTOTYPES (which this
    // build doesn't define), so resolve it through eglGetProcAddress instead
    // of calling it directly -- valid even before any display exists, which
    // is the documented bootstrap pattern for platform extensions.
    auto getPlatformDisplayEXT =
        reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
            eglGetProcAddress("eglGetPlatformDisplayEXT"));
    if (!getPlatformDisplayEXT) {
        logEGL("eglGetProcAddress(eglGetPlatformDisplayEXT)", false);
        return false;
    }

    // Whether a given session actually fails hardware D3D11 device creation
    // is out of our control (modern RDP often still succeeds against the
    // physical adapter), so there is no reliable way to exercise the WARP
    // path below by just picking an environment to run in. This lets a
    // developer force it deterministically instead, to check the WARP
    // retry logic and the resulting render independently of whatever
    // hardware happens to be reachable.
    char forceWarp[8] = {};
    bool skipHardware =
        GetEnvironmentVariableA("STARFISH_FORCE_WARP", forceWarp,
                                sizeof(forceWarp)) > 0 &&
        forceWarp[0] != '0';
    if (skipHardware) {
        std::fprintf(stderr,
                     "[StarfishShell] STARFISH_FORCE_WARP set, skipping the "
                     "hardware EGL attempt\n");
        std::fflush(stderr);
    } else {
        const EGLint hardwareAttributes[] = {
            EGL_PLATFORM_ANGLE_TYPE_ANGLE,
            EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
            EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
            EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
            EGL_EXPERIMENTAL_PRESENT_PATH_ANGLE,
            EGL_EXPERIMENTAL_PRESENT_PATH_COPY_ANGLE,
            EGL_NONE,
        };
        if (bringUpDisplay(getPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE,
                                                 EGL_DEFAULT_DISPLAY,
                                                 hardwareAttributes),
                           "hardware")) {
            return true;
        }
        teardownDisplay();
    }

    // A remote/RDP session or a machine with no usable D3D11 hardware
    // adapter fails somewhere above. Retry explicitly against ANGLE's D3D11
    // WARP (software rasterizer) device.
    std::fprintf(stderr,
                 "[StarfishShell] hardware EGL init failed, retrying with "
                 "D3D11 WARP (software)\n");
    std::fflush(stderr);
    const EGLint warpAttributes[] = {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_D3D_WARP_ANGLE,
        EGL_EXPERIMENTAL_PRESENT_PATH_ANGLE,
        EGL_EXPERIMENTAL_PRESENT_PATH_COPY_ANGLE,
        EGL_NONE,
    };
    if (bringUpDisplay(getPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE,
                                             EGL_DEFAULT_DISPLAY,
                                             warpAttributes),
                       "WARP")) {
        return true;
    }
    teardownDisplay();
    return false;
}

bool RendererANGLE::bringUpDisplay(EGLDisplay display, const char* label)
{
    std::fprintf(stderr, "[StarfishShell] EGL display attempt: %s\n", label);
    std::fflush(stderr);

    m_display = display;
    if (m_display == EGL_NO_DISPLAY ||
        !eglInitialize(m_display, nullptr, nullptr)) {
        logEGL("eglInitialize", false);
        return false;
    }
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        logEGL("eglBindAPI", false);
        return false;
    }

    const EGLint configAttributes[] = { EGL_SURFACE_TYPE,
                                        m_offscreen ? EGL_PBUFFER_BIT
                                                    : EGL_WINDOW_BIT,
                                        EGL_RENDERABLE_TYPE,
                                        EGL_OPENGL_ES2_BIT,
                                        EGL_RED_SIZE,
                                        8,
                                        EGL_GREEN_SIZE,
                                        8,
                                        EGL_BLUE_SIZE,
                                        8,
                                        EGL_ALPHA_SIZE,
                                        8,
                                        EGL_STENCIL_SIZE,
                                        1,
                                        EGL_NONE };
    EGLint count = 0;
    if (!eglChooseConfig(m_display, configAttributes, &m_config, 1, &count) ||
        count != 1) {
        logEGL("eglChooseConfig", false);
        return false;
    }
    if (m_offscreen) {
        const EGLint pbufferAttributes[] = {
            EGL_WIDTH,  static_cast<EGLint>(m_width),
            EGL_HEIGHT, static_cast<EGLint>(m_height),
            EGL_NONE,
        };
        m_surface =
            eglCreatePbufferSurface(m_display, m_config, pbufferAttributes);
        if (m_surface == EGL_NO_SURFACE) {
            logEGL("eglCreatePbufferSurface", false);
            return false;
        }
    } else {
        m_surface = eglCreateWindowSurface(
            m_display, m_config,
            reinterpret_cast<EGLNativeWindowType>(m_window), nullptr);
        if (m_surface == EGL_NO_SURFACE) {
            logEGL("eglCreateWindowSurface", false);
            return false;
        }
    }
    m_context = createContext(EGL_NO_CONTEXT);
    if (m_context == EGL_NO_CONTEXT) {
        logEGL("eglCreateContext", false);
        return false;
    }
    if (!eglMakeCurrent(m_display, m_surface, m_surface, m_context)) {
        logEGL("initial eglMakeCurrent", false);
        return false;
    }

    std::fprintf(stderr,
                 "[StarfishShell] GL version: %s\n"
                 "[StarfishShell] GL renderer: %s\n"
                 "[StarfishShell] GL vendor: %s\n",
                 describeGL(GL_VERSION), describeGL(GL_RENDERER),
                 describeGL(GL_VENDOR));
    std::fflush(stderr);
    loadExtensionString();

    // The real window still exists (and still resizes) in offscreen mode --
    // only its EGL surface changed kind -- so its client rect is still the
    // right thing to track m_width/m_height against, matching whatever size
    // the Pbuffer was actually just created at above.
    RECT client{};
    if (GetClientRect(m_window, &client)) {
        m_width = static_cast<unsigned>(client.right - client.left);
        m_height = static_cast<unsigned>(client.bottom - client.top);
    }

    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    return true;
}

void RendererANGLE::teardownDisplay()
{
    if (m_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                       EGL_NO_CONTEXT);
        if (m_context != EGL_NO_CONTEXT) {
            eglDestroyContext(m_display, m_context);
            m_context = EGL_NO_CONTEXT;
        }
        if (m_surface != EGL_NO_SURFACE) {
            eglDestroySurface(m_display, m_surface);
            m_surface = EGL_NO_SURFACE;
        }
        eglTerminate(m_display);
    }
    m_display = EGL_NO_DISPLAY;
    m_config = nullptr;
}

EGLContext RendererANGLE::createContext(EGLContext shareContext)
{
    const EGLint attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    return eglCreateContext(m_display, m_config, shareContext, attributes);
}

void RendererANGLE::deinitialize()
{
    teardownDisplay();
    if (m_dc && m_window) {
        ReleaseDC(m_window, m_dc);
    }
    m_dc = nullptr;
    m_window = nullptr;
}

bool RendererANGLE::recreateSurface()
{
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(m_display, m_surface);
    if (m_offscreen) {
        const EGLint pbufferAttributes[] = {
            EGL_WIDTH,  static_cast<EGLint>(m_width),
            EGL_HEIGHT, static_cast<EGLint>(m_height),
            EGL_NONE,
        };
        m_surface =
            eglCreatePbufferSurface(m_display, m_config, pbufferAttributes);
    } else {
        m_surface = eglCreateWindowSurface(
            m_display, m_config,
            reinterpret_cast<EGLNativeWindowType>(m_window), nullptr);
    }
    return m_surface != EGL_NO_SURFACE &&
           eglMakeCurrent(m_display, m_surface, m_surface, m_context) ==
               EGL_TRUE;
}

bool RendererANGLE::makeCurrent()
{
    // ANGLE's D3D11 window surface does not reliably notice an HWND resize
    // on its own between eglCreateWindowSurface calls -- the same gap that
    // left the window rendering at its pre-maximize size at startup before
    // initialize() was moved to run after ShowWindow(SW_SHOWMAXIMIZED).
    // Interactive resizes (including double-click-to-maximize the title
    // bar) hit the same gap on an already-live surface. This has to be
    // checked here, before the engine's draw calls for the frame -- doing
    // it in swapBuffers() instead (after the frame was already drawn, with
    // the new viewport, into the *old*-sized backbuffer) discarded that
    // frame and presented a blank recreated surface, which looked like the
    // page content shrank into a corner with everything else black.
    // The real window still exists (and still resizes, e.g. via the
    // screenshot nudge) in offscreen mode too, so this still needs to keep
    // the surface -- now possibly a Pbuffer, via the offscreen-aware
    // recreateSurface() below -- in lockstep with it.
    RECT client{};
    if (m_window && GetClientRect(m_window, &client)) {
        unsigned width = static_cast<unsigned>(client.right - client.left);
        unsigned height = static_cast<unsigned>(client.bottom - client.top);
        if (width > 0 && height > 0 &&
            (width != m_width || height != m_height)) {
            m_width = width;
            m_height = height;
            if (!recreateSurface()) {
                logEGL("resize surface recreate", false);
            }
        }
    }

    bool ok =
        eglMakeCurrent(m_display, m_surface, m_surface, m_context) == EGL_TRUE;
    if (!m_loggedMakeCurrent) {
        m_loggedMakeCurrent = true;
        logEGL("first engine makeCurrent", ok);
    }
    return ok;
}

bool RendererANGLE::swapBuffers()
{
    auto present = [this]() {
        if (eglSwapBuffers(m_display, m_surface) == EGL_TRUE)
            return true;
        EGLint error = eglGetError();
        if (error != EGL_BAD_SURFACE && error != EGL_BAD_NATIVE_WINDOW)
            return false;
        return recreateSurface();
    };
    ++m_swapCount;
    if (m_screenshotPending.load(std::memory_order_acquire)) {
        if (m_screenshotSkip > 0) {
            --m_screenshotSkip;
        } else {
            m_screenshotSucceeded = captureBackBuffer(m_screenshotPath);
            HWND notify = m_screenshotNotify;
            m_screenshotPending.store(false, std::memory_order_release);
            bool ok = present();
            if (notify) {
                PostMessageW(notify, WM_CLOSE, 0, 0);
            }
            return ok;
        }
    }
    return present();
}

void RendererANGLE::requestScreenshot(const std::string& path,
                                      unsigned skipFrames, HWND notifyWindow)
{
    m_screenshotPath = path;
    m_screenshotSkip = skipFrames;
    m_screenshotNotify = notifyWindow;
    m_screenshotPending.store(true, std::memory_order_release);
}

bool RendererANGLE::captureBackBuffer(const std::string& path)
{
    RECT client{};
    if (!m_window || !GetClientRect(m_window, &client)) {
        logEGL("screenshot GetClientRect", false);
        return false;
    }
    LONG width = client.right - client.left;
    LONG height = client.bottom - client.top;
    if (width <= 0 || height <= 0) {
        logEGL("screenshot client area", false);
        return false;
    }

    size_t stride = static_cast<size_t>(width) * 4;
    std::vector<unsigned char> pixels(stride * static_cast<size_t>(height));
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    if (GLenum error = glGetError()) {
        std::fprintf(stderr, "[StarfishShell] glReadPixels failed: 0x%04x\n",
                     error);
        std::fflush(stderr);
        return false;
    }
    for (size_t i = 0; i < pixels.size(); i += 4) {
        std::swap(pixels[i], pixels[i + 2]);
    }

    BITMAPINFOHEADER info{};
    info.biSize = sizeof(info);
    info.biWidth = width;
    info.biHeight = height;
    info.biPlanes = 1;
    info.biBitCount = 32;
    info.biCompression = BI_RGB;
    info.biSizeImage = static_cast<DWORD>(pixels.size());
    BITMAPFILEHEADER header{};
    header.bfType = 0x4d42;
    header.bfOffBits = sizeof(header) + sizeof(info);
    header.bfSize = header.bfOffBits + info.biSizeImage;

    FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) {
        return false;
    }
    bool written =
        std::fwrite(&header, sizeof(header), 1, file) == 1 &&
        std::fwrite(&info, sizeof(info), 1, file) == 1 &&
        std::fwrite(pixels.data(), 1, pixels.size(), file) == pixels.size();
    std::fclose(file);
    return written;
}

uintptr_t RendererANGLE::createSharedContext()
{
    EGLContext context = createContext(m_context);
    return context == EGL_NO_CONTEXT ? UINTPTR_MAX
                                     : reinterpret_cast<uintptr_t>(context);
}

bool RendererANGLE::destroyContext(uintptr_t context)
{
    return eglDestroyContext(m_display,
                             reinterpret_cast<EGLContext>(context)) == EGL_TRUE;
}

bool RendererANGLE::clearCurrentContext()
{
    return eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                          EGL_NO_CONTEXT) == EGL_TRUE;
}

bool RendererANGLE::makeCurrentWithContext(uintptr_t context)
{
    return eglMakeCurrent(m_display, m_surface, m_surface,
                          reinterpret_cast<EGLContext>(context)) == EGL_TRUE;
}

void* RendererANGLE::getProcAddress(const char* name)
{
    if (void* address = reinterpret_cast<void*>(eglGetProcAddress(name))) {
        return address;
    }
    bool isEGL = std::strncmp(name, "egl", 3) == 0;
    HMODULE& module = isEGL ? m_egl : m_gles;
    const wchar_t* moduleName = isEGL ? L"libEGL.dll" : L"libGLESv2.dll";
    if (!module) {
        module = GetModuleHandleW(moduleName);
        if (!module) {
            module = LoadLibraryW(moduleName);
        }
    }
    void* address = module
                        ? reinterpret_cast<void*>(GetProcAddress(module, name))
                        : nullptr;
    if (!address && m_missingProcCount++ < 32) {
        std::fprintf(stderr, "[StarfishShell] unresolved GL entry point: %s\n",
                     name);
        std::fflush(stderr);
    }
    return address;
}

void RendererANGLE::loadExtensionString()
{
    m_extensions.clear();
    if (const GLubyte* extensions = glGetString(GL_EXTENSIONS)) {
        m_extensions.assign(reinterpret_cast<const char*>(extensions));
    }
    if (const char* extensions = eglQueryString(m_display, EGL_EXTENSIONS)) {
        if (!m_extensions.empty()) {
            m_extensions.push_back(' ');
        }
        m_extensions.append(extensions);
    }
}

bool RendererANGLE::isSupportedExtension(const char* extension)
{
    if (!extension || !*extension || m_extensions.empty()) {
        return false;
    }
    size_t length = std::strlen(extension);
    size_t position = 0;
    while ((position = m_extensions.find(extension, position)) !=
           std::string::npos) {
        bool atStart = position == 0 || m_extensions[position - 1] == ' ';
        size_t end = position + length;
        bool atEnd = end == m_extensions.size() || m_extensions[end] == ' ';
        if (atStart && atEnd) {
            return true;
        }
        position = end;
    }
    return false;
}

} // namespace StarfishShell
#endif
