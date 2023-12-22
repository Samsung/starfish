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

Settings::Settings(const std::string& default_ua, const std::string& ua)
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
    UpdateSetting("idleModeJob", "IdleModeDefault");
    UpdateSetting("idleModeCheckIntervalInMS",
                  std::to_string(::LWE::IdleModeCheckDefaultIntervalInMS));
    UpdateSetting("needsDownloadWebFontsEarly", "False");
    UpdateSetting("useHttp2", "False");
    UpdateSetting("needsDownScaleImageResourceLargerThan", "0");
#ifndef TIZEN_COMPAT_HEADER_5_0
    UpdateSetting("scrollbarVisible", "True");
#endif
    UpdateSetting("useExternalPopup", "False");
    UpdateSetting("useSpatialNavigation", "False");
}

Settings::Settings(const Settings& other)
{
    m_settings = other.m_settings;
}

bool Settings::UpdateSetting(const std::string& key, const std::string& value)
{
    m_settings[key] = value;
    return true;
}

std::string Settings::GetSetting(std::string key) const
{
    std::string value = "";
    auto iter = m_settings.find(key);
    if (iter != m_settings.end()) {
        value = iter->second;
    }
    return value;
}

std::string Settings::GetDefaultUserAgent() const
{
    return GetSetting("defaultUserAgent");
}

std::string Settings::GetUserAgentString() const
{
    return GetSetting("userAgent");
}

std::string Settings::GetProxyURL() const
{
    return GetSetting("proxyURL");
}

int Settings::GetCacheMode() const
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

::LWE::TTSMode Settings::GetTTSMode() const
{
    std::string value = GetSetting("ttsMode");
    if (value.compare("Forced") == 0) {
        return ::LWE::TTSMode::Forced;
    }
    return ::LWE::TTSMode::Default;
}

std::string Settings::GetTTSLanguage() const
{
    return GetSetting("ttsLanguage");
}

::LWE::WebSecurityMode Settings::GetWebSecurityMode() const
{
    std::string value = GetSetting("webSecurityMode");
    if (value.compare("Enable") == 0) {
        return ::LWE::WebSecurityMode::Enable;
    }
    return ::LWE::WebSecurityMode::Disable;
}

::LWE::IdleModeJob Settings::GetIdleModeJob() const
{
    std::string value = GetSetting("idleModeJob");
    if (value.compare("ClearDrawnBuffers") == 0) {
        return ::LWE::IdleModeJob::ClearDrawnBuffers;
    } else if (value.compare("ForceGC") == 0) {
        return ::LWE::IdleModeJob::ForceGC;
    } else if (value.compare("DropDecodedImageBuffer") == 0) {
        return ::LWE::IdleModeJob::DropDecodedImageBuffer;
    } else if (value.compare("IdleModeFull") == 0) {
        return ::LWE::IdleModeJob::IdleModeFull;
    } else if (value.compare("IdleModeMiddle") == 0) {
        return ::LWE::IdleModeJob::IdleModeMiddle;
    } else if (value.compare("IdleModeNone") == 0) {
        return ::LWE::IdleModeJob::IdleModeNone;
    }
    return ::LWE::IdleModeJob::IdleModeDefault;
}

uint32_t Settings::GetIdleModeCheckIntervalInMS() const
{
    std::string value = GetSetting("idleModeCheckIntervalInMS");
    if (value.length() > 0) {
        return std::stoi(value);
    }
    return 0;
}

void Settings::GetBaseBackgroundColor(unsigned char& r, unsigned char& g,
                                      unsigned char& b, unsigned char& a) const
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

void Settings::GetBaseForegroundColor(unsigned char& r, unsigned char& g,
                                      unsigned char& b, unsigned char& a) const
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

bool Settings::NeedsDownloadWebFontsEarly() const
{
    std::string value = GetSetting("needsDownloadWebFontsEarly");
    if (value.compare("True") == 0) {
        return true;
    }
    return false;
}

bool Settings::UseHttp2() const
{
    std::string value = GetSetting("useHttp2");
    if (value.compare("True") == 0) {
        return true;
    }
    return false;
}

uint32_t Settings::NeedsDownScaleImageResourceLargerThan() const
{
    std::string value = GetSetting("needsDownScaleImageResourceLargerThan");
    if (value.length() > 0) {
        return std::stoi(value);
    }
    return 0;
}

bool Settings::ScrollbarVisible() const
{
    std::string value = GetSetting("scrollbarVisible");
    if (value.compare("True") == 0) {
        return true;
    }
    return false;
}

bool Settings::UseExternalPopup() const
{
    std::string value = GetSetting("useExternalPopup");
    if (value.compare("True") == 0) {
        return true;
    }
    return false;
}

bool Settings::UseSpatialNavigation() const
{
    std::string value = GetSetting("useSpatialNavigation");
    if (value.compare("True") == 0) {
        return true;
    }
    return false;
}

void Settings::SetUserAgentString(const std::string& ua)
{
    UpdateSetting("userAgent", ua);
}

void Settings::SetCacheMode(int mode)
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

void Settings::SetProxyURL(const std::string& proxyURL)
{
    UpdateSetting("proxyURL", proxyURL);
}

void Settings::setDefaultFontSize(int size)
{
    UpdateSetting("defaultFontSize", std::to_string(size));
}

void Settings::SetTTSMode(::LWE::TTSMode value)
{
    if (value == ::LWE::TTSMode::Forced) {
        UpdateSetting("ttsMode", "Forced");
    } else {
        UpdateSetting("ttsMode", "Default");
    }
}

void Settings::SetTTSLanguage(const std::string& language)
{
    UpdateSetting("ttsLanguage", language);
}

void Settings::SetBaseBackgroundColor(unsigned char r, unsigned char g,
                                      unsigned char b, unsigned char a)
{
    char color[50];
    sprintf(color, "%d, %d, %d ,%d", r, g, b, a);
    UpdateSetting("backgroundColor", color);
}

void Settings::SetBaseForegroundColor(unsigned char r, unsigned char g,
                                      unsigned char b, unsigned char a)
{
    char color[50];
    sprintf(color, "%d, %d, %d ,%d", r, g, b, a);
    UpdateSetting("foregroundColor", color);
}

void Settings::SetWebSecurityMode(::LWE::WebSecurityMode value)
{
    if (value == ::LWE::WebSecurityMode::Enable) {
        UpdateSetting("webSecurityMode", "Enable");
    } else {
        UpdateSetting("webSecurityMode", "Disable");
    }
}

void Settings::SetIdleModeJob(::LWE::IdleModeJob j)
{
    std::string value;
    switch (j) {
    case ::LWE::IdleModeJob::ClearDrawnBuffers:
        value = "ClearDrawnBuffers";
        break;
    case ::LWE::IdleModeJob::ForceGC:
        // case IdleModeJob::IdleModeMiddle:
        value = "ForceGC";
        break;
    case ::LWE::IdleModeJob::DropDecodedImageBuffer:
        value = "DropDecodedImageBuffer";
        break;
    case ::LWE::IdleModeJob::IdleModeNone:
        value = "IdleModeNone";
        break;
    case ::LWE::IdleModeJob::IdleModeDefault:
    // case IdleModeJob::IdleModeFull:
    default:
        value = "IdleModeDefault";
        break;
    }
    UpdateSetting("idleModeJob", value);
}

void Settings::SetIdleModeCheckIntervalInMS(uint32_t intervalInMS)
{
    UpdateSetting("idleModeCheckIntervalInMS", std::to_string(intervalInMS));
}

void Settings::SetNeedsDownloadWebFontsEarly(bool b)
{
    if (b) {
        UpdateSetting("needsDownloadWebFontsEarly", "True");
    } else {
        UpdateSetting("needsDownloadWebFontsEarly", "False");
    }
}

void Settings::SetUseHttp2(bool b)
{
    if (b) {
        UpdateSetting("useHttp2", "True");
    } else {
        UpdateSetting("useHttp2", "False");
    }
}

void Settings::SetNeedsDownScaleImageResourceLargerThan(
    uint32_t demention) // Experimental
{
    UpdateSetting("needsDownScaleImageResourceLargerThan",
                  std::to_string(demention));
}

void Settings::SetScrollbarVisible(bool visible)
{
    if (visible) {
        UpdateSetting("scrollbarVisible", "True");
    } else {
        UpdateSetting("scrollbarVisible", "False");
    }
}

void Settings::SetUseExternalPopup(bool useExternalPopup)
{
    if (useExternalPopup) {
        UpdateSetting("useExternalPopup", "True");
    } else {
        UpdateSetting("useExternalPopup", "False");
    }
}

void Settings::SetUseSpatialNavigation(bool useSpatialNavigation)
{
    if (useSpatialNavigation) {
        UpdateSetting("useSpatialNavigation", "True");
    } else {
        UpdateSetting("useSpatialNavigation", "False");
    }
}

void Settings::IterateSettings(
    std::function<void(const std::string&, const std::string&)> callback) const
{
    for (auto& setting : m_settings) {
        callback(setting.first, setting.second);
    }
}

} // namespace LWEDelegate
