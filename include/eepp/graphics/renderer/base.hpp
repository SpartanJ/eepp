#ifndef EE_GRAPHICS_RENDERER_BASE_HPP
#define EE_GRAPHICS_RENDERER_BASE_HPP

#include <eepp/graphics/base.hpp>
#include <eepp/graphics/renders.hpp>

//! It seems that it's not possible to compile GLM on GCC 4.4 ( Haiku GCC version )
#if defined( EE_SHADERS_SUPPORTED ) && EE_PLATFORM != EE_PLATFORM_HAIKU
	#define EE_GL3_ENABLED 1
#endif

#endif
