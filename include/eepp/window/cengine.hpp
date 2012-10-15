#ifndef EE_WINDOWCENGINE_H
#define EE_WINDOWCENGINE_H

#include <eepp/window/base.hpp>
#include <eepp/window/cbackend.hpp>
#include <eepp/window/cwindow.hpp>

namespace EE { namespace System { class cIniFile; } }

namespace EE { namespace Window {

/** @brief The window management class. Here the engine start working. (Singleton Class). */
class EE_API cEngine {
	SINGLETON_DECLARE_HEADERS(cEngine)

	public:
		~cEngine();

		/** Creates a new window.
		* Allegro 5 and SDL2 backends support more than one window creation, SDL 1.2 backend only 1 window.
		*/
		Window::cWindow * CreateWindow( WindowSettings Settings, ContextSettings Context );

		/** Destroy the window instance, and set as current other window running ( if any ).
		* This function is only usefull for multi-window environment. Avoid using it with one window context.
		*/
		void DestroyWindow( Window::cWindow * window );

		/** @return The current Window context. */
		Window::cWindow * GetCurrentWindow() const;

		/** Set the window as the current. */
		void SetCurrentWindow( Window::cWindow * window );

		/** @return The number of windows created. */
		Uint32	GetWindowCount() const;

		/** @return If any window is created. */
		bool Running() const;

		/** @return The current window elapsed time. */
		eeFloat Elapsed() const;

		/** @return The current window width. */
		const Uint32& GetWidth() const;

		/** @return The current window height */
		const Uint32& GetHeight() const;

		/** @return If the window instance is inside the window list. */
		bool ExistsWindow( Window::cWindow * window );

		/** Constructs WindowSettings from an ini file
		It will search for the following properties:
			Width			Window width
			Height			Window height
			BitColor		32,16,8
			Windowed		bool
			Resizeable		bool
			Backend			SDL or allegro
			WinIcon			The path to the window icon
			WinCaption		The window default title

			@param iniPath The ini file path
			@param iniKeyName The ini key name to search the properties
		*/
		WindowSettings CreateWindowSettings( std::string iniPath, std::string iniKeyName = "EEPP" );

		WindowSettings CreateWindowSettings( cIniFile * ini, std::string iniKeyName = "EEPP" );

		/** Constructs ContextSettings from an ini file\n
		It will search for the following properties:
			VSync				bool
			GLVersion			Selects the default renderer: 2 for OpenGL 2, 3 for OpenGL 3, 4 for OpenGL ES 2
			DoubleBuffering		bool
			DepthBufferSize		int
			StencilBufferSize	int

			@param iniPath The ini file path
			@param iniKeyName The ini key name to search the properties
		*/
		ContextSettings CreateContextSettings( std::string iniPath, std::string iniKeyName = "EEPP" );

		ContextSettings CreateContextSettings( cIniFile * ini, std::string iniKeyName = "EEPP" );
	protected:
		friend class cWindow;

		Backend::cBackend *	mBackend;
		std::list<cWindow*>	mWindows;
		cWindow *			mWindow;

		cEngine();

		void Destroy();

		Backend::cBackend * CreateSDLBackend( const WindowSettings& Settings );

		Backend::cBackend * CreateAllegroBackend( const WindowSettings& Settings );

		cWindow * CreateSDLWindow( const WindowSettings& Settings, const ContextSettings& Context );

		cWindow * CreateAllegroWindow( const WindowSettings& Settings, const ContextSettings& Context );

		cWindow * CreateDefaultWindow( const WindowSettings& Settings, const ContextSettings& Context );

		Uint32 GetDefaultBackend() const;
};

}}

#endif
