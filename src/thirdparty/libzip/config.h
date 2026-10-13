#ifndef ZIP_HAD_CONFIG_H
#define ZIP_HAD_CONFIG_H

#if !defined( __WIN32__ ) && !defined( _WIN32 )

	#define HAVE_MKSTEMP
	#define HAVE_FSEEKO
	#define HAVE_FTELLO

#else

	/* eepp: rename() cannot replace an existing file on Windows, which made every zip_close()
	   of an existing archive fail. zipint.h then uses MoveFileExA(MOVEFILE_REPLACE_EXISTING). */
	#define HAVE_MOVEFILEEXA

#endif

#if (!defined (_MSCVER) && !defined (_MSC_VER))
	#ifndef ZIP_HAVE_UNISTD_H
		#define ZIP_HAVE_UNISTD_H
	#endif
#endif

#define PACKAGE "libzip"
#define VERSION "0.10"

#endif
