#ifndef EE_PHYSICS_BASE
#define EE_PHYSICS_BASE

//! Chipmunk includes
#include <eepp/helper/chipmunk/chipmunk_private.h>

//! EE includes needed for the wrapper, all templates, so it will be easy to port this.

#include <eepp/math/vector2.hpp>
#include <eepp/math/rect.hpp>
using namespace EE::Math;

#include <eepp/core.hpp>
#include <eepp/system/colors.hpp>
#include <eepp/system/singleton.hpp>
using namespace EE::System;

//! Default settings are defined here
#include <eepp/physics/settings.hpp>

//! Some helpers for the wrapper ( most of them can be disabled in the settings )
#include <eepp/physics/physicshelper.hpp>

#endif
