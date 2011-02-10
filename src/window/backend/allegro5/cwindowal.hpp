#ifndef EE_WINDOWCWINDOWAl_HPP
#define EE_WINDOWCWINDOWAl_HPP

#include "../../cbackend.hpp"

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

#include <allegro5/allegro5.h>
#include "../../cwindow.hpp"

namespace EE { namespace Window { namespace Backend { namespace Al {

class EE_API cWindowAl : public cWindow {
	public:
		cWindowAl( WindowSettings Settings, ContextSettings Context );
		
		virtual ~cWindowAl();
		
		bool Create( WindowSettings Settings, ContextSettings Context );
		
		void ToggleFullscreen();
		
		void Caption( const std::string& Caption );

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

		eeWindowHandler	GetWindowHandler();

		void SetDefaultContext();

		ALLEGRO_DISPLAY * GetDisplay() const;
	protected:
		friend class cClipboardAl;
		friend class cInputAl;
		
		ALLEGRO_DISPLAY * 	mDisplay;
		bool				mActive;
		
		void SwapBuffers();
};

}}}}

#endif

#endif
