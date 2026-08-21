#if defined(STARFISH_WINDOWS)
/*
 * Copyright (c) 2026-present Samsung Electronics Co., Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include <windows.h>
#include <windowsx.h>
#include <imm.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdint>
#include <cwchar>
#include <string>

#include "LWEWebView.h"

#include "RendererWGL.h"

namespace {

constexpr wchar_t kWindowClassName[] = L"StarfishWin32OpenGLShell";
constexpr char kShellBuildID[] = "win32-libtuv-20260821-1";

void log(const char* message)
{
    std::fprintf(stderr, "[StarfishShell] %s\n", message);
    std::fflush(stderr);
}

void logWin32Error(const char* operation)
{
    std::fprintf(stderr, "[StarfishShell] ERROR: %s failed (Win32 error %lu)\n",
                 operation, static_cast<unsigned long>(GetLastError()));
    std::fflush(stderr);
}

std::string toUTF8(const wchar_t* value, int valueLength)
{
    if (!value || valueLength == 0) {
        return std::string();
    }
    int length = WideCharToMultiByte(CP_UTF8, 0, value, valueLength, nullptr, 0,
                                     nullptr, nullptr);
    if (length <= 0) {
        return std::string();
    }
    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, valueLength, &result[0], length,
                        nullptr, nullptr);
    if (valueLength < 0 && !result.empty()) {
        result.pop_back(); // drop the copied NUL
    }
    return result;
}

std::string toUTF8(const wchar_t* value)
{
    return toUTF8(value, -1);
}

std::string toUTF8(const std::wstring& value)
{
    return toUTF8(value.data(), static_cast<int>(value.size()));
}

std::string storageDirectory()
{
    wchar_t buffer[MAX_PATH];
    DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
    if (!length || length >= MAX_PATH) {
        length = GetTempPathW(MAX_PATH, buffer);
        if (!length || length >= MAX_PATH) {
            return std::string(".\\");
        }
    }
    std::string path = toUTF8(buffer, static_cast<int>(length));
    if (!path.empty() && path.back() != '\\' && path.back() != '/') {
        path.push_back('\\');
    }
    return path;
}

bool isASCIIPrintable(UINT character)
{
    return character >= 0x20 && character < 0x7f;
}

// Mirrors what the removed WinForms bridge did, minus its dependency on the
// engine's internal String helpers.
LWE::KeyValue virtualKeyCodeToKeyValue(UINT virtualKey,
                                       bool capsLockOrShiftPressed)
{
    switch (virtualKey) {
    case VK_LMENU:
        return LWE::KeyValue::AltLeftKey;
    case VK_RMENU:
        return LWE::KeyValue::AltRightKey;
    case VK_LCONTROL:
        return LWE::KeyValue::ControlLeftKey;
    case VK_RCONTROL:
        return LWE::KeyValue::ControlRightKey;
    case VK_CAPITAL:
        return LWE::KeyValue::CapsLockKey;
    case VK_NUMLOCK:
        return LWE::KeyValue::NumLockKey;
    case VK_SCROLL:
        return LWE::KeyValue::ScrollLockKey;
    case VK_LSHIFT:
        return LWE::KeyValue::ShiftLeftKey;
    case VK_RSHIFT:
        return LWE::KeyValue::ShiftRightKey;
    case VK_LEFT:
        return LWE::KeyValue::ArrowLeftKey;
    case VK_RIGHT:
        return LWE::KeyValue::ArrowRightKey;
    case VK_UP:
        return LWE::KeyValue::ArrowUpKey;
    case VK_DOWN:
        return LWE::KeyValue::ArrowDownKey;
    case VK_RETURN:
        return LWE::KeyValue::EnterKey;
    case VK_TAB:
        return LWE::KeyValue::TabKey;
    case VK_BACK:
        return LWE::KeyValue::BackspaceKey;
    case VK_DELETE:
        return LWE::KeyValue::DeleteKey;
    case VK_INSERT:
        return LWE::KeyValue::InsertKey;
    case VK_END:
        return LWE::KeyValue::EndKey;
    case VK_HOME:
        return LWE::KeyValue::HomeKey;
    case VK_NEXT:
        return LWE::KeyValue::PageDownKey;
    case VK_PRIOR:
        return LWE::KeyValue::PageUpKey;
    case VK_APPS:
        return LWE::KeyValue::ContextMenuKey;
    case VK_ESCAPE:
        return LWE::KeyValue::EscapeKey;
    default:
        break;
    }

    if (virtualKey >= VK_F1 && virtualKey <= VK_F20) {
        return static_cast<LWE::KeyValue>(LWE::KeyValue::F1Key + virtualKey -
                                          VK_F1);
    }

    UINT character = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_CHAR);
    if (isASCIIPrintable(character)) {
        if (std::isalpha(static_cast<int>(character)) &&
            !capsLockOrShiftPressed) {
            character =
                static_cast<UINT>(std::tolower(static_cast<int>(character)));
        }
        return static_cast<LWE::KeyValue>(character);
    }
    return LWE::KeyValue::UnidentifiedKey;
}

// Everything outside the unreserved set gets escaped. Hand-escaping a data URL
// is a trap: the previous literal left CSS percentages ("height:100%;") in
// place, which is not a valid %XX escape, and the whole page failed to load.
std::string percentEncode(const std::string& text)
{
    static const char kHexDigits[] = "0123456789ABCDEF";
    std::string encoded;
    encoded.reserve(text.size() * 3);
    for (unsigned char c : text) {
        bool unreserved = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                          (c >= '0' && c <= '9') || c == '-' || c == '_' ||
                          c == '.' || c == '~';
        if (unreserved) {
            encoded.push_back(static_cast<char>(c));
        } else {
            encoded.push_back('%');
            encoded.push_back(kHexDigits[c >> 4]);
            encoded.push_back(kHexDigits[c & 0x0f]);
        }
    }
    return encoded;
}

std::string defaultSmokeURL()
{
    static const char kSmokePage[] =
        "<!DOCTYPE html><title>Starfish smoke</title>"
        "<style>html,body{height:100%;margin:0}"
        "body{display:flex;align-items:center;justify-content:center;"
        "background:#102030;color:#fff;font:32px sans-serif}"
        "div{padding:48px;border:4px solid #5cf;border-radius:24px}</style>"
        "<div>Starfish Win32 OpenGL shell</div>";
    return "data:text/html," + percentEncode(kSmokePage);
}

std::string commandLineURL(const wchar_t* argument)
{
    std::string value = toUTF8(argument);
    if (value.find("://") != std::string::npos ||
        value.compare(0, 5, "data:") == 0 ||
        value.compare(0, 6, "about:") == 0) {
        return value;
    }

    wchar_t fullPath[MAX_PATH];
    DWORD length = GetFullPathNameW(argument, MAX_PATH, fullPath, nullptr);
    if (!length || length >= MAX_PATH) {
        return value;
    }
    value = "file:///" + toUTF8(fullPath);
    std::replace(value.begin(), value.end(), '\\', '/');
    return value;
}

// Returns false if the option is recognized but its value is unusable, so a
// typo in CI fails the run instead of silently loading a page named "--foo".
bool parseUnsignedOption(const std::wstring& argument, const wchar_t* name,
                         unsigned* out)
{
    std::wstring prefix = std::wstring(name) + L"=";
    if (argument.compare(0, prefix.size(), prefix) != 0) {
        return true;
    }
    std::wstring value = argument.substr(prefix.size());
    if (value.empty() ||
        value.find_first_not_of(L"0123456789") != std::wstring::npos) {
        return false;
    }
    *out = static_cast<unsigned>(std::wcstoul(value.c_str(), nullptr, 10));
    return true;
}

// Timer id for --timeout-ms. Anything that keeps the shell from reaching its
// screenshot -- a page that never loads, a compositor that never presents --
// would otherwise hang a CI job until the workflow's own timeout.
constexpr UINT_PTR kTimeoutTimerId = 1;
// Drives the repaint nudge below until the capture happens.
constexpr UINT_PTR kNudgeTimerId = 2;
constexpr UINT kNudgeIntervalMs = 250;

// Posted from the page-loaded callback, which runs on the engine thread, so
// that arming the capture and starting timers happens on the thread that owns
// the window.
constexpr UINT kMessagePageLoaded = WM_APP + 1;

// Screenshot mode uses a fixed window size instead of maximizing: the capture
// is then the same size on every machine and every runner, which is what makes
// one comparable to the next.
constexpr int kDefaultCaptureWidth = 1280;
constexpr int kDefaultCaptureHeight = 800;

// Parses WxH, e.g. "1280x800".
bool parseWindowSize(const std::wstring& value, int* width, int* height)
{
    size_t separator = value.find(L'x');
    if (separator == std::wstring::npos) {
        return false;
    }
    std::wstring w = value.substr(0, separator);
    std::wstring h = value.substr(separator + 1);
    if (w.empty() || h.empty() ||
        w.find_first_not_of(L"0123456789") != std::wstring::npos ||
        h.find_first_not_of(L"0123456789") != std::wstring::npos) {
        return false;
    }
    long parsedWidth = std::wcstol(w.c_str(), nullptr, 10);
    long parsedHeight = std::wcstol(h.c_str(), nullptr, 10);
    if (parsedWidth < 64 || parsedHeight < 64) {
        return false;
    }
    *width = static_cast<int>(parsedWidth);
    *height = static_cast<int>(parsedHeight);
    return true;
}

// The shell owns the window and the Win32 input loop and talks to the engine
// only through the public LWE embedding API, like the other ports. LWE runs on
// its own thread (InitializeOption::PreferSeparateThread) and the API marshals
// every call there, so this thread never touches engine internals.
class Shell {
public:
    Shell()
        : m_instance(GetModuleHandleW(nullptr))
    {
    }

    int run(int argumentCount, wchar_t** arguments)
    {
        log("starting Win32 shell");
        std::fprintf(stderr, "[StarfishShell] build: %s\n", kShellBuildID);
        std::fflush(stderr);

        if (!parseArguments(argumentCount, arguments)) {
            std::fprintf(stderr,
                         "usage: StarfishShell [URL-or-file] "
                         "[--screenshot=FILE.bmp] [--screenshot-frames=N] "
                         "[--window-size=WxH] "
                         "[--timeout-ms=N]\n");
            std::fflush(stderr);
            return 2;
        }

        enableDPIAwareness();
        if (!createWindow()) {
            return 1;
        }

        log("initializing OpenGL");
        if (!m_renderer.initialize(m_window)) {
            showError(L"OpenGL initialization failed");
            shutdownWindow();
            return 1;
        }
        log("OpenGL initialized");

        // The renderer must see the final client area on its first frame:
        // creating the WebContainer while the HWND is still hidden leaves some
        // drivers with the pre-maximized backing size and no later expose.
        //
        // Screenshot runs get a fixed size rather than the maximized one, so
        // the capture does not depend on the display the run happens to land
        // on -- and so the resize that forces the captured frame stays within
        // a size this run chose.
        if (!m_screenshotPath.empty() || m_fixedWindowSize) {
            // One pixel taller than asked for; see nudgeRepaint().
            setClientSize(m_captureWidth, m_captureHeight + 1);
            ShowWindow(m_window, SW_SHOWNORMAL);
        } else {
            ShowWindow(m_window, SW_SHOWMAXIMIZED);
        }
        UpdateWindow(m_window);
        SetFocus(m_window);

        if (!createWebContainer()) {
            showError(L"Starfish failed to create a WebContainer");
            m_renderer.deinitialize();
            shutdownWindow();
            return 1;
        }

        std::fprintf(stderr, "[StarfishShell] URL: %s\n", m_initialURL.c_str());
        std::fflush(stderr);
        m_container->LoadURL(m_initialURL);
        log("loadURL submitted");

        if (m_timeoutMs) {
            SetTimer(m_window, kTimeoutTimerId, m_timeoutMs, nullptr);
        }

        int exitCode = runMessageLoop();

        destroyWebContainer();
        // The screenshot is the whole point of the run when it was asked for,
        // so report it in the exit code: a timeout or a failed capture has to
        // fail the CI job, not pass quietly with no artifact.
        if (!m_screenshotPath.empty() && !m_renderer.screenshotSucceeded()) {
            log("ERROR: no screenshot was captured");
            exitCode = exitCode ? exitCode : 1;
        }
        m_renderer.deinitialize();
        shutdownWindow();
        return exitCode;
    }

private:
    // SetWindowPos sizes the whole window, border and title bar included, so
    // asking it for 1280x800 leaves a 1264x761 client area -- and the client
    // area is what gets captured. Convert the wanted client size to the window
    // size this style and DPI need.
    void setClientSize(int width, int height)
    {
        RECT rect{ 0, 0, width, height };
        DWORD style = static_cast<DWORD>(GetWindowLongW(m_window, GWL_STYLE));
        DWORD exStyle =
            static_cast<DWORD>(GetWindowLongW(m_window, GWL_EXSTYLE));

        // Per-monitor DPI v2 is enabled, so the frame metrics follow the
        // monitor; plain AdjustWindowRectEx only knows the system DPI and
        // would be wrong on a scaled display.
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        using AdjustForDPI = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        using DPIForWindow = UINT(WINAPI*)(HWND);
        auto adjustForDPI = reinterpret_cast<AdjustForDPI>(
            GetProcAddress(user32, "AdjustWindowRectExForDpi"));
        auto dpiForWindow = reinterpret_cast<DPIForWindow>(
            GetProcAddress(user32, "GetDpiForWindow"));
        if (!adjustForDPI || !dpiForWindow ||
            !adjustForDPI(&rect, style, FALSE, exStyle,
                          dpiForWindow(m_window))) {
            AdjustWindowRectEx(&rect, style, FALSE, exStyle);
        }
        SetWindowPos(m_window, nullptr, 0, 0, rect.right - rect.left,
                     rect.bottom - rect.top,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Resizing is the repaint trigger: WM_SIZE already forwards the new client
    // area to WebContainer::ResizeTo, which relayouts and repaints, which
    // produces the frame the capture is waiting for. A resize to the size it
    // already has is a no-op, so alternate one pixel of height. The window is
    // created one pixel taller, so the first nudge -- the one that normally
    // produces the captured frame -- lands on exactly the requested size.
    void nudgeRepaint()
    {
        m_captureNudge = !m_captureNudge;
        setClientSize(m_captureWidth,
                      m_captureHeight + (m_captureNudge ? 0 : 1));
    }

    bool parseArguments(int argumentCount, wchar_t** arguments)
    {
        std::wstring url;
        for (int i = 1; i < argumentCount; ++i) {
            std::wstring argument = arguments[i];
            if (argument.compare(0, 13, L"--screenshot=") == 0) {
                m_screenshotPath = toUTF8(argument.substr(13).c_str());
                if (m_screenshotPath.empty()) {
                    return false;
                }
            } else if (argument.compare(0, 20, L"--screenshot-frames=") == 0) {
                if (!parseUnsignedOption(argument, L"--screenshot-frames",
                                         &m_screenshotFrames)) {
                    return false;
                }
            } else if (argument.compare(0, 13, L"--timeout-ms=") == 0) {
                if (!parseUnsignedOption(argument, L"--timeout-ms",
                                         &m_timeoutMs)) {
                    return false;
                }
            } else if (argument.compare(0, 14, L"--window-size=") == 0) {
                if (!parseWindowSize(argument.substr(14), &m_captureWidth,
                                     &m_captureHeight)) {
                    return false;
                }
                m_fixedWindowSize = true;
            } else if (argument.compare(0, 2, L"--") == 0) {
                std::fprintf(stderr, "[StarfishShell] unknown option: %s\n",
                             toUTF8(argument.c_str()).c_str());
                return false;
            } else if (url.empty()) {
                url = argument;
            } else {
                std::fprintf(stderr, "[StarfishShell] more than one URL\n");
                return false;
            }
        }
        m_initialURL =
            url.empty() ? defaultSmokeURL() : commandLineURL(url.c_str());
        return true;
    }

    int runMessageLoop()
    {
        MSG message;
        while (true) {
            BOOL result = GetMessageW(&message, nullptr, 0, 0);
            if (result == 0) {
                return static_cast<int>(message.wParam);
            }
            if (result == -1) {
                return 1;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    bool createWebContainer()
    {
        std::string storage = storageDirectory();
        std::fprintf(stderr, "[StarfishShell] storage: %s\n", storage.c_str());
        std::fflush(stderr);
        LWE::LWE::Initialize(storage.c_str(),
                             LWE::InitializeOption::PreferSeparateThread);
        std::fprintf(stderr, "[StarfishShell] engine thread mode: %s\n",
                     LWE::LWE::IsUsingSeparateThread() ? "separate thread"
                                                       : "caller thread");
        std::fflush(stderr);

        RECT client;
        GetClientRect(m_window, &client);
        std::fprintf(stderr,
                     "[StarfishShell] creating WebContainer (%ldx%ld)\n",
                     client.right - client.left, client.bottom - client.top);
        std::fflush(stderr);

        LWE::WebContainer::WebContainerArguments args = {
            static_cast<unsigned>(client.right - client.left),
            static_cast<unsigned>(client.bottom - client.top),
            1.0f,
            "sans-serif",
            "ko-KR",
            "Asia/Seoul"
        };

        StarfishShell::RendererWGL* renderer = &m_renderer;
        LWE::WebContainer::RendererGLConfiguration config;
        config.onMakeCurrent = [renderer](LWE::WebContainer*) {
            renderer->makeCurrent();
        };
        config.onSwapBuffers = [renderer](LWE::WebContainer*, bool) {
            renderer->swapBuffers();
        };
        config.onCreateSharedContext =
            [renderer](LWE::WebContainer*) -> uintptr_t {
            return renderer->createSharedContext();
        };
        config.onDestroyContext = [renderer](LWE::WebContainer*,
                                             uintptr_t context) -> bool {
            return renderer->destroyContext(context);
        };
        config.onClearCurrentContext = [renderer](LWE::WebContainer*) -> bool {
            return renderer->clearCurrentContext();
        };
        config.onMakeCurrentWithContext =
            [renderer](LWE::WebContainer*, uintptr_t context) -> bool {
            return renderer->makeCurrentWithContext(context);
        };
        config.onGetProcAddress = [renderer](LWE::WebContainer*,
                                             const char* name) -> void* {
            return renderer->getProcAddress(name);
        };
        config.onIsSupportedExtension = [renderer](LWE::WebContainer*,
                                                   const char* name) -> bool {
            return renderer->isSupportedExtension(name);
        };

        m_container = LWE::WebContainer::CreateGL(args, config);
        if (!m_container) {
            log("ERROR: WebContainer::CreateGL returned null");
            return false;
        }
        std::fprintf(stderr, "[StarfishShell] WebContainer created: %p\n",
                     static_cast<void*>(m_container));
        std::fflush(stderr);

        m_container->RegisterOnPageStartedHandler(
            [](LWE::WebContainer*, const std::string& url) {
                std::fprintf(stderr, "[StarfishShell:page-started] %s\n",
                             url.c_str());
                std::fflush(stderr);
            });
        m_container->RegisterOnPageParsedHandler(
            [](LWE::WebContainer*, const std::string& url) {
                std::fprintf(stderr, "[StarfishShell:page-parsed] %s\n",
                             url.c_str());
                std::fflush(stderr);
            });
        m_container->RegisterOnPageLoadedHandler(
            [this](LWE::WebContainer*, const std::string& url) {
                std::fprintf(stderr, "[StarfishShell:page-loaded] %s\n",
                             url.c_str());
                std::fflush(stderr);
                // Hand off to the UI thread: arming the capture is cheap, but
                // the repaint nudge that follows it needs a window timer, and
                // those belong to the thread that owns the window.
                if (!m_screenshotPath.empty()) {
                    PostMessageW(m_window, kMessagePageLoaded, 0, 0);
                }
            });

        LWE::Settings settings = m_container->GetSettings();
        settings.SetWebSecurityMode(LWE::WebSecurityMode::Disable);
        m_container->SetSettings(settings);

        // Nothing renders or takes input in an unfocused container.
        m_container->Focus();
        return true;
    }

    void destroyWebContainer()
    {
        if (!m_container) {
            return;
        }
        LWE::WebContainer* container = m_container;
        m_container = nullptr;
        container->Blur();
        container->Destroy();
        LWE::LWE::Finalize();
        log("engine finalized");
    }

    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam,
                                       LPARAM lParam)
    {
        Shell* shell =
            reinterpret_cast<Shell*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
            shell = static_cast<Shell*>(create->lpCreateParams);
            shell->m_window = window;
            SetWindowLongPtrW(window, GWLP_USERDATA,
                              reinterpret_cast<LONG_PTR>(shell));
        }
        return shell ? shell->handleMessage(window, message, wParam, lParam)
                     : DefWindowProcW(window, message, wParam, lParam);
    }

    LRESULT handleMessage(HWND window, UINT message, WPARAM wParam,
                          LPARAM lParam)
    {
        switch (message) {
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE: {
            if (wParam != SIZE_MINIMIZED) {
                // WM_SIZE can remain queued from CreateWindowEx while the
                // renderer is started after ShowWindow maximizes the HWND. Its
                // lParam then holds the obsolete pre-maximized size, so query
                // the HWND at dispatch time instead.
                RECT client;
                GetClientRect(window, &client);
                unsigned width = static_cast<unsigned>(client.right);
                unsigned height = static_cast<unsigned>(client.bottom);
                std::fprintf(stderr,
                             "[StarfishShell] WM_SIZE: message=%ux%u "
                             "current=%ux%u\n",
                             LOWORD(lParam), HIWORD(lParam), width, height);
                std::fflush(stderr);
                if (m_container) {
                    m_container->ResizeTo(width, height);
                }
            }
            return 0;
        }
        case WM_DPICHANGED: {
            const RECT* suggested = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(window, nullptr, suggested->left, suggested->top,
                         suggested->right - suggested->left,
                         suggested->bottom - suggested->top,
                         SWP_NOACTIVATE | SWP_NOZORDER);
            return 0;
        }
        case WM_MOUSEMOVE:
            if (m_container) {
                bool left = (wParam & MK_LBUTTON) != 0;
                bool right = (wParam & MK_RBUTTON) != 0;
                unsigned buttons = LWE::MouseButtonsValue::NoButtonDown;
                if (left) {
                    buttons |= LWE::MouseButtonsValue::LeftButtonDown;
                }
                if (right) {
                    buttons |= LWE::MouseButtonsValue::RightButtonDown;
                }
                LWE::MouseButtonValue button = LWE::MouseButtonValue::NoButton;
                if (left) {
                    button = LWE::MouseButtonValue::LeftButton;
                } else if (right) {
                    button = LWE::MouseButtonValue::RightButton;
                }
                m_container->DispatchMouseMoveEvent(
                    button, static_cast<LWE::MouseButtonsValue>(buttons),
                    GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
            return 0;
        case WM_LBUTTONDOWN:
            SetCapture(window);
            SetFocus(window);
            if (m_container) {
                m_container->DispatchMouseDownEvent(
                    LWE::MouseButtonValue::LeftButton,
                    LWE::MouseButtonsValue::LeftButtonDown,
                    GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            }
            return 0;
        case WM_LBUTTONUP:
            ReleaseCapture();
            if (m_container) {
                m_container->DispatchMouseUpEvent(
                    LWE::MouseButtonValue::NoButton,
                    LWE::MouseButtonsValue::NoButtonDown, GET_X_LPARAM(lParam),
                    GET_Y_LPARAM(lParam));
            }
            return 0;
        case WM_MOUSEWHEEL: {
            POINT point{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(window, &point);
            if (m_container) {
                m_container->DispatchMouseWheelEvent(
                    point.x, point.y,
                    -GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
            }
            return 0;
        }
        case WM_SYSKEYDOWN:
            if (wParam == VK_F4 && (lParam & (1L << 29))) {
                return DefWindowProcW(window, message, wParam, lParam);
            }
            dispatchKey(true, wParam, lParam);
            return 0;
        case WM_KEYDOWN:
            dispatchKey(true, wParam, lParam);
            return 0;
        case WM_SYSKEYUP:
        case WM_KEYUP:
            dispatchKey(false, wParam, lParam);
            return 0;
        case WM_IME_STARTCOMPOSITION:
            m_isIMEActive = true;
            m_compositionCommitted = false;
            dispatchComposition(WM_IME_STARTCOMPOSITION, std::wstring());
            return DefWindowProcW(window, message, wParam, lParam);
        case WM_IME_COMPOSITION:
            if (lParam & GCS_RESULTSTR) {
                dispatchComposition(WM_IME_ENDCOMPOSITION,
                                    readCompositionString(GCS_RESULTSTR));
                m_compositionCommitted = true;
            } else if (lParam & GCS_COMPSTR) {
                dispatchComposition(WM_IME_COMPOSITION,
                                    readCompositionString(GCS_COMPSTR));
            }
            return DefWindowProcW(window, message, wParam, lParam);
        case WM_IME_ENDCOMPOSITION:
            if (!m_compositionCommitted) {
                dispatchComposition(WM_IME_ENDCOMPOSITION,
                                    readCompositionString(GCS_COMPSTR));
            }
            m_isIMEActive = false;
            m_compositionCommitted = false;
            return DefWindowProcW(window, message, wParam, lParam);
        case kMessagePageLoaded:
            m_renderer.requestScreenshot(m_screenshotPath, m_screenshotFrames,
                                         m_window);
            // First nudge immediately, then keep nudging: one forced frame is
            // enough unless --screenshot-frames asked for more, and retrying
            // costs nothing next to failing the run.
            nudgeRepaint();
            SetTimer(window, kNudgeTimerId, kNudgeIntervalMs, nullptr);
            return 0;
        case WM_TIMER:
            if (wParam == kTimeoutTimerId) {
                KillTimer(window, kTimeoutTimerId);
                std::fprintf(stderr,
                             "[StarfishShell] ERROR: --timeout-ms elapsed "
                             "(%lu frames presented, capture %s)\n",
                             m_renderer.swapCount(),
                             m_renderer.screenshotPending() ? "still armed"
                                                            : "not armed");
                std::fflush(stderr);
                // Nonzero, so a hung page fails the run. run() keeps this
                // over the screenshot check, which would report 1 anyway.
                PostQuitMessage(3);
                return 0;
            }
            if (wParam == kNudgeTimerId) {
                if (!m_renderer.screenshotPending()) {
                    KillTimer(window, kNudgeTimerId);
                    return 0;
                }
                nudgeRepaint();
                return 0;
            }
            return DefWindowProcW(window, message, wParam, lParam);
        case WM_CLOSE:
            // Keep the HWND valid: run() tears the engine and the GL context
            // down after the message loop returns.
            PostQuitMessage(0);
            return 0;
        case WM_DESTROY:
            m_window = nullptr;
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(window, message, wParam, lParam);
        }
    }

    void dispatchKey(bool pressed, WPARAM key, LPARAM keyData)
    {
        if (!m_container || m_isIMEActive) {
            return;
        }
        LWE::KeyValue value = virtualKeyCodeToKeyValue(
            normalizedVirtualKey(key, keyData), usesCapitalAlphabet());
        if (pressed) {
            m_container->DispatchKeyDownEvent(value);
        } else {
            m_container->DispatchKeyUpEvent(value);
        }
    }

    void dispatchComposition(UINT message, const std::wstring& text)
    {
        if (!m_container) {
            return;
        }
        std::string utf8 = toUTF8(text);
        if (message == WM_IME_STARTCOMPOSITION) {
            m_container->DispatchCompositionStartEvent(utf8);
        } else if (message == WM_IME_COMPOSITION) {
            m_container->DispatchCompositionUpdateEvent(utf8);
        } else {
            m_container->DispatchCompositionEndEvent(utf8);
        }
    }

    bool createWindow()
    {
        log("registering window class");
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = windowProc;
        windowClass.hInstance = m_instance;
        windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        windowClass.lpszClassName = kWindowClassName;
        if (!RegisterClassExW(&windowClass) &&
            GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            logWin32Error("RegisterClassExW");
            showError(L"RegisterClassExW failed");
            return false;
        }

        log("creating native window");
        m_window =
            CreateWindowExW(0, kWindowClassName, L"Starfish",
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                            1280, 720, nullptr, nullptr, m_instance, this);
        if (!m_window) {
            logWin32Error("CreateWindowExW");
            showError(L"CreateWindowExW failed");
            return false;
        }
        log("native window created");
        return true;
    }

    static bool usesCapitalAlphabet()
    {
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool capsLock = (GetKeyState(VK_CAPITAL) & 1) != 0;
        return shift != capsLock;
    }

    static UINT normalizedVirtualKey(WPARAM key, LPARAM keyData)
    {
        if (key == VK_SHIFT) {
            UINT scanCode = (static_cast<UINT>(keyData) >> 16) & 0xff;
            return MapVirtualKeyW(scanCode, MAPVK_VSC_TO_VK_EX);
        }
        if (key == VK_CONTROL) {
            return (keyData & (1L << 24)) ? VK_RCONTROL : VK_LCONTROL;
        }
        if (key == VK_MENU) {
            return (keyData & (1L << 24)) ? VK_RMENU : VK_LMENU;
        }
        return static_cast<UINT>(key);
    }

    std::wstring readCompositionString(DWORD kind) const
    {
        HIMC context = ImmGetContext(m_window);
        if (!context) {
            return std::wstring();
        }

        std::wstring result;
        LONG byteLength = ImmGetCompositionStringW(context, kind, nullptr, 0);
        if (byteLength > 0) {
            result.resize(static_cast<size_t>(byteLength) / sizeof(wchar_t));
            LONG copied = ImmGetCompositionStringW(
                context, kind, &result[0], static_cast<DWORD>(byteLength));
            if (copied < 0) {
                result.clear();
            } else {
                result.resize(static_cast<size_t>(copied) / sizeof(wchar_t));
            }
        }
        ImmReleaseContext(m_window, context);
        return result;
    }

    static void enableDPIAwareness()
    {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        using SetDPIAwareness = BOOL(WINAPI*)(HANDLE);
        auto setDPIAwareness = reinterpret_cast<SetDPIAwareness>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        HANDLE perMonitorV2 =
            reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4));
        if (!setDPIAwareness || !setDPIAwareness(perMonitorV2)) {
            SetProcessDPIAware();
        }
    }

    void showError(const wchar_t* message)
    {
        MessageBoxW(m_window, message, L"StarfishShell error",
                    MB_OK | MB_ICONERROR);
    }

    void shutdownWindow()
    {
        if (m_window) {
            DestroyWindow(m_window);
            m_window = nullptr;
        }
        UnregisterClassW(kWindowClassName, m_instance);
    }

    HINSTANCE m_instance{ nullptr };
    HWND m_window{ nullptr };
    StarfishShell::RendererWGL m_renderer;
    LWE::WebContainer* m_container{ nullptr };
    bool m_isIMEActive{ false };
    bool m_compositionCommitted{ false };
    std::string m_initialURL;
    // Empty unless --screenshot was given, which is what turns the shell into
    // a one-shot CI check instead of an interactive host.
    std::string m_screenshotPath;
    // Zero by default: the frame that gets captured is one this shell forced
    // after page load, so it is a complete repaint already. Raise it only to
    // skip past a driver that needs a warm-up frame.
    unsigned m_screenshotFrames{ 0 };
    unsigned m_timeoutMs{ 0 };
    int m_captureWidth{ kDefaultCaptureWidth };
    int m_captureHeight{ kDefaultCaptureHeight };
    bool m_fixedWindowSize{ false };
    bool m_captureNudge{ false };
};

} // namespace

int wmain(int argumentCount, wchar_t** arguments)
{
    Shell shell;
    return shell.run(argumentCount, arguments);
}
#endif
