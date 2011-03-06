#ifndef EE_GRAPHICS_BASE
#define EE_GRAPHICS_BASE

#include "../base.hpp"

#if ( defined( EE_GLES2 ) || defined( EE_GLES1 ) ) && !defined( EE_GLES )
	#define EE_GLES
#endif

#if ( EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM ) ) && !defined( EE_GLES )
	#define EE_GLEW_AVAILABLE
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
		#if defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_WIN
			#include <GL/glext.h>
		#elif EE_PLATFORM == EE_PLATFORM_MACOSX
			#include <OpenGL/glext.h>
		#endif
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

#ifdef EE_GLES2
typedef char GLchar;
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_TEXTURE 0x1702
#define GL_VERTEX_ARRAY 0x8074
#define GL_NORMAL_ARRAY 0x8075
#define GL_COLOR_ARRAY 0x8076
#define GL_INDEX_ARRAY 0x8077
#define GL_TEXTURE_COORD_ARRAY 0x8078
#define GL_EDGE_FLAG_ARRAY 0x8079
#define GL_LINE 0x1B01
#define GL_FILL 0x1B02
#define GL_LINE_SMOOTH 0x0B20
#define GL_LIGHTING 0x0B50
#define GL_CLIP_PLANE0 0x3000
#define GL_CLIP_PLANE1 0x3001
#define GL_CLIP_PLANE2 0x3002
#define GL_CLIP_PLANE3 0x3003
#define GL_CLIP_PLANE4 0x3004
#define GL_CLIP_PLANE5 0x3005
#define GL_POINT_SPRITE 0x8861
#define GL_VERTEX_PROGRAM_POINT_SIZE 0x8642
typedef GLfloat		GLdouble;
#endif

#define STBI_TYPE_SPECIFIC_FUNCTIONS
#include "../helper/SOIL/stb_image.h"
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
