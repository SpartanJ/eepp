#ifndef EE_WINDOWCWINDOWSDL2_HPP
#define EE_WINDOWCWINDOWSDL2_HPP

#include <eepp/window/cbackend.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/window/cwindow.hpp>

class SDL_SysWMinfo;

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
#define EE_USE_WMINFO
#endif

namespace EE { namespace System { class cZip; } }

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

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

		std::vector< std::pair<unsigned int, unsigned int> > GetPossibleResolutions() const;

		void SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue );

		eeWindowHandle	GetWindowHandler();

		virtual void Minimize();

		virtual void Maximize();

		virtual void Hide();

		virtual void Raise();

		virtual void Show();

		virtual void Position( Int16 Left, Int16 Top );

		virtual eeVector2i Position();

		const eeSize& GetDesktopResolution();

		SDL_Window *	GetSDLWindow() const;

		void StartTextInput();

		bool IsTextInputActive();

		void StopTextInput();

		void SetTextInputRect( eeRecti& rect );

		bool HasScreenKeyboardSupport();

		bool IsScreenKeyboardShown();

#if EE_PLATFORM == EE_PLATFORM_ANDROID
		void * GetJNIEnv();

		void * GetActivity();

		int GetExternalStorageState();

		std::string GetInternalStoragePath();

		std::string GetExternalStoragePath();

		std::string GetApkPath();
#endif
	protected:
		friend class cClipboardSDL;

		SDL_Window *	mSDLWindow;
		SDL_GLContext	mGLContext;

		#ifdef EE_USE_WMINFO
		SDL_SysWMinfo * mWMinfo;
		#endif

		#if EE_PLATFORM == EE_PLATFORM_ANDROID
		cZip *			mZip;
		#endif

		eeVector2i		mWinPos;

		void CreatePlatform();

		void SwapBuffers();

		void SetGLConfig();

		std::string GetVersion();

		void UpdateDesktopResolution();
};

}}}}

#endif

#endif
