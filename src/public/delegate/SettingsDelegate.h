/*
 * Copyright (c) 2023-present Samsung Electronics Co., Ltd
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
#ifndef __SettingsDelegate__
#define __SettingsDelegate__

#include "LWEDelegateConfig.h"

#include "PlatformIntegrationData.h"
#include <string>
#include <unordered_map>
#include <functional>

namespace LWEDelegate {

class EXPORT_UNMANAGED_API Settings {
public:
    static Settings* Create();
    static Settings* Create(const std::string& defaultUA,
                            const std::string& ua);
    static Settings* Create(Settings* other);

    Settings() = default;
    virtual ~Settings() = default;

    virtual bool UpdateSetting(const std::string& key,
                               const std::string& value) = 0;
    virtual std::string GetSetting(std::string key) const = 0;

    virtual std::string GetDefaultUserAgent() const = 0;
    virtual std::string GetUserAgentString() const = 0;
    virtual std::string GetProxyURL() const = 0;
    virtual int GetCacheMode() const = 0;
    virtual ::LWE::TTSMode GetTTSMode() const = 0;
    virtual std::string GetTTSLanguage() const = 0;
    virtual ::LWE::WebSecurityMode GetWebSecurityMode() const = 0;
    virtual ::LWE::IdleModeJob GetIdleModeJob() const = 0;
    virtual uint32_t GetIdleModeCheckIntervalInMS() const = 0;
    virtual void GetBaseBackgroundColor(unsigned char& r, unsigned char& g,
                                        unsigned char& b,
                                        unsigned char& a) const = 0;
    virtual void GetBaseForegroundColor(unsigned char& r, unsigned char& g,
                                        unsigned char& b,
                                        unsigned char& a) const = 0;
    virtual bool NeedsDownloadWebFontsEarly() const = 0;
    virtual bool UseHttp2() const = 0;
    virtual uint32_t NeedsDownScaleImageResourceLargerThan() const = 0;
    virtual bool ScrollbarVisible() const = 0;
    virtual bool UseExternalPopup() const = 0;
    virtual bool UseSpatialNavigation() const = 0;

    virtual void SetUserAgentString(const std::string& ua) = 0;
    virtual void SetCacheMode(int mode) = 0;
    virtual void SetProxyURL(const std::string& proxyURL) = 0;
    virtual void setDefaultFontSize(int size) = 0;
    virtual void SetTTSMode(::LWE::TTSMode value) = 0;
    virtual void SetTTSLanguage(const std::string& language) = 0;
    virtual void SetBaseBackgroundColor(unsigned char r, unsigned char g,
                                        unsigned char b, unsigned char a) = 0;
    virtual void SetBaseForegroundColor(unsigned char r, unsigned char g,
                                        unsigned char b, unsigned char a) = 0;

    virtual void SetWebSecurityMode(::LWE::WebSecurityMode value) = 0;
    virtual void SetIdleModeJob(::LWE::IdleModeJob j) = 0;
    virtual void SetIdleModeCheckIntervalInMS(uint32_t intervalInMS) = 0;
    virtual void SetNeedsDownloadWebFontsEarly(bool b) = 0;
    virtual void SetUseHttp2(bool b) = 0;
    virtual void SetNeedsDownScaleImageResourceLargerThan(
        uint32_t demention) = 0; // Experimental
    virtual void SetScrollbarVisible(bool visible) = 0;
    virtual void SetUseExternalPopup(bool useExternalPopup) = 0;
    virtual void SetUseSpatialNavigation(bool useSpatialNavigation) = 0;

    virtual void IterateSettings(
        std::function<void(const std::string&, const std::string&)> callback)
        const = 0;
};

} // namespace LWEDelegate

// C wrappers used for dlopen/dlsym.
extern "C" {

uintptr_t EXPORT_UNMANAGED_API
LWEDelegate_Settings_Create(const char* defaultUA, const char* ua);
uintptr_t EXPORT_UNMANAGED_API LWEDelegate_Settings_Create_Empty();
uintptr_t EXPORT_UNMANAGED_API
LWEDelegate_Settings_Create_From_Other(void* other);

typedef struct {
    uintptr_t (*Create)(const char* defaultUA, const char* ua);
    uintptr_t (*CreateEmpty)();
    uintptr_t (*CreateFromOther)(void* other);

} SettingsProcTable;
}

#endif
