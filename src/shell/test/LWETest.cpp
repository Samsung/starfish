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

class LWETestInitialize : public ::testing::Test {
public:
    LWETestInitialize() = default;

protected:
    void TearDown() override
    {
        LWE::LWE::Finalize();
    }
};

TEST_F(LWETestInitialize, Initialize)
{
    LWE::LWE::Initialize("/tmp/starfish_localStorage.txt",
                         "/tmp/starfish_cookieStore.txt",
                         "/tmp/starfish_cache/");
    // No error.
    EXPECT_TRUE(true);
}

class LWETestFinalize : public ::testing::Test {
public:
    LWETestFinalize() = default;

protected:
    void SetUp() override
    {
        LWE::LWE::Initialize("/tmp/starfish_localStorage.txt",
                             "/tmp/starfish_cookieStore.txt",
                             "/tmp/starfish_cache/");
    }
};

TEST_F(LWETestFinalize, Finalize)
{
    LWE::LWE::Finalize();
    // No error.
    EXPECT_TRUE(true);
}

class LWETest : public ::testing::Test {
public:
    LWETest() = default;

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

TEST_F(LWETest, IsInitialized)
{
    EXPECT_TRUE(LWE::LWE::IsInitialized());
}

TEST_F(LWETest, GetGCFrequency)
{
    EXPECT_TRUE(LWE::LWE::GetGCFrequency() != 0);
}

TEST_F(LWETest, SetGCFrequency)
{
    LWE::LWE::SetGCFrequency(32);
    // Expect no error.

    EXPECT_TRUE(LWE::LWE::GetGCFrequency() == 32);
}

TEST_F(LWETest, GetVersion)
{
    int major = -1;
    int minor = -1;
    int patch = -1;
    LWE::LWE::GetVersion(&major, &minor, &patch);
    // Expect no error.

    EXPECT_TRUE(major >= 0 && minor >= 0 && patch >= 0);
}

class LWETestWitoutInit : public ::testing::Test {
public:
    LWETestWitoutInit() = default;
};

TEST_F(LWETestWitoutInit, GetGCFrequency)
{
    EXPECT_EXIT(LWE::LWE::GetGCFrequency(), ::testing::KilledBySignal(SIGABRT),
                "RELEASE_ASSERT");
}

TEST_F(LWETestWitoutInit, SetGCFrequency)
{
    EXPECT_EXIT(LWE::LWE::SetGCFrequency(32),
                ::testing::KilledBySignal(SIGABRT), "RELEASE_ASSERT");
}

TEST_F(LWETestWitoutInit, GetVersion)
{
    int major = -1;
    int minor = -1;
    int patch = -1;
    EXPECT_EXIT(LWE::LWE::GetVersion(&major, &minor, &patch),
                ::testing::KilledBySignal(SIGABRT), "RELEASE_ASSERT");
}
} // namespace StarfishShell
