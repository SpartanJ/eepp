#ifndef EE_WINDOWCWINDOWSDL2_HPP
#define EE_WINDOWCWINDOWSDL2_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/window/window.hpp>
#include <eepp/window/backend/SDL2/wminfo.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_IOS
#define EE_USE_WMINFO
#endif

namespace EE { namespace System { class Zip; } }

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

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

		virtual void Minimize();

		virtual void Maximize();

		virtual void Hide();

		virtual void Raise();

		virtual void Show();

		virtual void Position( Int16 Left, Int16 Top );

		virtual Vector2i Position();

		const Sizei& GetDesktopResolution();

		SDL_Window *	GetSDLWindow() const;

		void StartTextInput();

		bool IsTextInputActive();

		void StopTextInput();

		void SetTextInputRect( Recti& rect );

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

		bool IsThreadedGLContext();

		void SetGLContextThread();

		void UnsetGLContextThread();
	protected:
		friend class ClipboardSDL;

		SDL_Window *	mSDLWindow;
		SDL_GLContext	mGLContext;
		SDL_GLContext	mGLContextThread;

		#ifdef EE_USE_WMINFO
		WMInfo *		mWMinfo;
		#endif

		#if EE_PLATFORM == EE_PLATFORM_ANDROID
		Zip *			mZip;
		#endif

		Vector2i		mWinPos;

		void CreatePlatform();

		void SwapBuffers();

		void SetGLConfig();

		std::string GetVersion();

		void UpdateDesktopResolution();
};

}}}}

#endif

#endif
