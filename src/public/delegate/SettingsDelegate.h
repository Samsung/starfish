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

#include "PlatformIntegrationData.h"
#include <string>
#include <unordered_map>
#include <functional>

namespace LWEDelegate {

class Settings {
public:
    Settings() = default;
    Settings(const std::string& defaultUA, const std::string& ua);
    Settings(const Settings& other);

    bool UpdateSetting(const std::string& key, const std::string& value);
    std::string GetSetting(std::string key) const;

    std::string GetDefaultUserAgent() const;
    std::string GetUserAgentString() const;
    std::string GetProxyURL() const;
    int GetCacheMode() const;
    ::LWE::TTSMode GetTTSMode() const;
    std::string GetTTSLanguage() const;
    ::LWE::WebSecurityMode GetWebSecurityMode() const;
    ::LWE::IdleModeJob GetIdleModeJob() const;
    uint32_t GetIdleModeCheckIntervalInMS() const;
    void GetBaseBackgroundColor(unsigned char& r, unsigned char& g,
                                unsigned char& b, unsigned char& a) const;
    void GetBaseForegroundColor(unsigned char& r, unsigned char& g,
                                unsigned char& b, unsigned char& a) const;
    bool NeedsDownloadWebFontsEarly() const;
    bool UseHttp2() const;
    uint32_t NeedsDownScaleImageResourceLargerThan() const;
    bool ScrollbarVisible() const;
    bool UseExternalPopup() const;
    bool UseSpatialNavigation() const;

    void SetUserAgentString(const std::string& ua);
    void SetCacheMode(int mode);
    void SetProxyURL(const std::string& proxyURL);
    void setDefaultFontSize(int size);
    void SetTTSMode(::LWE::TTSMode value);
    void SetTTSLanguage(const std::string& language);
    void SetBaseBackgroundColor(unsigned char r, unsigned char g,
                                unsigned char b, unsigned char a);
    void SetBaseForegroundColor(unsigned char r, unsigned char g,
                                unsigned char b, unsigned char a);

    void SetWebSecurityMode(::LWE::WebSecurityMode value);
    void SetIdleModeJob(::LWE::IdleModeJob j);
    void SetIdleModeCheckIntervalInMS(uint32_t intervalInMS);
    void SetNeedsDownloadWebFontsEarly(bool b);
    void SetUseHttp2(bool b);
    void SetNeedsDownScaleImageResourceLargerThan(
        uint32_t demention); // Experimental
    void SetScrollbarVisible(bool visible);
    void SetUseExternalPopup(bool useExternalPopup);
    void SetUseSpatialNavigation(bool useSpatialNavigation);

    void IterateSettings(
        std::function<void(const std::string&, const std::string&)> callback)
        const;

private:
    std::unordered_map<std::string, std::string> m_settings;
};

} // namespace LWEDelegate

#endif
