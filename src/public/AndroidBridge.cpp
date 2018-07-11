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

#if defined(STARFISH_ANDROID)
#include "StarFishConfig.h"
#include "StarFish.h"

#include "LWEWebView.h"

#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
// #include <android/graphics/Bitmap.h>

#define LOG_TAG "StarFish"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct WindowGlue {
    JNIEnv* m_env;
    jclass m_clazz;
    jmethodID m_startTimer;
    jmethodID m_startIdler;
    jmethodID m_cancelTimer;
    jmethodID m_cancelIdler;
    jmethodID m_runAllRemainingIdler;
    jmethodID m_flushRendering;
    jmethodID m_onLoadResource;
    jmethodID m_onReceivedError;
    jmethodID m_onPageParsed;
    jmethodID m_onPageStarted;
    jmethodID m_shouldOverrideUrlLoading;
    jmethodID m_onProgressed;
    jmethodID m_onDownloadStart;

    WindowGlue()
    {
        m_startTimer = m_startIdler = m_flushRendering = 0;
    }
} g_WindowGlue;
JavaVM* g_jvm;

std::map<LWE::WebContainer*, std::pair<jobject, void*>> g_webViews;

typedef bool (*TimerCallback)(int uid, void* data);
int startTimer(int ms, TimerCallback pointer, void* data);
void cancelTimer(int uid);
void runAllRemainingIdler();
void flushRenderingCB(void* view);

void callOnLoadResourceHandler(LWE::WebView* view, const char* url);
void callOnReceivedError(LWE::WebView* view, int errorCode, bool canGoBack,
                         bool canGoForward);
void callOnPageParsed(LWE::WebView* view, const char* url, bool canGoBack,
                      bool canGoForward);
void callOnPageStarted(LWE::WebView* view, const char* url, bool canGoBack,
                       bool canGoForward);
bool callShouldOverrideUrlLoading(LWE::WebContainer* view, const char* url);
void callOnProgressChanged(LWE::WebContainer* view, int newProgress);
void callOnDownloadStart(LWE::WebContainer* view, const char* url,
                         const char* userAgent, const char* contentDisposition,
                         const char* mimetype, long contentLength);

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
    g_WindowGlue.m_startIdler = GetJMethod(env, clazz, "startIdler", "(III)I");
    g_WindowGlue.m_cancelTimer = GetJMethod(env, clazz, "cancelTimer", "(I)V");
    g_WindowGlue.m_cancelIdler = GetJMethod(env, clazz, "cancelIdler", "(I)V");
    g_WindowGlue.m_runAllRemainingIdler =
        GetJMethod(env, clazz, "runAllRemainingIdler", "()V");
    g_WindowGlue.m_flushRendering =
        env->GetMethodID(clazz, "flushRendering", "()V");
    g_WindowGlue.m_onLoadResource =
        env->GetMethodID(clazz, "onLoadResource", "(Ljava/lang/String;)V");
    g_WindowGlue.m_onReceivedError =
        env->GetMethodID(clazz, "onReceivedError", "(IZZ)V");
    g_WindowGlue.m_onPageParsed =
        env->GetMethodID(clazz, "onPageFinished", "(Ljava/lang/String;ZZ)V");
    g_WindowGlue.m_onPageStarted =
        env->GetMethodID(clazz, "onPageStarted", "(Ljava/lang/String;ZZ)V");
    g_WindowGlue.m_shouldOverrideUrlLoading = env->GetMethodID(
        clazz, "shouldOverrideUrlLoading", "(Ljava/lang/String;)Z");
    g_WindowGlue.m_onProgressed =
        env->GetMethodID(clazz, "onProgressChanged", "(I)V");
    g_WindowGlue.m_onDownloadStart =
        env->GetMethodID(clazz, "onDownloadStart",
                         "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/"
                         "String;Ljava/lang/String;J)V");
    env->DeleteLocalRef(clazz);

    LOGI("Java_com_samsung_android_mobileservice_lwe_WebView_init call end");
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_serviceQueueTimer(
    JNIEnv* env, jobject thiz, jint uid, jint fn, jint data)
{
    //    STARFISH_RELEASE_ASSERT(StarFish::isMainThread());
    TimerCallback tc = (TimerCallback)fn;
    bool ret = (*tc)(uid, (void*)data);
    return ret;
}

void callOnLoadResourceHandler(LWE::WebContainer* view, const char* url)
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

    if (!env || !g_WindowGlue.m_onLoadResource) {
        LOGE("OnLoadResource error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    jstring jstr = env->NewStringUTF(url);
    env->CallVoidMethod(g_webViews[view].first, g_WindowGlue.m_onLoadResource,
                        jstr);
}

void callOnReceivedError(LWE::WebContainer* view, int errorCode, bool canGoBack,
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
void callOnPageParsed(LWE::WebContainer* view, const char* url, bool canGoBack,
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

    if (!env || !g_WindowGlue.m_onPageParsed) {
        LOGE("OnPageParsed error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }
    jstring jstr = env->NewStringUTF(url);
    jboolean jboolean1 = canGoBack;
    jboolean jboolean2 = canGoForward;
    env->CallVoidMethod(g_webViews[view].first, g_WindowGlue.m_onPageParsed,
                        jstr, jboolean1, jboolean2);
}
void callOnPageStarted(LWE::WebContainer* view, const char* url, bool canGoBack,
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

bool callShouldOverrideUrlLoading(LWE::WebContainer* view, const char* url)
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

    if (!env || !g_WindowGlue.m_shouldOverrideUrlLoading) {
        LOGE("ShouldOverrideUrlLoading error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    jstring jstr = env->NewStringUTF(url);
    bool ret = env->CallBooleanMethod(
        g_webViews[view].first, g_WindowGlue.m_shouldOverrideUrlLoading, jstr);
    return ret;
}

void callOnProgressChanged(LWE::WebContainer* view, int progress)
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

    if (!env || !g_WindowGlue.m_onProgressed) {
        LOGE("OnProgressChanged error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    jint newProgress = progress;
    env->CallVoidMethod(g_webViews[view].first, g_WindowGlue.m_onProgressed,
                        newProgress);
}

void callOnDownloadStart(LWE::WebContainer* view, const char* url,
                         const char* userAgent, const char* contentDisposition,
                         const char* mimetype, long contentLength)
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

    if (!env || !g_WindowGlue.m_onDownloadStart) {
        LOGE("OnDownloadStarted error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    LOGI("ryanc callOnDownloadStart");

    jstring jurl = env->NewStringUTF(url);
    jstring juserAgent = env->NewStringUTF(userAgent);
    jstring jcontentDisposition = env->NewStringUTF(contentDisposition);
    jstring jmimetype = env->NewStringUTF(mimetype);
    jlong jcontentLength = contentLength;
    env->CallVoidMethod(g_webViews[view].first, g_WindowGlue.m_onDownloadStart,
                        jurl, juserAgent, jcontentDisposition, jmimetype,
                        jcontentLength);
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

int startIdler(int ms, TimerCallback pointer, void* data)
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

    if (!env || !g_WindowGlue.m_startIdler) {
        LOGE("signalQueueTimer error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    int ret = env->CallStaticIntMethod(g_WindowGlue.m_clazz,
                                       g_WindowGlue.m_startIdler, ms,
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

void cancelIdler(int uid)
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
    if (!env || !g_WindowGlue.m_cancelIdler) {
        LOGE("cancel error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    env->CallStaticVoidMethod(g_WindowGlue.m_clazz, g_WindowGlue.m_cancelIdler,
                              uid);
}

void runAllRemainingIdler()
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

    // LOGE("runAllRemainingIdler");
    if (!env || !g_WindowGlue.m_runAllRemainingIdler) {
        LOGE("runAllRemainingTimer error");
        STARFISH_RELEASE_ASSERT_NOT_REACHED();
    }

    env->CallStaticVoidMethod(g_WindowGlue.m_clazz,
                              g_WindowGlue.m_runAllRemainingIdler);
}

void flushRenderingCB(void* view)
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

    if (!env || !g_WindowGlue.m_flushRendering) {
        LOGE("reuqest render error");
        return;
    }
    env->CallVoidMethod(g_webViews[(LWE::WebContainer*)view].first,
                        g_WindowGlue.m_flushRendering);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_Create(
    JNIEnv* env, jobject thiz, jint w, jint h, jfloat devicePixelRatio,
    jstring jua, jstring locale, jstring timezoneID, jstring localstoragePath,
    jstring cookiePath, jstring cachePath)
{
    const char* localeString = env->GetStringUTFChars(locale, 0);
    const char* timezoneIDString = env->GetStringUTFChars(timezoneID, 0);
    const char* localstoragePathString =
        env->GetStringUTFChars(localstoragePath, 0);
    const char* cookiePathString = env->GetStringUTFChars(cookiePath, 0);
    const char* cachePathString = env->GetStringUTFChars(cachePath, 0);

    LWE::WebContainer* webContainer = LWE::WebContainer::Create(
        nullptr, w, h, 0, devicePixelRatio, localeString, timezoneIDString,
        localstoragePathString, cookiePathString, cachePathString);

    env->ReleaseStringUTFChars(locale, localeString);
    env->ReleaseStringUTFChars(timezoneID, timezoneIDString);
    env->ReleaseStringUTFChars(localstoragePath, localstoragePathString);
    env->ReleaseStringUTFChars(cookiePath, cookiePathString);
    env->ReleaseStringUTFChars(cachePath, cachePathString);

    webContainer->RegisterOnReceivedErrorHandler(
        [](LWE::WebContainer* view, LWE::ResourceError error) -> void {
            callOnReceivedError(view, error.GetErrorCode(), view->CanGoBack(),
                                view->CanGoForward());
        });

    webContainer->RegisterOnPageParsedHandler(
        [](LWE::WebContainer* view, const std::string& url) -> void {
            callOnPageParsed(view, url.c_str(), view->CanGoBack(),
                             view->CanGoForward());
        });

    webContainer->RegisterOnPageStartedHandler(
        [](LWE::WebContainer* view, const std::string& url) -> void {
            callOnPageStarted(view, url.c_str(), view->CanGoBack(),
                              view->CanGoForward());
        });

    webContainer->RegisterOnLoadResourceHandler(
        [](LWE::WebContainer* view, const std::string& url) -> void {
            callOnLoadResourceHandler(view, url.c_str());
        });

    webContainer->RegisterOnRenderedHandler(
        [](LWE::WebContainer* wv, void* buffer) -> void {
            flushRenderingCB(wv);
        });

    webContainer->RegisterShouldOverrideUrlLoadingHandler(
        [](LWE::WebContainer* view, const std::string& url) -> bool {
            return callShouldOverrideUrlLoading(view, url.c_str());
        });

    webContainer->RegisterOnProgressChangedHandler(
        [](LWE::WebContainer* view, int newProgress) -> void {
            callOnProgressChanged(view, newProgress);
        });

    webContainer->RegisterOnDownloadStartHandler(
        [](LWE::WebContainer* view, const std::string& url,
           const std::string& userAgent, const std::string& contentDisposition,
           const std::string& mimetype, long contentLength) -> void {
            callOnDownloadStart(view, url.c_str(), userAgent.c_str(),
                                contentDisposition.c_str(), mimetype.c_str(),
                                contentLength);
        });

    jobject java_webview = env->NewGlobalRef(thiz);
    g_webViews.insert(
        std::make_pair(webContainer, std::make_pair(java_webview, nullptr)));

    return (jlong)webContainer;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_Destroy(JNIEnv* env,
                                                           jobject thiz,
                                                           jlong wv)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->Destroy();
    env->DeleteGlobalRef(g_webViews[webContainer].first);
    if (g_webViews[webContainer].second != nullptr) {
        AndroidBitmap_unlockPixels(env,
                                   (jobject)g_webViews[webContainer].second);
        /*
        android::bitmap::unlockPixels(env,
                                      (jobject)g_webViews[webContainer].second);
        */
        env->DeleteGlobalRef((jobject)g_webViews[webContainer].second);
    }
    g_webViews.erase(webContainer);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_updateBuffer(
    JNIEnv* env, jobject thiz, jlong sf, jobject bitmap, jint w, jint h,
    jint stride)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)sf;
    int ret;
    void* pixels;

    if (g_webViews[webContainer].second != nullptr) {
        jobject bObject_old = (jobject)g_webViews[webContainer].second;
        AndroidBitmap_unlockPixels(env, bObject_old);
        /*
        android::bitmap::unlockPixels(env, bObject_old);
        */
        env->DeleteGlobalRef(bObject_old);
    }
    jobject bObject_new = env->NewGlobalRef(bitmap);
    if ((ret = AndroidBitmap_lockPixels(env, bObject_new, &pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }
    /*
    pixels = android::bitmap::lockPixels(env, bObject_new);
    */
    webContainer->UpdateBuffer(pixels, w, h, stride);
    g_webViews[webContainer].second = bObject_new;
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

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->LoadURL(urlString);
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

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->LoadData(dataString);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_EvaluateJavaScript(
    JNIEnv* env, jobject thiz, jlong wv, jstring data)
{
    const char* nativeString = env->GetStringUTFChars(data, 0);
    std::string dataString = std::string(nativeString);
    env->ReleaseStringUTFChars(data, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    std::string result = webContainer->EvaluateJavaScript(dataString);
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
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->GoBack();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_GoForward(JNIEnv* env,
                                                             jobject thiz,
                                                             jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->GoForward();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_Reload(JNIEnv* env,
                                                          jobject thiz,
                                                          jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->Reload();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_StopLoading(JNIEnv* env,
                                                               jobject thiz,
                                                               jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->StopLoading();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_ClearHistory(JNIEnv* env,
                                                                jobject thiz,
                                                                jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->ClearHistory();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_Resume(JNIEnv* env,
                                                          jobject thiz,
                                                          jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->Resume();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_Pause(JNIEnv* env,
                                                         jobject thiz,
                                                         jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->Pause();
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

    std::function<std::string(const std::string&)> NB =
        [javaObjectRef, callback_obj, clz,
         callback_methodID](const std::string& param) -> std::string {

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

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->AddJavaScriptInterface(objNameString, functionNameString, NB);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_removeJavascriptInterface(
    JNIEnv* env, jobject thiz, jlong wv, jstring objName)
{
    const char* nativeString = env->GetStringUTFChars(objName, 0);
    const std::string objectName(nativeString);
    env->ReleaseStringUTFChars(objName, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->RemoveJavascriptInterface(objectName, "");
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_setUserAgentString(
    JNIEnv* env, jobject thiz, jlong wv, jstring userAgent)
{
    const char* nativeString = env->GetStringUTFChars(userAgent, 0);
    const std::string uaString(nativeString);
    env->ReleaseStringUTFChars(userAgent, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->SetUserAgentString(uaString);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_setCacheMode(JNIEnv* env,
                                                                jobject thiz,
                                                                jlong wv,
                                                                jint mode)
{
#ifdef STARFISH_ENABLE_HTTPCACHE
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->SetCacheMode(mode);
#endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_ClearCache(JNIEnv* env,
                                                              jobject thiz,
                                                              jlong wv)
{
#ifdef STARFISH_ENABLE_HTTPCACHE
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->ClearCache();
#endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_dispatchMouseDown(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->DispatchMouseDownEvent(
        LWE::LeftButton, LWE::MouseButtonsValue::LeftButtonDown, x, y);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_dispatchMouseMove(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y,
    bool isLButtonPressed, bool isRButtonPressed)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->DispatchMouseMoveEvent(
        isLButtonPressed ? LWE::MouseButtonValue::LeftButton
                         : LWE::MouseButtonValue::NoButton,
        isLButtonPressed ? LWE::MouseButtonsValue::LeftButtonDown
                         : LWE::MouseButtonsValue::NoButtonDown,
        x, y);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_mobileservice_lwe_WebView_dispatchMouseUp(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->DispatchMouseUpEvent(LWE::MouseButtonValue::NoButton,
                                       LWE::MouseButtonsValue::NoButtonDown, x,
                                       y);
}

#endif
