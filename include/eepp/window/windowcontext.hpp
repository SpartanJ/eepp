#ifndef EE_WINDOWCONTEXT_HPP
#define EE_WINDOWCONTEXT_HPP

#include <eepp/config.hpp>

#ifdef EE_GLEW_AVAILABLE
	#if EE_PLATFORM == EE_PLATFORM_WIN
	
		typedef void *		eeWindowContex;
		
	#elif defined( EE_X11_PLATFORM )

		#ifdef __sun
		struct __glXContextRec;
		typedef struct __glXContextRec *GLXContext;
		#else
		struct __GLXcontextRec;
		typedef struct __GLXcontextRec *GLXContext;
		#endif

		typedef GLXContext	eeWindowContex;

	#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	
		struct __AGLContextRec;

		typedef struct __AGLContextRec       *AGLContext;

		typedef AGLContext	eeWindowContex;
		
	#endif
#else
	typedef unsigned int	eeWindowContex;	//! Fallback
#endif

#endif
