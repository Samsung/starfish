/*
 * Copyright (c) 2015-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __StarFishConfig__
#define __StarFishConfig__

#if defined(STARFISH_EFL)
#define PORT_GRAPHIC_BACKEND_EFL
#define PORT_CANVAS_BACKEND_EFL
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_COMPOSITOR_BACKEND_EFL
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_EFL
#elif defined(STARFISH_EFL_CAIRO)
#define PORT_GRAPHIC_BACKEND_EFL_CAIRO
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_COMPOSITOR_BACKEND_EFL
#define PORT_EVENTLOOP_BACKEND_EFL
#define PORT_IMAGEDECODER_BACKEND_MISC
#elif defined(STARFISH_DALI)
#define PORT_GRAPHIC_BACKEND_GENERAL_BUFFER
#define PORT_CANVAS_BACKEND_CAIRO
#define PORT_EVENTLOOP_BACKEND_LIBUV
#define PORT_IMAGEDECODER_BACKEND_MISC
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

#include <GCUtil.h>

template <const int siz>
inline void __attribute__((optimize("O0"))) clearStack()
{
    volatile char a[siz] = { 0 };
}

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

/* COMPILER() - the compiler being used to build the project */
#define COMPILER(FEATURE) (defined COMPILER_##FEATURE && COMPILER_##FEATURE)

/* COMPILER(MSVC) - Microsoft Visual C++ */
#if defined(_MSC_VER)
#define COMPILER_MSVC 1

/* Specific compiler features */
#if !COMPILER(CLANG) && _MSC_VER >= 1600
#define COMPILER_SUPPORTS_CXX_NULLPTR 1
#endif

#if COMPILER(CLANG)
/* Keep strong enums turned off when building with clang-cl: We cannot yet build
 * all of Blink without fallback to cl.exe, and strong enums are exposed at ABI
 * boundaries. */
#undef COMPILER_SUPPORTS_CXX_STRONG_ENUMS
#else
#define COMPILER_SUPPORTS_CXX_OVERRIDE_CONTROL 1
#define COMPILER_QUIRK_FINAL_IS_CALLED_SEALED 1
#endif

#endif

/* COMPILER(GCC) - GNU Compiler Collection */
#if defined(__GNUC__)
#define COMPILER_GCC 1
#define GCC_VERSION \
    (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define GCC_VERSION_AT_LEAST(major, minor, patch) \
    (GCC_VERSION >= (major * 10000 + minor * 100 + patch))
#else
/* Define this for !GCC compilers, just so we can write things like
 * GCC_VERSION_AT_LEAST(4, 1, 0). */
#define GCC_VERSION_AT_LEAST(major, minor, patch) 0
#endif

/* ALWAYS_INLINE */
#ifndef ALWAYS_INLINE
#if COMPILER(GCC) && defined(NDEBUG) && !COMPILER(MINGW)
#define ALWAYS_INLINE inline
#elif COMPILER(MSVC) && defined(NDEBUG)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif
#endif

/* NEVER_INLINE */
#ifndef NEVER_INLINE
#if COMPILER(GCC)
#define NEVER_INLINE __attribute__((__noinline__))
#else
#define NEVER_INLINE
#endif
#endif

/* UNLIKELY */
#ifndef UNLIKELY
#if COMPILER(GCC)
#define UNLIKELY(x) __builtin_expect((x), 0)
#else
#define UNLIKELY(x) (x)
#endif
#endif

/* LIKELY */
#ifndef LIKELY
#if COMPILER(GCC)
#define LIKELY(x) __builtin_expect((x), 1)
#else
#define LIKELY(x) (x)
#endif
#endif

/* NO_RETURN */
#ifndef NO_RETURN
#if COMPILER(GCC)
#define NO_RETURN __attribute((__noreturn__))
#elif COMPILER(MSVC)
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN
#endif
#endif

#if !COMPILER(GCC)
#include <codecvt>
#endif

#if INTPTR_MAX == INT32_MAX
#define STARFISH_32
#elif INTPTR_MAX == INT64_MAX
#define STARFISH_64
#else
#error "Environment not 32 or 64-bit."
#endif

#define STARFISH_LOG_INFO(...) fprintf(stdout, __VA_ARGS__);
#ifdef STARFISH_TIZEN
#undef STARFISH_LOG_INFO
#include <dlog.h>
#define STARFISH_LOG_INFO(...) \
    dlog_print(DLOG_INFO, STARFISH_NAME, __VA_ARGS__);
#endif

#define STARFISH_LOG_ERROR(...) fprintf(stderr, __VA_ARGS__);
#ifdef STARFISH_TIZEN
#undef STARFISH_LOG_ERROR
#include <dlog.h>
#define STARFISH_LOG_ERROR(...) \
    dlog_print(DLOG_ERROR, STARFISH_NAME, __VA_ARGS__);
#endif

#define STARFISH_LOG_WARN(...) fprintf(stderr, __VA_ARGS__);
#ifdef STARFISH_TIZEN
#undef STARFISH_LOG_WARN
#include <dlog.h>
#define STARFISH_LOG_WARN(...) \
    dlog_print(DLOG_WARN, STARFISH_NAME, __VA_ARGS__);
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

#if !defined(WARN_UNUSED_RETURN) && COMPILER(GCC)
#define WARN_UNUSED_RETURN __attribute__((__warn_unused_result__))
#endif

#if !defined(WARN_UNUSED_RETURN)
#define WARN_UNUSED_RETURN
#endif

#define ALLOCA(bytes, typenameWithoutPointer)                     \
    (typenameWithoutPointer*)(LIKELY(bytes < 512) ? alloca(bytes) \
                                                  : GC_MALLOC(bytes))

#define APP_NAME "Netscape"
#define APP_CODE_NAME "Mozilla"
#define PRODUCT_NAME "Gecko"
#define STARFISH_NAME "StarFish"
#define VERSION "0.1.0"
#define USER_AGENT(STARFISH_NAME, VERSION) \
    "Mozilla/5.0 (like Gecko/20100101 Firefox/36.0) " STARFISH_NAME "/" VERSION
#define USER_AGENT_MAXIMUM_DATE_VALUE 8.64e15
#define VENDOR_NAME "Samsung Electronics Co., Ltd."

#include "StarFishExport.h"

template <typename T>
struct Nullable {
public:
    Nullable()
        : m_hasValue(false)
    {
    }

    Nullable(T value)
        : m_hasValue(true)
        , m_value(value)
    {
    }

    Nullable(nullptr_t value)
        : m_hasValue(false)
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
template <typename T, typename Allocator = gc_allocator_ignore_off_page<T>>
using GCVectorT = StarFish::Vector<T, Allocator>;

template <typename T, typename Allocator = gc_allocator_ignore_off_page<T>>
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
template <typename T, typename Allocator = gc_allocator_ignore_off_page<T>>
using GCListT = std::list<T, Allocator>;

template <typename T, typename Allocator = gc_allocator_ignore_off_page<T>>
class GCList : public GCListT<T, Allocator>, public gc {
};

// typedef of GC-aware deque
template <typename T, typename Allocator = gc_allocator_ignore_off_page<T>>
using GCDequeT = std::deque<T, Allocator>;

template <typename T, typename Allocator = gc_allocator_ignore_off_page<T>>
class GCDeque : public GCDequeT<T, Allocator>, public gc {
};

// typedef of GC-aware unordered_map
template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename Predicate = std::equal_to<Key>,
          typename Allocator =
              gc_allocator_ignore_off_page<std::pair<Key, Value>>>
using GCUnorderedMap =
    std::unordered_map<Key, Value, Hasher, Predicate, Allocator>;

template <typename Key, typename Value, typename Hasher = std::hash<Key>,
          typename Predicate = std::equal_to<Key>,
          typename Allocator =
              gc_allocator_ignore_off_page<std::pair<Key, Value>>>
using GCUnorderedMultiMap =
    std::unordered_multimap<Key, Value, Hasher, Predicate, Allocator>;

// typedef of GC-aware map
template <typename Key, typename Value, typename Comparator,
          typename Allocator =
              gc_allocator_ignore_off_page<std::pair<Key, Value>>>
using GCMap = std::map<Key, Value, Comparator, Allocator>;

// typedef of GC-aware unordered_set
template <typename T, typename Hasher = std::hash<T>,
          typename Predicate = std::equal_to<T>,
          typename Allocator = gc_allocator_ignore_off_page<T>>
using GCUnorderedSet = std::unordered_set<T, Hasher, Predicate, Allocator>;

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
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
