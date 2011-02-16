#ifndef EE_WINDOWCWINDOWSDL_HPP
#define EE_WINDOWCWINDOWSDL_HPP

#include "../../cbackend.hpp"

#ifdef EE_BACKEND_SDL_ACTIVE

#include "../../cwindow.hpp"
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API cWindowSDL : public cWindow {
	public:
		cWindowSDL( WindowSettings Settings, ContextSettings Context );
		
		virtual ~cWindowSDL();
		
		bool Create( WindowSettings Settings, ContextSettings Context );
		
		void ToggleFullscreen();
		
		void Caption( const std::string& Caption );

		bool Icon( const std::string& Path );

		bool Active();

		bool Visible();

		void Size( Uint32 Width, Uint32 Height, bool Windowed );
		
		void ShowCursor( const bool& showcursor );

		std::vector< std::pair<unsigned int, unsigned int> > GetPossibleResolutions() const;

		void SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue );

		eeWindowHandler	GetWindowHandler();
	protected:
		friend class cClipboardSDL;

		SDL_Surface *	mSurface;
		SDL_SysWMinfo 	mWMinfo;

		void CreatePlatform();

		void SwapBuffers();

		void SetGLConfig();
};

}}}}

#endif

#endif
