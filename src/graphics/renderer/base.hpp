#ifndef EE_GRAPHICS_RENDERER_BASE_HPP
#define EE_GRAPHICS_RENDERER_BASE_HPP

#include "../base.hpp"
#include "../renders.hpp"

#if !defined( EE_GLES2 ) || !defined( EE_GLES1 )
	#define EE_GL3_ENABLED 1
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
#endif

#endif
