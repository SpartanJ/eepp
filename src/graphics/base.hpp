#ifndef EE_GRAPHICS_BASE
#define EE_GRAPHICS_BASE

#include "../base.hpp"

#include "../helper/SOIL/SOIL.h"

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
#include "../system/tsingleton.hpp"
#include "../system/clog.hpp"
#include "../system/cpack.hpp"
#include "../system/tresourcemanager.hpp"
#include "../system/tcontainer.hpp"
using namespace EE::System;

#include "renders.hpp"

#ifdef EE_GLES
	const GLubyte EE_GLES_INDICES [] = {0, 3, 1, 2};
	#define EE_QUAD_VERTEX 6
#else
	#define EE_QUAD_VERTEX 4
#endif

#endif
