#ifndef EE_PHYSICS_BASE
#define EE_PHYSICS_BASE

//! Chipmunk includes
#include <eepp/thirdparty/chipmunk/chipmunk_private.h>

//! EE includes needed for the wrapper, all templates, so it will be easy to port this.

#include <eepp/math/rect.hpp>
#include <eepp/math/vector2.hpp>
using namespace EE::Math;

#include <eepp/core.hpp>
#include <eepp/system/color.hpp>
#include <eepp/system/singleton.hpp>
using namespace EE::System;

//! Default settings are defined here
#include <eepp/physics/settings.hpp>

//! Some helpers for the wrapper ( most of them can be disabled in the settings )
#include <eepp/physics/physicshelper.hpp>

#ifndef EE_PHYSICS_STATIC
#if EE_PLATFORM == EE_PLATFORM_WIN
// Windows platforms
#ifndef EE_PHYSICS_API
#ifdef EE_PHYSICS_EXPORTS
// From DLL side, we must export
#define EE_PHYSICS_API __declspec( dllexport )
#else
// From client application side, we must import
#define EE_PHYSICS_API __declspec( dllimport )
#endif
#endif
#else
#if ( __GNUC__ >= 4 ) && !defined( EE_PHYSICS_API )
#define EE_PHYSICS_API __attribute__( ( visibility( "default" ) ) )
#endif
#endif
#endif

#endif

#ifndef EE_PHYSICS_API
#define EE_PHYSICS_API
#endif
