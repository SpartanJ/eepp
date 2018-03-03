#ifndef EE_GLEXTENSIONS_HPP
#define EE_GLEXTENSIONS_HPP

#include <eepp/config.hpp>

#ifndef EE_GLES
	//! GL2 and GL3 ( PC platform )
	#ifdef EE_GLEW_AVAILABLE
		#define GLEW_STATIC
		#define GLEW_NO_GLU
		#include <glew/glew.h>
	#else
		#ifndef GL_GLEXT_PROTOTYPES
			#define GL_GLEXT_PROTOTYPES
		#endif
	#endif
#endif

#include <eepp/graphics/renderer/opengl.hpp>

#endif
