#ifndef EE_MAPS__BASE
#define EE_MAPS__BASE

#include <eepp/core.hpp>

#include <eepp/math/math.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/math/vector2.hpp>
using namespace EE::Math;

#include <eepp/system/bitop.hpp>
#include <eepp/system/color.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/time.hpp>
using namespace EE::System;

#ifndef EE_MAPS_STATIC
#if EE_PLATFORM == EE_PLATFORM_WIN
// Windows platforms
#ifdef EE_MAPS_EXPORTS
// From DLL side, we must export
#define EE_MAPS_API __declspec( dllexport )
#else
// From client application side, we must import
#define EE_MAPS_API __declspec( dllimport )
#endif
#else
#if ( __GNUC__ >= 4 ) && !defined( EE_MAPS_API )
#define EE_MAPS_API __attribute__( ( visibility( "default" ) ) )
#endif
#endif
#endif

#endif

#ifndef EE_MAPS_API
#define EE_MAPS_API
#endif
