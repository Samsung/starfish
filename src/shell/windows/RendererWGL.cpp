#if defined(STARFISH_WINDOWS)
/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include "RendererWGL.h"

#include <GL/gl.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace StarfishShell {

namespace {

    // From WGL_ARB_create_context / WGL_ARB_extensions_string. Declared here so
    // the shell needs no GL loader of its own: GLEW lives inside the engine DLL
    // and initializing a second copy in this process would only add ordering
    // requirements between the two.
    constexpr int kContextMajorVersionARB = 0x2091;
    constexpr int kContextMinorVersionARB = 0x2092;
    constexpr int kContextFlagsARB = 0x2094;

    using CreateContextAttribsARB = HGLRC(WINAPI*)(HDC, HGLRC, const int*);
    using GetExtensionsStringARB = const char*(WINAPI*)(HDC);
    using GetStringi = const GLubyte*(WINAPI*)(GLenum, GLuint);

    constexpr GLenum kNumExtensions = 0x821D; // GL_NUM_EXTENSIONS
    void logGL(const char* what, bool ok)
    {
        std::fprintf(stderr, "[StarfishShell] GL %s: %s\n", what,
                     ok ? "ok" : "failed");
        std::fflush(stderr);
    }

} // namespace

bool RendererWGL::initialize(HWND window)
{
    m_window = window;
    m_dc = GetDC(window);
    if (!m_dc) {
        logGL("GetDC", false);
        return false;
    }

    // Matches the engine's expectations: double buffered RGBA, no depth, one
    // stencil bit (the compositor uses stencil for clipping, never depth).
    PIXELFORMATDESCRIPTOR descriptor{};
    descriptor.nSize = sizeof(descriptor);
    descriptor.nVersion = 1;
    descriptor.dwFlags =
        PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    descriptor.iPixelType = PFD_TYPE_RGBA;
    descriptor.cColorBits = 32;
    descriptor.cDepthBits = 0;
    descriptor.cStencilBits = 1;
    descriptor.iLayerType = PFD_MAIN_PLANE;

    int format = ChoosePixelFormat(m_dc, &descriptor);
    if (!format || !SetPixelFormat(m_dc, format, &descriptor)) {
        logGL("SetPixelFormat", false);
        return false;
    }
    std::fprintf(stderr, "[StarfishShell] GL pixel format: %d\n", format);
    std::fflush(stderr);

    // wglCreateContextAttribsARB can only be resolved through a context, so
    // bootstrap with a legacy one and throw it away.
    HGLRC bootstrap = wglCreateContext(m_dc);
    if (!bootstrap || !wglMakeCurrent(m_dc, bootstrap)) {
        logGL("bootstrap context", false);
        return false;
    }
    m_createContextAttribs = reinterpret_cast<void*>(
        wglGetProcAddress("wglCreateContextAttribsARB"));
    loadExtensionString();

    m_context = createContext(nullptr);
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(bootstrap);

    if (!m_context) {
        logGL("context creation", false);
        return false;
    }

    // Report what the engine will actually run on, from the real context.
    // GL_RENDERER, not just GL_VERSION: an app-local Mesa opengl32.dll is how
    // both CI and a local software-rendering check work, and on a machine that
    // also has a GPU driver the renderer string ("llvmpipe (LLVM ...)") is the
    // only thing that says which one actually got loaded.
    if (wglMakeCurrent(m_dc, m_context)) {
        auto describe = [](GLenum name) -> const char* {
            const GLubyte* value = glGetString(name);
            return value ? reinterpret_cast<const char*>(value) : "(null)";
        };
        std::fprintf(stderr,
                     "[StarfishShell] GL version: %s\n"
                     "[StarfishShell] GL renderer: %s\n"
                     "[StarfishShell] GL vendor: %s\n",
                     describe(GL_VERSION), describe(GL_RENDERER),
                     describe(GL_VENDOR));
        std::fflush(stderr);
        loadExtensionString();
    }
    // The engine thread binds this context itself; it cannot be current here.
    wglMakeCurrent(nullptr, nullptr);
    return true;
}

HGLRC RendererWGL::createContext(HGLRC shareContext)
{
    if (m_createContextAttribs) {
        const int attributes[] = { kContextMajorVersionARB,
                                   3,
                                   kContextMinorVersionARB,
                                   1,
                                   kContextFlagsARB,
                                   0,
                                   0 };
        HGLRC context = reinterpret_cast<CreateContextAttribsARB>(
            m_createContextAttribs)(m_dc, shareContext, attributes);
        if (context) {
            return context;
        }
    }

    // No WGL_ARB_create_context: fall back to a legacy context. wglShareLists
    // has to run before either context has been used, which holds here --
    // shared contexts are created during WebContainer construction.
    HGLRC context = wglCreateContext(m_dc);
    if (context && shareContext && !wglShareLists(shareContext, context)) {
        logGL("wglShareLists", false);
        wglDeleteContext(context);
        return nullptr;
    }
    return context;
}

void RendererWGL::deinitialize()
{
    wglMakeCurrent(nullptr, nullptr);
    if (m_context) {
        wglDeleteContext(m_context);
        m_context = nullptr;
    }
    if (m_dc && m_window) {
        ReleaseDC(m_window, m_dc);
        m_dc = nullptr;
    }
    m_window = nullptr;
}

bool RendererWGL::makeCurrent()
{
    bool ok = wglMakeCurrent(m_dc, m_context) == TRUE;
    if (!m_loggedMakeCurrent) {
        m_loggedMakeCurrent = true;
        logGL("first engine makeCurrent", ok);
    }
    return ok;
}

bool RendererWGL::swapBuffers()
{
    ++m_swapCount;
    if (m_screenshotPending.load(std::memory_order_acquire)) {
        if (m_screenshotSkip > 0) {
            --m_screenshotSkip;
        } else {
            // Before the swap, not after: for a double-buffered context
            // glReadPixels defaults to GL_BACK, which is where the frame
            // about to be presented still is.
            m_screenshotSucceeded = captureBackBuffer(m_screenshotPath);
            HWND notify = m_screenshotNotify;
            m_screenshotPending.store(false, std::memory_order_release);
            bool ok = SwapBuffers(m_dc) == TRUE;
            if (notify) {
                PostMessageW(notify, WM_CLOSE, 0, 0);
            }
            return ok;
        }
    }
    return SwapBuffers(m_dc) == TRUE;
}

void RendererWGL::requestScreenshot(const std::string& path,
                                    unsigned skipFrames, HWND notifyWindow)
{
    m_screenshotPath = path;
    m_screenshotSkip = skipFrames;
    m_screenshotNotify = notifyWindow;
    m_screenshotPending.store(true, std::memory_order_release);
}

bool RendererWGL::captureBackBuffer(const std::string& path)
{
    RECT client{};
    if (!m_window || !GetClientRect(m_window, &client)) {
        logGL("screenshot GetClientRect", false);
        return false;
    }
    LONG width = client.right - client.left;
    LONG height = client.bottom - client.top;
    if (width <= 0 || height <= 0) {
        logGL("screenshot client area", false);
        return false;
    }

    // GL_RGBA rather than GL_BGRA: BGRA reads only became core in GL 1.2 and
    // are an extension before that, and this has to work on whatever GL the
    // CI runner ends up with. The channel swap below costs one pass.
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

    // A BMP's rows run bottom-up, which is also glReadPixels' row order, so
    // the readback goes to the file as is. 32bpp BI_RGB keeps the writer
    // trivial -- no PNG encoder, so the shell needs no extra library.
    BITMAPINFOHEADER info{};
    info.biSize = sizeof(info);
    info.biWidth = width;
    info.biHeight = height;
    info.biPlanes = 1;
    info.biBitCount = 32;
    info.biCompression = BI_RGB;
    info.biSizeImage = static_cast<DWORD>(pixels.size());

    BITMAPFILEHEADER header{};
    header.bfType = 0x4d42; // "BM"
    header.bfOffBits = sizeof(header) + sizeof(info);
    header.bfSize = header.bfOffBits + info.biSizeImage;

    FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) {
        std::fprintf(stderr, "[StarfishShell] cannot open screenshot '%s'\n",
                     path.c_str());
        std::fflush(stderr);
        return false;
    }
    bool written =
        std::fwrite(&header, sizeof(header), 1, file) == 1 &&
        std::fwrite(&info, sizeof(info), 1, file) == 1 &&
        std::fwrite(pixels.data(), 1, pixels.size(), file) == pixels.size();
    std::fclose(file);
    if (!written) {
        std::fprintf(stderr, "[StarfishShell] short write to '%s'\n",
                     path.c_str());
        std::fflush(stderr);
        return false;
    }
    std::fprintf(stderr, "[StarfishShell] screenshot %ldx%ld -> %s\n", width,
                 height, path.c_str());
    std::fflush(stderr);
    return true;
}

uintptr_t RendererWGL::createSharedContext()
{
    HGLRC context = createContext(m_context);
    if (!context) {
        return UINTPTR_MAX;
    }
    return reinterpret_cast<uintptr_t>(context);
}

bool RendererWGL::destroyContext(uintptr_t context)
{
    return wglDeleteContext(reinterpret_cast<HGLRC>(context)) == TRUE;
}

bool RendererWGL::clearCurrentContext()
{
    return wglMakeCurrent(nullptr, nullptr) == TRUE;
}

bool RendererWGL::makeCurrentWithContext(uintptr_t context)
{
    return wglMakeCurrent(m_dc, reinterpret_cast<HGLRC>(context)) == TRUE;
}

void* RendererWGL::getProcAddress(const char* name)
{
    // wglGetProcAddress only knows the entry points the ICD adds on top of
    // OpenGL 1.1; for everything in opengl32.dll's own export table (glClear,
    // glTexImage2D, glDrawArrays, ...) it returns NULL. Without this fallback
    // the engine loads a half-empty function table and draws nothing.
    if (void* address = reinterpret_cast<void*>(wglGetProcAddress(name))) {
        return address;
    }
    if (!m_opengl32) {
        m_opengl32 = GetModuleHandleW(L"opengl32.dll");
        if (!m_opengl32) {
            m_opengl32 = LoadLibraryW(L"opengl32.dll");
        }
    }
    if (!m_opengl32) {
        return nullptr;
    }
    void* address = reinterpret_cast<void*>(GetProcAddress(m_opengl32, name));
    // egl* lookups are optional probes (the TBM/EGLImage path); WGL never has
    // them, so reporting those would only be noise.
    if (!address && m_missingProcCount < 32 &&
        std::strncmp(name, "egl", 3) != 0) {
        ++m_missingProcCount;
        std::fprintf(stderr, "[StarfishShell] unresolved GL entry point: %s\n",
                     name);
        std::fflush(stderr);
    }
    return address;
}

void RendererWGL::loadExtensionString()
{
    m_extensions.clear();

    if (const GLubyte* legacy = glGetString(GL_EXTENSIONS)) {
        m_extensions.assign(reinterpret_cast<const char*>(legacy));
    } else {
        // Core profiles removed the flat GL_EXTENSIONS string.
        auto getStringi =
            reinterpret_cast<GetStringi>(wglGetProcAddress("glGetStringi"));
        GLint count = 0;
        glGetIntegerv(kNumExtensions, &count);
        if (getStringi) {
            for (GLint i = 0; i < count; ++i) {
                const GLubyte* name =
                    getStringi(GL_EXTENSIONS, static_cast<GLuint>(i));
                if (name) {
                    m_extensions.append(reinterpret_cast<const char*>(name));
                    m_extensions.push_back(' ');
                }
            }
        }
    }

    auto getWGLExtensions = reinterpret_cast<GetExtensionsStringARB>(
        wglGetProcAddress("wglGetExtensionsStringARB"));
    if (getWGLExtensions) {
        if (const char* wgl = getWGLExtensions(m_dc)) {
            m_extensions.push_back(' ');
            m_extensions.append(wgl);
        }
    }
}

bool RendererWGL::isSupportedExtension(const char* extension)
{
    if (!extension || !*extension || m_extensions.empty()) {
        return false;
    }
    // Whole-token match: "GL_ARB_foo" must not be reported for a driver that
    // only lists "GL_ARB_foobar".
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
