#ifndef EE_PHYSICS_BASE
#define EE_PHYSICS_BASE

#include "../base.hpp"

#include "../math/math.hpp"
using namespace EE::Math;

#include "../window/cengine.hpp"
using namespace EE::Window;

#include "../system/tsingleton.hpp"
using namespace EE::System;

#include "../graphics/cprimitives.hpp"
#include "../graphics/cbatchrenderer.hpp"
#include "../graphics/cglobalbatchrenderer.hpp"

#include "../helper/chipmunk/chipmunk_private.h"
#include "../helper/chipmunk/chipmunk_unsafe.h"

#define USE_EE_VECTOR
#define USE_EE_AABB
#define BB_INVERT_Y_AXIS

#include "physicshelper.hpp"

#endif
