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

#include "SettingsDelegate.h"

#include <sstream>

#define STR_INDIR(x) #x
#define TO_STR(x) STR_INDIR(x)

#define LWE_DEFAULT_FONT_SIZE 16

namespace LWEDelegate {

class SettingsImpl : public Settings {
public:
    SettingsImpl() = default;
    SettingsImpl(const std::string& defaultUA, const std::string& ua);
    SettingsImpl(SettingsImpl* other);

    ~SettingsImpl() = default;

    bool UpdateSetting(const std::string& key,
                       const std::string& value) override;
    std::string GetSetting(std::string key) const override;

    std::string GetDefaultUserAgent() const override;
    std::string GetUserAgentString() const override;
    std::string GetProxyURL() const override;
    int GetCacheMode() const override;
    ::LWE::TTSMode GetTTSMode() const override;
    std::string GetTTSLanguage() const override;
    ::LWE::WebSecurityMode GetWebSecurityMode() const override;
    ::LWE::IdleModeJob GetIdleModeJob() const override;
    uint32_t GetIdleModeCheckIntervalInMS() const override;
    void GetBaseBackgroundColor(unsigned char& r, unsigned char& g,
                                unsigned char& b,
                                unsigned char& a) const override;
    void GetBaseForegroundColor(unsigned char& r, unsigned char& g,
                                unsigned char& b,
                                unsigned char& a) const override;
    bool NeedsDownloadWebFontsEarly() const override;
    bool UseHttp2() const override;
    uint32_t NeedsDownScaleImageResourceLargerThan() const override;
    bool ScrollbarVisible() const override;
    bool UseExternalPopup() const override;
    bool UseSpatialNavigation() const override;

    void SetUserAgentString(const std::string& ua) override;
    void SetCacheMode(int mode) override;
    void SetProxyURL(const std::string& proxyURL) override;
    void setDefaultFontSize(int size) override;
    void SetTTSMode(::LWE::TTSMode value) override;
    void SetTTSLanguage(const std::string& language) override;
    void SetBaseBackgroundColor(unsigned char r, unsigned char g,
                                unsigned char b, unsigned char a) override;
    void SetBaseForegroundColor(unsigned char r, unsigned char g,
                                unsigned char b, unsigned char a) override;

    void SetWebSecurityMode(::LWE::WebSecurityMode value) override;
    void SetIdleModeJob(::LWE::IdleModeJob j) override;
    void SetIdleModeCheckIntervalInMS(uint32_t intervalInMS) override;
    void SetNeedsDownloadWebFontsEarly(bool b) override;
    void SetUseHttp2(bool b) override;
    void SetNeedsDownScaleImageResourceLargerThan(uint32_t demention) override;
    void SetScrollbarVisible(bool visible) override;
    void SetUseExternalPopup(bool useExternalPopup) override;
    void SetUseSpatialNavigation(bool useSpatialNavigation) override;

    void IterateSettings(
        std::function<void(const std::string&, const std::string&)> callback)
        const override;

private:
    std::unordered_map<std::string, std::string> m_settings;
};

SettingsImpl::SettingsImpl(const std::string& default_ua, const std::string& ua)
{
    UpdateSetting("defaultUserAgent", default_ua);
    UpdateSetting("userAgent", ua);
    UpdateSetting("proxyURL", "");
#if defined(STARFISH_ENABLE_HTTPCACHE)
    UpdateSetting("cacheMode", "LOAD_DEFAULT");
#else
    UpdateSetting("cacheMode", "LOAD_NO_CACHE");
#endif
    UpdateSetting("defaultFontSize", TO_STR(LWE_DEFAULT_FONT_SIZE));
    UpdateSetting("ttsMode", "Default");
    UpdateSetting("ttsLanguage", "");
    UpdateSetting("backgroundColor", "255, 255, 255, 255");
    UpdateSetting("foregroundColor", "0, 0, 0, 255");
    UpdateSetting("webSecurityMode", "Enable");
    UpdateSetting(
        "idleModeJob",
        std::to_string((unsigned)::LWE::IdleModeJob::IdleModeDefault));
    UpdateSetting("idleModeCheckIntervalInMS",
                  std::to_string(::LWE::IdleModeCheckDefaultIntervalInMS));
    UpdateSetting("needsDownloadWebFontsEarly", "False");
    UpdateSetting("useHttp2", "False");
    UpdateSetting("needsDownScaleImageResourceLargerThan", "0");
    UpdateSetting("scrollbarVisible", "True");
    UpdateSetting("useExternalPopup", "False");
    UpdateSetting("useSpatialNavigation", "False");
    UpdateSetting("showLoadFailMsg", "true");
}

SettingsImpl::SettingsImpl(SettingsImpl* other)
{
    m_settings = other->m_settings;
}

bool SettingsImpl::UpdateSetting(const std::string& key,
                                 const std::string& value)
{
    m_settings[key] = value;
    return true;
}

std::string SettingsImpl::GetSetting(std::string key) const
{
    std::string value = "";
    auto iter = m_settings.find(key);
    if (iter != m_settings.end()) {
        value = iter->second;
    }
    return value;
}

std::string SettingsImpl::GetDefaultUserAgent() const
{
    return GetSetting("defaultUserAgent");
}

std::string SettingsImpl::GetUserAgentString() const
{
    return GetSetting("userAgent");
}

std::string SettingsImpl::GetProxyURL() const
{
    return GetSetting("proxyURL");
}

int SettingsImpl::GetCacheMode() const
{
    std::string value = GetSetting("cacheMode");
    if (value.compare("LOAD_NORMAL") == 0) {
        return 0;
    } else if (value.compare("LOAD_CACHE_ELSE_NETWORK") == 0) {
        return 1;
    } else if (value.compare("LOAD_NO_CACHE") == 0) {
        return 2;
    } else if (value.compare("LOAD_CACHE_ONLY") == 0) {
        return 3;
    }
    return -1;
}

::LWE::TTSMode SettingsImpl::GetTTSMode() const
{
    std::string value = GetSetting("ttsMode");
    if (value.compare("Forced") == 0) {
        return ::LWE::TTSMode::Forced;
    }
    return ::LWE::TTSMode::Default;
}

std::string SettingsImpl::GetTTSLanguage() const
{
    return GetSetting("ttsLanguage");
}

::LWE::WebSecurityMode SettingsImpl::GetWebSecurityMode() const
{
    std::string value = GetSetting("webSecurityMode");
    if (value.compare("Enable") == 0) {
        return ::LWE::WebSecurityMode::Enable;
    }
    return ::LWE::WebSecurityMode::Disable;
}

::LWE::IdleModeJob SettingsImpl::GetIdleModeJob() const
{
    std::string value = GetSetting("idleModeJob");
    if (value.length()) {
        return (::LWE::IdleModeJob)std::stoi(value);
    }
    return ::LWE::IdleModeJob::IdleModeDefault;
}

uint32_t SettingsImpl::GetIdleModeCheckIntervalInMS() const
{
    std::string value = GetSetting("idleModeCheckIntervalInMS");
    if (value.length() > 0) {
        return std::stoi(value);
    }
    return 0;
}

void SettingsImpl::GetBaseBackgroundColor(unsigned char& r, unsigned char& g,
                                          unsigned char& b,
                                          unsigned char& a) const
{
    std::string value = GetSetting("backgroundColor");
    std::stringstream ss(value);
    std::string temp;

    if (std::getline(ss, temp, ',')) {
        r = atoi(temp.c_str());
    }

    if (std::getline(ss, temp, ',')) {
        g = atoi(temp.c_str());
    }

    if (std::getline(ss, temp, ',')) {
        b = atoi(temp.c_str());
    }

    if (std::getline(ss, temp, ',')) {
        a = atoi(temp.c_str());
    }
}

void SettingsImpl::GetBaseForegroundColor(unsigned char& r, unsigned char& g,
                                          unsigned char& b,
                                          unsigned char& a) const
{
    std::string value = GetSetting("foregroundColor");
    std::stringstream ss(value);
    std::string temp;

    if (std::getline(ss, temp, ',')) {
        r = atoi(temp.c_str());
    }

    if (std::getline(ss, temp, ',')) {
        g = atoi(temp.c_str());
    }

    if (std::getline(ss, temp, ',')) {
        b = atoi(temp.c_str());
    }

    if (std::getline(ss, temp, ',')) {
        a = atoi(temp.c_str());
    }
}

bool SettingsImpl::NeedsDownloadWebFontsEarly() const
{
    std::string value = GetSetting("needsDownloadWebFontsEarly");
    if (value.compare("True") == 0) {
        return true;
    }
    return false;
}

bool SettingsImpl::UseHttp2() const
{
    std::string value = GetSetting("useHttp2");
    if (value.compare("True") == 0) {
        return true;
    }
    return false;
}

uint32_t SettingsImpl::NeedsDownScaleImageResourceLargerThan() const
{
    std::string value = GetSetting("needsDownScaleImageResourceLargerThan");
    if (value.length() > 0) {
        return std::stoi(value);
    }
    return 0;
}

bool SettingsImpl::ScrollbarVisible() const
{
    std::string value = GetSetting("scrollbarVisible");
    if (value.compare("True") == 0) {
        return true;
    }
    return false;
}

bool SettingsImpl::UseExternalPopup() const
{
    std::string value = GetSetting("useExternalPopup");
    if (value.compare("True") == 0) {
        return true;
    }
    return false;
}

bool SettingsImpl::UseSpatialNavigation() const
{
    std::string value = GetSetting("useSpatialNavigation");
    if (value.compare("True") == 0) {
        return true;
    }
    return false;
}

void SettingsImpl::SetUserAgentString(const std::string& ua)
{
    UpdateSetting("userAgent", ua);
}

void SettingsImpl::SetCacheMode(int mode)
{
    std::string cacheMode;
    switch (mode) {
    case 0:
        cacheMode = "LOAD_NORMAL";
        break;
    case 1:
        cacheMode = "LOAD_CACHE_ELSE_NETWORK";
        break;
    case 2:
        cacheMode = "LOAD_NO_CACHE";
        break;
    case 3:
        cacheMode = "LOAD_CACHE_ONLY";
        break;
    default:
        cacheMode = "LOAD_DEFAULT";
        break;
    }
    UpdateSetting("cacheMode", cacheMode);
}

void SettingsImpl::SetProxyURL(const std::string& proxyURL)
{
    UpdateSetting("proxyURL", proxyURL);
}

void SettingsImpl::setDefaultFontSize(int size)
{
    UpdateSetting("defaultFontSize", std::to_string(size));
}

void SettingsImpl::SetTTSMode(::LWE::TTSMode value)
{
    if (value == ::LWE::TTSMode::Forced) {
        UpdateSetting("ttsMode", "Forced");
    } else {
        UpdateSetting("ttsMode", "Default");
    }
}

void SettingsImpl::SetTTSLanguage(const std::string& language)
{
    UpdateSetting("ttsLanguage", language);
}

void SettingsImpl::SetBaseBackgroundColor(unsigned char r, unsigned char g,
                                          unsigned char b, unsigned char a)
{
    char color[50];
    sprintf(color, "%d, %d, %d ,%d", r, g, b, a);
    UpdateSetting("backgroundColor", color);
}

void SettingsImpl::SetBaseForegroundColor(unsigned char r, unsigned char g,
                                          unsigned char b, unsigned char a)
{
    char color[50];
    sprintf(color, "%d, %d, %d ,%d", r, g, b, a);
    UpdateSetting("foregroundColor", color);
}

void SettingsImpl::SetWebSecurityMode(::LWE::WebSecurityMode value)
{
    if (value == ::LWE::WebSecurityMode::Enable) {
        UpdateSetting("webSecurityMode", "Enable");
    } else {
        UpdateSetting("webSecurityMode", "Disable");
    }
}

void SettingsImpl::SetIdleModeJob(::LWE::IdleModeJob j)
{
    UpdateSetting("idleModeJob", std::to_string((unsigned)j));
}

void SettingsImpl::SetIdleModeCheckIntervalInMS(uint32_t intervalInMS)
{
    UpdateSetting("idleModeCheckIntervalInMS", std::to_string(intervalInMS));
}

void SettingsImpl::SetNeedsDownloadWebFontsEarly(bool b)
{
    if (b) {
        UpdateSetting("needsDownloadWebFontsEarly", "True");
    } else {
        UpdateSetting("needsDownloadWebFontsEarly", "False");
    }
}

void SettingsImpl::SetUseHttp2(bool b)
{
    if (b) {
        UpdateSetting("useHttp2", "True");
    } else {
        UpdateSetting("useHttp2", "False");
    }
}

void SettingsImpl::SetNeedsDownScaleImageResourceLargerThan(
    uint32_t demention) // Experimental
{
    UpdateSetting("needsDownScaleImageResourceLargerThan",
                  std::to_string(demention));
}

void SettingsImpl::SetScrollbarVisible(bool visible)
{
    if (visible) {
        UpdateSetting("scrollbarVisible", "True");
    } else {
        UpdateSetting("scrollbarVisible", "False");
    }
}

void SettingsImpl::SetUseExternalPopup(bool useExternalPopup)
{
    if (useExternalPopup) {
        UpdateSetting("useExternalPopup", "True");
    } else {
        UpdateSetting("useExternalPopup", "False");
    }
}

void SettingsImpl::SetUseSpatialNavigation(bool useSpatialNavigation)
{
    if (useSpatialNavigation) {
        UpdateSetting("useSpatialNavigation", "True");
    } else {
        UpdateSetting("useSpatialNavigation", "False");
    }
}

void SettingsImpl::IterateSettings(
    std::function<void(const std::string&, const std::string&)> callback) const
{
    for (auto& setting : m_settings) {
        callback(setting.first, setting.second);
    }
}

Settings* Settings::Create()
{
    return new SettingsImpl();
}

Settings* Settings::Create(const std::string& defaultUA, const std::string& ua)
{
    return new SettingsImpl(defaultUA, ua);
}

Settings* Settings::Create(Settings* other)
{
    return new SettingsImpl(static_cast<SettingsImpl*>(other));
}

} // namespace LWEDelegate

extern "C" {
uintptr_t LWEDelegate_Settings_Create_Empty()
{
    return reinterpret_cast<uintptr_t>(LWEDelegate::Settings::Create());
}

uintptr_t LWEDelegate_Settings_Create(const char* defaultUA, const char* ua)
{
    return reinterpret_cast<uintptr_t>(
        LWEDelegate::Settings::Create(defaultUA, ua));
}

uintptr_t LWEDelegate_Settings_Create_From_Other(void* other)
{
    return reinterpret_cast<uintptr_t>(LWEDelegate::Settings::Create(
        static_cast<LWEDelegate::Settings*>(other)));
}
}
