#ifndef EE_WINDOWCWINDOWSDL_HPP
#define EE_WINDOWCWINDOWSDL_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL/base.hpp>

#ifdef EE_BACKEND_SDL_1_2

#include <eepp/window/window.hpp>

struct SDL_Surface;
struct SDL_SysWMinfo;

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API WindowSDL : public Window {
	public:
		WindowSDL( WindowSettings Settings, ContextSettings Context );

		virtual ~WindowSDL();

		bool Create( WindowSettings Settings, ContextSettings Context );

		void ToggleFullscreen();

		void Caption( const std::string& Caption );

		bool Icon( const std::string& Path );

		bool Active();

		bool Visible();

		void Size( Uint32 Width, Uint32 Height, bool Windowed );

		std::vector<DisplayMode> GetDisplayModes() const;

		void SetGamma( Float Red, Float Green, Float Blue );

		eeWindowHandle	GetWindowHandler();
	protected:
		friend class ClipboardSDL;

		SDL_Surface *	mSurface;

		#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
		SDL_SysWMinfo *	mWMinfo;
		#endif

		Vector2i		mWinPos;

		void CreatePlatform();

		void SwapBuffers();

		void SetGLConfig();

		std::string GetVersion();
};

}}}}

#endif

#endif
