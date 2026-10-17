#ifndef EE_VFS_CONFIG_HPP
#define EE_VFS_CONFIG_HPP

#include <eepp/config.hpp>

#ifndef EE_VFS_API
#if defined( EE_VFS_STATIC )
#define EE_VFS_API
#elif EE_PLATFORM == EE_PLATFORM_WIN
#if defined( EE_VFS_EXPORTS )
#define EE_VFS_API __declspec( dllexport )
#else
#define EE_VFS_API __declspec( dllimport )
#endif
#elif defined( __GNUC__ ) && __GNUC__ >= 4
#define EE_VFS_API __attribute__( ( visibility( "default" ) ) )
#else
#define EE_VFS_API
#endif
#endif

#endif
