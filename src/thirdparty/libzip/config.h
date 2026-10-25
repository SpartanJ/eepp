#ifndef HAD_CONFIG_H
#define HAD_CONFIG_H
#ifndef _HAD_ZIPCONF_H
#include "zipconf.h"
#endif

/* eepp: hand-written replacement for the CMake feature checks in config.h.in.
   Only deflate/store and traditional PKWARE encryption are built: bzip2, xz,
   zstd and WinZip AES (HAVE_CRYPTO) are disabled. Platform file backends live
   in posix/ and win32/. */

#define ENABLE_FDOPEN

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define HAVE__CLOSE
#define HAVE__DUP
#define HAVE__FDOPEN
#define HAVE__FILENO
#define HAVE__FSEEKI64
#define HAVE__FSTAT64
#define HAVE__FTELLI64
#define HAVE__SETMODE
#define HAVE__SNPRINTF
#define HAVE__STRDUP
#define HAVE__STRICMP
#define HAVE__STRTOI64
#define HAVE__STRTOUI64
#define HAVE__UNLINK
#define HAVE_GETSECURITYINFO
#define HAVE_SNPRINTF
#define HAVE_STRTOLL
#define HAVE_STRTOULL

/* MSVC and MinGW-w64 (MINGW_HAS_SECURE_API, on by default) both declare these. compat.h would
   otherwise define replacement macros that break MinGW's own declarations. */
#define HAVE__SNPRINTF_S
#define HAVE__SNWPRINTF_S
#define HAVE_LOCALTIME_S
#define HAVE_MEMCPY_S
#define HAVE_STRERROR_S
#define HAVE_STRNCPY_S

#ifndef _MSC_VER
/* MinGW declares ftello; libzip still uses _fseeki64/_ftelli64 on Windows. */
#define HAVE_FTELLO
/* MSVC defines DWORD_MAX in intsafe.h; MinGW-w64 headers do not (win32/ named source). */
#ifndef DWORD_MAX
#define DWORD_MAX 0xffffffffUL
#endif
#endif

#else /* POSIX */

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(__DragonFly__) || defined(__ANDROID__)
#define HAVE_ARC4RANDOM
#endif

#define HAVE_FILENO
#define HAVE_FSEEKO
#define HAVE_FTELLO
#define HAVE_LOCALTIME_R
#define HAVE_RANDOM
#define HAVE_SNPRINTF
#define HAVE_STRCASECMP
#define HAVE_STRDUP
#define HAVE_STRINGS_H
#define HAVE_STRTOLL
#define HAVE_STRTOULL
#ifndef HAVE_UNISTD_H
#define HAVE_UNISTD_H
#endif

/* off_t is 64-bit on these platforms regardless of _FILE_OFFSET_BITS. */
#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
    || defined(__DragonFly__) || defined(__HAIKU__) || defined(__EMSCRIPTEN__) || defined(__LP64__) \
    || defined(_LP64) || (defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS == 64)
#define SIZEOF_OFF_T 8
#else
#define SIZEOF_OFF_T 4
#endif

#include <sys/types.h>
/* Fails to compile if the off_t guess above is wrong for a new target. */
typedef char zip_eepp_off_t_size_check[sizeof(off_t) == SIZEOF_OFF_T ? 1 : -1];

#endif /* _WIN32 */

#define HAVE_STDBOOL_H

#if defined(__SIZEOF_SIZE_T__)
#define SIZEOF_SIZE_T __SIZEOF_SIZE_T__
#elif defined(_WIN64)
#define SIZEOF_SIZE_T 8
#else
#define SIZEOF_SIZE_T 4
#endif

#define PACKAGE "libzip"
#define VERSION "1.12.1"

#endif /* HAD_CONFIG_H */
