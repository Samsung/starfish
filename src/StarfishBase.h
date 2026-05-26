/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2019-present Samsung Electronics Co., Ltd
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

#ifndef __StarfishBase__
#define __StarfishBase__

#include "StarfishInfo.h"
#include "StarfishPlatform.h"

#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <deque>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <cstring>
#include <sstream>
#include <cassert>
#include <functional>
#include <algorithm>
#include <cmath>
#include <limits>
#include <locale>
#include <clocale>
#include <cwchar>
#include <numeric>
#include <stdarg.h>
#include <future>
#include <type_traits>
#include <random>
#include <cfloat>
#include <thread>

#include <tsl/robin_set.h>
#include <tsl/robin_map.h>

#if defined(__clang__)
#define COMPILER_CLANG 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC 1
#elif (__GNUC__)
#define COMPILER_GCC 1
#else
#error "Compiler dectection failed"
#endif

#if defined(COMPILER_CLANG)
/* Keep strong enums turned off when building with clang-cl: We cannot yet build
 * all of Blink without fallback to cl.exe, and strong enums are exposed at ABI
 * boundaries. */
#undef COMPILER_SUPPORTS_CXX_STRONG_ENUMS
#else
#define COMPILER_SUPPORTS_CXX_OVERRIDE_CONTROL 1
#define COMPILER_QUIRK_FINAL_IS_CALLED_SEALED 1
#endif

/* ALWAYS_INLINE */
#ifndef ALWAYS_INLINE
#if (defined(COMPILER_GCC) || defined(COMPILER_CLANG)) && defined(NDEBUG) && \
    !defined(COMPILER_MINGW)
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(COMPILER_MSVC) && defined(NDEBUG)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif
#endif

/* NEVER_INLINE */
#ifndef NEVER_INLINE
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define NEVER_INLINE __attribute__((__noinline__))
#else
#define NEVER_INLINE
#endif
#endif

/* UNLIKELY */
#ifndef UNLIKELY
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define UNLIKELY(x) __builtin_expect((x), 0)
#else
#define UNLIKELY(x) (x)
#endif
#endif

/* LIKELY */
#ifndef LIKELY
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define LIKELY(x) __builtin_expect((x), 1)
#else
#define LIKELY(x) (x)
#endif
#endif

/* NO_RETURN */
#ifndef NO_RETURN
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#define NO_RETURN __attribute((__noreturn__))
#elif defined(COMPILER_MSVC)
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN
#endif
#endif

/* EXPORT */
#ifndef EXPORT
#if defined(COMPILER_MSVC)
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
#endif

/* FALLTHROUGH */
#if !defined(FALLTHROUGH) && defined(COMPILER_GCC)
#if __GNUC__ >= 7
#define FALLTHROUGH __attribute__((fallthrough))
#else
#define FALLTHROUGH /* fall through */
#endif
#elif !defined(FALLTHROUGH) && defined(COMPILER_CLANG)
#define FALLTHROUGH /* fall through */
#else
#define FALLTHROUGH
#endif

#define NULLABLE

#if defined(COMPILER_MSVC)
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

#if defined(COMPILER_MSVC)
#define ENSURE_ENUM_UNSIGNED : unsigned int
#else
#define ENSURE_ENUM_UNSIGNED
#endif

#ifdef _WIN32
#define OS_WINDOWS 1
#elif _WIN64
#define OS_WINDOWS 1
#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_IPHONE_SIMULATOR
#define OS_POSIX 1
#elif TARGET_OS_IPHONE
#define OS_POSIX 1
#elif TARGET_OS_MAC
#define OS_POSIX 1
#else
#error "Unknown Apple platform"
#endif
#elif __linux__
#define OS_POSIX 1
#elif __unix__ // all unices not caught above
#define OS_POSIX 1
#elif defined(_POSIX_VERSION)
#define OS_POSIX 1
#else
#error "failed to detect target OS"
#endif

#if defined(OS_WINDOWS)
#define NOMINMAX
#define setenv(a, b, c) _putenv_s(a, b)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#define _SSIZE_T_
#define _SSIZE_T_DEFINED
#define _INC_CTYPE // for preventing include windows version of ctype header
#include <inttypes.h>
typedef unsigned int uint;
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#ifndef NO_EXPOSE_GC
#ifndef ESCARGOT
#define ESCARGOT // for use additional functions in GCutil
#endif
#include <GCUtil.h>
#undef ESCARGOT
#endif // NO_EXPOSE_GC

#if defined(STARFISH_ENABLE_RUNTIME_ICU_BINDER)
#include <RuntimeICUBinder.h>
#include <ICUPolyfill.h>
#else
#if defined(STARFISH_WINDOWS)
#include <icu.h>
#else
#include <unicode/locid.h>
#include <unicode/brkiter.h>
#include <unicode/ubidi.h>
#include <unicode/uchar.h>
#include <unicode/ucnv.h>
#include <unicode/ucsdet.h>
#include <unicode/uscript.h>
#include <unicode/unorm.h>
#include <unicode/rbbi.h>
#endif
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include <pthread.h>
#include <semaphore.h>

#if defined(OS_WINDOWS)
#ifdef DELETE
#undef DELETE
#endif
#ifdef ERROR
#undef ERROR
#endif
#endif

#if !defined(__PRETTY_FUNCTION__) && defined(COMPILER_MSVC)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#define DEFAULT_CLEAR_STACK_SIZE 102400
#define ELABORATE_CLEAR_STACK_SIZE DEFAULT_CLEAR_STACK_SIZE * 4

#if defined(COMPILER_GCC)
template <const int siz>
inline void __attribute__((optimize("O0"))) clearStack()
{
    volatile char a[siz] = { 0 };
}
#elif defined(COMPILER_CLANG)
template <const int siz>
[[clang::optnone]] inline void clearStack()
{
    volatile char a[siz] = { 0 };
}
#elif defined(COMPILER_MSVC)
#pragma optimize("", off)
template <const int siz>
inline void clearStack()
{
    volatile char a[siz] = { 0 };
}
#pragma optimize("", on)
#else
#error
#endif

// https://sourceforge.net/p/predef/wiki/Architectures/
#if INTPTR_MAX == INT32_MAX
#define STARFISH_32
#elif INTPTR_MAX == INT64_MAX
#define STARFISH_64
#else
#error "Environment not 32 or 64-bit."
#endif

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || \
    defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define STARFISH_X86_64

#elif defined(i386) || defined(__i386) || defined(__i386__) ||      \
    defined(__IA32__) || defined(_M_IX86) || defined(__X86__) ||    \
    defined(_X86_) || defined(__THW_INTEL__) || defined(__I86__) || \
    defined(__INTEL__) || defined(__386)
#define STARFISH_X86

#elif defined(__arm__) || defined(__thumb__) || defined(_ARM) || \
    defined(_M_ARM) || defined(_M_ARMT) || defined(__arm) || defined(__arm)
#define STARFISH_ARM
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#define STARFISH_ARM_NEON
#endif

#elif defined(__aarch64__)
#define STARFISH_ARM64
#define STARFISH_ARM_NEON

#elif defined(__riscv) && defined(__riscv_xlen) && __riscv_xlen == 32
#define STARFISH_RISCV32

#elif defined(__riscv) && defined(__riscv_xlen) && __riscv_xlen == 64
#define STARFISH_RISCV64

#else
#error "Could't find cpu arch."
#endif

#if defined(OS_WINDOWS)
namespace Starfish {
void forwardPrintingLogInfo(const char* fmt, ...);
void forwardPrintingLogError(const char* fmt, ...);
void forwardPrintingLogWarn(const char* fmt, ...);
const char* getWindowsTempDir();
} // namespace Starfish
#endif

#include "core/util/ProgramOptions.h"

#if defined(STARFISH_ENABLE_PROFILE)
#define STARFISH_ENABLE_PROFILE_TIMER
#define STARFISH_ENABLE_PROFILE_LOADING
#endif

#if defined(STARFISH_WEBWORKER_HOST)
#define STARFISH_LOG_TAG "[WORKER] "
#else
#define STARFISH_LOG_TAG ""
#endif

#define CSTR(stringPtr) ((stringPtr)->toUTF8NonGCString().c_str())

#ifndef __MODULE__
#define __MODULE__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#if !defined(STARFISH_WINDOWS)
#define LOG_FUNCTION_TYPE(functionName, param1, param2, ansi1, ansi2, fmt,   \
                          arg...)                                            \
    functionName(param1, param2 ansi1 "%s: %s(%d) > " fmt ansi2, __MODULE__, \
                 __func__, __LINE__, ##arg);
#define LOG_FUNCTION_TYPE2(functionName, param1, param2, ansi1, ansi2, fmt,    \
                           arg...)                                             \
    functionName(param1, param2, ansi1 "%s: %s(%d) > " VERSION ": " fmt ansi2, \
                 __MODULE__, __func__, __LINE__, ##arg);
#else
#define LOG_FUNCTION_TYPE(functionName, param1, param2, ansi1, ansi2, fmt,   \
                          arg, ...)                                          \
    functionName(param1, param2 ansi1 "%s: %s(%d) > " fmt ansi2, __MODULE__, \
                 __func__, __LINE__, ##arg);
#define LOG_FUNCTION_TYPE2(functionName, param1, param2, ansi1, ansi2, fmt,    \
                           arg, ...)                                           \
    functionName(param1, param2, ansi1 "%s: %s(%d) > " VERSION ": " fmt ansi2, \
                 __MODULE__, __func__, __LINE__, ##arg);
#endif

#if defined(STARFISH_TIZEN) && !defined(STARFISH_ENABLE_TEST)
#include <dlog.h>
#define STARFISH_LOG_INFO(fmt, arg...) \
    LOG_FUNCTION_TYPE2(dlog_print, DLOG_INFO, STARFISH_NAME, "", "", fmt, ##arg)
#elif defined(STARFISH_ANDROID)
#include <android/log.h>
#define STARFISH_LOG_INFO(fmt, arg...)                                       \
    LOG_FUNCTION_TYPE2(__android_log_print, ANDROID_LOG_INFO, STARFISH_NAME, \
                       "", "", fmt, ##arg)
#elif defined(STARFISH_WINDOWS)
#define STARFISH_LOG_INFO(...) ::Starfish::forwardPrintingLogInfo(__VA_ARGS__);
#else
#define STARFISH_LOG_INFO(fmt, arg...)                                         \
    if (LoggerOption::instance()->isLogEnable(__MODULE__)) {                   \
        LOG_FUNCTION_TYPE(fprintf, stdout, STARFISH_LOG_TAG, "", "", fmt "\n", \
                          ##arg)                                               \
    }
#endif

#if defined(STARFISH_TIZEN) && !defined(STARFISH_ENABLE_TEST)
#include <dlog.h>
#define STARFISH_LOG_ERROR(fmt, arg...)                                    \
    LOG_FUNCTION_TYPE2(dlog_print, DLOG_ERROR, STARFISH_NAME, "", "", fmt, \
                       ##arg)
#elif defined(STARFISH_ANDROID)
#include <android/log.h>
#define STARFISH_LOG_ERROR(fmt, arg...)                                       \
    LOG_FUNCTION_TYPE2(__android_log_print, ANDROID_LOG_ERROR, STARFISH_NAME, \
                       "", "", fmt, ##arg)

#elif defined(STARFISH_WINDOWS)
#define STARFISH_LOG_ERROR(...) \
    ::Starfish::forwardPrintingLogError(__VA_ARGS__);
#else
#define STARFISH_LOG_ERROR(fmt, arg...)                                    \
    do {                                                                   \
        LOG_FUNCTION_TYPE(fprintf, stderr, STARFISH_LOG_TAG, "\033[0;31m", \
                          "\033[0m", fmt "\n", ##arg)                      \
    } while (0);
#endif

#if defined(STARFISH_TIZEN) && !defined(STARFISH_ENABLE_TEST)
#include <dlog.h>
#define STARFISH_LOG_WARN(fmt, arg...) \
    LOG_FUNCTION_TYPE2(dlog_print, DLOG_WARN, STARFISH_NAME, "", "", fmt, ##arg)
#elif defined(STARFISH_ANDROID)
#include <android/log.h>
#define STARFISH_LOG_WARN(fmt, arg...)                                       \
    LOG_FUNCTION_TYPE2(__android_log_print, ANDROID_LOG_WARN, STARFISH_NAME, \
                       "", "", fmt, ##arg)
#elif defined(STARFISH_WINDOWS)
#define STARFISH_LOG_WARN(...) ::Starfish::forwardPrintingLogWarn(__VA_ARGS__);
#else
#define STARFISH_LOG_WARN(fmt, arg...)                                     \
    do {                                                                   \
        LOG_FUNCTION_TYPE(fprintf, stderr, STARFISH_LOG_TAG, "\033[0;33m", \
                          "\033[0m", fmt "\n", ##arg)                      \
    } while (0);

#endif

#if defined(STARFISH_TIZEN) && !defined(STARFISH_ENABLE_TEST)
#include <dlog.h>
#define STARFISH_LOG_DEBUG(fmt, arg...)                                    \
    LOG_FUNCTION_TYPE2(dlog_print, DLOG_DEBUG, STARFISH_NAME, "", "", fmt, \
                       ##arg)
#elif defined(STARFISH_ANDROID)
#include <android/log.h>
#define STARFISH_LOG_DEBUG(fmt, arg...)                                       \
    LOG_FUNCTION_TYPE2(__android_log_print, ANDROID_LOG_DEBUG, STARFISH_NAME, \
                       "", "", fmt, ##arg)
#elif defined(STARFISH_WINDOWS)
#define STARFISH_LOG_DEBUG(...) ::Starfish::forwardPrintingLogWarn(__VA_ARGS__);
#else
#define STARFISH_LOG_DEBUG(fmt, arg...)                                        \
    do {                                                                       \
        LOG_FUNCTION_TYPE(fprintf, stderr, STARFISH_LOG_TAG, "", "", fmt "\n", \
                          ##arg)                                               \
    } while (0);
#endif

#define STARFISH_CRASH STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE

#if defined(NDEBUG)
#define STARFISH_ASSERT(assertion) ((void)0)
#define STARFISH_ASSERT_NOT_REACHED() ((void)0)
#define STARFISH_ASSERT_STATIC(assertion, reason)
#else
#define STARFISH_ASSERT(assertion) assert(assertion);
#define STARFISH_ASSERT_NOT_REACHED() \
    do {                              \
        assert(false);                \
    } while (0)
#define STARFISH_ASSERT_STATIC(assertion, reason) \
    static_assert(assertion, reason)
#endif

#define STARFISH_ASSERT_UNUSED(variable, assertion) STARFISH_ASSERT(assertion)

/* COMPILE_ASSERT */
#ifndef STARFISH_COMPILE_ASSERT
#define STARFISH_COMPILE_ASSERT(exp, name) static_assert((exp), #name)
#endif

#define STARFISH_RELEASE_ASSERT(assertion)        \
    do {                                          \
        if (!(assertion)) {                       \
            STARFISH_LOG_ERROR("RELEASE_ASSERT"); \
            ::abort();                            \
        }                                         \
    } while (0);
#define STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE()      \
    do {                                                  \
        STARFISH_LOG_ERROR("RELEASE_ASSERT_NOT_REACHED"); \
        ::abort();                                        \
    } while (0)

#define RETURN_FALSE_IF_FAILED(condition)          \
    do {                                           \
        if (condition == false) {                  \
            STARFISH_LOG_WARN("Fail " #condition); \
            return false;                          \
        }                                          \
    } while (0);

#define STARFISH_UNIMPLEMENTED(...)                       \
    do {                                                  \
        STARFISH_LOG_WARN("UNIMPLEMENTED: " __VA_ARGS__); \
    } while (0)

#define STARFISH_UNSUPPORTED(...)                      \
    do {                                               \
        STARFISH_LOG_WARN("Unsupported " __VA_ARGS__); \
    } while (0)

#define STARFISH_UNSUPPORTED_METHOD()                           \
    do {                                                        \
        STARFISH_UNSUPPORTED("method %s", __PRETTY_FUNCTION__); \
    } while (0)

#define DEFINE_GETTER(Type, MemberName) \
    Type MemberName() const             \
    {                                   \
        return m_##MemberName;          \
    }

#define DEFINE_SETTER(Type, MemberName, FaceName) \
    void set##FaceName(Type value)                \
    {                                             \
        m_##MemberName = value;                   \
    }

#define DEFINE_SETTER_WITH_HASFLAG(Type, MemberName, FaceName) \
    void set##FaceName(Type value)                             \
    {                                                          \
        m_##MemberName = value;                                \
        m_has##FaceName = true;                                \
    }

#define DEFINE_HASFLAG_GETTER(FaceName) \
    bool has##FaceName() const          \
    {                                   \
        return m_has##FaceName;         \
    }

#define DEFINE_GETTER_SETTER(Type, MemberName, FaceName) \
    DEFINE_GETTER(Type, MemberName)                      \
    DEFINE_SETTER(Type, MemberName, FaceName)

#define DEFINE_GETTER_SETTER_WITH_HASFLAG(Type, MemberName, FaceName) \
    DEFINE_GETTER(Type, MemberName)                                   \
    DEFINE_SETTER_WITH_HASFLAG(Type, MemberName, FaceName)            \
    DEFINE_HASFLAG_GETTER(FaceName)

#define DEFINE_MEMBER_WITH_HASFLAG(Type, MemberName, FaceName) \
    Type m_##MemberName;                                       \
    bool m_has##FaceName = false;

#define STARFISH_MAKE_STACK_ALLOCATED()              \
    inline void* operator new(size_t size) = delete; \
    inline void* operator new(size_t size, void* p) = delete;

#if !defined(WARN_UNUSED_RETURN) && defined(COMPILER_GCC)
#define WARN_UNUSED_RETURN __attribute__((__warn_unused_result__))
#endif

#if !defined(WARN_UNUSED_RETURN)
#define WARN_UNUSED_RETURN
#endif

#ifndef NO_EXPOSE_GC

#define ALLOCA(bytes, typenameWithoutPointer)                      \
    (typenameWithoutPointer*)(LIKELY(bytes < 4096) ? alloca(bytes) \
                                                   : GC_MALLOC(bytes))

enum NullOptionType { NullOption };

template <typename T>
struct Optional : public gc {
public:
    Optional()
        : m_value()
        , m_hasValue(false)
    {
    }

    Optional(T value)
        : m_value(value)
        , m_hasValue(true)
    {
    }

    Optional(std::nullptr_t value)
        : m_value()
        , m_hasValue(false)
    {
    }

    Optional(NullOptionType)
        : m_value()
        , m_hasValue(false)
    {
    }

    T& value()
    {
        STARFISH_ASSERT(m_hasValue);
        return m_value;
    }

    const T& value() const
    {
        STARFISH_ASSERT(m_hasValue);
        return m_value;
    }

    T valueOr(T defaultValue)
    {
        if (m_hasValue) {
            return m_value;
        }
        return defaultValue;
    }

    const T valueOr(T defaultValue) const
    {
        if (m_hasValue) {
            return m_value;
        }
        return defaultValue;
    }

    T& getValue()
    {
        STARFISH_ASSERT(m_hasValue);
        return m_value;
    }

    const T getValue() const
    {
        STARFISH_ASSERT(m_hasValue);
        return m_value;
    }

    bool hasValue() const
    {
        return m_hasValue;
    }

    void reset()
    {
        *this = Optional<T>();
    }

    operator bool() const
    {
        return m_hasValue;
    }

    bool operator==(const Optional<T>& other) const
    {
        if (m_hasValue != other.hasValue()) {
            return false;
        }
        return m_hasValue ? m_value == other.m_value : true;
    }

    bool operator!=(const Optional<T>& other) const
    {
        return !this->operator==(other);
    }

    bool operator==(const T& other) const
    {
        if (m_hasValue) {
            return getValue() == other;
        }
        return false;
    }

    bool operator!=(const T& other) const
    {
        return !operator==(other);
    }

protected:
    T m_value;
    bool m_hasValue;
};

template <typename T>
inline bool operator==(const T& a, const Optional<T>& b)
{
    return b == a;
}

template <typename T>
inline bool operator!=(const T& a, const Optional<T>& b)
{
    return b != a;
}

template <typename T>
class Optional<T*> : public gc {
public:
    Optional()
        : m_value(nullptr)
    {
    }

    Optional(T* value)
        : m_value(value)
    {
    }

    Optional(std::nullptr_t value)
        : m_value(nullptr)
    {
    }

    Optional(NullOptionType)
        : m_value(nullptr)
    {
    }

    T* value() const
    {
        STARFISH_ASSERT(hasValue());
        return m_value;
    }

    T* valueOrNull() const
    {
        if (hasValue()) {
            return m_value;
        }
        return nullptr;
    }

    T* valueOr(T* defaultValue) const
    {
        if (hasValue()) {
            return m_value;
        }
        return defaultValue;
    }

    T* getValue() const
    {
        STARFISH_ASSERT(hasValue());
        return m_value;
    }

    bool hasValue() const
    {
        return !!m_value;
    }

    void reset()
    {
        *this = Optional<T*>();
    }

    operator bool() const
    {
        return hasValue();
    }

    T* operator->()
    {
        STARFISH_ASSERT(hasValue());
        return m_value;
    }

    T* operator->() const
    {
        STARFISH_ASSERT(hasValue());
        return m_value;
    }

    const T& operator*() const
    {
        STARFISH_ASSERT(hasValue());
        return *m_value;
    }

    T& operator*()
    {
        STARFISH_ASSERT(hasValue());
        return *m_value;
    }

    bool operator==(const Optional<T*>& other) const
    {
        if (hasValue() != other.hasValue()) {
            return false;
        }
        return hasValue() ? m_value == other.m_value : true;
    }

    bool operator!=(const Optional<T*>& other) const
    {
        return !this->operator==(other);
    }

    bool operator==(const T*& other) const
    {
        if (hasValue()) {
            return value() == other;
        }
        return false;
    }

    bool operator!=(const T*& other) const
    {
        return !operator==(other);
    }

protected:
    T* m_value;
};

template <typename T>
inline bool operator==(const T*& a, const Optional<T*>& b)
{
    return b == a;
}

template <typename T>
inline bool operator!=(const T*& a, const Optional<T*>& b)
{
    return b != a;
}
#endif // NO_EXPOSE_GC

class StorePositiveIntergerAsOdd {
public:
    StorePositiveIntergerAsOdd()
        : StorePositiveIntergerAsOdd(0)
    {
    }

    StorePositiveIntergerAsOdd(const size_t& src)
    {
        STARFISH_ASSERT(src < std::numeric_limits<size_t>::max() / 2);
        m_data = (src << 1) | 0x1;
    }

    operator size_t() const
    {
        return m_data >> 1;
    }

private:
    size_t m_data;
};

#ifndef NO_EXPOSE_GC

#include "core/util/Vector.h"
#include "core/util/TightVector.h"

// typedef of GC-aware vector
template <typename T, typename Allocator = GCUtil::gc_malloc_allocator<T>>
using GCVectorT = Starfish::Vector<T, Allocator>;

template <typename T, typename Allocator = GCUtil::gc_malloc_allocator<T>>
class GCVector : public GCVectorT<T, Allocator>, public gc {
};

template <typename T, typename Allocator = GCUtil::gc_malloc_allocator<T>>
using GCTightVectorT = Starfish::TightVector<T, Allocator>;

template <typename T, typename Allocator = GCUtil::gc_malloc_allocator<T>>
class GCTightVector : public GCTightVectorT<T, Allocator>, public gc {
};

// typedef of GC-aware vector with atomic contents
template <typename T,
          typename Allocator = GCUtil::gc_malloc_atomic_allocator<T>>
using GCAtomicVectorT = Starfish::Vector<T, Allocator>;

template <typename T,
          typename Allocator = GCUtil::gc_malloc_atomic_allocator<T>>
class GCAtomicVector : public GCAtomicVectorT<T, Allocator>, public gc {
};

template <typename T,
          typename Allocator = GCUtil::gc_malloc_atomic_allocator<T>>
using GCAtomicTightVectorT = Starfish::TightVector<T, Allocator>;

template <typename T,
          typename Allocator = GCUtil::gc_malloc_atomic_allocator<T>>
class GCAtomicTightVector : public GCAtomicTightVectorT<T, Allocator>,
                            public gc {
};

template <class Key, class T, class Hash = std::hash<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<Key, T>>,
          bool StoreHash = false,
          class GrowthPolicy = tsl::rh::power_of_two_growth_policy<2>>
using HashMap =
    tsl::robin_map<Key, T, Hash, KeyEqual, Allocator, StoreHash, GrowthPolicy>;

// typedef of GC-aware unordered_map
template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename Predicate = std::equal_to<Key>,
          typename Allocator =
              GCUtil::gc_malloc_allocator<std::pair<Key const, Value>>>
using GCUnorderedMapT = HashMap<Key, Value, Hasher, Predicate, Allocator>;

template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename Predicate = std::equal_to<Key>,
          typename Allocator =
              GCUtil::gc_malloc_allocator<std::pair<Key const, Value>>>
class GCUnorderedMap
    : public GCUnorderedMapT<Key, Value, Hasher, Predicate, Allocator>,
      public gc {
};

template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename Predicate = std::equal_to<Key>,
          typename Allocator =
              GCUtil::gc_malloc_atomic_allocator<std::pair<Key const, Value>>>
using GCAtomicUnorderedMapT = HashMap<Key, Value, Hasher, Predicate, Allocator>;

template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename Predicate = std::equal_to<Key>,
          typename Allocator =
              GCUtil::gc_malloc_atomic_allocator<std::pair<Key const, Value>>>
class GCAtomicUnorderedMap
    : public GCAtomicUnorderedMapT<Key, Value, Hasher, Predicate, Allocator>,
      public gc {
};

template <class Key, class Hash = std::hash<Key>,
          class KeyEqual = std::equal_to<Key>,
          class Allocator = std::allocator<Key>, bool StoreHash = false,
          class GrowthPolicy = tsl::rh::power_of_two_growth_policy<2>>
using HashSet =
    tsl::robin_set<Key, Hash, KeyEqual, Allocator, StoreHash, GrowthPolicy>;

// typedef of GC-aware unordered_set
template <typename T, typename Hasher = std::hash<T>,
          typename Predicate = std::equal_to<T>,
          typename Allocator = GCUtil::gc_malloc_allocator<T>>
class GCUnorderedSet : public HashSet<T, Hasher, Predicate, Allocator>,
                       public gc {
};

#endif // NO_EXPOSE_GC

inline void markHashTable(GC_word* desc, size_t base)
{
#if defined(COMPILER_MSVC) || defined(COMPILER_CLANG_CL)
    GC_set_bit(desc, base + 2); // m_ht.m_buckets_data
    GC_set_bit(desc, base + 5); // m_ht.m_buckets
#else
    GC_set_bit(desc, base + 1); // m_ht.m_buckets_data
    GC_set_bit(desc, base + 4); // m_ht.m_buckets
#endif
}

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T1, typename T2>
inline T2 narrow_cast(T1 v)
{
    static_assert(std::numeric_limits<T2>::max() <
                      std::numeric_limits<T1>::max(),
                  "Wrong type cast");
    return static_cast<T2>(
        v % (static_cast<T1>(std::numeric_limits<T2>::max()) + 1));
}

template <typename Target, typename Source>
inline Target downcast(Source* source)
{
#if !defined(NDEBUG)
    STARFISH_ASSERT(source != nullptr);
    typedef typename std::remove_pointer<Target>::type TargetType;
    static_assert(std::is_base_of<Source, TargetType>::value == true,
                  "Wrong type cast");
#endif

#if !defined(NDEBUG) && (defined(__GXX_RTTI) || defined(_CPPRTTI))
    auto casted = dynamic_cast<Target>(source);
    STARFISH_ASSERT(casted != nullptr);
    return casted;
#else
    return static_cast<Target>(source);
#endif
}

// if c++14 or above is not supported
#if __cplusplus < 201402L || (defined(_MSC_VER) && _MSC_VER < 1915)
namespace std {
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
} // namespace std
#endif

template <typename Target>
inline Target castTo(void* source)
{
    STARFISH_ASSERT(source != nullptr);
    return static_cast<Target>(source);
}

template <typename Type>
inline bool isInfOrNan(Type value)
{
    const double v = static_cast<double>(value);
    return std::isinf(v) || std::isnan(v);
}

inline bool isInfOrNan(float value)
{
    return std::isinf(value) || std::isnan(value);
}

class OnScopeLeave {
public:
    using Function = std::function<void()>;

    explicit OnScopeLeave(Function&& function)
        : m_function(std::move(function))
    {
    }

    OnScopeLeave(OnScopeLeave&& other)
        : m_function(std::move(other.m_function))
    {
        other.m_function = nullptr;
    }

    OnScopeLeave(const OnScopeLeave& other) = delete;
    OnScopeLeave& operator=(const OnScopeLeave& other) = delete;

    ~OnScopeLeave()
    {
        if (m_function) {
            m_function();
        }
    }

    static WARN_UNUSED_RETURN OnScopeLeave create(Function&& function)
    {
        return OnScopeLeave(std::move(function));
    }

private:
    Function m_function;
};

#if defined(PORT_PIXEL_ORDER_RGBA)
#define STARFISH_PIXEL_R_INDEX 0
#define STARFISH_PIXEL_G_INDEX 1
#define STARFISH_PIXEL_B_INDEX 2
#define STARFISH_PIXEL_A_INDEX 3
#elif defined(PORT_PIXEL_ORDER_BGRA)
#define STARFISH_PIXEL_R_INDEX 2
#define STARFISH_PIXEL_G_INDEX 1
#define STARFISH_PIXEL_B_INDEX 0
#define STARFISH_PIXEL_A_INDEX 3
#endif

template <class T>
constexpr const T& clamp(const T& value, const T& low, const T& high)
{
    return (value < low) ? low : (high < value) ? high : value;
}

#endif
