/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
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

#if defined(STARFISH_ANDROID)
#include "StarfishConfig.h"
#include "Starfish.h"

#include "LWEWebView.h"

#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

struct WindowGlue {
    JNIEnv* m_env;
    jclass m_clazz;

    jmethodID m_onLoadResource;
    jmethodID m_onReceivedError;
    jmethodID m_onPageParsed;
    jmethodID m_onPageStarted;
    jmethodID m_shouldOverrideUrlLoading;
    jmethodID m_onProgressed;
    jmethodID m_onDownloadStart;
    jmethodID m_showDropdownMenu;
    jmethodID m_showAlert;
    jmethodID m_showIME;
    jmethodID m_hideIME;
    jmethodID m_glMakeCurrent;
    jmethodID m_glSwapBuffers;
    jmethodID m_canUseGL;

    WindowGlue()
    {
    }
} g_WindowGlue;
JavaVM* g_jvm;

std::map<LWE::WebContainer*, jobject> g_webViews;

void callOnLoadResourceHandler(LWE::WebView* view, const char* url);
void callOnReceivedError(LWE::WebView* view, int errorCode);
void callOnPageParsed(LWE::WebView* view, const char* url);
void callOnPageStarted(LWE::WebView* view, const char* url);
bool callShouldOverrideUrlLoading(LWE::WebContainer* view, const char* url);
void callOnProgressChanged(LWE::WebContainer* view, int newProgress);
void callOnDownloadStart(LWE::WebContainer* view, const char* url,
                         const char* userAgent, const char* contentDisposition,
                         const char* mimetype, long contentLength);

void callShowDropdownMenu(LWE::WebContainer* view,
                          const std::vector<std::string>* list,
                          int checkedPosition);
void callShowAlert(LWE::WebContainer* view, const std::string& title,
                   const std::string& message);

void showIME(void* view);
void hideIME(void* view);

LWE::KeyValue virtualKeyCodeToKeyValue(char ch, bool capsLockOrShiftPressed)
{
    switch (ch) {
    case 22:
        return LWE::KeyValue::ArrowRightKey;
    case 21:
        return LWE::KeyValue::ArrowLeftKey;
    case 20:
        return LWE::KeyValue::ArrowUpKey;
    case 19:
        return LWE::KeyValue::ArrowDownKey;
    case 18:
        return LWE::KeyValue::TabKey;
    case 13:
        return LWE::KeyValue::EnterKey;
    case 8:
        return LWE::KeyValue::BackspaceKey;
    default:
        break;
    }

    if (Starfish::String::isASCIIPrintableKey(ch)) {
        if (isalpha(ch)) {
            if (!capsLockOrShiftPressed) {
                ch = tolower(ch);
            }
        }
        return (LWE::KeyValue)ch;
    }
    return LWE::KeyValue::UnidentifiedKey;
}

static jmethodID GetJMethod(JNIEnv* env, jclass clazz, const char name[],
                            const char signature[])
{
    jmethodID m = env->GetStaticMethodID(clazz, name, signature);
    if (!m) {
        STARFISH_LOG_ERROR("Could not find Java method %s", name);
    }
    return m;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_init(
    JNIEnv* env, jobject thiz)
{
    STARFISH_LOG_INFO(
        "Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_"
        "init called "
        "%p "
        "%p",
        env, thiz);

    if (g_jvm) {
        return;
    }
    env->GetJavaVM(&g_jvm);
    jclass clazz = env->FindClass(
        "com/samsung/android/lightweightwebengine/internal/LweWebViewImpl");
    g_WindowGlue.m_clazz = (jclass)env->NewGlobalRef(clazz);
    g_WindowGlue.m_env = env;
    g_WindowGlue.m_onLoadResource =
        env->GetMethodID(clazz, "onLoadResource", "(Ljava/lang/String;)V");
    g_WindowGlue.m_onReceivedError =
        env->GetMethodID(clazz, "onReceivedError", "(ILjava/lang/String;)V");
    g_WindowGlue.m_onPageParsed =
        env->GetMethodID(clazz, "onPageFinished", "(Ljava/lang/String;)V");
    g_WindowGlue.m_onPageStarted =
        env->GetMethodID(clazz, "onPageStarted", "(Ljava/lang/String;)V");
    g_WindowGlue.m_shouldOverrideUrlLoading = env->GetMethodID(
        clazz, "shouldOverrideUrlLoading", "(Ljava/lang/String;)Z");
    g_WindowGlue.m_onProgressed =
        env->GetMethodID(clazz, "onProgressChanged", "(I)V");
    g_WindowGlue.m_onDownloadStart =
        env->GetMethodID(clazz, "onDownloadStart",
                         "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/"
                         "String;Ljava/lang/String;J)V");
    g_WindowGlue.m_showDropdownMenu =
        env->GetMethodID(clazz, "showDropdownMenu", "([Ljava/lang/String;I)V");
    g_WindowGlue.m_showAlert = env->GetMethodID(
        clazz, "showAlert", "(Ljava/lang/String;Ljava/lang/String;)V");
    g_WindowGlue.m_showIME = env->GetMethodID(clazz, "showSoftKeyboard", "()V");
    g_WindowGlue.m_hideIME = env->GetMethodID(clazz, "hideSoftKeyboard", "()V");
    g_WindowGlue.m_glMakeCurrent =
        env->GetMethodID(clazz, "glMakeCurrent", "()V");
    g_WindowGlue.m_glSwapBuffers =
        env->GetMethodID(clazz, "glSwapBuffers", "()V");
    g_WindowGlue.m_canUseGL = env->GetMethodID(clazz, "canUseGL", "()Z");
    env->DeleteLocalRef(clazz);

    STARFISH_LOG_INFO(
        "Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_"
        "init call "
        "end");
}

void callOnLoadResourceHandler(LWE::WebContainer* view, const char* url)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_onLoadResource) {
        STARFISH_LOG_ERROR("OnLoadResource error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    jstring jstr = env->NewStringUTF(url);
    env->CallVoidMethod(g_webViews[view], g_WindowGlue.m_onLoadResource, jstr);
    env->DeleteLocalRef(jstr);
}

void callOnReceivedError(LWE::WebContainer* view, int errorCode,
                         const char* url)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_onReceivedError) {
        STARFISH_LOG_ERROR("OnReceived: error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    jint jint1 = errorCode;
    jstring jstr = env->NewStringUTF(url);
    env->CallVoidMethod(g_webViews[view], g_WindowGlue.m_onReceivedError, jint1,
                        jstr);
    env->DeleteLocalRef(jstr);
}

void callOnPageParsed(LWE::WebContainer* view, const char* url)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_onPageParsed) {
        STARFISH_LOG_ERROR("OnPageParsed error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    jstring jstr = env->NewStringUTF(url);
    env->CallVoidMethod(g_webViews[view], g_WindowGlue.m_onPageParsed, jstr);
    env->DeleteLocalRef(jstr);
}

void callOnPageStarted(LWE::WebContainer* view, const char* url)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_onPageStarted) {
        STARFISH_LOG_ERROR("OnPageStarted error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    jstring jstr = env->NewStringUTF(url);

    env->CallVoidMethod(g_webViews[view], g_WindowGlue.m_onPageStarted, jstr);
    env->DeleteLocalRef(jstr);
}

bool callShouldOverrideUrlLoading(LWE::WebContainer* view, const char* url)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_shouldOverrideUrlLoading) {
        STARFISH_LOG_ERROR("ShouldOverrideUrlLoading error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    jstring jstr = env->NewStringUTF(url);
    bool ret = env->CallBooleanMethod(
        g_webViews[view], g_WindowGlue.m_shouldOverrideUrlLoading, jstr);
    env->DeleteLocalRef(jstr);

    return ret;
}

void callOnProgressChanged(LWE::WebContainer* view, int progress)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_onProgressed) {
        STARFISH_LOG_ERROR("OnProgressChanged error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    jint newProgress = progress;
    env->CallVoidMethod(g_webViews[view], g_WindowGlue.m_onProgressed,
                        newProgress);
}

void callOnDownloadStart(LWE::WebContainer* view, const char* url,
                         const char* userAgent, const char* contentDisposition,
                         const char* mimetype, long contentLength)
{
    STARFISH_LOG_INFO("OnDownloadStarted: started");
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_onDownloadStart) {
        STARFISH_LOG_ERROR("OnDownloadStarted: error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    jstring jurl = env->NewStringUTF(url);
    jstring juserAgent = env->NewStringUTF(userAgent);
    jstring jcontentDisposition = env->NewStringUTF(contentDisposition);
    jstring jmimetype = env->NewStringUTF(mimetype);
    jlong jcontentLength = contentLength;
    env->CallVoidMethod(g_webViews[view], g_WindowGlue.m_onDownloadStart, jurl,
                        juserAgent, jcontentDisposition, jmimetype,
                        jcontentLength);
    env->DeleteLocalRef(jurl);
    env->DeleteLocalRef(juserAgent);
    env->DeleteLocalRef(jcontentDisposition);
    env->DeleteLocalRef(jmimetype);
}

void callShowDropdownMenu(LWE::WebContainer* view,
                          const std::vector<std::string>* list,
                          int checkedPosition)
{
    STARFISH_LOG_INFO("ShowDropdownMenu: started");
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_showDropdownMenu) {
        STARFISH_LOG_ERROR("ShowDropdownMenu: error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    jsize len = list->size();
    jstring emptyStr = env->NewStringUTF("");
    jobjectArray jlist =
        env->NewObjectArray(len, env->FindClass("java/lang/String"), emptyStr);
    for (size_t i = 0; i < len; i++) {
        jstring str = env->NewStringUTF((*list)[i].c_str());
        env->SetObjectArrayElement(jlist, i, str);
        env->DeleteLocalRef(str);
    }
    env->DeleteLocalRef(emptyStr);
    jint jcheckedPosition = checkedPosition;

    env->CallVoidMethod(g_webViews[view], g_WindowGlue.m_showDropdownMenu,
                        jlist, jcheckedPosition);
    env->DeleteLocalRef(jlist);
}

void callShowAlert(LWE::WebContainer* view, const std::string& title,
                   const std::string& message)
{
    STARFISH_LOG_INFO("showAlert: started");
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_showAlert) {
        STARFISH_LOG_ERROR("showAlert: error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    jstring jtitle = env->NewStringUTF(title.c_str());
    jstring jmessage = env->NewStringUTF(message.c_str());
    env->CallVoidMethod(g_webViews[view], g_WindowGlue.m_showAlert, jtitle,
                        jmessage);
    env->DeleteLocalRef(jtitle);
    env->DeleteLocalRef(jmessage);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_onDropdownMenuItemSelected(
    JNIEnv* env, jobject thiz, jlong data, jint position)
{
    STARFISH_LOG_INFO("onDropdownmenuselected: started");
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;

    struct Param {
        int position;
    };
    Param* p = new Param();
    p->position = position;

    webContainer->CallHandler(std::string("onDropdownMenuItemSelected"),
                              (void*)p);
}

void showIME(void* view)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_showIME) {
        STARFISH_LOG_ERROR("showIME error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    env->CallVoidMethod(g_webViews[(LWE::WebContainer*)view],
                        g_WindowGlue.m_showIME);
}

void hideIME(void* view)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv: version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_hideIME) {
        STARFISH_LOG_ERROR("hideIME error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    env->CallVoidMethod(g_webViews[(LWE::WebContainer*)view],
                        g_WindowGlue.m_hideIME);
}

void glMakeCurrent(LWE::WebContainer* view)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv : version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_glMakeCurrent) {
        STARFISH_LOG_ERROR("glMakeCurrent error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    env->CallVoidMethod(g_webViews[view], g_WindowGlue.m_glMakeCurrent);
}

void glSwapBuffers(LWE::WebContainer* view)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv : version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_glSwapBuffers) {
        STARFISH_LOG_ERROR("glSwapBuffers error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    env->CallVoidMethod(g_webViews[view], g_WindowGlue.m_glSwapBuffers);
}

bool canUseGL(LWE::WebContainer* view)
{
    JNIEnv* env = g_WindowGlue.m_env;
    int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (getEnvStat == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread(&env, nullptr) != 0) {
            STARFISH_LOG_ERROR("Failed to attach");
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
    } else if (getEnvStat == JNI_OK) {
    } else if (getEnvStat == JNI_EVERSION) {
        STARFISH_LOG_ERROR("GetEnv : version not supported");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }

    if (!env || !g_WindowGlue.m_canUseGL) {
        STARFISH_LOG_ERROR("canUseGL error");
        STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
    }
    return env->CallBooleanMethod(g_webViews[view], g_WindowGlue.m_canUseGL);
}

void registerWebContainerHandler(LWE::WebContainer* webContainer)
{
    webContainer->RegisterOnReceivedErrorHandler(
        [](LWE::WebContainer* view, LWE::ResourceError error) -> void {
            callOnReceivedError(view, error.GetErrorCode(),
                                error.GetUrl().c_str());
        });

    webContainer->RegisterOnPageParsedHandler(
        [](LWE::WebContainer* view, const std::string& url) -> void {
            callOnPageParsed(view, url.c_str());
        });

    webContainer->RegisterOnPageStartedHandler(
        [](LWE::WebContainer* view, const std::string& url) -> void {
            callOnPageStarted(view, url.c_str());
        });

    webContainer->RegisterOnLoadResourceHandler(
        [](LWE::WebContainer* view, const std::string& url) -> void {
            callOnLoadResourceHandler(view, url.c_str());
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

    webContainer->RegisterShowDropdownMenuHandler(
        [](LWE::WebContainer* view, const std::vector<std::string>* list,
           int checkedPosition) -> void {
            callShowDropdownMenu(view, list, checkedPosition);
        });

    webContainer->RegisterShowAlertHandler(
        [](LWE::WebContainer* view, const std::string& title,
           const std::string& message) -> void {
            callShowAlert(view, title, message);
        });

    webContainer->RegisterOnShowSoftwareKeyboardIfPossibleHandler(
        [](LWE::WebContainer* wv) -> void { showIME(wv); });

    webContainer->RegisterOnHideSoftwareKeyboardIfPossibleHandler(
        [](LWE::WebContainer* wv) -> void { hideIME(wv); });
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_resizeTo(
    JNIEnv* env, jobject thiz, jlong container, jint width, jint height)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)container;
    webContainer->ResizeTo(width, height);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_create(
    JNIEnv* env, jobject thiz, jobject assetManager, jint w, jint h,
    jfloat devicePixelRatio, jstring jua, jstring locale, jstring timezoneID,
    jstring localstoragePath, jstring cookiePath, jstring cachePath)
{
    const char* localeString = env->GetStringUTFChars(locale, 0);
    const char* timezoneIDString = env->GetStringUTFChars(timezoneID, 0);
    const char* localstoragePathString =
        env->GetStringUTFChars(localstoragePath, 0);
    const char* cookiePathString = env->GetStringUTFChars(cookiePath, 0);
    const char* cachePathString = env->GetStringUTFChars(cachePath, 0);

    if (!LWE::LWE::IsInitialized()) {
        LWE::LWE::Initialize(localstoragePathString, cookiePathString,
                             cachePathString);
    }

    ::LWE::WebContainer::WebContainerArguments args = {
        w, h, devicePixelRatio, "serif", localeString, timezoneIDString
    };

    ::LWE::WebContainer::RendererGLConfiguration glConf;
    glConf.onMakeCurrent = [](::LWE::WebContainer* wc) { glMakeCurrent(wc); };
    glConf.onSwapBuffers = [](::LWE::WebContainer* wc, bool) {
        glSwapBuffers(wc);
    };
    glConf.onGetProcAddress = [](::LWE::WebContainer*,
                                 const char* name) -> void* {
        return reinterpret_cast<void*>(eglGetProcAddress(name));
    };
    glConf.onIsSupportedExtension = [](::LWE::WebContainer*,
                                       const char* name) -> bool {
        return strstr(eglQueryString(eglGetCurrentDisplay(), EGL_EXTENSIONS),
                      name) != nullptr;
    };

    ::LWE::WebContainer* webContainer =
        ::LWE::WebContainer::CreateGL(args, glConf);

    auto settings = webContainer->GetSettings();
    settings.SetIdleModeJob(LWE::IdleModeJob::ForceGC);
    webContainer->SetSettings(settings);

    AAssetManager* am = AAssetManager_fromJava(env, assetManager);
    webContainer->RegisterCustomFileResourceRequestHandlers(
        [](const char* path) -> const char* { return path; },
        [am](const char* path) -> void* {
            std::string p = path;
            if (p.find("/android_asset/") == 0) {
                p = p.substr(sizeof("/android_asset/") - 1);
                AAsset* as =
                    AAssetManager_open(am, p.data(), AASSET_MODE_BUFFER);
                STARFISH_LOG_ERROR("AAssetManager_open %s %p", p.data(), as);
                if (!as) {
                    return nullptr;
                }
                return (void*)((size_t)as + 1);
            } else {
                return fopen(path, "rb");
            }
        },
        [](uint8_t* destBuffer, size_t size, void* handle) -> size_t {
            if (((size_t)handle) & 1) {
                AAsset* as = (AAsset*)((size_t)handle - 1);
                return AAsset_read(as, destBuffer, size);
            }
            FILE* fp = (FILE*)handle;
            return fread(destBuffer, size, 1, fp);
        },
        [](void* handle) -> long int {
            if (((size_t)handle) & 1) {
                AAsset* as = (AAsset*)((size_t)handle - 1);
                return AAsset_getLength64(as);
            }
            FILE* fp = (FILE*)handle;
            size_t currentPosition = ftell(fp);
            fseek(fp, 0, SEEK_END);
            size_t size = ftell(fp);
            fseek(fp, currentPosition, SEEK_CUR);
            return size;
        },
        [](void* handle) {
            if (((size_t)handle) & 1) {
                AAsset* as = (AAsset*)((size_t)handle - 1);
                AAsset_close(as);
                return;
            }
            FILE* fp = (FILE*)handle;
            fclose(fp);
        });

    env->ReleaseStringUTFChars(locale, localeString);
    env->ReleaseStringUTFChars(timezoneID, timezoneIDString);
    env->ReleaseStringUTFChars(localstoragePath, localstoragePathString);
    env->ReleaseStringUTFChars(cookiePath, cookiePathString);
    env->ReleaseStringUTFChars(cachePath, cachePathString);

    registerWebContainerHandler(webContainer);

    jobject java_webview = env->NewGlobalRef(thiz);
    g_webViews.insert(std::make_pair(webContainer, java_webview));

    webContainer->RegisterCanRenderingHandler(
        [](LWE::WebContainer* wc) -> bool { return canUseGL(wc); });

    return (jlong)webContainer;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_destroy(
    JNIEnv* env, jobject thiz, jlong wv)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->Destroy();
    env->DeleteGlobalRef(g_webViews[webContainer]);
    g_webViews.erase(webContainer);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_loadUrl(
    JNIEnv* env, jobject thiz, jlong wv, jstring url)
{
    const char* nativeString = env->GetStringUTFChars(url, 0);
    std::string urlString = std::string(nativeString);
    env->ReleaseStringUTFChars(url, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->LoadURL(urlString);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_loadData(
    JNIEnv* env, jobject thiz, jlong wv, jstring data)
{
    const char* nativeString = env->GetStringUTFChars(data, 0);
    std::string dataString = std::string(nativeString);
    env->ReleaseStringUTFChars(data, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->LoadData(dataString);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_evaluateJavaScript(
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
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_getDefaultUserAgent(
    JNIEnv* env, jobject thiz)
{
    std::string result = USER_AGENT(STARFISH_NAME, VERSION);
    jstring jstr = env->NewStringUTF(result.c_str());
    return jstr;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_getUrl(
    JNIEnv* env, jobject thiz, jlong wv)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    return env->NewStringUTF(webContainer->GetURL().c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_goBack(
    JNIEnv* env, jobject thiz, jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->GoBack();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_goForward(
    JNIEnv* env, jobject thiz, jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->GoForward();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_canGoBack(
    JNIEnv* env, jobject thiz, jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    return webContainer->CanGoBack();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_canGoForward(
    JNIEnv* env, jobject thiz, jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    return webContainer->CanGoForward();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_reload(
    JNIEnv* env, jobject thiz, jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->Reload();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_stopLoading(
    JNIEnv* env, jobject thiz, jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->StopLoading();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_clearHistory(
    JNIEnv* env, jobject thiz, jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->ClearHistory();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_resume(
    JNIEnv* env, jobject thiz, jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->Resume();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_pause(
    JNIEnv* env, jobject thiz, jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->Pause();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_focus(
    JNIEnv* env, jobject thiz, jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->Focus();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_blur(
    JNIEnv* env, jobject thiz, jlong data)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->Blur();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_addJavascriptInterface(
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
                STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            }
        } else if (getEnvStat == JNI_OK) {
        } else if (getEnvStat == JNI_EVERSION) {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        if (!env) {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        env->DeleteGlobalRef(ref);
    });

    std::function<std::string(const std::string&)> NB =
        [javaObjectRef, callback_obj,
         callback_methodID](const std::string& param) -> std::string {
        JNIEnv* env = g_WindowGlue.m_env;

        int getEnvStat = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
        if (getEnvStat == JNI_EDETACHED) {
            if (g_jvm->AttachCurrentThread(&env, NULL) != 0) {
                STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
            }
        } else if (getEnvStat == JNI_OK) {
        } else if (getEnvStat == JNI_EVERSION) {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        if (!env) {
            STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE();
        }
        jstring paramStr = env->NewStringUTF(param.c_str());
        jstring result = (jstring)env->CallObjectMethod(
            callback_obj, callback_methodID, paramStr);
        env->DeleteLocalRef(paramStr);

        const char* nativeString3 = env->GetStringUTFChars(result, 0);
        std::string resultStr = std::string(nativeString3);
        env->ReleaseStringUTFChars(result, nativeString3);

        return resultStr;
    };

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->AddJavaScriptInterface(objNameString, functionNameString, NB);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_removeJavascriptInterface(
    JNIEnv* env, jobject thiz, jlong wv, jstring objName)
{
    const char* nativeString = env->GetStringUTFChars(objName, 0);
    const std::string objectName(nativeString);
    env->ReleaseStringUTFChars(objName, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->RemoveJavascriptInterface(objectName, "");
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_setUserAgentString(
    JNIEnv* env, jobject thiz, jlong wv, jstring userAgent)
{
    const char* nativeString = env->GetStringUTFChars(userAgent, 0);
    const std::string uaString(nativeString);
    env->ReleaseStringUTFChars(userAgent, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->SetUserAgentString(uaString);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_getUserAgentString(
    JNIEnv* env, jobject thiz, jlong wv)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    std::string result = webContainer->GetUserAgentString();
    jstring jstr = env->NewStringUTF(result.c_str());
    return jstr;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_setCacheMode(
    JNIEnv* env, jobject thiz, jlong wv, jint mode)
{
#ifdef STARFISH_ENABLE_HTTPCACHE
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->SetCacheMode(mode);
#endif
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_getCacheMode(
    JNIEnv* env, jobject thiz, jlong wv)
{
    jint ret = 0;
#ifdef STARFISH_ENABLE_HTTPCACHE
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    ret = webContainer->GetCacheMode();
#endif
    return ret;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_setDefaultFontSize(
    JNIEnv* env, jobject thiz, jlong wv, jint size)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->SetDefaultFontSize(size);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_getDefaultFontSize(
    JNIEnv* env, jobject thiz, jlong wv)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    return webContainer->GetDefaultFontSize();
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_setWebSecurityEnable(
    JNIEnv* env, jobject thiz, jlong wv, jboolean e)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    auto s = webContainer->GetSettings();
    s.SetWebSecurityMode(e ? LWE::WebSecurityMode::Enable
                           : LWE::WebSecurityMode::Disable);
    webContainer->SetSettings(s);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_getWebSecurityEnable(
    JNIEnv* env, jobject thiz, jlong wv)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    auto s = webContainer->GetSettings();
    return s.GetWebSecurityMode() != LWE::WebSecurityMode::Disable;
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_clearCache(
    JNIEnv* env, jobject thiz, jlong wv)
{
#ifdef STARFISH_ENABLE_HTTPCACHE
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->ClearCache();
#endif
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_dispatchMouseDown(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->DispatchMouseDownEvent(
        LWE::LeftButton, LWE::MouseButtonsValue::LeftButtonDown, x, y);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_dispatchMouseMove(
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
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_dispatchMouseUp(
    JNIEnv* env, jobject thiz, jlong data, jfloat x, jfloat y)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)data;
    webContainer->DispatchMouseUpEvent(LWE::MouseButtonValue::NoButton,
                                       LWE::MouseButtonsValue::NoButtonDown, x,
                                       y);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_dispatchKeyDown(
    JNIEnv* env, jobject thiz, jlong wv, jint keyCode, jint modifier)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->DispatchKeyDownEvent(
        virtualKeyCodeToKeyValue((char)keyCode, modifier));
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_dispatchKeyUp(
    JNIEnv* env, jobject thiz, jlong wv, jint keyCode, jint modifier)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->DispatchKeyUpEvent(
        virtualKeyCodeToKeyValue((char)keyCode, modifier));
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_dispatchKeyPress(
    JNIEnv* env, jobject thiz, jlong wv, jint keyCode, jint modifier)
{
    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->DispatchKeyPressEvent(
        virtualKeyCodeToKeyValue((char)keyCode, modifier));
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_dispatchCompositionStart(
    JNIEnv* env, jobject thiz, jlong wv, jstring keyValue)
{
    const char* nativeString = env->GetStringUTFChars(keyValue, 0);
    const std::string keyString = std::string(nativeString);
    env->ReleaseStringUTFChars(keyValue, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->DispatchCompositionStartEvent(keyString);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_dispatchCompositionUpdate(
    JNIEnv* env, jobject thiz, jlong wv, jstring keyValue)
{
    const char* nativeString = env->GetStringUTFChars(keyValue, 0);
    const std::string keyString = std::string(nativeString);
    env->ReleaseStringUTFChars(keyValue, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->DispatchCompositionUpdateEvent(keyString);
}

extern "C" JNIEXPORT void JNICALL
Java_com_samsung_android_lightweightwebengine_internal_LweWebViewImpl_dispatchCompositionEnd(
    JNIEnv* env, jobject thiz, jlong wv, jstring keyValue)
{
    const char* nativeString = env->GetStringUTFChars(keyValue, 0);
    const std::string keyString = std::string(nativeString);
    env->ReleaseStringUTFChars(keyValue, nativeString);

    LWE::WebContainer* webContainer = (LWE::WebContainer*)wv;
    webContainer->DispatchCompositionEndEvent(keyString);
}

#endif
