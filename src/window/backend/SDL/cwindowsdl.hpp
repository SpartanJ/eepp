#ifndef EE_WINDOWCWINDOWSDL_HPP
#define EE_WINDOWCWINDOWSDL_HPP

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
		
		std::string Caption();

		bool Icon( const std::string& Path );

		void Minimize();

		void Maximize();

		void Hide();

		void Raise();

		void Show();

		void Position( Int16 Left, Int16 Top );

		bool Active();

		bool Visible();

		eeVector2i Position();

		void Size( const Uint32& Width, const Uint32& Height );

		void Size( const Uint16& Width, const Uint16& Height, const bool& Windowed );
		
		void ShowCursor( const bool& showcursor );

		std::vector< std::pair<unsigned int, unsigned int> > GetPossibleResolutions() const;

		void SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue );

		void SetCurrentContext( eeWindowContex Context );

		eeWindowContex GetContext() const;

		eeWindowHandler	GetWindowHandler();

		void SetDefaultContext();
	protected:
		friend class cClipboardSDL;

		SDL_Surface *	mSurface;
		SDL_SysWMinfo 	mWMinfo;

		void SwapBuffers();

		void GetMainContext();

		void SetGLConfig();
};

}}}}

#endif
