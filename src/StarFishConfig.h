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
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __StarFishConfig__
#define __StarFishConfig__

#if defined(STARFISH_EFL)
#define PORT_WINDOW_BACKEND_EFL
#define PORT_GRAPHIC_BACKEND_EFL
#define PORT_CANVAS_BACKEND_EFL
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_COMPOSITOR_BACKEND_EFL
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_EFL
#define PORT_PIXEL_ORDER_BGRA
#elif defined(STARFISH_EFL_CAIRO)
#define PORT_WINDOW_BACKEND_EFL
#define PORT_GRAPHIC_BACKEND_EFL_CAIRO
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MISC
#if defined(STARFISH_ENABLE_TEST)
#define PORT_COMPOSITOR_BACKEND_EFL
#define PORT_PIXEL_ORDER_BGRA
#else
#define PORT_COMPOSITOR_BACKEND_GL
#define PORT_PIXEL_ORDER_RGBA
#endif
#elif defined(STARFISH_EFL_CAIRO_HEADLESS)
#define PORT_WINDOW_BACKEND_EFL_HEADLESS
#define PORT_GRAPHIC_BACKEND_MOCK
#define PORT_CANVAS_BACKEND_MOCK
#define PORT_COMPOSITOR_BACKEND_MOCK
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MOCK
#define PORT_PIXEL_ORDER_BGRA
#elif defined(STARFISH_EFL_SKIA)
#define PORT_GRAPHIC_BACKEND_EFL_SKIA
#define PORT_WINDOW_BACKEND_EFL
#define PORT_CANVAS_BACKEND_SKIA
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_COMPOSITOR_BACKEND_EFL
#define PORT_PIXEL_ORDER_BGRA
#define SK_SAMPLES_FOR_X
#elif defined(STARFISH_GLFW_CAIRO_GL)
#define PORT_WINDOW_BACKEND_GLFW
#define PORT_WINDOW_BACKEND_GL
#define PORT_GRAPHIC_BACKEND_GLFW_CAIRO
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_COMPOSITOR_BACKEND_GL
#define PORT_PIXEL_ORDER_RGBA
#elif defined(STARFISH_DALI)
#define PORT_WINDOW_BACKEND_GB
#define PORT_GRAPHIC_BACKEND_GENERAL_BUFFER
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_COMPOSITOR_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_LIBUV
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#elif defined(STARFISH_TIZEN_WEARABLE_WIDGET)
#define PORT_WINDOW_BACKEND_EFL
#define PORT_GRAPHIC_BACKEND_EFL
#define PORT_CANVAS_BACKEND_EFL
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_COMPOSITOR_BACKEND_EFL
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_EFL
#define PORT_PIXEL_ORDER_BGRA
#elif defined(STARFISH_ANDROID)
#define PORT_WINDOW_BACKEND_GB
#define PORT_GRAPHIC_BACKEND_GENERAL_BUFFER
#define PORT_CANVAS_BACKEND_SKIA
#define PORT_COMPOSITOR_BACKEND_SKIA
#define PORT_EVENTLOOP_BACKEND_ANDROID
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_RGBA
#elif defined(STARFISH_WINDOWS)
#define PORT_WINDOW_BACKEND_GB
#define PORT_GRAPHIC_BACKEND_GENERAL_BUFFER
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_COMPOSITOR_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_WINDOWS
#define PORT_IMAGEDECODER_BACKEND_MISC
#define PORT_PIXEL_ORDER_BGRA
#endif

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

#if defined(COMPILER_MSVC)
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#ifndef NDEBUG
#define _ITERATOR_DEBUG_LEVEL 0
#endif
#define rand_r(x) rand()
#endif

#if defined(COMPILER_MSVC)
#define ENSURE_ENUM_UNSIGNED : unsigned int
#else
#define ENSURE_ENUM_UNSIGNED
#endif

#ifdef _WIN32
#define OS_WINDOWS 1
#elif _WIN64
#define OS_WINODWS 1
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
#define _INC_CTYPE // for preventing include windows version of ctype header
#include <inttypes.h>
typedef unsigned int uint;
#endif

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

#ifndef ESCARGOT
#define ESCARGOT // for use additional functions in GCutil
#endif
#include <GCUtil.h>
#undef ESCARGOT

#include <SkMatrix.h>

#include <unicode/locid.h>
#include <unicode/brkiter.h>
#include <unicode/ubidi.h>
#include <unicode/uchar.h>
#include <unicode/ucnv.h>
#include <unicode/ucsdet.h>
#include <unicode/uscript.h>
#include <unicode/rbbi.h>

#include <pthread.h>
#include <semaphore.h>

#include <curl/curl.h>

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

#if INTPTR_MAX == INT32_MAX
#define STARFISH_32
#elif INTPTR_MAX == INT64_MAX
#define STARFISH_64
#else
#error "Environment not 32 or 64-bit."
#endif

#if defined(OS_WINDOWS)
namespace StarFish {
void forwardPrintingLogInfo(const char* fmt, ...);
void forwardPrintingLogError(const char* fmt, ...);
void forwardPrintingLogWarn(const char* fmt, ...);
const char* getWindowsTempDir();
}
#endif

#define STARFISH_LOG_INFO(...) fprintf(stdout, __VA_ARGS__);
#ifdef STARFISH_TIZEN
#undef STARFISH_LOG_INFO
#include <dlog.h>
#define STARFISH_LOG_INFO(...) \
    dlog_print(DLOG_INFO, STARFISH_NAME, __VA_ARGS__);
#endif
#ifdef STARFISH_ANDROID
#include <android/log.h>
#undef STARFISH_LOG_INFO
#define STARFISH_LOG_INFO(...) \
    __android_log_print(ANDROID_LOG_INFO, STARFISH_NAME, __VA_ARGS__);
#endif
#ifdef STARFISH_WINDOWS
#undef STARFISH_LOG_INFO
#define STARFISH_LOG_INFO(...) ::StarFish::forwardPrintingLogInfo(__VA_ARGS__);
#endif

#define STARFISH_LOG_ERROR(...) fprintf(stderr, __VA_ARGS__);
#ifdef STARFISH_TIZEN
#undef STARFISH_LOG_ERROR
#include <dlog.h>
#define STARFISH_LOG_ERROR(...) \
    dlog_print(DLOG_ERROR, STARFISH_NAME, __VA_ARGS__);
#endif
#ifdef STARFISH_ANDROID
#include <android/log.h>
#undef STARFISH_LOG_ERROR
#define STARFISH_LOG_ERROR(...) \
    __android_log_print(ANDROID_LOG_ERROR, STARFISH_NAME, __VA_ARGS__);
#endif
#ifdef STARFISH_WINDOWS
#undef STARFISH_LOG_ERROR
#define STARFISH_LOG_ERROR(...) \
    ::StarFish::forwardPrintingLogError(__VA_ARGS__);
#endif

#define STARFISH_LOG_WARN(...) fprintf(stderr, __VA_ARGS__);
#ifdef STARFISH_TIZEN
#undef STARFISH_LOG_WARN
#include <dlog.h>
#define STARFISH_LOG_WARN(...) \
    dlog_print(DLOG_WARN, STARFISH_NAME, __VA_ARGS__);
#endif
#ifdef STARFISH_ANDROID
#include <android/log.h>
#undef STARFISH_LOG_WARN
#define STARFISH_LOG_WARN(...) \
    __android_log_print(ANDROID_LOG_WARN, STARFISH_NAME, __VA_ARGS__);
#endif
#ifdef STARFISH_WINDOWS
#undef STARFISH_LOG_WARN
#define STARFISH_LOG_WARN(...) ::StarFish::forwardPrintingLogWarn(__VA_ARGS__);
#endif

#define STARFISH_CRASH STARFISH_RELEASE_ASSERT_NOT_REACHED

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

#define STARFISH_RELEASE_ASSERT(assertion)                              \
    do {                                                                \
        if (!(assertion)) {                                             \
            STARFISH_LOG_ERROR("RELEASE_ASSERT at %s (%d)\n", __FILE__, \
                               __LINE__);                               \
            ::abort();                                                  \
        }                                                               \
    } while (0);
#define STARFISH_RELEASE_ASSERT_NOT_REACHED()                         \
    do {                                                              \
        STARFISH_LOG_ERROR("RELEASE_ASSERT_NOT_REACHED at %s (%d)\n", \
                           __FILE__, __LINE__);                       \
        ::abort();                                                    \
    } while (0)

#ifdef STARFISH_ENABLE_TEST
#define STARFISH_RELEASE_ASSERT_UNIMPLEMENTED()                      \
    do {                                                             \
        STARFISH_LOG_ERROR(                                          \
            "STARFISH_RELEASE_ASSERT_UNIMPLEMENTED at %s (%s:%d)\n", \
            __PRETTY_FUNCTION__, __FILE__, __LINE__);                \
    } while (0)

#define STARFISH_BINDING_ASSERT_UNIMPLEMENTED(...)                   \
    do {                                                             \
        STARFISH_LOG_ERROR(                                          \
            "STARFISH_BINDING_ASSERT_UNIMPLEMENTED at %s (%s:%d)\n", \
            __PRETTY_FUNCTION__, __FILE__, __LINE__);                \
        STARFISH_LOG_ERROR(__VA_ARGS__);                             \
    } while (0)
#else
#define STARFISH_RELEASE_ASSERT_UNIMPLEMENTED()
#define STARFISH_BINDING_ASSERT_UNIMPLEMENTED(...)
#endif

#define STARFISH_MAKE_STACK_ALLOCATED()              \
    inline void* operator new(size_t size) = delete; \
    inline void* operator new(size_t size, void* p) = delete;

#if !defined(WARN_UNUSED_RETURN) && defined(COMPILER_GCC)
#define WARN_UNUSED_RETURN __attribute__((__warn_unused_result__))
#endif

#if !defined(WARN_UNUSED_RETURN)
#define WARN_UNUSED_RETURN
#endif

#define ALLOCA(bytes, typenameWithoutPointer)                      \
    (typenameWithoutPointer*)(LIKELY(bytes < 4096) ? alloca(bytes) \
                                                   : GC_MALLOC(bytes))

#define APP_NAME "Netscape"
#define APP_CODE_NAME "Mozilla"
#define PRODUCT_NAME "Gecko"
#define STARFISH_NAME "StarFish"
#define VERSION "0.1.0"
#define USER_AGENT(STARFISH_NAME, VERSION) \
    "Mozilla/5.0 (like Gecko/54.0 Firefox/54.0) " STARFISH_NAME "/" VERSION
#define USER_AGENT_MAXIMUM_DATE_VALUE 8.64e15
#define VENDOR_NAME "Samsung Electronics Co., Ltd."

#include "StarFishExport.h"

template <typename T>
struct Nullable {
public:
    Nullable()
        : m_hasValue(false)
        , m_value()
    {
    }

    Nullable(T value)
        : m_hasValue(true)
        , m_value(value)
    {
    }

    Nullable(std::nullptr_t value)
        : m_hasValue(false)
        , m_value()
    {
    }

    T getValue()
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

    bool operator==(const Nullable<T>& other) const
    {
        if (m_hasValue != other.hasValue()) {
            return false;
        }
        return m_hasValue ? m_value == other.m_value : true;
    }

    bool operator!=(const Nullable<T>& other) const
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
    bool m_hasValue;
    T m_value;
};

template <typename T>
inline bool operator==(const T& a, const Nullable<T>& b)
{
    return b == a;
}

template <typename T>
inline bool operator!=(const T& a, const Nullable<T>& b)
{
    return b != a;
}

#include "core/util/Vector.h"

// typedef of GC-aware vector
template <typename T,
          typename Allocator = GCUtil::gc_malloc_ignore_off_page_allocator<T>>
using GCVectorT = StarFish::Vector<T, Allocator>;

template <typename T,
          typename Allocator = GCUtil::gc_malloc_ignore_off_page_allocator<T>>
class GCVector : public GCVectorT<T, Allocator>, public gc {
};

// typedef of GC-aware vector with atomic contents
template <typename T, typename Allocator =
                          GCUtil::gc_malloc_atomic_ignore_off_page_allocator<T>>
using GCAtomicVectorT = StarFish::Vector<T, Allocator>;

template <typename T, typename Allocator =
                          GCUtil::gc_malloc_atomic_ignore_off_page_allocator<T>>
class GCAtomicVector : public GCAtomicVectorT<T, Allocator>, public gc {
};

// typedef of GC-aware list
template <typename T,
          typename Allocator = GCUtil::gc_malloc_ignore_off_page_allocator<T>>
using GCListT = std::list<T, Allocator>;

template <typename T,
          typename Allocator = GCUtil::gc_malloc_ignore_off_page_allocator<T>>
class GCList : public GCListT<T, Allocator>, public gc {
};

// typedef of GC-aware deque
template <typename T,
          typename Allocator = GCUtil::gc_malloc_ignore_off_page_allocator<T>>
using GCDequeT = std::deque<T, Allocator>;

template <typename T,
          typename Allocator = GCUtil::gc_malloc_ignore_off_page_allocator<T>>
class GCDeque : public GCDequeT<T, Allocator>, public gc {
};

// typedef of GC-aware unordered_map
template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename Predicate = std::equal_to<Key>,
          typename Allocator = GCUtil::gc_malloc_ignore_off_page_allocator<
              std::pair<Key const, Value>>>
using GCUnorderedMap =
    std::unordered_map<Key, Value, Hasher, Predicate, Allocator>;

template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename Predicate = std::equal_to<Key>,
          typename Allocator = GCUtil::gc_malloc_ignore_off_page_allocator<
              std::pair<Key const, Value>>>
using GCUnorderedMultiMap =
    std::unordered_multimap<Key, Value, Hasher, Predicate, Allocator>;

// typedef of GC-aware map
template <typename Key, typename Value, typename Comparator,
          typename Allocator = GCUtil::gc_malloc_ignore_off_page_allocator<
              std::pair<Key const, Value>>>
using GCMap = std::map<Key, Value, Comparator, Allocator>;

// typedef of GC-aware unordered_set
template <typename T, typename Hasher = std::hash<T>,
          typename Predicate = std::equal_to<T>,
          typename Allocator = GCUtil::gc_malloc_ignore_off_page_allocator<T>>
using GCUnorderedSet = std::unordered_set<T, Hasher, Predicate, Allocator>;

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

#include "core/layout/LayoutUtil.h"
#include "core/util/String.h"
#include "core/util/AtomicString.h"
#include "core/util/QualifiedName.h"
#include "core/util/Messages.h"
#include "core/style/Length.h"
#include "core/style/Unit.h"
#include "core/style/UnitHelper.h"
#include "core/modules/canvas/font/Font.h"
#include "core/modules/profiling/Profiling.h"
#include "platform/loader/ResourceURL.h"

#endif
