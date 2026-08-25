// Copyright (c) 2026-present Samsung Electronics Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// ABI fingerprint target for tool/lint/check_contract_abi.py.
//
// The API .so and the impl .so (see docs on UWE) are never compiled
// together, so the only thing that has to agree between an old API .so and
// a new impl .so is the *layout* the compiler derives from
// src/public/contract/*.h: vtable slot order, extern "C" wrapper
// signatures, and the layout of enums/structs that cross the boundary by
// value. That layout is a pure function of these headers -- it does not
// require the real impl .so to observe. This file exists so
// check_contract_abi.py can compile a few hundred KB from *only* these
// headers and let a debug-info reader (abidw) tell us that layout, instead
// of digging it out of a 500+ MB engine binary.
//
// check_contract_abi.py compiles this file twice -- once against the
// contract headers at some git base commit, once against the working tree
// -- with byte-identical flags, and diffs the resulting ABI dumps. That
// means this file itself is also compiled from each side's own copy: do not
// assume "the version of this file in the working tree" when editing it,
// because half of every comparison uses whatever this file looked like at
// the base commit.
//
// Compile with (see check_contract_abi.py for the authoritative flags):
//   g++ -std=c++11 -g3 -fvisibility=hidden -femit-class-debug-always
//       -DSTARFISH_WEBWORKER_HOST -shared -fPIC
//       -I<contract-root>/src -I<contract-root>/inc contract_shim.cpp
//
// -femit-class-debug-always is required: without it, a class that is never
// "ODR-used" as a complete type in this TU (which is every class here --
// they are pure interfaces, only ever touched through a pointer) is emitted
// to DWARF as a declaration only, with none of its virtual methods, sizes,
// or member layouts -- silently. A checker built on that would look "clean"
// while covering nothing. check_contract_abi.py independently asserts a
// minimum vtable-offset count as a second guard against exactly that.
//
// Every interface is touched through one real (not synthesized) virtual
// call or delete below, and every enum/struct that crosses the boundary by
// value is named in the witness. Both are deliberate: referencing a class
// only as a bare pointer parameter, or a type only inside another type's
// signature, is not reliably enough to make GCC keep it complete even with
// -femit-class-debug-always in all toolchain versions this has been
// checked against -- an actual use removes the ambiguity.
//
// This file is compiled and inspected by abidw/abidiff only. It is never
// linked into a runnable program and the functions below are never called
// for real -- do not add a main(), do not dlopen this shim, and do not run
// it. Doing so would mean running code compiled from a PR's own,
// potentially untrusted, contract headers.

// The contract headers use std::string/size_t without including <string>/
// <cstddef> themselves (they rely on being included after something that
// already pulled those in, inside the real engine sources). Do that here
// explicitly rather than fix the headers, which is an unrelated, purely
// mechanical, separately-landable cleanup.
#include <cstddef>
#include <string>
using std::size_t;

// LWEWorker.h must precede LWEWorkerDelegate.h: the latter only forward-
// declares `enum class LWE::WorkerProcessState` (it crosses the boundary by
// value inside std::function<void(WorkerProcessState)>), and the real
// engine sources that consume both always pull in the full definition from
// here first (see src/public/delegate/LWEWorkerDelegate.cpp). Without it,
// WorkerProcessState stays opaque and a renumbering of its enumerators is
// undetectable.
#include "LWEWorker.h"

#include "public/contract/LWEDelegate.h"
#include "public/contract/LWEDelegateConfig.h"
#include "public/contract/LWEDelegateContract.h"
#include "public/contract/CookieManagerDelegate.h"
#include "public/contract/ResourceErrorDelegate.h"
#include "public/contract/SettingsDelegate.h"
#include "public/contract/LWEWebContainerDelegate.h"
#include "public/contract/LWEWebViewDelegate.h"
#include "public/contract/LWEWorkerDelegate.h"

// ---------------------------------------------------------------------------
// I3: the extern "C" wrapper declared in each contract header must have the
// exact type the corresponding ProcTable function-pointer member expects.
// LWEDelegateLoader.cpp/LWEWorkerDelegateLoader.cpp already assume this by
// casting through decltype(ProcTable::Member), which means the loader's own
// cast can never notice if the two drift apart -- it would just compile
// against whichever type decltype resolves to. This is a compile-time,
// base-independent check (it holds or it doesn't, regardless of git base),
// so a failure here is a bug in the headers today, not a reported ABI
// change against some other commit.
//
// There is no mechanical name mapping from a ProcTable member to its
// wrapper (LWEDelegate_WebContainer_CreateGL but
// LWEDelegate_WebContainer_Create_With_PlatformImage), so this list is
// hand-maintained. check_contract_abi.py separately asserts its length
// equals the number of extern "C" wrappers declared across the contract
// headers, so a wrapper added without a matching line here fails loudly
// instead of silently leaving I3 incomplete.
// ---------------------------------------------------------------------------
#define CONTRACT_ABI_CHECK_WRAPPER(ProcTable, Member, Wrapper)                \
    static_assert(                                                            \
        std::is_same<decltype(&Wrapper), decltype(ProcTable::Member)>::value, \
        #ProcTable "::" #Member " no longer matches " #Wrapper)

#include <type_traits>

CONTRACT_ABI_CHECK_WRAPPER(DelegateContractProcTable, GetAbiEpoch,
                           LWEDelegate_GetAbiEpoch);

CONTRACT_ABI_CHECK_WRAPPER(LWEProcTable, Initialize,
                           LWEDelegate_LWE_Initialize);
CONTRACT_ABI_CHECK_WRAPPER(LWEProcTable, IsInitialized,
                           LWEDelegate_LWE_IsInitialized);
CONTRACT_ABI_CHECK_WRAPPER(LWEProcTable, Finalize, LWEDelegate_LWE_Finalize);
CONTRACT_ABI_CHECK_WRAPPER(LWEProcTable, GetGCFrequency,
                           LWEDelegate_LWE_GetGCFrequency);
CONTRACT_ABI_CHECK_WRAPPER(LWEProcTable, SetGCFrequency,
                           LWEDelegate_LWE_SetGCFrequency);
CONTRACT_ABI_CHECK_WRAPPER(LWEProcTable, GetVersion,
                           LWEDelegate_LWE_GetVersion);
CONTRACT_ABI_CHECK_WRAPPER(LWEProcTable, IsUsingSeparateThread,
                           LWEDelegate_LWE_IsUsingSeparateThread);

CONTRACT_ABI_CHECK_WRAPPER(CookieManagerProcTable, GetInstance,
                           LWEDelegate_CookieManager_GetInstance);
CONTRACT_ABI_CHECK_WRAPPER(CookieManagerProcTable, Destroy,
                           LWEDelegate_CookieManager_Destroy);

CONTRACT_ABI_CHECK_WRAPPER(ResourceErrorProcTable, Create,
                           LWEDelegate_ResourceError_Create);

CONTRACT_ABI_CHECK_WRAPPER(SettingsProcTable, Create,
                           LWEDelegate_Settings_Create);
CONTRACT_ABI_CHECK_WRAPPER(SettingsProcTable, CreateEmpty,
                           LWEDelegate_Settings_Create_Empty);
CONTRACT_ABI_CHECK_WRAPPER(SettingsProcTable, CreateFromOther,
                           LWEDelegate_Settings_Create_From_Other);

CONTRACT_ABI_CHECK_WRAPPER(WebContainerProcTable, Create,
                           LWEDelegate_WebContainer_Create);
CONTRACT_ABI_CHECK_WRAPPER(WebContainerProcTable, CreateWithBuffer,
                           LWEDelegate_WebContainer_CreateWithBuffer);
CONTRACT_ABI_CHECK_WRAPPER(WebContainerProcTable, CreateWithPlatformImage,
                           LWEDelegate_WebContainer_Create_With_PlatformImage);
CONTRACT_ABI_CHECK_WRAPPER(WebContainerProcTable, CreateGL,
                           LWEDelegate_WebContainer_CreateGL);
CONTRACT_ABI_CHECK_WRAPPER(WebContainerProcTable, CreateGLWithPlatformImage,
                           LWEDelegate_WebContainer_CreateGLWithPlatformImage);
CONTRACT_ABI_CHECK_WRAPPER(WebContainerProcTable, CreateHeadless,
                           LWEDelegate_WebContainer_CreateHeadless);

CONTRACT_ABI_CHECK_WRAPPER(WebViewProcTable, Create,
                           LWEDelegate_WebView_Create);

CONTRACT_ABI_CHECK_WRAPPER(LWEWorkerProcTable, Initialize,
                           LWEWorkerDelegate_LWEWorker_Initialize);
CONTRACT_ABI_CHECK_WRAPPER(
    LWEWorkerProcTable, RegisterOnStatusChangedHandler,
    LWEWorkerDelegate_LWEWorker_RegisterOnStatusChangedHandler);
CONTRACT_ABI_CHECK_WRAPPER(LWEWorkerProcTable, Finalize,
                           LWEWorkerDelegate_LWEWorker_Finalize);

// A hardcoded count here could only ever assert against itself, so it isn't
// one: check_contract_abi.py counts CONTRACT_ABI_CHECK_WRAPPER invocations
// in this file's source text and separately counts the wrapper names
// declared across src/public/contract/*.h (the same names I1, the
// header/loader wrapper-name-set check, extracts), and fails if a wrapper
// was added to a header without a matching line above.

// ---------------------------------------------------------------------------
// Witness: exported storage that references every enum and every by-value
// struct that crosses the API/impl boundary, so each is forced into this
// TU's DWARF (and therefore into the ABI dump) even though nothing else in
// this file constructs or reads one. Without this, types that appear only
// as a bare parameter of a std::function crossing the boundary (the
// WorkerProcessState case) or only behind a uintptr_t-erased pointer (the
// WebContainerArguments/RendererGLConfiguration case) can end up absent, or
// present but incomplete, in the dump depending on how the compiler decided
// to treat the reference -- reachability through a signature is an
// inference about compiler behavior, not a guarantee.
// ---------------------------------------------------------------------------
// NOTE: the struct definition and the variable definition are deliberately
// two separate statements, and `extern "C"` is deliberately not used here.
// `extern "C" struct Foo { ... } bar;` (attaching the linkage-specification
// to a struct-definition-plus-declarator in one statement) silently drops
// the variable under this GCC -- no error, no warning even with -Wall
// -Wextra, just an empty symbol table entry -- confirmed empirically before
// writing this comment. A plain global at namespace scope does not need
// `extern "C"` for an unmangled name under the Itanium C++ ABI anyway; only
// the visibility attribute is needed to survive -fvisibility=hidden.
struct ContractAbiWitness {
    ::LWE::KeyValue keyValue;
    ::LWE::MouseButtonValue mouseButtonValue;
    ::LWE::MouseButtonsValue mouseButtonsValue;
    ::LWE::TTSMode ttsMode;
    ::LWE::WebSecurityMode webSecurityMode;
    ::LWE::IdleModeJob idleModeJob;
    ::LWE::WorkerProcessState workerProcessState;

    LWEDelegate::WebContainer::RenderInfo renderInfo;
    LWEDelegate::WebContainer::ExternalImageInfo externalImageInfo;
    LWEDelegate::WebContainer::RenderResult renderResult;
    LWEDelegate::WebContainer::WebContainerArguments webContainerArguments;
    LWEDelegate::WebContainer::TransformationMatrix transformationMatrix;
    LWEDelegate::WebContainer::RendererGLConfiguration rendererGLConfiguration;

    // inc/PlatformIntegrationData.h:274 is baked by value into the API .so
    // at compile time -- no ABI tool can see a change to it from the impl
    // side alone, but pinning the value here at least makes it visible in
    // this side's own fingerprint.
    int idleModeCheckDefaultIntervalInMS =
        ::LWE::IdleModeCheckDefaultIntervalInMS;
};

__attribute__((visibility("default"))) ContractAbiWitness gContractAbiWitness;

// ---------------------------------------------------------------------------
// Anchor: one real virtual call (or delete, where the destructor is the
// only public virtual) through each of the five vtable-bearing interfaces.
// A bare pointer parameter is not enough to make every toolchain treat the
// pointee as a required-complete type; an actual use removes the doubt.
// check_contract_abi.py's minimum-vtable-offset-count guard exists
// specifically to catch a future toolchain where even this stops being
// sufficient, rather than trusting this comment.
// ---------------------------------------------------------------------------
extern "C" __attribute__((visibility("default"))) void ContractAbiAnchor(
    LWEDelegate::WebContainer* webContainer, LWEDelegate::WebView* webView,
    LWEDelegate::Settings* settings, LWEDelegate::CookieManager* cookieManager,
    LWEDelegate::ResourceError* resourceError, LWEDelegate::LWEWorker* worker)
{
    webContainer->Destroy();
    webView->Destroy();
    delete settings;
    cookieManager->HasCookies();
    resourceError->GetErrorCode();
    (void)worker; // LWEWorker has no virtuals; its ABI is its ProcTable only.
}

// Every ProcTable also needs to be forced into the corpus -- referencing a
// typedef struct only inside a CONTRACT_ABI_CHECK_WRAPPER static_assert
// does not emit it, since static_assert is compile-time-only and leaves no
// DWARF. An exported function taking each by pointer does.
extern "C" __attribute__((visibility("default"))) void
ContractAbiAnchorProcTables(LWEProcTable* lwe,
                            DelegateContractProcTable* delegateContract,
                            WebContainerProcTable* webContainer,
                            WebViewProcTable* webView,
                            SettingsProcTable* settings,
                            CookieManagerProcTable* cookieManager,
                            ResourceErrorProcTable* resourceError,
                            LWEWorkerProcTable* worker)
{
    (void)lwe;
    (void)delegateContract;
    (void)webContainer;
    (void)webView;
    (void)settings;
    (void)cookieManager;
    (void)resourceError;
    (void)worker;
}
