#ifndef EE_PHYSICS_BASE
#define EE_PHYSICS_BASE

//! Chipmunk includes
#include <eepp/helper/chipmunk/chipmunk_private.h>
#include <eepp/helper/chipmunk/chipmunk_unsafe.h>

//! EE includes needed for the wrapper, all templates, so it will be easy to port this.
#include <eepp/base.hpp>
#include <eepp/utils/vector2.hpp>
#include <eepp/utils/rect.hpp>
#include <eepp/utils/colors.hpp>
using namespace EE::Utils;

#include <eepp/system/tsingleton.hpp>
using namespace EE::System;

//! Default settings are defined here
#include <eepp/physics/settings.hpp>

//! Some helpers for the wrapper ( most of them can be disabled in the settings )
#include <eepp/physics/physicshelper.hpp>

#endif
