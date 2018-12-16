#ifndef EE_WINDOWCWINDOWSDL2_HPP
#define EE_WINDOWCWINDOWSDL2_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/window/window.hpp>
#include <eepp/window/backend/SDL2/wminfo.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_IOS || EE_PLATFORM == EE_PLATFORM_ANDROID
#define EE_USE_WMINFO
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API WindowSDL : public Window {
	public:
		WindowSDL( WindowSettings Settings, ContextSettings Context );
		
		virtual ~WindowSDL();
		
		bool create( WindowSettings Settings, ContextSettings Context );
		
		void toggleFullscreen();
		
		void setCaption( const std::string& setCaption );

		bool setIcon( const std::string& Path );

		bool isActive();

		bool isVisible();

		void setSize( Uint32 Width, Uint32 Height, bool isWindowed );

		std::vector<DisplayMode> getDisplayModes() const;

		void setGamma( Float Red, Float Green, Float Blue );

		eeWindowHandle	getWindowHandler();

		virtual void minimize();

		virtual void maximize();

		virtual void hide();

		virtual void raise();

		virtual void show();

		virtual void setPosition( Int16 Left, Int16 Top );

		virtual Vector2i getPosition();

		const Sizei& getDesktopResolution();

		virtual Rect getBorderSize();

		SDL_Window *	GetSDLWindow() const;

		void startTextInput();

		bool isTextInputActive();

		void stopTextInput();

		void setTextInputRect( Rect& rect );

		bool hasScreenKeyboardSupport();

		bool isScreenKeyboardShown();

		bool isThreadedGLContext();

		void setGLContextThread();

		void unsetGLContextThread();

		int getCurrentDisplayIndex();
	protected:
		friend class ClipboardSDL;

		SDL_Window *	mSDLWindow;
		SDL_GLContext	mGLContext;
		SDL_GLContext	mGLContextThread;

		#ifdef EE_USE_WMINFO
		WMInfo *		mWMinfo;
		#endif

		Vector2i		mWinPos;

		void createPlatform();

		void swapBuffers();

		void setGLConfig();

		std::string getVersion();

		void updateDesktopResolution();

		void onWindowResize( Uint32 Width, Uint32 Height );
};

}}}}

#endif

#endif
