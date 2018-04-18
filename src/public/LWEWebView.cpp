/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#include "StarFishConfig.h"
#include "StarFish.h"

#include "LWEWebView.h"
#include "platform/window/PlatformWindow.h"
#include "core/page/BrowsingContext.h"
#include "core/page/Window.h"
#include "core/page/WebView.h"
#include "core/page/History.h"
#include "core/page/Location.h"
#include "core/dom/Document.h"
#include "binding/ScriptWrappable.h"
#include "JavaScriptNativeHandler.h"
#include "core/modules/threading/Thread.h"
#include "core/dom/MouseEvent.h"
#include "core/dom/KeyboardEvent.h"
#include "platform/network/HTTPCache.h"

#include <EscargotPublic.h>

#ifdef PORT_WINDOW_BACKEND_ANDROID
#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

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
    jmethodID m_onLoadResource;
    jmethodID m_onReceivedError;
    jmethodID m_onPageFinished;
    jmethodID m_onPageStarted;
    WindowGlue()
    {
        m_startTimer = m_requestRender = 0;
    }
} g_WindowGlue;
JavaVM* g_jvm;

std::map<LWE::WebView*, std::pair<jobject, LWE::WebViewClient*>> g_webViews;

typedef bool (*TimerCallback)(int uid, void* data);
int startTimer(int ms, TimerCallback pointer, void* data);
void cancelTimer(int uid);

void callOnLoadResourceHandler(LWE::WebView* view, const char* url);
void callOnReceivedError(LWE::WebView* view, int errorCode, bool canGoBack,
                         bool canGoForward);
void callOnPageFinished(LWE::WebView* view, const char* url, bool canGoBack,
                        bool canGoForward);
void callOnPageStarted(LWE::WebView* view, const char* url, bool canGoBack,
                       bool canGoForward);

extern unsigned char* g_androidBitmapAddress;
extern size_t g_androidBitmapWidth;
extern size_t g_androidBitmapHeight;
extern size_t g_androidBitmapStride;

#endif

#define TO_STARFISH(ptr) ((StarFish::StarFish*)ptr)
#define TO_HISTORY(ptr)         \
    ((StarFish::StarFish*)ptr)  \
        ->platformWindow()      \
        ->webView()             \
        ->mainBrowsingContext() \
        ->window()              \
        ->history()

#define TO_LOCATION(ptr)        \
    ((StarFish::StarFish*)ptr)  \
        ->platformWindow()      \
        ->webView()             \
        ->mainBrowsingContext() \
        ->window()              \
        ->location()

#define TO_RESOURCE_LOADER(ptr) \
    ((StarFish::StarFish*)ptr)  \
        ->platformWindow()      \
        ->webView()             \
        ->mainBrowsingContext() \
        ->document()            \
        ->resourceLoader()

#define TO_SCRIPT_BINDING_INSTANCE(ptr) \
    ((StarFish::StarFish*)ptr)          \
        ->platformWindow()              \
        ->webView()                     \
        ->mainBrowsingContext()         \
        ->window()                      \
        ->scriptBindingInstance()

namespace LWE {

static StarFish::ScriptValue nativeCallbackFunction(
    StarFish::ScriptExecutionState state, StarFish::ScriptValue thisValue,
    size_t argc, StarFish::ScriptValue* argv, bool isNewExpression)
{
    auto callee = StarFish::toCalleeObject(state);
    if (callee) {
        void* data = callee->extraData();
        if (data) {
            StarFish::ScriptWrappable* w = (StarFish::ScriptWrappable*)data;
            if (w->isJavaScriptNativeHandler()) {
                StarFish::JavaScriptNativeHandler* jsNhandler =
                    (StarFish::JavaScriptNativeHandler*)w;
                StarFish::String* result = StarFish::String::emptyString;
                StarFish::String* param = StarFish::String::emptyString;
                if (argc > 0) {
                    StarFish::ScriptValue arg0 = argv[0];
                    param = StarFish::toBrowserString(state, arg0);
                }
                result = jsNhandler->callNativeHandler(param);
                return StarFish::createScriptValue(
                    StarFish::createScriptString(result));
            }
        }
    }
    return StarFish::scriptUndefined();
}

Settings::Settings(std::string default_ua, std::string ua)
    : m_defaultUserAgent(default_ua)
    , m_UserAgent(ua)
#if defined(STARFISH_ENABLE_HTTPCACHE)
    , m_cacheMode(StarFish::HTTPCache::LOAD_DEFAULT)
#endif
{
}

std::string Settings::GetDefaultUserAgent()
{
    return m_defaultUserAgent;
}

std::string Settings::GetUserAgentString()
{
    return m_UserAgent;
}

void Settings::SetUserAgentString(std::string ua)
{
    m_UserAgent = ua;
}

int Settings::GetCacheMode()
{
    return m_cacheMode;
}
void Settings::SetCacheMode(int mode)
{
    m_cacheMode = mode;
}

ResourceError::ResourceError(int code, std::string description)
    : m_errorCode(code)
    , m_description(description)
{
}

int ResourceError::GetErrorCode()
{
    return m_errorCode;
}

std::string ResourceError::GetDescription()
{
    return m_description;
}

WebView* WebView::Create(void* win, int x, int y, int width, int height)
{
    // elm_init(0, 0);
    // elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

    std::string screenShot;
    std::string customUserAgentString;
    std::string builtinPolyfillPathString;
    int flag = 0;
    float scaleFactor = 1;

    StarFish::ScreenInfo info;
    info.rect.setWidth(width);
    info.rect.setHeight(height);
    info.availableRect.setWidth(width);
    info.availableRect.setHeight(height);
    info.deviceScaleFactor = scaleFactor;

    std::string cacheDir(getenv("HOME"));
    cacheDir += "/Starfish-cache";
    StarFish::StarFish* starfish = new StarFish::StarFish(
        (StarFish::StarFishStartUpFlag)flag, "ko-KR", "Asia/Seoul", win, width,
        height, x, y, 1, StarFish::String::createASCIIString("sans-serif"),
        info, "/tmp/StarFish_localStorage.txt", "/tmp/StarFish_Cookies.txt",
        cacheDir.data(),
        StarFish::String::fromUTF8(customUserAgentString.data()),
        StarFish::String::fromUTF8(builtinPolyfillPathString.data()));

    return new WebView(starfish);
}

WebView* WebView::Create(void* starFish)
{
    return new WebView(starFish);
}

WebView::WebView(void* starFish)
    : m_starfish(starFish)
{
}

Settings WebView::GetSettings()
{
    STARFISH_ASSERT(m_starfish);
    Settings result(USER_AGENT(STARFISH_NAME, VERSION),
                    TO_STARFISH(m_starfish)->userAgent()->toUTF8NonGCString());
#ifdef STARFISH_ENABLE_HTTPCACHE
    result.SetCacheMode(TO_STARFISH(m_starfish)->httpCache()->cacheMode());
#endif
    return result;
}

void WebView::LoadURL(std::string url)
{
    STARFISH_ASSERT(m_starfish);
    TO_STARFISH(m_starfish)
        ->loadHTMLDocument(StarFish::String::fromUTF8(url.data()));
}

std::string WebView::GetURL()
{
    STARFISH_ASSERT(m_starfish);
    return TO_LOCATION(m_starfish)->url()->urlString()->toUTF8NonGCString();
}

void WebView::LoadData(std::string data)
{
    STARFISH_ASSERT(m_starfish);
    unsigned int dataLength = data.size();
    // base64 encode
    if (data.size() > 0) {
        const char* originData = data.c_str();
        std::string dataURI = "data:text/html;charset=utf-8;base64,";
        std::string base64Chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        int i = 0, j = 0;
        unsigned char charArray3[3];
        unsigned char charArray4[4];

        while (dataLength--) {
            charArray3[i++] = *(originData++);
            if (i == 3) {
                charArray4[0] = (charArray3[0] & 0xfc) >> 2;
                charArray4[1] = ((charArray3[0] & 0x03) << 4) +
                                ((charArray3[1] & 0xf0) >> 4);
                charArray4[2] = ((charArray3[1] & 0x0f) << 2) +
                                ((charArray3[2] & 0xc0) >> 6);
                charArray4[3] = charArray3[2] & 0x3f;

                for (i = 0; (i < 4); i++) {
                    dataURI += base64Chars[charArray4[i]];
                }
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 3; j++) {
                charArray3[j] = '\0';
            }

            charArray4[0] = (charArray3[0] & 0xfc) >> 2;
            charArray4[1] =
                ((charArray3[0] & 0x03) << 4) + ((charArray3[1] & 0xf0) >> 4);
            charArray4[2] =
                ((charArray3[1] & 0x0f) << 2) + ((charArray3[2] & 0xc0) >> 6);
            charArray4[3] = charArray3[2] & 0x3f;

            for (j = 0; (j < i + 1); j++) {
                dataURI += base64Chars[charArray4[j]];
            }

            while ((i++ < 3)) {
                dataURI += '=';
            }
        }
        TO_STARFISH(m_starfish)
            ->loadHTMLDocument(StarFish::String::fromUTF8(dataURI.data()));
    }
}

void WebView::Reload()
{
    STARFISH_ASSERT(m_starfish);
    TO_LOCATION(m_starfish)->reload();
}

void WebView::StopLoading()
{
    STARFISH_ASSERT(m_starfish);
    TO_RESOURCE_LOADER(m_starfish).clear();
}

void WebView::GoBack()
{
    STARFISH_ASSERT(m_starfish);
    TO_HISTORY(m_starfish)->back();
}

void WebView::GoForward()
{
    STARFISH_ASSERT(m_starfish);
    TO_HISTORY(m_starfish)->forward();
}

bool WebView::CanGoBack()
{
    STARFISH_ASSERT(m_starfish);
    return TO_HISTORY(m_starfish)->canGoBack();
}

bool WebView::CanGoForward()
{
    STARFISH_ASSERT(m_starfish);
    return TO_HISTORY(m_starfish)->canGoForward();
}

void WebView::AddJavaScriptInterface(std::string exposedObjectName,
                                     std::string jsFunctionName,
                                     std::function<std::string(std::string)> cb)
{
    STARFISH_ASSERT(m_starfish);

    StarFish::String* objectName =
        StarFish::String::fromUTF8(exposedObjectName.c_str());
    StarFish::String* functionName =
        StarFish::String::fromUTF8(jsFunctionName.c_str());

    StarFish::registerJavaScriptNativeInterface(
        TO_SCRIPT_BINDING_INSTANCE(m_starfish), objectName, functionName,
        new StarFish::JavaScriptNativeHandler(TO_STARFISH(m_starfish),
                                              functionName, cb),
        nativeCallbackFunction);
}

std::string WebView::EvaluateJavaScript(std::string script)
{
    STARFISH_ASSERT(m_starfish);
    return TO_STARFISH(m_starfish)
        ->evaluate(StarFish::String::fromUTF8(script.c_str()))
        ->toUTF8NonGCString();
}

void WebView::ClearHistory()
{
    STARFISH_ASSERT(m_starfish);
    TO_STARFISH(m_starfish)
        ->platformWindow()
        ->webView()
        ->historyManager()
        ->clear();
}

void WebView::Destroy()
{
    STARFISH_ASSERT(m_starfish);
    delete TO_STARFISH(m_starfish);
}

void WebView::SetSettings(LWE::Settings setttings)
{
    STARFISH_ASSERT(m_starfish);
    TO_STARFISH(m_starfish)
        ->setCustomUserAgentString(
            StarFish::String::fromUTF8(setttings.GetUserAgentString().c_str()));
#ifdef STARFISH_ENABLE_HTTPCACHE
    TO_STARFISH(m_starfish)
        ->httpCache()
        ->setCacheMode(setttings.GetCacheMode());
#endif
}

void WebView::RemoveJavascriptInterface(std::string exposedObjectName,
                                        std::string jsFunctionName)
{
    STARFISH_ASSERT(m_starfish);
    StarFish::String* objectName =
        StarFish::String::fromUTF8(exposedObjectName.c_str());
    StarFish::String* functionName =
        StarFish::String::fromUTF8(jsFunctionName.c_str());

    StarFish::unregisterJavaScriptNativeInterface(
        TO_SCRIPT_BINDING_INSTANCE(m_starfish), objectName, functionName);
}

void WebView::SetWebViewClient(LWE::WebViewClient* client)
{
    m_webViewClient = client;

    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnReceivedError"),
            [this](StarFish::String* url, int errorCode) -> void {
                // make error description
                this->m_webViewClient->OnReceivedError(
                    this, ResourceError(errorCode, std::string()));
            });

    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnPageFinished"),
            [this](StarFish::String* url, int errorCode) -> void {
                this->m_webViewClient->OnPageFinished(this,
                                                      url->toUTF8NonGCString());
            });

    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnPageStarted"),
            [this](StarFish::String* url, int errorCode) -> void {
                this->m_webViewClient->OnPageStarted(this,
                                                     url->toUTF8NonGCString());
            });

    TO_STARFISH(m_starfish)
        ->registerWebViewHandler(
            std::string("OnLoadResource"),
            [this](StarFish::String* url, int errorCode) -> void {
                this->m_webViewClient->OnLoadResource(this,
                                                      url->toUTF8NonGCString());
            });
}

void WebView::ClearCache()
{
    STARFISH_ASSERT(m_starfish);
#ifdef STARFISH_ENABLE_HTTPCACHE
    TO_STARFISH(m_starfish)->httpCache()->clear();
#endif
}

void* WebView::getInternalPtr()
{
    return m_starfish;
}

void* WebView::unwrap()
{
    if (m_starfish) {
        return ((StarFish::StarFish*)m_starfish)->LWEWebViewDelegator();
    }
    return nullptr;
}
}

#ifdef PORT_WINDOW_BACKEND_ANDROID

using namespace StarFish;

static jmethodID GetJMethod(JNIEnv* env, jclass clazz, const char name[],
                            const char signature[])
{
    jmethodID m = env->GetStaticMethodID(clazz, name, signature);
    if (!m) {
        LOGE("Could not find Java method %s\n", name);
    }
    return m;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_init(JNIEnv* env,
                                                        jobject thiz)
{
    LOGI(
        "Java_com_samsung_android_mobileservice_lwe_WebView_init called %p "
        "%p",
        env, thiz);

    if (g_jvm) {
        return;
    }
    env->GetJavaVM(&g_jvm);
    jclass clazz =
        env->FindClass("com/samsung/android/mobileservice/lwe/WebView");
    g_WindowGlue.m_clazz = (jclass)env->NewGlobalRef(clazz);
    g_WindowGlue.m_env = env;
    g_WindowGlue.m_startTimer = GetJMethod(env, clazz, "startTimer", "(III)I");
    g_WindowGlue.m_cancelTimer = GetJMethod(env, clazz, "cancelTimer", "(I)V");

    g_WindowGlue.m_requestRender =
        env->GetMethodID(clazz, "requestRender", "()V");
    g_WindowGlue.m_onLoadResource =
        env->GetMethodID(clazz, "onLoadResource", "(Ljava/lang/String;)V");
    g_WindowGlue.m_onReceivedError =
        env->GetMethodID(clazz, "onReceivedError", "(IZZ)V");
    g_WindowGlue.m_onPageFinished =
        env->GetMethodID(clazz, "onPageFinished", "(Ljava/lang/String;ZZ)V");
    g_WindowGlue.m_onPageStarted =
        env->GetMethodID(clazz, "onPageStarted", "(Ljava/lang/String;ZZ)V");

    env->DeleteLocalRef(clazz);

    LOGI("Java_com_samsung_android_mobileservice_lwe_WebView_init call end");
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_serviceQueueTimer(
    JNIEnv* env, jobject thiz, jint uid, jint fn, jint data)
{
    STARFISH_RELEASE_ASSERT(StarFish::isMainThread());
    TimerCallback tc = (TimerCallback)fn;
    bool ret = (*tc)(uid, (void*)data);
    return ret;
}

void callOnLoadResourceHandler(LWE::WebView* view, const char* url)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    LOGE("OnLoadResource");
    if (!env || !g_WindowGlue.m_onLoadResource) {
        LOGE("OnLoadResource error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    jstring jstr = env->NewStringUTF(url);
    env->CallVoidMethod(g_webViews[view].first, g_WindowGlue.m_onLoadResource,
                        jstr);
}

void callOnReceivedError(LWE::WebView* view, int errorCode, bool canGoBack,
                         bool canGoForward)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    LOGE("OnReceivedError");
    if (!env || !g_WindowGlue.m_onReceivedError) {
        LOGE("OnPageStarted error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    jint jint1 = errorCode;
    jboolean jboolean1 = canGoBack;
    jboolean jboolean2 = canGoForward;
    env->CallVoidMethod(g_webViews[view].first, g_WindowGlue.m_onReceivedError,
                        jint1, jboolean1, jboolean2);
}
void callOnPageFinished(LWE::WebView* view, const char* url, bool canGoBack,
                        bool canGoForward)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    LOGE("OnPageFinished");
    if (!env || !g_WindowGlue.m_onPageFinished) {
        LOGE("OnPageFinished error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    jstring jstr = env->NewStringUTF(url);
    jboolean jboolean1 = canGoBack;
    jboolean jboolean2 = canGoForward;
    env->CallVoidMethod(g_webViews[view].first, g_WindowGlue.m_onPageFinished,
                        jstr, jboolean1, jboolean2);
}
void callOnPageStarted(LWE::WebView* view, const char* url, bool canGoBack,
                       bool canGoForward)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            LOGE("Failed to attach");
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    LOGE("OnPageStarted");
    if (!env || !g_WindowGlue.m_onPageStarted) {
        LOGE("OnPageStarted error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    jstring jstr = env->NewStringUTF(url);
    jboolean jboolean1 = canGoBack;
    jboolean jboolean2 = canGoForward;

    env->CallVoidMethod(g_webViews[view].first, g_WindowGlue.m_onPageStarted,
                        jstr, jboolean1, jboolean2);
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

void requestRender(void* view)
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
    } else if (getEnvStat == JNI_EVERSION) {
        LOGE("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    // LOGE("requestRender");
    if (!env || !g_WindowGlue.m_requestRender) {
        LOGE("reuqest render error");
        return;
    }
    env->CallVoidMethod(g_webViews[(LWE::WebView*)view].first,
                        g_WindowGlue.m_requestRender);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_Create(
    JNIEnv* env, jobject thiz, jint w, jint h, jfloat devicePixelRatio,
    jstring jua)
{
    ScreenInfo info;
    info.rect.setWidth(w);
    info.rect.setHeight(h);
    info.availableRect.setWidth(w);
    info.availableRect.setHeight(h);
    info.deviceScaleFactor = devicePixelRatio;

    const char* locale = "ko-KR";
    const char* timezoneID = "Asia/Seoul";
    const char* cacheDir = "/mnt/sdcard/TMP";
    float defaultFontSizeMultiplier = 1;

    const char* cstr = env->GetStringUTFChars(jua, NULL);
    String* ua = String::fromUTF8(cstr);

    StarFish::StarFish* starfish = new (NoGC) StarFish::StarFish(
        (StarFish::StarFishStartUpFlag)0, locale, timezoneID, nullptr, w, h, 0,
        0, defaultFontSizeMultiplier, String::fromUTF8("Roboto"), info, "", "",
        cacheDir, ua);
    env->ReleaseStringUTFChars(jua, cstr);

    LWE::WebView* webView = LWE::WebView::Create(starfish);
    starfish->setLWEWebView((void*)webView);

    class AndroidWebViewClient : public LWE::WebViewClient {
        virtual void OnReceivedError(LWE::WebView* view,
                                     LWE::ResourceError error) override
        {
            callOnReceivedError(view, error.GetErrorCode(), view->CanGoBack(),
                                view->CanGoBack());
        }
        virtual void OnPageFinished(LWE::WebView* view,
                                    std::string url) override
        {
            callOnPageFinished(view, url.c_str(), view->CanGoBack(),
                               view->CanGoBack());
        }
        virtual void OnPageStarted(LWE::WebView* view, std::string url) override
        {
            callOnPageStarted(view, url.c_str(), view->CanGoBack(),
                              view->CanGoBack());
        }
        virtual void OnLoadResource(LWE::WebView* view,
                                    std::string url) override
        {
            callOnLoadResourceHandler(view, url.c_str());
        }
    };
    AndroidWebViewClient* client = new AndroidWebViewClient();
    webView->SetWebViewClient(client);

    jobject java_webview = env->NewGlobalRef(thiz);
    g_webViews.insert(
        std::make_pair(webView, std::make_pair(java_webview, client)));

    return (jlong)webView;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_Destroy(JNIEnv* env,
                                                           jobject thiz,
                                                           jlong wv)
{
    LWE::WebView* webView = (LWE::WebView*)wv;
    webView->Destroy();
    env->DeleteGlobalRef(g_webViews[webView].first);
    delete (g_webViews[webView].second);
    g_webViews.erase(webView);
    delete webView;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_resizeWebView(JNIEnv* env,
                                                                 jobject thiz,
                                                                 jlong sf,
                                                                 jint w, jint h)
{
    LWE::WebView* webView = (LWE::WebView*)sf;
    ((StarFish::StarFish*)webView->getInternalPtr())
        ->platformWindow()
        ->resizeTo(w, h);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_loadUrl(JNIEnv* env,
                                                           jobject thiz,
                                                           jlong wv,
                                                           jstring url)
{
    const char* nativeString = env->GetStringUTFChars(url, 0);
    std::string urlString = std::string(nativeString);
    env->ReleaseStringUTFChars(url, nativeString);

    LWE::WebView* webView = (LWE::WebView*)wv;
    webView->LoadURL(urlString);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_loadData(JNIEnv* env,
                                                            jobject thiz,
                                                            jlong wv,
                                                            jstring data)
{
    const char* nativeString = env->GetStringUTFChars(data, 0);
    std::string dataString = std::string(nativeString);
    env->ReleaseStringUTFChars(data, nativeString);

    LWE::WebView* webView = (LWE::WebView*)wv;
    webView->LoadData(dataString);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_EvaluateJavaScript(
    JNIEnv* env, jobject thiz, jlong wv, jstring data)
{
    const char* nativeString = env->GetStringUTFChars(data, 0);
    std::string dataString = std::string(nativeString);
    env->ReleaseStringUTFChars(data, nativeString);

    LWE::WebView* webView = (LWE::WebView*)wv;
    std::string result = webView->EvaluateJavaScript(dataString);
    jstring jstr = env->NewStringUTF(result.c_str());
    return jstr;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_getDefaultUserAgent(
    JNIEnv* env, jobject thiz)
{
    std::string result = USER_AGENT(STARFISH_NAME, VERSION);
    jstring jstr = env->NewStringUTF(result.c_str());
    return jstr;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_GoBack(JNIEnv* env,
                                                          jobject thiz,
                                                          jlong data)
{
    LWE::WebView* webView = (LWE::WebView*)data;
    webView->GoBack();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_GoForward(JNIEnv* env,
                                                             jobject thiz,
                                                             jlong data)
{
    LWE::WebView* webView = (LWE::WebView*)data;
    webView->GoForward();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_Reload(JNIEnv* env,
                                                          jobject thiz,
                                                          jlong data)
{
    LWE::WebView* webView = (LWE::WebView*)data;
    webView->Reload();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_StopLoading(JNIEnv* env,
                                                               jobject thiz,
                                                               jlong data)
{
    LWE::WebView* webView = (LWE::WebView*)data;
    webView->StopLoading();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_ClearHistory(JNIEnv* env,
                                                                jobject thiz,
                                                                jlong data)
{
    LWE::WebView* webView = (LWE::WebView*)data;
    webView->ClearHistory();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_addJavascriptInterface(
    JNIEnv* env, jobject thiz, jlong wv, jstring objName, jstring funtionName,
    jobject instance)
{
    const char* nativeString1 = env->GetStringUTFChars(objName, 0);
    std::string objNameString = std::string(nativeString1);
    env->ReleaseStringUTFChars(objName, nativeString1);

    const char* nativeString2 = env->GetStringUTFChars(funtionName, 0);
    std::string functionNameString = std::string(nativeString2);
    env->ReleaseStringUTFChars(funtionName, nativeString2);

    jobject callback_obj = env->NewGlobalRef(instance);
    jclass clz = env->GetObjectClass(callback_obj);
    jmethodID callback_methodID = env->GetMethodID(
        clz, nativeString2, "(Ljava/lang/String;)Ljava/lang/String;");

    std::shared_ptr<_jobject> javaObjectRef(callback_obj, [](jobject ref) {
        JNIEnv* env = g_WindowGlue.m_env;

        int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (getEnvStat == JNI_EDETACHED) {
            if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else if (getEnvStat == JNI_OK) {
        } else if (getEnvStat == JNI_EVERSION) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        if (!env) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        env->DeleteGlobalRef(ref);
    });

    std::function<std::string(std::string)> NB =
        [javaObjectRef, callback_obj, clz,
         callback_methodID](std::string param) -> std::string {

        JNIEnv* env = g_WindowGlue.m_env;

        int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (getEnvStat == JNI_EDETACHED) {
            if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
                STARFISH_RELEASE_ASSERT_NOT_REACHED();
            }
        } else if (getEnvStat == JNI_OK) {
        } else if (getEnvStat == JNI_EVERSION) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        if (!env) {
            STARFISH_RELEASE_ASSERT_NOT_REACHED();
        }
        jstring result = (jstring)env->CallObjectMethod(
            callback_obj, callback_methodID, env->NewStringUTF(param.c_str()));

        const char* nativeString3 = env->GetStringUTFChars(result, 0);
        std::string resultStr = std::string(nativeString3);
        env->ReleaseStringUTFChars(result, nativeString3);

        return resultStr;
    };

    LWE::WebView* webView = (LWE::WebView*)wv;
    webView->AddJavaScriptInterface(objNameString, functionNameString, NB);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_removeJavascriptInterface(
    JNIEnv* env, jobject thiz, jlong wv, jstring objName)
{
    const char* nativeString = env->GetStringUTFChars(objName, 0);
    StarFish::String* objectName = StarFish::String::fromUTF8(nativeString);
    env->ReleaseStringUTFChars(objName, nativeString);

    LWE::WebView* webView = (LWE::WebView*)wv;
    StarFish::StarFish* starFish =
        (StarFish::StarFish*)webView->getInternalPtr();

    STARFISH_ASSERT(starFish);
    StarFish::unregisterJavaScriptNativeInterface(
        TO_SCRIPT_BINDING_INSTANCE(starFish), objectName);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_rendering(JNIEnv* env,
                                                             jobject thiz,
                                                             jlong wv,
                                                             jobject bitmap)
{
    LWE::WebView* webView = (LWE::WebView*)wv;

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

    ((StarFish::StarFish*)webView->getInternalPtr())
        ->platformWindow()
        ->rendering();

    AndroidBitmap_unlockPixels(env, bitmap);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_setUserAgentString(
    JNIEnv* env, jobject thiz, jlong wv, jstring userAgent)
{
    const char* nativeString = env->GetStringUTFChars(userAgent, 0);
    StarFish::String* uaString = StarFish::String::fromUTF8(nativeString);
    env->ReleaseStringUTFChars(userAgent, nativeString);

    LWE::WebView* webView = (LWE::WebView*)wv;
    ((StarFish::StarFish*)webView->getInternalPtr())
        ->setCustomUserAgentString(uaString);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_setCacheMode(JNIEnv* env,
                                                                jobject thiz,
                                                                jlong wv,
                                                                jint mode)
{
#ifdef STARFISH_ENABLE_HTTPCACHE
    LWE::WebView* webView = (LWE::WebView*)wv;
    ((StarFish::StarFish*)webView->getInternalPtr())
        ->httpCache()
        ->setCacheMode(mode);
#endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_ClearCache(JNIEnv* env,
                                                              jobject thiz,
                                                              jlong wv)
{
#ifdef STARFISH_ENABLE_HTTPCACHE
    LWE::WebView* webView = (LWE::WebView*)wv;
    ((StarFish::StarFish*)webView->getInternalPtr())->httpCache()->clear();
#endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_dispatchMouseDown(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    LWE::WebView* webView = (LWE::WebView*)data;
    PlatformWindow* sf =
        (PlatformWindow*)((StarFish::StarFish*)webView->getInternalPtr())
            ->platformWindow();
    StarFishEnterer enter(sf->starFish());
    MouseData mdata(MouseData::MouseButtonValue::LeftButton,
                    MouseData::MouseButtonsValue::LeftButtonDown,
                    x / sf->starFish()->screenInfo().deviceScaleFactor,
                    y / sf->starFish()->screenInfo().deviceScaleFactor, 1);
    sf->dispatchMouseEvent(MouseEventKind::MouseEventDown, mdata);
    // sf->m_isMouseLbuttonDown = true;

    LOGE("Mouse down=%f %f", x, y);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_dispatchMouseMove(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    LWE::WebView* webView = (LWE::WebView*)data;
    PlatformWindow* sf =
        (PlatformWindow*)((StarFish::StarFish*)webView->getInternalPtr())
            ->platformWindow();

    StarFishEnterer enter(sf->starFish());
    // unsigned char buttons = sf->m_isMouseLbuttonDown
    //                       ?
    //                       MouseData::MouseButtonsValue::LeftButtonDown
    //                       : 0;
    unsigned char buttons = MouseData::MouseButtonsValue::LeftButtonDown;
    MouseData mdata(0, buttons,
                    x / sf->starFish()->screenInfo().deviceScaleFactor,
                    y / sf->starFish()->screenInfo().deviceScaleFactor, 0);
    sf->dispatchMouseEvent(MouseEventKind::MouseEventMove, mdata);

    LOGE("Mouse move=%f %f", x, y);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_dispatchMouseUp(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    LWE::WebView* webView = (LWE::WebView*)data;
    PlatformWindow* sf =
        (PlatformWindow*)((StarFish::StarFish*)webView->getInternalPtr())
            ->platformWindow();

    StarFishEnterer enter(sf->starFish());
    MouseData mdata(MouseData::MouseButtonValue::NoButton,
                    MouseData::MouseButtonsValue::NoButtonDown,
                    x / sf->starFish()->screenInfo().deviceScaleFactor,
                    y / sf->starFish()->screenInfo().deviceScaleFactor, 1);
    sf->dispatchMouseEvent(MouseEventKind::MouseEventUp, mdata);
    // sf->m_isMouseLbuttonDown = false;

    LOGE("Mouse up=%f %f", x, y);
}

#endif
