#ifndef HAD_CONFIG_H
#define HAD_CONFIG_H

#if !defined( __WIN32__ ) && !defined( _WIN32 )

#define HAVE_MKSTEMP
#define HAVE_FSEEKO
#define HAVE_FTELLO

#endif

#if (!defined (_MSCVER) && !defined (_MSC_VER))
#define HAVE_UNISTD_H
#endif


#define PACKAGE "libzip"
#define VERSION "0.9.3a"
#endif /* HAD_CONFIG_H */
