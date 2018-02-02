/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "StarFishConfig.h"
#ifdef PORT_WINDOW_BACKEND_ANDROID

#include "StarFish.h"
#include <cairo.h>
#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#include "core/animation/Animation.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "core/modules/canvas/Canvas.h"
#include "core/modules/canvas/Compositor.h"
#include "core/modules/threading/Locker.h"
#include "core/modules/message_loop/MessageLoop.h"
#include "core/modules/threading/Thread.h"

#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "platform/window/PlatformWindow.h"

#define LOG_TAG "StarFish"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct WindowGlue {
    JNIEnv* m_env;
    jclass m_clazz;
    jmethodID m_startTimer;
    jmethodID m_cancelTimer;
    jmethodID m_requestRender;
    WindowGlue()
    {
        m_startTimer = m_requestRender = 0;
    }
} g_WindowGlue;
JavaVM* g_jvm;

static jmethodID GetJMethod(JNIEnv* env, jclass clazz, const char name[],
                            const char signature[])
{
    jmethodID m = env->GetStaticMethodID(clazz, name, signature);
    if (!m)
        LOGE("Could not find Java method %s\n", name);
    return m;
}

extern "C" JNIEXPORT void JNICALL
Java_android_sec_com_lightweightwebengine_MainActivity_initStarFish(
    JNIEnv* env, jobject thiz)
{
    LOGI(
        "Java_android_sec_com_lightweightwebengine_MainActivity_init called %p "
        "%p",
        env, thiz);

    if (g_jvm) {
        return;
    }
    env->GetJavaVM(&g_jvm);
    jclass clazz =
        env->FindClass("android/sec/com/lightweightwebengine/MainActivity");
    g_WindowGlue.m_clazz = (jclass)env->NewGlobalRef(clazz);
    g_WindowGlue.m_env = env;
    g_WindowGlue.m_startTimer = GetJMethod(env, clazz, "startTimer", "(III)I");
    g_WindowGlue.m_cancelTimer = GetJMethod(env, clazz, "cancelTimer", "(I)V");
    g_WindowGlue.m_requestRender =
        GetJMethod(env, clazz, "requestRender", "()V");
    env->DeleteLocalRef(clazz);

    LOGI(
        "Java_android_sec_com_lightweightwebengine_MainActivity_init call end");
}

typedef bool (*TimerCallback)(int uid, void* data);
int startTimer(int ms, TimerCallback pointer, void* data);
void cancelTimer(int uid);
void requestRender();

extern "C" JNIEXPORT jboolean JNICALL
Java_android_sec_com_lightweightwebengine_MainActivity_serviceQueueTimer(
    JNIEnv* env, jobject thiz, jint uid, jint fn, jint data)
{
    STARFISH_RELEASE_ASSERT(StarFish::isMainThread());
    TimerCallback tc = (TimerCallback)fn;
    bool ret = (*tc)(uid, (void*)data);
    return ret;
}

int startTimer(int ms, TimerCallback pointer, void* data)
{
    JNIEnv* env = g_WindowGlue.m_env;

    // double check it's all ok
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        // std::cout << "GetEnv: not attached" << std::endl;
        // LOGE("GetEnv: not attached");
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            // std::cout << "Failed to attach" << std::endl;
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
        //
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    if (!env || !g_WindowGlue.m_startTimer) {
        LOGE("signalQueueTimer error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    int ret = env->CallStaticIntMethod(g_WindowGlue.m_clazz,
                                       g_WindowGlue.m_startTimer, ms,
                                       (long)pointer, (long)data);

    return ret;
}

void cancelTimer(int uid)
{
    JNIEnv* env = g_WindowGlue.m_env;

    // double check it's all ok
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        // std::cout << "GetEnv: not attached" << std::endl;
        // LOGE("GetEnv: not attached");
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            // std::cout << "Failed to attach" << std::endl;
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
        //
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    // LOGE("cancelTimer");
    if (!env || !g_WindowGlue.m_cancelTimer) {
        LOGE("cancel error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    env->CallStaticVoidMethod(g_WindowGlue.m_clazz, g_WindowGlue.m_cancelTimer,
                              uid);
}

void requestRender()
{
    JNIEnv* env = g_WindowGlue.m_env;

    // double check it's all ok
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        // std::cout << "GetEnv: not attached" << std::endl;
        // LOGE("GetEnv: not attached");
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            // std::cout << "Failed to attach" << std::endl;
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
        //
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    // LOGE("requestRender");
    if (!env || !g_WindowGlue.m_requestRender) {
        LOGE("reuqest render error");
        return;
    }

    env->CallStaticVoidMethod(g_WindowGlue.m_clazz,
                              g_WindowGlue.m_requestRender);
}

using namespace StarFish;

extern "C" JNIEXPORT jlong JNICALL
Java_android_sec_com_lightweightwebengine_MainActivity_initWebView(JNIEnv* env,
                                                                   jobject thiz,
                                                                   jint w,
                                                                   jint h)
{
    ScreenInfo info;
    info.rect.setWidth(w);
    info.rect.setHeight(h);
    info.availableRect.setWidth(w);
    info.availableRect.setHeight(h);

    const char* locale = "ko-KR";
    const char* timezoneID = "Asia/Seoul";
    float defaultFontSizeMultiplier = 1;

    StarFish::StarFish* starfish = new (NoGC) StarFish::StarFish(
        (StarFish::StarFishStartUpFlag)0, locale, timezoneID, nullptr, w, h, 0,
        0, defaultFontSizeMultiplier, String::fromUTF8("sans-serif"), info, "",
        "", nullptr);

    return (jlong)starfish;
}

extern "C" JNIEXPORT void JNICALL
Java_android_sec_com_lightweightwebengine_MainActivity_removeWebView(
    JNIEnv* env, jobject thiz, jlong sf)
{
    StarFish::StarFish* starfish = (StarFish::StarFish*)sf;
    delete starfish;
}

extern "C" JNIEXPORT void JNICALL
Java_android_sec_com_lightweightwebengine_MainActivity_resizeWebView(
    JNIEnv* env, jobject thiz, jlong sf, jint w, jint h)
{
    StarFish::StarFish* starfish = (StarFish::StarFish*)sf;
    starfish->platformWindow()->resizeTo(w, h);
}

unsigned char* g_androidBitmapAddress;
size_t g_androidBitmapWidth;
size_t g_androidBitmapHeight;
size_t g_androidBitmapStride;

extern "C" JNIEXPORT void JNICALL
Java_android_sec_com_lightweightwebengine_MainActivity_navigate(JNIEnv* env,
                                                                jobject thiz,
                                                                jlong sf,
                                                                jstring url)
{
    const char* nativeString = env->GetStringUTFChars(url, 0);
    String* urlString = String::fromUTF8(nativeString);
    env->ReleaseStringUTFChars(url, nativeString);

    StarFish::StarFish* starfish = (StarFish::StarFish*)sf;
    starfish->loadHTMLDocument(urlString);
}

namespace StarFish {

struct IdlerData {
    void (*m_fn)(void*);
    void* m_data;
};

class WindowImplAndroid : public PlatformWindow {
public:
    WindowImplAndroid(StarFish* sf, int32_t width, int32_t height)
        : PlatformWindow(sf)
        , m_width(width)
        , m_height(height)
        , m_rendingLockMutex(new Mutex())
    {
        m_renderingAnimator = 0;
        m_renderingIdlerData = nullptr;
        m_lastKeyPressedTimestamp = 0;
        m_offsetYDueToSoftwareKeyboard = 0;
        m_lastMouseX = m_lastMouseY = 0;
        m_isMouseLbuttonDown = false;

        GC_REGISTER_FINALIZER_NO_ORDER(
            this,
            [](void* obj, void* cd) {
                STARFISH_LOG_INFO("WindowImplAndroid::~WindowImplAndroid\n");
                WindowImplAndroid* s = (WindowImplAndroid*)obj;
            },
            NULL, NULL, NULL);
    }

    virtual int32_t width() override
    {
        return m_width;
    }

    virtual int32_t height() override
    {
        return m_height;
    }

    virtual void resizeTo(int w, int h)
    {
        m_width = w;
        m_height = h;
        onResize();
    }

    virtual void* unwrap()
    {
        return nullptr;
    }

    bool rendering()
    {
        m_stride = g_androidBitmapStride;
        m_surface = cairo_image_surface_create_for_data(
            (unsigned char*)g_androidBitmapAddress, CAIRO_FORMAT_ARGB32,
            g_androidBitmapWidth, g_androidBitmapHeight, m_stride);
        m_cairo = cairo_create(m_surface);
        bool ret = PlatformWindow::rendering();

        cairo_destroy(m_cairo);
        cairo_surface_destroy(m_surface);

        return ret;
    }

    virtual void clearResources();
    virtual Canvas* preparePainting();
    virtual Compositor* prepareCompositor();

    int32_t m_width;
    int32_t m_height;
    size_t m_renderingAnimator;
    IdlerData* m_renderingIdlerData;
    size_t m_stride;
    Mutex* m_rendingLockMutex;

    float m_lastMouseX, m_lastMouseY;
    bool m_isMouseLbuttonDown;
    bool m_isKeyDown;
    bool m_canRendering;
    uint32_t m_lastClickedTimestamp;
    uint32_t m_clickedCount;
    uint32_t m_lastKeyPressedTimestamp;
    int m_offsetYDueToSoftwareKeyboard;

    cairo_surface_t* m_surface;
    cairo_t* m_cairo;
};

class CanvasSurfaceAndroid : public CanvasSurface {
public:
    CanvasSurfaceAndroid(PlatformWindow* wnd, size_t w, size_t h)
    {
        m_width = w;
        m_height = h;
        m_window = (WindowImplAndroid*)wnd;

        m_bufferStride = m_imageWidth = m_bufferWidth = m_width = SIZE_MAX;
        m_imageHeight = m_bufferHeight = m_height = SIZE_MAX;

        m_bufferWidth = m_width = -1;
        m_bufferHeight = m_height = -1;
        m_pixelRatio = 1;
        attachNativeBuffer(w, h);

        resize(w, h);
        GC_REGISTER_FINALIZER_NO_ORDER(this,
                                       [](void* obj, void* cd) {
                                           CanvasSurfaceAndroid* s =
                                               (CanvasSurfaceAndroid*)obj;
                                           // STARFISH_LOG_INFO("release
                                           // CanvasSurfaceDALI %p\n", s);
                                           s->detachNativeBuffer();
                                       },
                                       NULL, NULL, NULL);
    }

    virtual void detachNativeBuffer()
    {
        free(m_buffer);
        m_buffer = nullptr;
    }

    void attachNativeBuffer(size_t w, size_t h)
    {
        if (m_width != w || m_height != h) {
            m_width = w;
            m_height = h;

            if ((int)w < m_window->starFish()->screenInfo().rect.width()) {
                w += STARFISH_CANVAS_SURFACE_MARGIN;
            }
            if ((int)h < m_window->starFish()->screenInfo().rect.height()) {
                h += STARFISH_CANVAS_SURFACE_MARGIN;
            }

            m_pixelRatio = 1;

            while ((m_width / m_pixelRatio > 20000) ||
                   (m_height / m_pixelRatio > 20000)) {
                m_pixelRatio++;
            }

            m_imageWidth = std::max((size_t)1, m_width / m_pixelRatio);
            m_imageHeight = std::max((size_t)1, m_height / m_pixelRatio);

            m_bufferWidth = std::max((size_t)1, w / m_pixelRatio);
            m_bufferHeight = std::max((size_t)1, h / m_pixelRatio);
            m_bufferStride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
                                                           m_bufferWidth);

            detachNativeBuffer();
            m_buffer = (unsigned char*)malloc(m_bufferWidth * m_bufferHeight *
                                              sizeof(uint32_t));
        }
    }

    virtual void resize(size_t w, size_t h)
    {
        if (m_width != w || m_height != h) {
            m_pixelRatio = 1;

            while ((w / m_pixelRatio > 10000) || (h / m_pixelRatio > 10000)) {
                m_pixelRatio++;
            }

            m_width = w;
            m_height = h;

            m_imageWidth = std::max((size_t)1, m_width / m_pixelRatio);
            m_imageHeight = std::max((size_t)1, m_height / m_pixelRatio);

            STARFISH_RELEASE_ASSERT(m_imageWidth <= m_bufferWidth);
            STARFISH_RELEASE_ASSERT(m_imageHeight <= m_bufferHeight);
        }
    }

    virtual void* unwrap()
    {
        return (void*)m_buffer;
    }

    virtual uint8_t* data()
    {
        return m_buffer;
    }

    virtual size_t width()
    {
        return m_width;
    }

    virtual size_t height()
    {
        return m_height;
    }

    virtual size_t bufferWidth()
    {
        return m_bufferWidth;
    }

    virtual size_t bufferHeight()
    {
        return m_bufferHeight;
    }

    virtual size_t imageWidth()
    {
        return m_imageWidth;
    }

    virtual size_t imageHeight()
    {
        return m_imageHeight;
    }

    virtual size_t pixelRatio()
    {
        return m_pixelRatio;
    }

    virtual size_t bufferStride()
    {
        return m_bufferStride;
    }

    virtual void clear()
    {
        size_t end = m_bufferWidth * m_bufferHeight * sizeof(uint32_t);
        memset(m_buffer, 0x00, end);
    }

protected:
    WindowImplAndroid* m_window;
    unsigned char* m_buffer;
    size_t m_width;
    size_t m_height;
    size_t m_imageWidth;
    size_t m_imageHeight;
    size_t m_bufferWidth;
    size_t m_bufferHeight;
    size_t m_bufferStride;
    size_t m_pixelRatio;
};

CanvasSurface* CanvasSurface::create(PlatformWindow* wnd, size_t w, size_t h)
{
    return new CanvasSurfaceAndroid(wnd, w, h);
}

PlatformWindow* PlatformWindow::create(StarFish* sf, void* win, int width,
                                       int height)
{
    auto wnd = new WindowImplAndroid(sf, width, height);
    wnd->m_starFish = sf;
    return wnd;
}

PlatformWindow::~PlatformWindow()
{
    STARFISH_LOG_INFO("PlatformWindow::~PlatformWindow\n");
}

void WebView::setNeedsRendering()
{
    WindowImplAndroid* wnd = (WindowImplAndroid*)starFish()->platformWindow();
    m_needsRendering = true;

    requestRender();
}

Canvas* WindowImplAndroid::preparePainting()
{
    struct dummy {
        cairo_t* cairo;
        cairo_surface_t* surface;
        int w;
        int h;
    };

    dummy* d = new dummy;

    d->cairo = m_cairo;
    d->surface = m_surface;
    d->w = width();
    d->h = height();

    Canvas* canvas = Canvas::createDirect(starFish(), d);
    delete d;

    return canvas;
}

Compositor* WindowImplAndroid::prepareCompositor()
{
    struct dummy {
        cairo_t* cairo;
        cairo_surface_t* surface;
        int w;
        int h;
    } d;
    d.cairo = m_cairo;
    d.surface = m_surface;
    d.w = width();
    d.h = height();
    return Compositor::create(starFish(), &d);
}

void WindowImplAndroid::clearResources()
{
    if (m_renderingAnimator) {
        starFish()->messageLoop()->removeIdler(m_renderingAnimator);
        m_renderingAnimator = 0;
        GC_FREE(m_renderingIdlerData);
    }

    webView()->clearStackingContext(false);
}
}

extern "C" JNIEXPORT void JNICALL
Java_android_sec_com_lightweightwebengine_MainActivity_rendering(JNIEnv* env,
                                                                 jobject thiz,
                                                                 jlong sf,
                                                                 jobject bitmap)
{
    StarFish::StarFish* starfish = (StarFish::StarFish*)sf;

    int ret;
    AndroidBitmapInfo info;
    if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return;
    }

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888 !");
        return;
    }

    void* pixels;
    if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

    g_androidBitmapAddress = (unsigned char*)pixels;
    g_androidBitmapWidth = info.width;
    g_androidBitmapHeight = info.height;
    g_androidBitmapStride = info.stride;

    ((WindowImplAndroid*)starfish->platformWindow())
        ->WindowImplAndroid::rendering();

    AndroidBitmap_unlockPixels(env, bitmap);
}

extern "C" JNIEXPORT void JNICALL
Java_android_sec_com_lightweightwebengine_MainActivity_dispatchMouseDown(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    StarFish::StarFish* starfish = (StarFish::StarFish*)data;
    WindowImplAndroid* sf = (WindowImplAndroid*)starfish->platformWindow();
    StarFishEnterer enter(sf->starFish());
    MouseData mdata(MouseData::MouseButtonValue::LeftButton,
                    MouseData::MouseButtonsValue::LeftButtonDown,
                    x * sf->starFish()->screenInfo().deviceScaleFactor,
                    y * sf->starFish()->screenInfo().deviceScaleFactor, 1);
    sf->dispatchMouseEvent(PlatformWindow::MouseEventDown, mdata);
    sf->m_isMouseLbuttonDown = true;

    LOGE("Mouse down=%f %f", x, y);
}

extern "C" JNIEXPORT void JNICALL
Java_android_sec_com_lightweightwebengine_MainActivity_dispatchMouseMove(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    StarFish::StarFish* starfish = (StarFish::StarFish*)data;
    WindowImplAndroid* sf = (WindowImplAndroid*)starfish->platformWindow();

    StarFishEnterer enter(sf->starFish());
    unsigned char buttons = sf->m_isMouseLbuttonDown
                                ? MouseData::MouseButtonsValue::LeftButtonDown
                                : 0;
    MouseData mdata(0, buttons,
                    x * sf->starFish()->screenInfo().deviceScaleFactor,
                    y * sf->starFish()->screenInfo().deviceScaleFactor, 0);
    sf->dispatchMouseEvent(PlatformWindow::MouseEventMove, mdata);

    LOGE("Mouse move=%f %f", x, y);
}

extern "C" JNIEXPORT void JNICALL
Java_android_sec_com_lightweightwebengine_MainActivity_dispatchMouseUp(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    StarFish::StarFish* starfish = (StarFish::StarFish*)data;
    WindowImplAndroid* sf = (WindowImplAndroid*)starfish->platformWindow();

    StarFishEnterer enter(sf->starFish());
    MouseData mdata(MouseData::MouseButtonValue::NoButton,
                    MouseData::MouseButtonsValue::NoButtonDown,
                    x * sf->starFish()->screenInfo().deviceScaleFactor,
                    y * sf->starFish()->screenInfo().deviceScaleFactor, 1);
    sf->dispatchMouseEvent(PlatformWindow::MouseEventUp, mdata);
    sf->m_isMouseLbuttonDown = false;

    LOGE("Mouse up=%f %f", x, y);
}

extern "C" JNIEXPORT bool JNICALL
Java_android_sec_com_lightweightwebengine_MainActivity_goHistoryBack(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    StarFish::StarFish* starfish = (StarFish::StarFish*)data;
    return starfish->platformWindow()->webView()->historyManager()->go(-1);
}

#endif
