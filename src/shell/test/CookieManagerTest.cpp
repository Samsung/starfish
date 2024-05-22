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

namespace StarfishShell {

class CookieManagerBasicTest : public ::testing::Test {
public:
    CookieManagerBasicTest() = default;

protected:
    static void SetUpTestCase()
    {
        LWE::LWE::Initialize("/tmp/starfish_localStorage.txt",
                             "/tmp/starfish_cookieStore.txt",
                             "/tmp/starfish_cache/");
    }

    static void TearDownTestCase()
    {
        LWE::LWE::Finalize();
    }
};

TEST_F(CookieManagerBasicTest, GetInstance)
{
    LWE::CookieManager* instance = nullptr;
    instance = LWE::CookieManager::GetInstance();
    EXPECT_TRUE(instance != nullptr);
}

TEST_F(CookieManagerBasicTest, EnsureSingleton)
{
    LWE::CookieManager* instance = nullptr;
    instance = LWE::CookieManager::GetInstance();
    EXPECT_TRUE(instance == LWE::CookieManager::GetInstance());
}

TEST_F(CookieManagerBasicTest, Destroy)
{
    LWE::CookieManager::Destroy();
    // Expect no error.

    EXPECT_TRUE(true);
}

class CookieManagerBasicTestWithoutInit : public ::testing::Test {
public:
    CookieManagerBasicTestWithoutInit() = default;
};

TEST_F(CookieManagerBasicTestWithoutInit, GetInstance)
{
    EXPECT_EXIT(LWE::CookieManager::GetInstance(),
                ::testing::KilledBySignal(SIGABRT), "");
}

TEST_F(CookieManagerBasicTestWithoutInit, Destroy)
{
    LWE::CookieManager::Destroy();
    // Expect no error.
    EXPECT_TRUE(true);
}

class CookieManagerTest : public ::testing::Test {
public:
    CookieManagerTest() = default;

protected:
    static void SetUpTestCase()
    {
        LWE::LWE::Initialize("/tmp/starfish_localStorage.txt",
                             "/tmp/starfish_cookieStore.txt",
                             "/tmp/starfish_cache/");
    }

    static void TearDownTestCase()
    {
        LWE::LWE::Finalize();
    }

    void SetUp() override
    {
        m_cookieManager = LWE::CookieManager::GetInstance();
    }

    LWE::CookieManager* m_cookieManager = nullptr;
};

TEST_F(CookieManagerTest, HasCookies)
{
    EXPECT_FALSE(m_cookieManager->HasCookies());
}

TEST_F(CookieManagerTest, GetCookie)
{
    EXPECT_EQ(m_cookieManager->GetCookie("http://www.dummy.com"), "");
}

TEST_F(CookieManagerTest, ClearCookies)
{
    m_cookieManager->ClearCookies();
    // Expect no error.
    EXPECT_TRUE(true);
}
} // namespace StarfishShell
