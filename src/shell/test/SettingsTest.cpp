/*
 * Copyright (c) 2024-present Samsung Electronics Co., Ltd
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

#include "ShellConfig.h"
#include "gtest/gtest.h"

#include "LWEWebView.h"

static const std::string kDefaultUA = "defaultUserAgent";
static const std::string kUA = "userAgent";

namespace StarfishShell {

class SettingsTest : public ::testing::Test {
public:
    SettingsTest() = default;

protected:
    static void SetUpTestCase()
    {
        LWE::LWE::Initialize("/tmp/starfish_storage/");
    }

    static void TearDownTestCase()
    {
        LWE::LWE::Finalize();
    }
};

TEST_F(SettingsTest, DefaultUserAgent)
{
    LWE::Settings settings(kDefaultUA, kUA);
    EXPECT_TRUE(settings.GetDefaultUserAgent() == kDefaultUA);
}

TEST_F(SettingsTest, UserAgentString)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.GetUserAgentString() == kUA);

    std::string newUA = "test";
    settings.SetUserAgentString(newUA);

    EXPECT_TRUE(settings.GetUserAgentString() == newUA);
}

TEST_F(SettingsTest, ProxyURL)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.GetProxyURL() == "");

    std::string proxyURL = "proxyURL";
    settings.SetProxyURL(proxyURL);

    EXPECT_TRUE(settings.GetProxyURL() == proxyURL);
}

TEST_F(SettingsTest, CacheMode)
{
    LWE::Settings settings(kDefaultUA, kUA);

#if defined(STARFISH_ENABLE_HTTPCACHE)
    EXPECT_TRUE(settings.GetCacheMode() == -1);
#else
    EXPECT_TRUE(settings.GetCacheMode() == 2);
#endif
    settings.SetCacheMode(3);
    EXPECT_TRUE(settings.GetCacheMode() == 3);
}

TEST_F(SettingsTest, TTSMode)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.GetTTSMode() == LWE::TTSMode::Default);

    settings.SetTTSMode(LWE::TTSMode::Forced);
    EXPECT_TRUE(settings.GetTTSMode() == LWE::TTSMode::Forced);
}

TEST_F(SettingsTest, TTSLanguage)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.GetTTSLanguage() == "");
    std::string newLang = "newLang";
    settings.SetTTSLanguage(newLang);
    EXPECT_TRUE(settings.GetTTSLanguage() == newLang);
}

TEST_F(SettingsTest, WebSecurityMode)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.GetWebSecurityMode() == LWE::WebSecurityMode::Enable);

    settings.SetWebSecurityMode(LWE::WebSecurityMode::Disable);
    EXPECT_TRUE(settings.GetWebSecurityMode() == LWE::WebSecurityMode::Disable);
}

TEST_F(SettingsTest, IdleModeJob)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.GetIdleModeJob() == LWE::IdleModeJob::IdleModeDefault);

    settings.SetIdleModeJob(LWE::IdleModeJob::ClearDrawnBuffers);
    EXPECT_TRUE(settings.GetIdleModeJob() ==
                LWE::IdleModeJob::ClearDrawnBuffers);
}

TEST_F(SettingsTest, IdleModeCheckIntervalInMS)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.GetIdleModeCheckIntervalInMS() ==
                LWE::IdleModeCheckDefaultIntervalInMS);

    settings.SetIdleModeCheckIntervalInMS(5000);
    EXPECT_TRUE(settings.GetIdleModeCheckIntervalInMS() == 5000);
}

TEST_F(SettingsTest, BaseBackgroundColor)
{
    LWE::Settings settings(kDefaultUA, kUA);
    unsigned char r, g, b, a;
    settings.GetBaseBackgroundColor(r, g, b, a);
    EXPECT_TRUE(r == 255 && g == 255 && b == 255 && a == 255);

    settings.SetBaseBackgroundColor(100, 100, 150, 0);
    settings.GetBaseBackgroundColor(r, g, b, a);
    EXPECT_TRUE(r == 100 && g == 100 && b == 150 && a == 0);
}

TEST_F(SettingsTest, BaseForegroundColor)
{
    LWE::Settings settings(kDefaultUA, kUA);
    unsigned char r, g, b, a;
    settings.GetBaseForegroundColor(r, g, b, a);
    EXPECT_TRUE(r == 0 && g == 0 && b == 0 && a == 255);

    settings.SetBaseForegroundColor(100, 100, 150, 0);
    settings.GetBaseForegroundColor(r, g, b, a);
    EXPECT_TRUE(r == 100 && g == 100 && b == 150 && a == 0);
}

TEST_F(SettingsTest, NeedsDownloadWebFontsEarly)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.NeedsDownloadWebFontsEarly() == false);

    settings.SetNeedsDownloadWebFontsEarly(true);
    EXPECT_TRUE(settings.NeedsDownloadWebFontsEarly() == true);
}

TEST_F(SettingsTest, UseHttp2)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.UseHttp2() == false);

    settings.SetUseHttp2(true);
    EXPECT_TRUE(settings.UseHttp2() == true);
}

TEST_F(SettingsTest, NeedsDownScaleImageResourceLargerThan)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.NeedsDownScaleImageResourceLargerThan() == 0);

    settings.SetNeedsDownScaleImageResourceLargerThan(1024);
    EXPECT_TRUE(settings.NeedsDownScaleImageResourceLargerThan() == 1024);
}

TEST_F(SettingsTest, ScrollbarVisible)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.ScrollbarVisible() == true);

    settings.SetScrollbarVisible(false);
    EXPECT_TRUE(settings.NeedsDownScaleImageResourceLargerThan() == false);
}

TEST_F(SettingsTest, UseExternalPopup)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.UseExternalPopup() == false);

    settings.SetUseExternalPopup(true);
    EXPECT_TRUE(settings.UseExternalPopup() == true);
}

TEST_F(SettingsTest, UseSpatialNavigation)
{
    LWE::Settings settings(kDefaultUA, kUA);

    EXPECT_TRUE(settings.UseSpatialNavigation() == false);

    settings.SetUseSpatialNavigation(true);
    EXPECT_TRUE(settings.UseSpatialNavigation() == true);
}

TEST_F(SettingsTest, UpdateSetting)
{
    LWE::Settings settings(kDefaultUA, kUA);

    settings.UpdateSetting("key", "value");
    EXPECT_TRUE(settings.GetSetting("key") == "value");
}

TEST_F(SettingsTest, IterateSettings)
{
    LWE::Settings settings(kDefaultUA, kUA);
    bool called = false;
    settings.IterateSettings(
        [&called](const std::string key, const std::string value) {
            called = true;
        });
    EXPECT_TRUE(called);
}

} // namespace StarfishShell
