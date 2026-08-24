#pragma once

/* adapted from krisvers/vkom */
#if defined(_WIN32) || defined(_WIN64)
#define KLATR_PLATFORM_FAMILY_NT
#define KLATR_PLATFORM_OS_WINDOWS
#elif defined(__APPLE__)
#define KLATR_PLATFORM_FAMILY_APPLE
#include <TargetConditionals.h>

#if TARGET_OS_IPHONE == 1
#define KLATR_PLATFORM_FAMILY_UNIX
#define KLATR_PLATFORM_FAMILY_DARWIN
#define KLATR_PLATFORM_OS_IOS
#elif TARGET_OS_MAC == 1
#define KLATR_PLATFORM_FAMILY_UNIX
#define KLATR_PLATFORM_FAMILY_DARWIN
#define KLATR_PLATFORM_OS_MACOS
#endif
#elif defined(__ANDROID__)
#define KLATR_PLATFORM_FAMILY_UNIX
#define KLATR_PLATFORM_OS_ANDROID
#elif defined(__linux__)
#define KLATR_PLATFORM_FAMILY_UNIX
#define KLATR_PLATFORM_OS_LINUX
#elif defined(__FreeBSD__)
#define KLATR_PLATFORM_FAMILY_UNIX
#define KLATR_PLATFORM_FAMILY_BSD
#define KLATR_PLATFORM_OS_FREEBSD
#endif

#ifdef _MSC_VER
#define KLATR_COMPILER_MSVC
#elif defined(__MINGW32__)
#define KLATR_COMPILER_MINGW
#elif defined(__clang__)
#define KLATR_COMPILER_CLANG
#elif defined(__GNUC__)
#define KLATR_COMPILER_GCC
#else
#define KLATR_COMPILER_UNKNOWN
#endif

#ifdef KLATR_COMPILER_MSVC
#define KLATR_FORCE_EXPORT __declspec(dllexport)
#elif defined(KLATR_COMPILER_MINGW)
#define KLATR_FORCE_EXPORT __attribute__((dllexport))
#elif defined(KLATR_COMPILER_CLANG)
#define KLATR_FORCE_EXPORT __attribute__((__visibility__("default")))
#elif defined(KLATR_COMPILER_GCC)
#define KLATR_FORCE_EXPORT __attribute__((__visibility__("default")))
#endif

#ifdef KLATR_COMPILING_DYNAMIC_LIBRARY
#define KLATR_VISIBLE KLATR_FORCE_EXPORT
#else
#define KLATR_VISIBLE
#endif
