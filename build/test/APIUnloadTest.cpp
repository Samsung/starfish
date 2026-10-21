/* Copyright (c) 2026-present Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "LWEWebView.h"
#include "public/LWEDelegateLoader.h"

// Compile-time test bridge: the driver links neither API nor impl and uses
// only C signatures, so it cannot keep either DSO resident via DT_NEEDED.
static unsigned g_apiUnloadTestState;

extern "C" {
void LWE_EXPORT LWEAPITest_Initialize(const char* storage, unsigned option)
{
    ::LWE::LWE::Initialize(storage,
                           static_cast<::LWE::InitializeOption>(option));
}

void LWE_EXPORT LWEAPITest_Finalize()
{
    ::LWE::LWE::Finalize();
}

bool LWE_EXPORT LWEAPITest_IsInitialized()
{
    return ::LWE::LWE::IsInitialized();
}

bool LWE_EXPORT LWEAPITest_HasCookies()
{
    return ::LWE::CookieManager::GetInstance()->HasCookies();
}

bool LWE_EXPORT LWEAPITest_LoadAgain()
{
    return ::LWE::LWEDelegateLoader::getInstance()->load();
}

bool LWE_EXPORT LWEAPITest_CanUnload()
{
    return ::LWE::LWE::CanUnload();
}

unsigned LWE_EXPORT LWEAPITest_SwapGlobal(unsigned value)
{
    unsigned previous = g_apiUnloadTestState;
    g_apiUnloadTestState = value;
    return previous;
}

unsigned LWE_EXPORT LWEAPITest_SwapStatic(unsigned value)
{
    static unsigned state;
    unsigned previous = state;
    state = value;
    return previous;
}
}
