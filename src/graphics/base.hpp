#ifndef EE_GRAPHICS_BASE
#define EE_GRAPHICS_BASE

#include "../base.hpp"

#include <SDL/SDL_syswm.h>

#include "../helper/SOIL/SOIL.h"
#include "../helper/SDL_ttf/SDL_ttf.h"

#include "../utils/colors.hpp"
#include "../utils/rect.hpp"
#include "../utils/vector2.hpp"
#include "../utils/string.hpp"
#include "../utils/utils.hpp"
#include "../utils/polygon2.hpp"
#include "../utils/vector3.hpp"
using namespace EE::Utils;

#include "../math/math.hpp"
using namespace EE::Math;

#include "../system/ctimeelapsed.hpp"
#include "../system/singleton.hpp"
#include "../system/clog.hpp"
#include "../system/cpack.hpp"
using namespace EE::System;

#include "renders.hpp"

#endif
