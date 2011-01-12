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

#define CP_ALLOW_PRIVATE_ACCESS 1
#include "../helper/chipmunk/chipmunk_private.h"
#include "../helper/chipmunk/chipmunk_unsafe.h"
#include "../helper/chipmunk/chipmunk.h"

#include "physicshelper.hpp"

#endif
