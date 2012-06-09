#ifndef EE_WINDOWCWINDOWAl_HPP
#define EE_WINDOWCWINDOWAl_HPP

#include <eepp/window/cbackend.hpp>

#ifdef EE_BACKEND_ALLEGRO_ACTIVE

#define ALLEGRO_LIB_BUILD
#include <allegro5/debug.h>
#include <allegro5/allegro.h>
#include <eepp/window/cwindow.hpp>

namespace EE { namespace Window { namespace Backend { namespace Al {

class EE_API cWindowAl : public cWindow {
	public:
		cWindowAl( WindowSettings Settings, ContextSettings Context );

		virtual ~cWindowAl();

		bool Create( WindowSettings Settings, ContextSettings Context );

		void ToggleFullscreen();

		void Caption( const std::string& Caption );

		bool Icon( const std::string& Path );

		bool Active();

		bool Visible();

		eeVector2i Position();

		void Size( Uint32 Width, Uint32 Height, bool Windowed  );

		std::vector< std::pair<unsigned int, unsigned int> > GetPossibleResolutions() const;

		void SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue );

		eeWindowHandler	GetWindowHandler();

		#if defined( EE_X11_PLATFORM )
		X11Window GetX11Window();
		#endif

		void SetDefaultContext();

		ALLEGRO_DISPLAY * GetDisplay() const;
	protected:
		friend class cClipboardAl;
		friend class cInputAl;

		ALLEGRO_DISPLAY * 	mDisplay;
		bool				mActive;

		void CreatePlatform();

		void SetCurrent();

		void SwapBuffers();

		void DestroyDisplay();

		Uint32 CreateFlags( const WindowSettings& Settings, const ContextSettings& Context );
};

}}}}

#endif

#endif
