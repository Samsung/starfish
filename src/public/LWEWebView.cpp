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

#include "LWEWebView.h"
#ifdef STARFISH_API_ENABLE_LOADER
#include "LWEDelegateLoader.h"
#else
#include "public/delegate/LWEDelegate.h"
#include "public/delegate/ResourceErrorDelegate.h"
#include "public/delegate/SettingsDelegate.h"
#include "public/delegate/CookieManagerDelegate.h"
#include "public/delegate/LWEWebContainerDelegate.h"
#include "public/delegate/LWEWebViewDelegate.h"
#endif

#include "APIRecorder.h"

#include <assert.h>
#include <iostream>

#if defined(NDEBUG)
#define LWE_ASSERT(assertion) ((void)0)
#else
#define LWE_ASSERT(assertion) assert(assertion);
#endif

#if defined(STARFISH_TIZEN_VERSION_5_0) && !defined(TIZEN_COMPAT_HEADER_5_0)
#error "Version Mismatch: You must build LWE on Tizen 5.5 Environment"
#endif

#if defined(STARFISH_TIZEN_VERSION_5_5) && defined(TIZEN_COMPAT_HEADER_5_0)
#error "Version Mismatch: You must build LWE on Tizen 5.0 Environment"
#endif

namespace LWE {

template <typename T>
T* toImpl(void* ptr)
{
    return static_cast<T*>(ptr);
}

void LWE::SetVersionPreference(bool preferUpdatedVersion)
{
#ifdef STARFISH_API_ENABLE_LOADER
    LWEDelegateLoader::getInstance()->setVersionPreference(
        preferUpdatedVersion);
#endif
    // Supported only when using a loader.
}

void LWE::Initialize(const char* storageDirectoryPath, bool preferMainThread)
{
    STARFISH_API_RECORD_INIT();
#ifdef STARFISH_API_ENABLE_LOADER
    if (!LWEDelegateLoader::getInstance()->load()) {
        LWE_ASSERT(false);
    }
    LWEDelegateLoader::getSafeInstance()->kLWEProcTable.Initialize(
        storageDirectoryPath, preferMainThread);
#else
    LWEDelegate::LWE::Initialize(storageDirectoryPath, preferMainThread);
#endif
}

bool LWE::IsInitialized()
{
#ifdef STARFISH_API_ENABLE_LOADER
    if (!LWEDelegateLoader::getInstance()->isLoaded()) {
        return false;
    }
    return LWEDelegateLoader::getSafeInstance()->kLWEProcTable.IsInitialized();
#else
    return LWEDelegate::LWE::IsInitialized();
#endif
}

void LWE::Finalize()
{
#ifdef STARFISH_API_ENABLE_LOADER
    LWEDelegateLoader::getSafeInstance()->kLWEProcTable.Finalize();
    LWEDelegateLoader::getSafeInstance()->unload();
#else
    LWEDelegate::LWE::Finalize();
#endif
}

unsigned char LWE::GetGCFrequency()
{
#ifdef STARFISH_API_ENABLE_LOADER
    return LWEDelegateLoader::getSafeInstance()->kLWEProcTable.GetGCFrequency();
#else
    return LWEDelegate::LWE::GetGCFrequency();
#endif
}

void LWE::SetGCFrequency(unsigned char freq)
{
    STARFISH_API_RECORD_EVENT_FMT("SetGCFrequency", "{\"freq\":%u}",
                                  (unsigned)freq);
#ifdef STARFISH_API_ENABLE_LOADER
    LWEDelegateLoader::getSafeInstance()->kLWEProcTable.SetGCFrequency(freq);
#else
    LWEDelegate::LWE::SetGCFrequency(freq);
#endif
}

void LWE::GetVersion(int* major, int* minor, int* patch)
{
#ifdef STARFISH_API_ENABLE_LOADER
    return LWEDelegateLoader::getSafeInstance()->kLWEProcTable.GetVersion(
        major, minor, patch);
#else
    return LWEDelegate::LWE::GetVersion(major, minor, patch);
#endif
}

bool LWE::IsUsingSeparateThread()
{
#ifdef STARFISH_API_ENABLE_LOADER
    return LWEDelegateLoader::getSafeInstance()
        ->kLWEProcTable.IsUsingSeparateThread();
#else
    return LWEDelegate::LWE::IsUsingSeparateThread();
#endif
}

ResourceError::ResourceError(int code, const std::string& description,
                             const std::string& url)
{
#ifdef STARFISH_API_ENABLE_LOADER
    LWEDelegate::ResourceError* resourceError =
        reinterpret_cast<LWEDelegate::ResourceError*>(
            LWEDelegateLoader::getSafeInstance()
                ->kResourceErrorProcTable.Create(code, description.c_str(),
                                                 url.c_str()));
#else
    LWEDelegate::ResourceError* resourceError =
        LWEDelegate::ResourceError::Create(code, description, url);
#endif
    m_delegate =
        LWEDelegateRef(static_cast<void*>(resourceError), [](void* ptr) {
            delete toImpl<LWEDelegate::ResourceError>(ptr);
        });
}

ResourceError::ResourceError(const ResourceError& other)
{
#ifdef STARFISH_API_ENABLE_LOADER
    LWEDelegate::ResourceError* resourceError =
        reinterpret_cast<LWEDelegate::ResourceError*>(
            LWEDelegateLoader::getSafeInstance()
                ->kResourceErrorProcTable.Create(other.GetErrorCode(),
                                                 other.GetDescription().c_str(),
                                                 other.GetUrl().c_str()));
#else
    LWEDelegate::ResourceError* resourceError =
        LWEDelegate::ResourceError::Create(
            other.GetErrorCode(), other.GetDescription(), other.GetUrl());

#endif
    m_delegate =
        LWEDelegateRef(static_cast<void*>(resourceError), [](void* ptr) {
            delete toImpl<LWEDelegate::ResourceError>(ptr);
        });
}

ResourceError::~ResourceError()
{
}

int ResourceError::GetErrorCode() const
{
    return toImpl<LWEDelegate::ResourceError>(m_delegate.get())->GetErrorCode();
}

std::string ResourceError::GetDescription() const
{
    return toImpl<LWEDelegate::ResourceError>(m_delegate.get())
        ->GetDescription();
}

std::string ResourceError::GetUrl() const
{
    return toImpl<LWEDelegate::ResourceError>(m_delegate.get())->GetUrl();
}

Settings::Settings()
{
#ifdef STARFISH_API_ENABLE_LOADER
    LWEDelegate::Settings* delegate = reinterpret_cast<LWEDelegate::Settings*>(
        LWEDelegateLoader::getSafeInstance()->kSettingsProcTable.CreateEmpty());
#else
    LWEDelegate::Settings* delegate = LWEDelegate::Settings::Create();
#endif
    m_delegate = LWEDelegateRef(static_cast<void*>(delegate), [](void* ptr) {
        delete toImpl<LWEDelegate::Settings>(ptr);
    });
}

Settings::Settings(const std::string& defaultUA, const std::string& ua)
{
#ifdef STARFISH_API_ENABLE_LOADER
    LWEDelegate::Settings* delegate = reinterpret_cast<LWEDelegate::Settings*>(
        LWEDelegateLoader::getSafeInstance()->kSettingsProcTable.Create(
            defaultUA.c_str(), ua.c_str()));
#else
    LWEDelegate::Settings* delegate =
        LWEDelegate::Settings::Create(defaultUA, ua);
#endif
    m_delegate = LWEDelegateRef(static_cast<void*>(delegate), [](void* ptr) {
        delete toImpl<LWEDelegate::Settings>(ptr);
    });
}

Settings::Settings(const Settings& other)
{
#ifdef STARFISH_API_ENABLE_LOADER
    LWEDelegate::Settings* delegate = reinterpret_cast<LWEDelegate::Settings*>(
        LWEDelegateLoader::getSafeInstance()
            ->kSettingsProcTable.CreateFromOther(other.m_delegate.get()));
#else
    LWEDelegate::Settings* delegate = LWEDelegate::Settings::Create(
        toImpl<LWEDelegate::Settings>(other.m_delegate.get()));
#endif
    m_delegate = LWEDelegateRef(static_cast<void*>(delegate), [](void* ptr) {
        delete toImpl<LWEDelegate::Settings>(ptr);
    });
}

Settings::~Settings()
{
}

bool Settings::UpdateSetting(const std::string& key, const std::string& value)
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->UpdateSetting(key, value);
}

std::string Settings::GetSetting(std::string key) const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())->GetSetting(key);
}

std::string Settings::GetDefaultUserAgent() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->GetDefaultUserAgent();
}

std::string Settings::GetUserAgentString() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->GetUserAgentString();
}

std::string Settings::GetProxyURL() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())->GetProxyURL();
}

int Settings::GetCacheMode() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())->GetCacheMode();
}

TTSMode Settings::GetTTSMode() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())->GetTTSMode();
}

std::string Settings::GetTTSLanguage() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())->GetTTSLanguage();
}

WebSecurityMode Settings::GetWebSecurityMode() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->GetWebSecurityMode();
}

IdleModeJob Settings::GetIdleModeJob() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())->GetIdleModeJob();
}

uint32_t Settings::GetIdleModeCheckIntervalInMS() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->GetIdleModeCheckIntervalInMS();
}

void Settings::GetBaseBackgroundColor(unsigned char& r, unsigned char& g,
                                      unsigned char& b, unsigned char& a) const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->GetBaseBackgroundColor(r, g, b, a);
}

void Settings::GetBaseForegroundColor(unsigned char& r, unsigned char& g,
                                      unsigned char& b, unsigned char& a) const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->GetBaseForegroundColor(r, g, b, a);
}

bool Settings::NeedsDownloadWebFontsEarly() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->NeedsDownloadWebFontsEarly();
}

bool Settings::UseHttp2() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())->UseHttp2();
}

uint32_t Settings::NeedsDownScaleImageResourceLargerThan() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->NeedsDownScaleImageResourceLargerThan();
}

bool Settings::ScrollbarVisible() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())->ScrollbarVisible();
}

bool Settings::UseExternalPopup() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())->UseExternalPopup();
}

bool Settings::UseSpatialNavigation() const
{
    return toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->UseSpatialNavigation();
}

void Settings::SetUserAgentString(const std::string& ua)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())->SetUserAgentString(ua);
}

void Settings::SetCacheMode(int mode)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())->SetCacheMode(mode);
}

void Settings::SetProxyURL(const std::string& proxyURL)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())->SetProxyURL(proxyURL);
}

void Settings::setDefaultFontSize(int size)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())->setDefaultFontSize(size);
}

void Settings::SetTTSMode(TTSMode value)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())->SetTTSMode(value);
}

void Settings::SetTTSLanguage(const std::string& language)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())->SetTTSLanguage(language);
}

void Settings::SetBaseBackgroundColor(unsigned char r, unsigned char g,
                                      unsigned char b, unsigned char a)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->SetBaseBackgroundColor(r, g, b, a);
}

void Settings::SetBaseForegroundColor(unsigned char r, unsigned char g,
                                      unsigned char b, unsigned char a)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->SetBaseForegroundColor(r, g, b, a);
}

void Settings::SetWebSecurityMode(WebSecurityMode value)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())->SetWebSecurityMode(value);
}

void Settings::SetIdleModeJob(IdleModeJob j)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())->SetIdleModeJob(j);
}

void Settings::SetIdleModeCheckIntervalInMS(uint32_t intervalInMS)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->SetIdleModeCheckIntervalInMS(intervalInMS);
}

void Settings::SetNeedsDownloadWebFontsEarly(bool b)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->SetNeedsDownloadWebFontsEarly(b);
}

void Settings::SetUseHttp2(bool b)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())->SetUseHttp2(b);
}

void Settings::SetNeedsDownScaleImageResourceLargerThan(uint32_t demention)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->SetNeedsDownScaleImageResourceLargerThan(demention);
}

void Settings::SetScrollbarVisible(bool visible)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->SetScrollbarVisible(visible);
}

void Settings::SetUseExternalPopup(bool useExternalPopup)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->SetUseExternalPopup(useExternalPopup);
}

void Settings::SetUseSpatialNavigation(bool useSpatialNavigation)
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())
        ->SetUseSpatialNavigation(useSpatialNavigation);
}

void Settings::IterateSettings(
    std::function<void(const std::string&, const std::string&)> callback) const
{
    toImpl<LWEDelegate::Settings>(m_delegate.get())->IterateSettings(callback);
}

CookieManager::CookieManager()
{
}

CookieManager::~CookieManager()
{
}

std::string CookieManager::GetCookie(std::string url)
{
    return toImpl<LWEDelegate::CookieManager>(m_delegate.get())->GetCookie(url);
}

bool CookieManager::HasCookies()
{
    return toImpl<LWEDelegate::CookieManager>(m_delegate.get())->HasCookies();
}

void CookieManager::ClearCookies()
{
    STARFISH_API_RECORD_EVENT("ClearCookies", "{}");
    toImpl<LWEDelegate::CookieManager>(m_delegate.get())->ClearCookies();
}

static CookieManager* g_instance = nullptr;

CookieManager* CookieManager::GetInstance()
{
    if (!g_instance) {
#ifdef STARFISH_API_ENABLE_LOADER
        LWEDelegate::CookieManager* delegate =
            reinterpret_cast<LWEDelegate::CookieManager*>(
                LWEDelegateLoader::getSafeInstance()
                    ->kCookieManagerProcTable.GetInstance());
#else
        LWEDelegate::CookieManager* delegate =
            LWEDelegate::CookieManager::GetInstance();
#endif
        if (delegate) {
            g_instance = new CookieManager();
            g_instance->m_delegate =
                LWEDelegateRef(static_cast<void*>(delegate), [](void* ptr) {
                    // Do nothing, use Destroy to release delegate.
                });
        }
    }

    return g_instance;
}

void CookieManager::Destroy()
{
    if (g_instance) {
#ifdef STARFISH_API_ENABLE_LOADER
        LWEDelegateLoader::getSafeInstance()->kCookieManagerProcTable.Destroy();
#else
        LWEDelegate::CookieManager::Destroy();
#endif
        g_instance->m_delegate = nullptr;
        delete g_instance;
        g_instance = nullptr;
    }
}
#if defined(STARFISH_TIZEN_VERSION_5_0)
WebContainer* WebContainer::Create(void* buffer, unsigned bufferWidth,
                                   unsigned bufferHeight, unsigned bufferStride,
                                   float devicePixelRatio,
                                   const char* defaultFontName,
                                   const char* locale, const char* timezoneID)
{
    WebContainer* instance = new WebContainer();
#ifdef STARFISH_API_ENABLE_LOADER
    auto delegate = reinterpret_cast<LWEDelegate::WebContainer*>(
        LWEDelegateLoader::getSafeInstance()
            ->kWebContainerProcTable.CreateWithBuffer(
                buffer, bufferWidth, bufferHeight, bufferStride,
                devicePixelRatio, defaultFontName, locale, timezoneID));
#else
    auto delegate = LWEDelegate::WebContainer::CreateWithBuffer(
        buffer, bufferWidth, bufferHeight, bufferStride, devicePixelRatio,
        defaultFontName, locale, timezoneID);
#endif
    instance->m_delegate =
        LWEDelegateRef(static_cast<void*>(delegate), [](void* ptr) {
            // Do nothing, use Destroy to release delegate.
        });
    return instance;
}
#else
WebContainer* WebContainer::Create(unsigned width, unsigned height,
                                   float devicePixelRatio,
                                   const char* defaultFontName,
                                   const char* locale, const char* timezoneID)
{
    STARFISH_API_RECORD_HEADER(width, height, devicePixelRatio, defaultFontName,
                               locale, timezoneID);
    WebContainer* instance = new WebContainer();
#ifdef STARFISH_API_ENABLE_LOADER
    auto delegate = reinterpret_cast<LWEDelegate::WebContainer*>(
        LWEDelegateLoader::getSafeInstance()->kWebContainerProcTable.Create(
            width, height, devicePixelRatio, defaultFontName, locale,
            timezoneID));
#else
    auto delegate = LWEDelegate::WebContainer::Create(
        width, height, devicePixelRatio, defaultFontName, locale, timezoneID);
#endif
    instance->m_delegate =
        LWEDelegateRef(static_cast<void*>(delegate), [](void* ptr) {
            // Do nothing, use Destroy to release delegate.
        });
    return instance;
}
#endif

WebContainer* WebContainer::CreateWithPlatformImage(
    const WebContainerArguments& args,
    const std::function<ExternalImageInfo(void)>& prepareImageCb,
    const std::function<void(WebContainer*, bool needsFlush)>& flushCb)
{
    STARFISH_API_RECORD_HEADER(args.width, args.height, args.devicePixelRatio,
                               args.defaultFontName, args.locale,
                               args.timezoneID);
    LWEDelegate::WebContainer::WebContainerArguments arguments{
        args.width,           args.height, args.devicePixelRatio,
        args.defaultFontName, args.locale, args.timezoneID
    };
    WebContainer* instance = new WebContainer();
    const LWEDelegate::WebContainer::OnPrepareImage prepareImageCbWrapper =
        [prepareImageCb](void) -> LWEDelegate::WebContainer::ExternalImageInfo {
        return { prepareImageCb().imageAddress };
    };
    const LWEDelegate::WebContainer::OnFlush flushCbWrapper =
        [instance, flushCb](LWEDelegate::WebContainer* container,
                            bool needsFlush) {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            flushCb(instance, needsFlush);
        };

#ifdef STARFISH_API_ENABLE_LOADER
    auto delegate = reinterpret_cast<LWEDelegate::WebContainer*>(
        LWEDelegateLoader::getSafeInstance()
            ->kWebContainerProcTable.CreateWithPlatformImage(
                reinterpret_cast<uintptr_t>(&arguments),
                reinterpret_cast<uintptr_t>(&prepareImageCbWrapper),
                reinterpret_cast<uintptr_t>(&flushCbWrapper)));
#else
    auto delegate = LWEDelegate::WebContainer::CreateWithPlatformImage(
        arguments, prepareImageCbWrapper, flushCbWrapper);
#endif
    instance->m_delegate =
        LWEDelegateRef(static_cast<void*>(delegate), [](void* ptr) {
            // Do nothing, use Destroy to release delegate.
        });
    return instance;
}

WebContainer* WebContainer::CreateGL(const WebContainerArguments& args,
                                     const RendererGLConfiguration& config)
{
    STARFISH_API_RECORD_HEADER(args.width, args.height, args.devicePixelRatio,
                               args.defaultFontName, args.locale,
                               args.timezoneID);
    WebContainer* instance = new WebContainer();
    LWEDelegate::WebContainer::WebContainerArguments arguments{
        args.width,           args.height, args.devicePixelRatio,
        args.defaultFontName, args.locale, args.timezoneID
    };

    const LWEDelegate::WebContainer::OnMakeCurrent onMakeCurrentWrapper =
        [instance, config](LWEDelegate::WebContainer* container) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                       instance->m_delegate.get()) == container);
        config.onMakeCurrent(instance);
    };
    const LWEDelegate::WebContainer::OnSwapBuffers onSwapBuffersWrapper =
        [instance, config](LWEDelegate::WebContainer* container,
                           bool mayNeedsSync) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                       instance->m_delegate.get()) == container);
        config.onSwapBuffers(instance, mayNeedsSync);
    };
    LWEDelegate::WebContainer::RendererGLConfiguration configration;
    configration.onMakeCurrent = onMakeCurrentWrapper;
    configration.onSwapBuffers = onSwapBuffersWrapper;

    // Optional callbacks for webgl.
    if (config.onCreateSharedContext) {
        const LWEDelegate::WebContainer::OnCreateSharedContext
            onCreateSharedContextWrapper =
                [instance,
                 config](LWEDelegate::WebContainer* container) -> uintptr_t {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            return config.onCreateSharedContext(instance);
        };
        configration.onCreateSharedContext = onCreateSharedContextWrapper;
    }
    if (config.onDestroyContext) {
        const LWEDelegate::WebContainer::OnDestroyContext
            onDestroyContextWrapper =
                [instance, config](LWEDelegate::WebContainer* container,
                                   uintptr_t context) -> bool {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            return config.onDestroyContext(instance, context);
        };
        configration.onDestroyContext = onDestroyContextWrapper;
    }
    if (config.onClearCurrentContext) {
        const LWEDelegate::WebContainer::OnClearCurrentContext
            onClearCurrentContextWrapper =
                [instance,
                 config](LWEDelegate::WebContainer* container) -> bool {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            return config.onClearCurrentContext(instance);
        };
        configration.onClearCurrentContext = onClearCurrentContextWrapper;
    }
    if (config.onMakeCurrentWithContext) {
        const LWEDelegate::WebContainer::OnMakeCurrentWithContext
            onMakeCurrentWithContextWrapper =
                [instance, config](LWEDelegate::WebContainer* container,
                                   uintptr_t context) -> bool {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            return config.onMakeCurrentWithContext(instance, context);
        };
        configration.onMakeCurrentWithContext = onMakeCurrentWithContextWrapper;
    }
    if (config.onGetProcAddress) {
        const LWEDelegate::WebContainer::OnGetProcAddress
            onGetProcAddressWrapper =
                [instance, config](LWEDelegate::WebContainer* container,
                                   const char* name) -> void* {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            return config.onGetProcAddress(instance, name);
        };
        configration.onGetProcAddress = onGetProcAddressWrapper;
    }
    if (config.onIsSupportedExtension) {
        const LWEDelegate::WebContainer::OnIsSupportedExtension
            onIsSupportedExtensionWrapper =
                [instance, config](LWEDelegate::WebContainer* container,
                                   const char* extension) -> bool {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            return config.onIsSupportedExtension(instance, extension);
        };
        configration.onIsSupportedExtension = onIsSupportedExtensionWrapper;
    }

#ifdef STARFISH_API_ENABLE_LOADER
    auto delegate = reinterpret_cast<LWEDelegate::WebContainer*>(
        LWEDelegateLoader::getSafeInstance()->kWebContainerProcTable.CreateGL(
            reinterpret_cast<uintptr_t>(&arguments),
            reinterpret_cast<uintptr_t>(&configration)));
#else
    auto delegate =
        LWEDelegate::WebContainer::CreateGL(arguments, configration);
#endif
    instance->m_delegate =
        LWEDelegateRef(static_cast<void*>(delegate), [](void* ptr) {
            // Do nothing, use Destroy to release delegate.
        });
    return instance;
}

WebContainer* WebContainer::CreateWebContainer(void* delegate)
{
    WebContainer* instance = new WebContainer();
    instance->m_delegate = LWEDelegateRef(delegate, [](void* ptr) {});
    return instance;
}

WebContainer* WebContainer::CreateGLWithPlatformImage(
    const WebContainerArguments& args, const RendererGLConfiguration& config,
    const std::function<ExternalImageInfo(void)>& prepareImageCb,
    const std::function<void(WebContainer*, bool needsFlush)>& flushCb)
{
    STARFISH_API_RECORD_HEADER(args.width, args.height, args.devicePixelRatio,
                               args.defaultFontName, args.locale,
                               args.timezoneID);
    WebContainer* instance = new WebContainer();

    LWEDelegate::WebContainer::WebContainerArguments arguments{
        args.width,           args.height, args.devicePixelRatio,
        args.defaultFontName, args.locale, args.timezoneID
    };

    const LWEDelegate::WebContainer::OnMakeCurrent onMakeCurrentWrapper =
        [instance, config](LWEDelegate::WebContainer* container) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                       instance->m_delegate.get()) == container);
        config.onMakeCurrent(instance);
    };
    const LWEDelegate::WebContainer::OnSwapBuffers onSwapBuffersWrapper =
        [instance, config](LWEDelegate::WebContainer* container,
                           bool mayNeedsSync) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                       instance->m_delegate.get()) == container);
        config.onSwapBuffers(instance, mayNeedsSync);
    };
    LWEDelegate::WebContainer::RendererGLConfiguration configration;
    configration.onMakeCurrent = onMakeCurrentWrapper;
    configration.onSwapBuffers = onSwapBuffersWrapper;

    // Optional callbacks for webgl.
    if (config.onCreateSharedContext) {
        const LWEDelegate::WebContainer::OnCreateSharedContext
            onCreateSharedContextWrapper =
                [instance,
                 config](LWEDelegate::WebContainer* container) -> uintptr_t {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            return config.onCreateSharedContext(instance);
        };
        configration.onCreateSharedContext = onCreateSharedContextWrapper;
    }
    if (config.onDestroyContext) {
        const LWEDelegate::WebContainer::OnDestroyContext
            onDestroyContextWrapper =
                [instance, config](LWEDelegate::WebContainer* container,
                                   uintptr_t context) -> bool {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            return config.onDestroyContext(instance, context);
        };
        configration.onDestroyContext = onDestroyContextWrapper;
    }
    if (config.onClearCurrentContext) {
        const LWEDelegate::WebContainer::OnClearCurrentContext
            onClearCurrentContextWrapper =
                [instance,
                 config](LWEDelegate::WebContainer* container) -> bool {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            return config.onClearCurrentContext(instance);
        };
        configration.onClearCurrentContext = onClearCurrentContextWrapper;
    }
    if (config.onMakeCurrentWithContext) {
        const LWEDelegate::WebContainer::OnMakeCurrentWithContext
            onMakeCurrentWithContextWrapper =
                [instance, config](LWEDelegate::WebContainer* container,
                                   uintptr_t context) -> bool {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            return config.onMakeCurrentWithContext(instance, context);
        };
        configration.onMakeCurrentWithContext = onMakeCurrentWithContextWrapper;
    }
    if (config.onGetProcAddress) {
        const LWEDelegate::WebContainer::OnGetProcAddress
            onGetProcAddressWrapper =
                [instance, config](LWEDelegate::WebContainer* container,
                                   const char* name) -> void* {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            return config.onGetProcAddress(instance, name);
        };
        configration.onGetProcAddress = onGetProcAddressWrapper;
    }
    if (config.onIsSupportedExtension) {
        const LWEDelegate::WebContainer::OnIsSupportedExtension
            onIsSupportedExtensionWrapper =
                [instance, config](LWEDelegate::WebContainer* container,
                                   const char* extension) -> bool {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            return config.onIsSupportedExtension(instance, extension);
        };
        configration.onIsSupportedExtension = onIsSupportedExtensionWrapper;
    }

    const LWEDelegate::WebContainer::OnPrepareImage prepareImageCbWrapper =
        [prepareImageCb](void) -> LWEDelegate::WebContainer::ExternalImageInfo {
        return { prepareImageCb().imageAddress };
    };

    const LWEDelegate::WebContainer::OnFlush flushCbWrapper =
        [instance, flushCb](LWEDelegate::WebContainer* container,
                            bool needsFlush) {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(
                           instance->m_delegate.get()) == container);
            flushCb(instance, needsFlush);
        };
#ifdef STARFISH_API_ENABLE_LOADER
    auto delegate = reinterpret_cast<LWEDelegate::WebContainer*>(
        LWEDelegateLoader::getSafeInstance()
            ->kWebContainerProcTable.CreateGLWithPlatformImage(
                reinterpret_cast<uintptr_t>(&arguments),
                reinterpret_cast<uintptr_t>(&configration),
                reinterpret_cast<uintptr_t>(&prepareImageCbWrapper),
                reinterpret_cast<uintptr_t>(&flushCbWrapper)));
#else
    auto delegate = LWEDelegate::WebContainer::CreateGLWithPlatformImage(
        arguments, configration, prepareImageCbWrapper, flushCbWrapper);
#endif

    instance->m_delegate =
        LWEDelegateRef(static_cast<void*>(delegate), [](void* ptr) {
            // Do nothing, use Destroy to release delegate.
        });
    return instance;
}

WebContainer* WebContainer::CreateHeadless(unsigned width, unsigned height,
                                           float devicePixelRatio,
                                           const char* defaultFontName,
                                           const char* locale,
                                           const char* timezoneID)
{
    STARFISH_API_RECORD_HEADER(width, height, devicePixelRatio, defaultFontName,
                               locale, timezoneID);
    WebContainer* instance = new WebContainer();
#ifdef STARFISH_API_ENABLE_LOADER
    auto delegate = reinterpret_cast<LWEDelegate::WebContainer*>(
        LWEDelegateLoader::getSafeInstance()
            ->kWebContainerProcTable.CreateHeadless(
                width, height, devicePixelRatio, defaultFontName, locale,
                timezoneID));
#else
    auto delegate = LWEDelegate::WebContainer::CreateHeadless(
        width, height, devicePixelRatio, defaultFontName, locale, timezoneID);
#endif
    instance->m_delegate =
        LWEDelegateRef(static_cast<void*>(delegate), [](void* ptr) {
            // Do nothing, use Destroy to release delegate.
        });
    return instance;
}

WebContainer::WebContainer()
{
}

WebContainer::~WebContainer()
{
}

#if defined(STARFISH_TIZEN_VERSION_5_0)
void WebContainer::UpdateBuffer(void* buffer, unsigned width, unsigned height,
                                unsigned stride)
{
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->UpdateBuffer(buffer, width, height, stride);
}
#endif

#if !defined(STARFISH_TIZEN_VERSION_5_0)
void WebContainer::RegisterPreRenderingHandler(
    const std::function<RenderInfo(void)>& cb)
{
    const auto wrapper = [cb]() -> LWEDelegate::WebContainer::RenderInfo {
        auto result = cb();
        return { result.updatedBufferAddress, result.bufferStride };
    };

    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterPreRenderingHandler(wrapper);
}
#endif

void WebContainer::RegisterOnRenderedHandler(
    const std::function<void(WebContainer*, const RenderResult& renderResult)>&
        cb)
{
    const auto wrapper =
        [this,
         cb](LWEDelegate::WebContainer* container,
             const LWEDelegate::WebContainer::RenderResult& renderResult) {
            LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                       container);
            cb(this, { renderResult.updatedX, renderResult.updatedY,
                       renderResult.updatedWidth, renderResult.updatedHeight,
                       renderResult.updatedBufferAddress,
                       renderResult.bufferImageWidth,
                       renderResult.bufferImageHeight });
        };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterOnRenderedHandler(wrapper);
}

void WebContainer::AddIdleCallback(void (*callback)(void*), void* data)
{
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->AddIdleCallback(callback, data);
}

size_t WebContainer::AddTimeout(void (*callback)(void*), void* data,
                                size_t timeoutInMS)
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->AddTimeout(callback, data, timeoutInMS);
}

void WebContainer::ClearTimeout(size_t handle)
{
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->ClearTimeout(handle);
}

void WebContainer::RegisterCanRenderingHandler(
    const std::function<bool(WebContainer*)>& cb)
{
    const auto wrapper = [this,
                          cb](LWEDelegate::WebContainer* container) -> bool {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        return cb(this);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterCanRenderingHandler(wrapper);
}

Settings WebContainer::GetSettings()
{
    LWEDelegate::Settings* delegate =
        toImpl<LWEDelegate::WebContainer>(m_delegate.get())->GetSettings();

    Settings settings;
    delegate->IterateSettings(
        [&settings](const std::string& key, const std::string& value) {
            settings.UpdateSetting(key, value);
        });
    delete delegate;

    return settings;
}

void WebContainer::LoadURL(const std::string& url)
{
    STARFISH_API_RECORD_EVENT_STR("LoadURL", "url", url);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->LoadURL(url);
}

std::string WebContainer::GetURL()
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())->GetURL();
}

void WebContainer::LoadData(const std::string& data)
{
    STARFISH_API_RECORD_EVENT_STR("LoadData", "data", data);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->LoadData(data);
}

void WebContainer::Reload()
{
    STARFISH_API_RECORD_EVENT("Reload", "{}");
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->Reload();
}

void WebContainer::StopLoading()
{
    STARFISH_API_RECORD_EVENT("StopLoading", "{}");
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->StopLoading();
}

void WebContainer::GoBack()
{
    STARFISH_API_RECORD_EVENT("GoBack", "{}");
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->GoBack();
}

void WebContainer::GoForward()
{
    STARFISH_API_RECORD_EVENT("GoForward", "{}");
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->GoForward();
}

bool WebContainer::CanGoBack()
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())->CanGoBack();
}

bool WebContainer::CanGoForward()
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())->CanGoForward();
}

void WebContainer::AddJavaScriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb)
{
    // The native callback cannot be serialized; only the registered names are
    // recorded. Replay registers an echo stub so the JS object/function still
    // exists (return values will not match the original handler).
    STARFISH_API_RECORD_EVENT_STR2("AddJavaScriptInterface", "object",
                                   exposedObjectName, "function",
                                   jsFunctionName);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->AddJavaScriptInterface(exposedObjectName, jsFunctionName, cb);
}

std::string WebContainer::EvaluateJavaScript(const std::string& script)
{
    STARFISH_API_RECORD_EVENT_STR("EvaluateJavaScript", "script", script);
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->EvaluateJavaScript(script);
}

void WebContainer::EvaluateJavaScript(
    const std::string& script, std::function<void(const std::string&)> cb)
{
    STARFISH_API_RECORD_EVENT_STR("EvaluateJavaScript", "script", script);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->EvaluateJavaScript(script, cb);
}

void WebContainer::ClearHistory()
{
    STARFISH_API_RECORD_EVENT("ClearHistory", "{}");
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->ClearHistory();
}

void WebContainer::Destroy()
{
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->Destroy();
    m_delegate = nullptr;
    delete this;
}

void WebContainer::Pause()
{
    STARFISH_API_RECORD_EVENT("Pause", "{}");
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->Pause();
}

void WebContainer::Resume()
{
    STARFISH_API_RECORD_EVENT("Resume", "{}");
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->Resume();
}

void WebContainer::ResizeTo(size_t width, size_t height)
{
    STARFISH_API_RECORD_EVENT_FMT("ResizeTo", "{\"w\":%zu,\"h\":%zu}", width,
                                  height);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->ResizeTo(width, height);
}

void WebContainer::Focus()
{
    STARFISH_API_RECORD_EVENT("Focus", "{}");
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->Focus();
}

void WebContainer::Blur()
{
    STARFISH_API_RECORD_EVENT("Blur", "{}");
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->Blur();
}

void WebContainer::SetSettings(const Settings& settings)
{
#if defined(STARFISH_ENABLE_TEST)
    if (LWERecord::APIRecorder::instance().isRecording()) {
        // Serialize all key/value settings into a single JSON object.
        std::string args = "{";
        bool first = true;
        settings.IterateSettings(
            [&args, &first](const std::string& key, const std::string& value) {
                if (!first)
                    args += ",";
                first = false;
                args += "\"" + LWERecord::escapeJsonString(key) + "\":\"" +
                        LWERecord::escapeJsonString(value) + "\"";
            });
        args += "}";
        LWERecord::APIRecorder::instance().recordEvent("SetSettings",
                                                       args.c_str());
    }
#endif
#ifdef STARFISH_API_ENABLE_LOADER
    LWEDelegate::Settings* delegate = reinterpret_cast<LWEDelegate::Settings*>(
        LWEDelegateLoader::getSafeInstance()->kSettingsProcTable.CreateEmpty());
#else
    LWEDelegate::Settings* delegate = LWEDelegate::Settings::Create();
#endif
    settings.IterateSettings(
        [delegate](const std::string& key, const std::string& value) {
            delegate->UpdateSetting(key, value);
        });
    // Delete delegate in LWEDelegate::WebContainer::SetSettings;
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->SetSettings(delegate);
}

void WebContainer::RemoveJavascriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName)
{
    STARFISH_API_RECORD_EVENT_STR2("RemoveJavascriptInterface", "object",
                                   exposedObjectName, "function",
                                   jsFunctionName);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RemoveJavascriptInterface(exposedObjectName, jsFunctionName);
}

void WebContainer::ClearCache()
{
    STARFISH_API_RECORD_EVENT("ClearCache", "{}");
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->ClearCache();
}

void WebContainer::RegisterOnReceivedErrorHandler(
    const std::function<void(WebContainer*, ResourceError)>& cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebContainer* container,
                                    LWEDelegate::ResourceError* error) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this,
           { error->GetErrorCode(), error->GetDescription(), error->GetUrl() });
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterOnReceivedErrorHandler(wrapper);
}

void WebContainer::RegisterOnPageParsedHandler(
    std::function<void(WebContainer*, const std::string& data)> cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebContainer* container,
                                    const std::string& url) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this, url);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterOnPageParsedHandler(wrapper);
}

void WebContainer::RegisterOnPageLoadedHandler(
    std::function<void(WebContainer*, const std::string&)> cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebContainer* container,
                                    const std::string& url) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this, url);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterOnPageLoadedHandler(wrapper);
}

void WebContainer::RegisterOnPageStartedHandler(
    const std::function<void(WebContainer*, const std::string&)>& cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebContainer* container,
                                    const std::string& url) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this, url);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterOnPageStartedHandler(wrapper);
}

void WebContainer::RegisterOnLoadResourceHandler(
    const std::function<void(WebContainer*, const std::string&)>& cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebContainer* container,
                                    const std::string& url) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this, url);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterOnLoadResourceHandler(wrapper);
}

void WebContainer::RegisterShouldOverrideUrlLoadingHandler(
    const std::function<bool(WebContainer*, const std::string&)>& cb)
{
    auto wrapper = [this, cb](LWEDelegate::WebContainer* container,
                              const std::string& url) -> bool {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        return cb(this, url);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterShouldOverrideUrlLoadingHandler(wrapper);
}

void WebContainer::RegisterOnProgressChangedHandler(
    const std::function<void(WebContainer*, int progress)>& cb)
{
    auto wrapper = [this, cb](LWEDelegate::WebContainer* container,
                              int progress) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this, progress);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterOnProgressChangedHandler(wrapper);
}

void WebContainer::RegisterOnDownloadStartHandler(
    const std::function<void(WebContainer*, const std::string&,
                             const std::string&, const std::string&,
                             const std::string&, long)>& cb)
{
    const auto wrapper =
        [this, cb](LWEDelegate::WebContainer* container, const std::string& url,
                   const std::string& userAgent,
                   const std::string& contentDisposition,
                   const std::string& mimetype, long contentLength) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this, url, userAgent, contentDisposition, mimetype, contentLength);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterOnDownloadStartHandler(wrapper);
}

void WebContainer::RegisterShowDropdownMenuHandler(
    const std::function<void(WebContainer*, const std::vector<std::string>*,
                             int)>& cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebContainer* container,
                                    const std::vector<std::string>* list,
                                    int checkedPosition) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this, list, checkedPosition);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterShowDropdownMenuHandler(wrapper);
}

void WebContainer::RegisterShowAlertHandler(
    const std::function<void(WebContainer*, const std::string&,
                             const std::string&)>& cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebContainer* container,
                                    const std::string& title,
                                    const std::string& message) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this, title, message);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterShowAlertHandler(wrapper);
}

void WebContainer::RegisterCustomFileResourceRequestHandlers(
    std::function<const char*(const char* path)> resolveFilePathCallback,
    std::function<void*(const char* path)> fileOpenCallback,
    std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
        fileReadCallback,
    std::function<long int(void* handle)> fileLengthCallback,
    std::function<void(void* handle)> fileCloseCallback)
{
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterCustomFileResourceRequestHandlers(
            resolveFilePathCallback, fileOpenCallback, fileReadCallback,
            fileLengthCallback, fileCloseCallback);
}

void WebContainer::RegisterDebuggerShouldInitHandler(
    const std::function<void(const std::string& url, int port,
                             bool& shouldInit)>& cb)
{
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterDebuggerShouldInitHandler(cb);
}

void WebContainer::RegisterDebuggerShouldContinueWaitingHandler(
    const std::function<void(const std::string& url, int port,
                             bool& shouldWait)>& cb)
{
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterDebuggerShouldContinueWaitingHandler(cb);
}

void WebContainer::RegisterOnIdleHandler(
    const std::function<void(WebContainer*)>& cb)
{
    const auto wrapper = [this,
                          cb](LWEDelegate::WebContainer* container) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this);
    };

    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterOnIdleHandler(wrapper);
}

void WebContainer::CallHandler(const std::string& handler, void* param)
{
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->CallHandler(handler, param);
}

void WebContainer::SetUserAgentString(const std::string& userAgent)
{
    STARFISH_API_RECORD_EVENT_STR("SetUserAgentString", "ua", userAgent);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->SetUserAgentString(userAgent);
}

std::string WebContainer::GetUserAgentString()
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->GetUserAgentString();
}

void WebContainer::SetCacheMode(int mode)
{
    STARFISH_API_RECORD_EVENT_FMT("SetCacheMode", "{\"mode\":%d}", mode);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->SetCacheMode(mode);
}

int WebContainer::GetCacheMode()
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())->GetCacheMode();
}

void WebContainer::SetDefaultFontSize(uint32_t size)
{
    STARFISH_API_RECORD_EVENT_FMT("SetDefaultFontSize", "{\"size\":%u}", size);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->SetDefaultFontSize(size);
}

uint32_t WebContainer::GetDefaultFontSize()
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->GetDefaultFontSize();
}

void WebContainer::DispatchMouseMoveEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    STARFISH_API_RECORD_EVENT_FMT(
        "DispatchMouseMoveEvent",
        "{\"button\":%d,\"buttons\":%d,\"x\":%.1f,\"y\":%.1f}", (int)button,
        (int)buttons, x, y);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchMouseMoveEvent(button, buttons, x, y);
}

void WebContainer::DispatchMouseDownEvent(MouseButtonValue button,
                                          MouseButtonsValue buttons, double x,
                                          double y)
{
    STARFISH_API_RECORD_EVENT_FMT(
        "DispatchMouseDownEvent",
        "{\"button\":%d,\"buttons\":%d,\"x\":%.1f,\"y\":%.1f}", (int)button,
        (int)buttons, x, y);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchMouseDownEvent(button, buttons, x, y);
}

void WebContainer::DispatchMouseUpEvent(MouseButtonValue button,
                                        MouseButtonsValue buttons, double x,
                                        double y)
{
    STARFISH_API_RECORD_EVENT_FMT(
        "DispatchMouseUpEvent",
        "{\"button\":%d,\"buttons\":%d,\"x\":%.1f,\"y\":%.1f}", (int)button,
        (int)buttons, x, y);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchMouseUpEvent(button, buttons, x, y);
}

void WebContainer::DispatchMouseWheelEvent(double x, double y, int delta)
{
    STARFISH_API_RECORD_EVENT_FMT("DispatchMouseWheelEvent",
                                  "{\"x\":%.1f,\"y\":%.1f,\"delta\":%d}", x, y,
                                  delta);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchMouseWheelEvent(x, y, delta);
}

void WebContainer::DispatchTouchStartEvent(const float* points, const int* ids,
                                           size_t pointCount)
{
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchTouchStartEvent(points, ids, pointCount);
}

void WebContainer::DispatchTouchMoveEvent(const float* points, const int* ids,
                                          size_t pointCount)
{
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchTouchMoveEvent(points, ids, pointCount);
}

void WebContainer::DispatchTouchEndEvent(const float* points, const int* ids,
                                         size_t pointCount)
{
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchTouchEndEvent(points, ids, pointCount);
}

void WebContainer::DispatchKeyDownEvent(KeyValue keyCode)
{
    STARFISH_API_RECORD_EVENT_FMT("DispatchKeyDownEvent", "{\"key\":%d}",
                                  (int)keyCode);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchKeyDownEvent(keyCode);
}

void WebContainer::DispatchKeyPressEvent(KeyValue keyCode)
{
    STARFISH_API_RECORD_EVENT_FMT("DispatchKeyPressEvent", "{\"key\":%d}",
                                  (int)keyCode);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchKeyPressEvent(keyCode);
}

void WebContainer::DispatchKeyUpEvent(KeyValue keyCode)
{
    STARFISH_API_RECORD_EVENT_FMT("DispatchKeyUpEvent", "{\"key\":%d}",
                                  (int)keyCode);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchKeyUpEvent(keyCode);
}

void WebContainer::DispatchCompositionStartEvent(
    const std::string& currentCompositionString)
{
    STARFISH_API_RECORD_EVENT_STR("DispatchCompositionStartEvent", "text",
                                  currentCompositionString);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchCompositionStartEvent(currentCompositionString);
}

void WebContainer::DispatchCompositionUpdateEvent(
    const std::string& currentCompositionString)
{
    STARFISH_API_RECORD_EVENT_STR("DispatchCompositionUpdateEvent", "text",
                                  currentCompositionString);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchCompositionUpdateEvent(currentCompositionString);
}

void WebContainer::DispatchCompositionEndEvent(
    const std::string& currentCompositionString)
{
    STARFISH_API_RECORD_EVENT_STR("DispatchCompositionEndEvent", "text",
                                  currentCompositionString);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->DispatchCompositionEndEvent(currentCompositionString);
}

void WebContainer::RegisterOnShowSoftwareKeyboardIfPossibleHandler(
    const std::function<void(WebContainer*)>& cb)
{
    const auto wrapper = [this,
                          cb](LWEDelegate::WebContainer* container) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterOnShowSoftwareKeyboardIfPossibleHandler(wrapper);
}

void WebContainer::RegisterOnHideSoftwareKeyboardIfPossibleHandler(
    const std::function<void(WebContainer*)>& cb)
{
    const auto wrapper = [this,
                          cb](LWEDelegate::WebContainer* container) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterOnHideSoftwareKeyboardIfPossibleHandler(wrapper);
}

void WebContainer::SetUserData(const std::string& key, void* data)
{
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->SetUserData(key, data);
}

void* WebContainer::GetUserData(const std::string& key)
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->GetUserData(key);
}

std::string WebContainer::GetTitle()
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())->GetTitle();
}

void WebContainer::ScrollTo(int x, int y)
{
    STARFISH_API_RECORD_EVENT_FMT("ScrollTo", "{\"x\":%d,\"y\":%d}", x, y);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->ScrollTo(x, y);
}

void WebContainer::ScrollBy(int x, int y)
{
    STARFISH_API_RECORD_EVENT_FMT("ScrollBy", "{\"x\":%d,\"y\":%d}", x, y);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())->ScrollBy(x, y);
}

int WebContainer::GetScrollX()
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())->GetScrollX();
}

int WebContainer::GetScrollY()
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())->GetScrollY();
}

size_t WebContainer::Width()
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())->Width();
}

size_t WebContainer::Height()
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())->Height();
}

void WebContainer::RegisterSetNeedsRenderingCallback(
    const std::function<void(
        WebContainer*, const std::function<void()>& doRenderingFunction)>& cb)
{
    const auto wrapper =
        [this, cb](LWEDelegate::WebContainer* container,
                   const std::function<void()>& doRenderingFunction) -> void {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        cb(this, doRenderingFunction);
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterSetNeedsRenderingCallback(wrapper);
}

void WebContainer::SetDevicePixelRatio(float dpr)
{
    STARFISH_API_RECORD_EVENT_FMT("SetDevicePixelRatio", "{\"dpr\":%.3f}", dpr);
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->SetDevicePixelRatio(dpr);
}

float WebContainer::GetDevicePixelRatio()
{
    return toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->GetDevicePixelRatio();
}

void WebContainer::RegisterGetScreenMatrixHandler(
    const std::function<TransformationMatrix(WebContainer*)>& cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebContainer* container)
        -> LWEDelegate::WebContainer::TransformationMatrix {
        LWE_ASSERT(toImpl<LWEDelegate::WebContainer>(m_delegate.get()) ==
                   container);
        TransformationMatrix m = cb(this);
        return {
            m.scaleX,       m.skewX,        m.translateX,
            m.skewY,        m.scaleY,       m.translateY,
            m.perspectiveX, m.perspectiveY, m.perspectiveScale,
        };
    };
    toImpl<LWEDelegate::WebContainer>(m_delegate.get())
        ->RegisterGetScreenMatrixHandler(wrapper);
}

WebView* WebView::Create(void* win, unsigned x, unsigned y, unsigned width,
                         unsigned height, float devicePixelRatio,
                         const char* defaultFontName, const char* locale,
                         const char* timezoneID)
{
    STARFISH_API_RECORD_HEADER(width, height, devicePixelRatio, defaultFontName,
                               locale, timezoneID);
#ifdef STARFISH_API_ENABLE_LOADER
    auto delegate = reinterpret_cast<LWEDelegate::WebView*>(
        LWEDelegateLoader::getSafeInstance()->kWebViewProcTable.Create(
            win, x, y, width, height, devicePixelRatio, defaultFontName, locale,
            timezoneID));
#else
    auto delegate =
        LWEDelegate::WebView::Create(win, x, y, width, height, devicePixelRatio,
                                     defaultFontName, locale, timezoneID);
#endif
    WebView* instance = new WebView();
    instance->m_delegate =
        LWEDelegateRef(static_cast<void*>(delegate), [](void* ptr) {
            // Do nothing, use Destroy to release delegate.
        });
    return instance;
}

WebView::WebView()
{
}

WebView::~WebView()
{
}

Settings WebView::GetSettings()
{
    LWEDelegate::Settings* delegate =
        toImpl<LWEDelegate::WebView>(m_delegate.get())->GetSettings();

    Settings settings;
    delegate->IterateSettings(
        [&settings](const std::string& key, const std::string& value) {
            settings.UpdateSetting(key, value);
        });
    delete delegate;

    return settings;
}

void WebView::LoadURL(const std::string& url)
{
    STARFISH_API_RECORD_EVENT_STR("LoadURL", "url", url);
    toImpl<LWEDelegate::WebView>(m_delegate.get())->LoadURL(url);
}

std::string WebView::GetURL()
{
    return toImpl<LWEDelegate::WebView>(m_delegate.get())->GetURL();
}

void WebView::LoadData(const std::string& data)
{
    STARFISH_API_RECORD_EVENT_STR("LoadData", "data", data);
    toImpl<LWEDelegate::WebView>(m_delegate.get())->LoadData(data);
}

void WebView::Reload()
{
    STARFISH_API_RECORD_EVENT("Reload", "{}");
    toImpl<LWEDelegate::WebView>(m_delegate.get())->Reload();
}

void WebView::StopLoading()
{
    STARFISH_API_RECORD_EVENT("StopLoading", "{}");
    toImpl<LWEDelegate::WebView>(m_delegate.get())->StopLoading();
}

void WebView::GoBack()
{
    STARFISH_API_RECORD_EVENT("GoBack", "{}");
    toImpl<LWEDelegate::WebView>(m_delegate.get())->GoBack();
}

void WebView::GoForward()
{
    STARFISH_API_RECORD_EVENT("GoForward", "{}");
    toImpl<LWEDelegate::WebView>(m_delegate.get())->GoForward();
}

bool WebView::CanGoBack()
{
    return toImpl<LWEDelegate::WebView>(m_delegate.get())->CanGoBack();
}

bool WebView::CanGoForward()
{
    return toImpl<LWEDelegate::WebView>(m_delegate.get())->CanGoForward();
}

void WebView::AddJavaScriptInterface(
    const std::string& exposedObjectName, const std::string& jsFunctionName,
    std::function<std::string(const std::string&)> cb)
{
    STARFISH_API_RECORD_EVENT_STR2("AddJavaScriptInterface", "object",
                                   exposedObjectName, "function",
                                   jsFunctionName);
    toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->AddJavaScriptInterface(exposedObjectName, jsFunctionName, cb);
}

std::string WebView::EvaluateJavaScript(const std::string& script)
{
    STARFISH_API_RECORD_EVENT_STR("EvaluateJavaScript", "script", script);
    return toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->EvaluateJavaScript(script);
}

void WebView::EvaluateJavaScript(const std::string& script,
                                 std::function<void(const std::string&)> cb)
{
    STARFISH_API_RECORD_EVENT_STR("EvaluateJavaScript", "script", script);
    return toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->EvaluateJavaScript(script, cb);
}

void WebView::ClearHistory()
{
    STARFISH_API_RECORD_EVENT("ClearHistory", "{}");
    toImpl<LWEDelegate::WebView>(m_delegate.get())->ClearHistory();
}

void WebView::Destroy()
{
    toImpl<LWEDelegate::WebView>(m_delegate.get())->Destroy();
    m_delegate = nullptr;
    delete this;
}

void WebView::SetSettings(const Settings& settings)
{
#if defined(STARFISH_ENABLE_TEST)
    if (LWERecord::APIRecorder::instance().isRecording()) {
        std::string args = "{";
        bool first = true;
        settings.IterateSettings(
            [&args, &first](const std::string& key, const std::string& value) {
                if (!first)
                    args += ",";
                first = false;
                args += "\"" + LWERecord::escapeJsonString(key) + "\":\"" +
                        LWERecord::escapeJsonString(value) + "\"";
            });
        args += "}";
        LWERecord::APIRecorder::instance().recordEvent("SetSettings",
                                                       args.c_str());
    }
#endif
#ifdef STARFISH_API_ENABLE_LOADER
    LWEDelegate::Settings* delegate = reinterpret_cast<LWEDelegate::Settings*>(
        LWEDelegateLoader::getSafeInstance()->kSettingsProcTable.CreateEmpty());
#else
    LWEDelegate::Settings* delegate = LWEDelegate::Settings::Create();
#endif
    settings.IterateSettings(
        [delegate](const std::string& key, const std::string& value) {
            delegate->UpdateSetting(key, value);
        });
    // Delete delegate in LWEDelegate::WebContainer::SetSettings;
    toImpl<LWEDelegate::WebView>(m_delegate.get())->SetSettings(delegate);
}

void WebView::RemoveJavascriptInterface(const std::string& exposedObjectName,
                                        const std::string& jsFunctionName)
{
    STARFISH_API_RECORD_EVENT_STR2("RemoveJavascriptInterface", "object",
                                   exposedObjectName, "function",
                                   jsFunctionName);
    toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->RemoveJavascriptInterface(exposedObjectName, jsFunctionName);
}

void WebView::ClearCache()
{
    STARFISH_API_RECORD_EVENT("ClearCache", "{}");
    toImpl<LWEDelegate::WebView>(m_delegate.get())->ClearCache();
}

void WebView::RegisterOnReceivedErrorHandler(
    std::function<void(WebView*, ResourceError)> cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebView*,
                                    LWEDelegate::ResourceError* error) -> void {
        cb(this,
           { error->GetErrorCode(), error->GetDescription(), error->GetUrl() });
    };
    toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->RegisterOnReceivedErrorHandler(wrapper);
}

void WebView::RegisterOnPageParsedHandler(
    std::function<void(WebView*, const std::string&)> cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebView*,
                                    const std::string& url) -> void {
        cb(this, url);
    };
    toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->RegisterOnPageParsedHandler(wrapper);
}

void WebView::RegisterOnPageLoadedHandler(
    std::function<void(WebView*, const std::string&)> cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebView*,
                                    const std::string& url) -> void {
        cb(this, url);
    };
    toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->RegisterOnPageLoadedHandler(wrapper);
}

void WebView::RegisterOnPageStartedHandler(
    std::function<void(WebView*, const std::string&)> cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebView*,
                                    const std::string& url) -> void {
        cb(this, url);
    };
    toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->RegisterOnPageStartedHandler(wrapper);
}

void WebView::RegisterOnLoadResourceHandler(
    std::function<void(WebView*, const std::string&)> cb)
{
    const auto wrapper = [this, cb](LWEDelegate::WebView*,
                                    const std::string& url) -> void {
        cb(this, url);
    };
    toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->RegisterOnLoadResourceHandler(wrapper);
}

void WebView::Pause()
{
    STARFISH_API_RECORD_EVENT("Pause", "{}");
    toImpl<LWEDelegate::WebView>(m_delegate.get())->Pause();
}

void WebView::Resume()
{
    STARFISH_API_RECORD_EVENT("Resume", "{}");
    toImpl<LWEDelegate::WebView>(m_delegate.get())->Resume();
}

void WebView::RegisterCustomFileResourceRequestHandlers(
    std::function<const char*(const char* path)> resolveFilePathCallback,
    std::function<void*(const char* path)> fileOpenCallback,
    std::function<size_t(uint8_t* destBuffer, size_t size, void* handle)>
        fileReadCallback,
    std::function<long int(void* handle)> fileLengthCallback,
    std::function<void(void* handle)> fileCloseCallback)
{
    toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->RegisterCustomFileResourceRequestHandlers(
            resolveFilePathCallback, fileOpenCallback, fileReadCallback,
            fileLengthCallback, fileCloseCallback);
}

void WebView::RegisterDebuggerShouldInitHandler(
    const std::function<void(const std::string& url, int port, bool& ret)>& cb)
{
    toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->RegisterDebuggerShouldInitHandler(cb);
}

void WebView::RegisterDebuggerShouldContinueWaitingHandler(
    const std::function<void(const std::string& url, int port, bool& ret)>& cb)
{
    toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->RegisterDebuggerShouldContinueWaitingHandler(cb);
}

void WebView::SetUserData(const std::string& key, void* data)
{
    toImpl<LWEDelegate::WebView>(m_delegate.get())->SetUserData(key, data);
}

void* WebView::GetUserData(const std::string& key)
{
    return toImpl<LWEDelegate::WebView>(m_delegate.get())->GetUserData(key);
}

std::string WebView::GetTitle()
{
    return toImpl<LWEDelegate::WebView>(m_delegate.get())->GetTitle();
}

void WebView::ScrollTo(int x, int y)
{
    STARFISH_API_RECORD_EVENT_FMT("ScrollTo", "{\"x\":%d,\"y\":%d}", x, y);
    toImpl<LWEDelegate::WebView>(m_delegate.get())->ScrollTo(x, y);
}

void WebView::ScrollBy(int x, int y)
{
    STARFISH_API_RECORD_EVENT_FMT("ScrollBy", "{\"x\":%d,\"y\":%d}", x, y);
    toImpl<LWEDelegate::WebView>(m_delegate.get())->ScrollBy(x, y);
}

int WebView::GetScrollX()
{
    return toImpl<LWEDelegate::WebView>(m_delegate.get())->GetScrollX();
}

int WebView::GetScrollY()
{
    return toImpl<LWEDelegate::WebView>(m_delegate.get())->GetScrollY();
}

void* WebView::Unwrap()
{
    return toImpl<LWEDelegate::WebView>(m_delegate.get())->Unwrap();
}

void WebView::Focus()
{
    STARFISH_API_RECORD_EVENT("Focus", "{}");
    toImpl<LWEDelegate::WebView>(m_delegate.get())->Focus();
}

void WebView::Blur()
{
    STARFISH_API_RECORD_EVENT("Blur", "{}");
    toImpl<LWEDelegate::WebView>(m_delegate.get())->Blur();
}

void WebView::SetDevicePixelRatio(float dpr)
{
    STARFISH_API_RECORD_EVENT_FMT("SetDevicePixelRatio", "{\"dpr\":%.3f}", dpr);
    toImpl<LWEDelegate::WebView>(m_delegate.get())->SetDevicePixelRatio(dpr);
}

float WebView::GetDevicePixelRatio()
{
    return toImpl<LWEDelegate::WebView>(m_delegate.get())
        ->GetDevicePixelRatio();
}

WebContainer* WebView::FetchWebContainer()
{
    return WebContainer::CreateWebContainer(
        toImpl<LWEDelegate::WebView>(m_delegate.get())->FetchWebContainer());
}

} // namespace LWE
