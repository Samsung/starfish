#ifndef __StarFishConfig__
#define __StarFishConfig__

#include <cstdlib>
#include <cstdio>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <cstring>
#include <cassert>
#include <functional>
#include <algorithm>
#include <cmath>
#include <limits>
#include <locale>
#include <clocale>
#include <cwchar>

#include <gc_cpp.h>
#include <gc_allocator.h>

#include <SkMatrix.h>

#include <unicode/locid.h>
#include <unicode/brkiter.h>
#include <unicode/ubidi.h>
#include <unicode/uchar.h>


/* COMPILER() - the compiler being used to build the project */
#define COMPILER(FEATURE) (defined COMPILER_##FEATURE  && COMPILER_##FEATURE)


/* COMPILER(MSVC) - Microsoft Visual C++ */
#if defined(_MSC_VER)
#define COMPILER_MSVC 1

/* Specific compiler features */
#if !COMPILER(CLANG) && _MSC_VER >= 1600
#define COMPILER_SUPPORTS_CXX_NULLPTR 1
#endif

#if COMPILER(CLANG)
/* Keep strong enums turned off when building with clang-cl: We cannot yet build all of Blink without fallback to cl.exe, and strong enums are exposed at ABI boundaries. */
#undef COMPILER_SUPPORTS_CXX_STRONG_ENUMS
#else
#define COMPILER_SUPPORTS_CXX_OVERRIDE_CONTROL 1
#define COMPILER_QUIRK_FINAL_IS_CALLED_SEALED 1
#endif

#endif

/* COMPILER(GCC) - GNU Compiler Collection */
#if defined(__GNUC__)
#define COMPILER_GCC 1
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define GCC_VERSION_AT_LEAST(major, minor, patch) (GCC_VERSION >= (major * 10000 + minor * 100 + patch))
#else
/* Define this for !GCC compilers, just so we can write things like GCC_VERSION_AT_LEAST(4, 1, 0). */
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

#define STARFISH_LOG_INFO(...) fprintf(stdout, __VA_ARGS__);
#ifdef STARFISH_TIZEN_WEARABLE
#undef STARFISH_LOG_INFO
#include <dlog.h>
#define STARFISH_LOG_INFO(...) dlog_print(DLOG_INFO, "StarFish", __VA_ARGS__);
#endif

#define STARFISH_LOG_ERROR(...) fprintf(stderr, __VA_ARGS__);
#ifdef STARFISH_TIZEN_WEARABLE
#undef STARFISH_LOG_ERROR
#include <dlog.h>
#define STARFISH_LOG_ERROR(...) dlog_print(DLOG_ERROR, "StarFish", __VA_ARGS__);
#endif

#define STARFISH_CRASH STARFISH_RELEASE_ASSERT_NOT_REACHED

#if defined(NDEBUG)
#define STARFISH_ASSERT(assertion) ((void)0)
#define STARFISH_ASSERT_NOT_REACHED() ((void)0)
#define STARFISH_ASSERT_STATIC(assertion, reason)
#else
#define STARFISH_ASSERT(assertion) assert(assertion);
#define STARFISH_ASSERT_NOT_REACHED() do { assert(false); } while (0)
#define STARFISH_ASSERT_STATIC(assertion, reason) static_assert(assertion, reason)
#endif

/* COMPILE_ASSERT */
#ifndef STARFISH_COMPILE_ASSERT
#define STARFISH_COMPILE_ASSERT(exp, name) static_assert((exp), #name)
#endif

#define STARFISH_RELEASE_ASSERT(assertion) do { if (!(assertion)) { STARFISH_LOG_ERROR("RELEASE_ASSERT at %s (%d)\n", __FILE__, __LINE__); abort(); } } while (0);
#define STARFISH_RELEASE_ASSERT_NOT_REACHED() do { STARFISH_LOG_ERROR("RELEASE_ASSERT_NOT_REACHED at %s (%d)\n", __FILE__, __LINE__); abort(); } while (0)

#define STARFISH_MAKE_STACK_ALLOCATED() \
    inline void* operator new(size_t size) = delete; \
    inline void* operator new(size_t size, void* p) = delete;

#if !defined(WARN_UNUSED_RETURN) && COMPILER(GCC)
#define WARN_UNUSED_RETURN __attribute__((__warn_unused_result__))
#endif

#if !defined(WARN_UNUSED_RETURN)
#define WARN_UNUSED_RETURN
#endif

#define APP_NAME "StarFish"
#define APP_CODE_NAME "StarFish"
#define VERSION "0.1.0"
#define APP_VERSION(APP_NAME, VERSION) APP_NAME "/" VERSION
#define USER_AGENT(APP_CODE_NAME, VERSION) APP_CODE_NAME "/"  VERSION
#define VENDOR_NAME "Samsung Electronics Co., Ltd."

#include "StarFishExport.h"

#include "util/String.h"
#include "util/QualifiedName.h"
#include "util/URL.h"
#include "style/Unit.h"

#endif
