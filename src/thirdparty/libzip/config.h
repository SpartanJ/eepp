#ifndef ZIP_HAD_CONFIG_H
#define ZIP_HAD_CONFIG_H

#if !defined( __WIN32__ ) && !defined( _WIN32 )

	#define HAVE_MKSTEMP
	#define HAVE_FSEEKO
	#define HAVE_FTELLO

#endif

#if (!defined (_MSCVER) && !defined (_MSC_VER))
	#ifndef ZIP_HAVE_UNISTD_H
		#define ZIP_HAVE_UNISTD_H
	#endif
#endif

#define PACKAGE "libzip"
#define VERSION "0.10"

#endif
