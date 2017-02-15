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

		bool create( WindowSettings Settings, ContextSettings Context );

		void toggleFullscreen();

		void caption( const std::string& caption );

		bool icon( const std::string& Path );

		bool active();

		bool visible();

		void size( Uint32 Width, Uint32 Height, bool Windowed );

		std::vector<DisplayMode> getDisplayModes() const;

		void setGamma( Float Red, Float Green, Float Blue );

		eeWindowHandle	getWindowHandler();
	protected:
		friend class ClipboardSDL;

		SDL_Surface *	mSurface;

		#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
		SDL_SysWMinfo *	mWMinfo;
		#endif

		Vector2i		mWinPos;

		void createPlatform();

		void swapBuffers();

		void setGLConfig();

		std::string getVersion();
};

}}}}

#endif

#endif
