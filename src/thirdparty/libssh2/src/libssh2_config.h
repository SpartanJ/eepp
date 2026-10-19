/* Static configuration for the libssh2 copy vendored by eepp.
 *
 * libssh2 normally generates this file with CMake or autotools. eepp builds it from Premake, so
 * the feature set is selected here per platform. The crypto backend is selected by the build
 * (LIBSSH2_MBEDTLS by default, LIBSSH2_OPENSSL when eepp is built with --with-openssl).
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef EEPP_LIBSSH2_CONFIG_H
#define EEPP_LIBSSH2_CONFIG_H

#if defined( _WIN32 )

#ifdef __MINGW32__
#define HAVE_UNISTD_H
#define HAVE_INTTYPES_H
#define HAVE_SYS_TIME_H
#define HAVE_GETTIMEOFDAY
#define HAVE_STRTOLL
#else
#define HAVE_STRTOI64
#endif
#define HAVE_SNPRINTF
#define HAVE_SELECT

#else /* POSIX */

#define HAVE_UNISTD_H
#define HAVE_INTTYPES_H
#define HAVE_SYS_SELECT_H
#define HAVE_SYS_UIO_H
#define HAVE_SYS_SOCKET_H
#define HAVE_SYS_IOCTL_H
#define HAVE_SYS_TIME_H
#define HAVE_SYS_UN_H
#define HAVE_ARPA_INET_H
#define HAVE_NETINET_IN_H
#define HAVE_GETTIMEOFDAY
#define HAVE_STRTOLL
#define HAVE_SNPRINTF
#define HAVE_POLL
#define HAVE_SELECT
#define HAVE_O_NONBLOCK

#if defined( __linux__ ) && defined( __GLIBC__ )
#define HAVE_EXPLICIT_BZERO
#elif defined( __FreeBSD__ ) || defined( __OpenBSD__ )
#define HAVE_EXPLICIT_BZERO
#elif defined( __NetBSD__ )
#define HAVE_EXPLICIT_MEMSET
#endif

#endif

/* Diffie-Hellman group exchange with the RFC 4419 message format. */
#define LIBSSH2_DH_GEX_NEW 1

#endif
