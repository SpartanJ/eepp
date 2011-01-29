#ifndef EE_GRAPHICS_BASE
#define EE_GRAPHICS_BASE

#include "../base.hpp"

#if ( defined( EE_GLES2 ) || defined( EE_GLES1 ) ) && !defined( EE_GLES )
	#define EE_GLES
#endif

#ifndef EE_GLES
	//! GL2 and GL3 ( PC platform )

	#ifdef EE_GLEW_AVAILABLE
		#include "../helper/glew/glew.h"
	#else
		#define GL_GLEXT_PROTOTYPES
	#endif

	#if EE_PLATFORM == EE_PLATFORM_MACOSX
		#include <OpenGL/gl.h>
	#else
		#include <GL/gl.h>
	#endif

	#ifndef EE_GLEW_AVAILABLE
		#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_WIN
			#include <GL/glext.h>
		#elif EE_PLATFORM == EE_PLATFORM_MACOSX
			#include <OpenGL/glext.h>
		#endif

		#include <GL/glu.h>
	#endif

#else
	//! Mobile platform ( Android / iPhone / Maemo )

	//! GLES2 ( programmable pipeline )
	#ifdef EE_GLES2
		#include <GLES2/gl2.h>
		#include <GLES2/gl2ext.h>

	//! GLES1 ( fixed pipeline )
	#elif defined( EE_GLES1 )
		#include <GLES/gl.h>
	#endif
#endif

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
	#define EE_QUAD_VERTEX 6
#else
	#define EE_QUAD_VERTEX 4
#endif

#endif
