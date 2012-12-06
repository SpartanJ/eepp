#ifndef EE_WINDOWCONTEXT_HPP
#define EE_WINDOWCONTEXT_HPP

#include <eepp/declares.hpp>

#ifdef EE_GLEW_AVAILABLE
	#if EE_PLATFORM == EE_PLATFORM_WIN
	
		typedef void *		eeWindowContex;
		
	#elif defined( EE_X11_PLATFORM )

		#include <eepp/helper/glew/glxew.h>
		#undef Window
		#undef Display
		#undef Cursor
		typedef GLXContext	eeWindowContex;

	#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	
		#include <AGL/agl.h>
		typedef AGLContext	eeWindowContex;
		
	#endif
#else
	typedef unsigned int	eeWindowContex;	//! Fallback
#endif

#endif
