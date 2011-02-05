#ifndef EE_PHYSICS_BASE
#define EE_PHYSICS_BASE

//! Chipmunk includes
#include "../helper/chipmunk/chipmunk_private.h"
#include "../helper/chipmunk/chipmunk_unsafe.h"

//! EE includes needed for the wrapper, all templates, so it will be easy to port this.
#include "../base.hpp"
#include "../utils/vector2.hpp"
#include "../utils/rect.hpp"
#include "../utils/colors.hpp"
using namespace EE::Utils;

#include "../system/tsingleton.hpp"
using namespace EE::System;

//! Default settings are defined here
#include "settings.hpp"

//! Some helpers for the wrapper ( most of them can be disabled in the settings )
#include "physicshelper.hpp"

#endif
