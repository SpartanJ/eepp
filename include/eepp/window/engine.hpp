#ifndef EE_WINDOWCENGINE_H
#define EE_WINDOWCENGINE_H

#include <eepp/window/base.hpp>
#include <eepp/window/displaymanager.hpp>
#include <eepp/window/platformhelper.hpp>
#include <eepp/window/window.hpp>
#include <list>

namespace EE { namespace System {
class IniFile;
class Pack;
}} // namespace EE::System
namespace EE { namespace Window { namespace Backend {
class WindowBackendLibrary;
}}} // namespace EE::Window::Backend

namespace EE { namespace Window {

/** @brief The window management class. Here the engine starts working. (Singleton Class). */
class EE_API Engine {
	SINGLETON_DECLARE_HEADERS( Engine )

  public:
	~Engine();

	/** Creates a new window. */
	EE::Window::Window* createWindow( WindowSettings Settings,
									  ContextSettings Context = ContextSettings() );

	/** Destroy the window instance, and set as current other window running ( if any ).
	 * This function is only useful for multi-window environment. Avoid using it with one window
	 * context.
	 */
	void destroyWindow( EE::Window::Window* window );

	/** @return The current Window context. */
	EE::Window::Window* getCurrentWindow() const;

	/** Set the window as the current. */
	void setCurrentWindow( EE::Window::Window* window );

	/** @return The number of windows created. */
	Uint32 getWindowCount() const;

	/** @return If any window is created. */
	bool isRunning() const;

	/** @return If the window instance is inside the window list. */
	bool existsWindow( EE::Window::Window* window );

	/** Constructs WindowSettings from an ini file
	It will search for the following properties:
		Width			Window width
		Height			Window height
		BitColor		32,16,8
		Windowed		bool
		Resizeable		bool
		Backend			SDL2
		WinIcon			The path to the window icon
		WinTitle		The window default title

		@param iniPath The ini file path
		@param iniKeyName The ini key name to search the properties
	*/
	WindowSettings createWindowSettings( std::string iniPath, std::string iniKeyName = "EEPP" );

	/** Constructs WindowSettings from an ini file instance
	It will search for the following properties:
		Width			Window width
		Height			Window height
		BitColor		32,16,8
		Windowed		bool
		Resizeable		bool
		Backend			SDL2
		WinIcon			The path to the window icon
		WinTitle		The window default title

		@param ini The ini file instance
		@param iniKeyName The ini key name to search the properties
	*/
	WindowSettings createWindowSettings( IniFile* ini, std::string iniKeyName = "EEPP" );

	/** Constructs ContextSettings from an ini file\n
	It will search for the following properties:
		VSync				bool
		GLVersion			Selects the default renderer: 2 for OpenGL 2, 3 for OpenGL 3, 4 for
	OpenGL ES 2 DoubleBuffering		bool DepthBufferSize		int StencilBufferSize	int

		@param iniPath The ini file path
		@param iniKeyName The ini key name to search the properties
	*/
	ContextSettings createContextSettings( std::string iniPath, std::string iniKeyName = "EEPP" );

	/** Constructs ContextSettings from an ini file instance\n
	It will search for the following properties:
		VSync				bool
		GLVersion			Selects the default renderer: 2 for OpenGL 2, 3 for OpenGL 3, 4 for
	OpenGL ES 2 DoubleBuffering		bool DepthBufferSize		int StencilBufferSize	int

		@param ini The ini file instance
		@param iniKeyName The ini key name to search the properties
	*/
	ContextSettings createContextSettings( IniFile* ini, std::string iniKeyName = "EEPP" );

	/** Enabling Shared GL Context allows asynchronous OpenGL resource loading ( only if is
	 *supported by the backend and the OS, SDL 2 backend is the only one supported ). *	If the
	 *TextureLoader is threaded, will upload the texture in another thread to the GPU. So, it will
	 *not block the main rendering thread. *	Shared GL Context is disabled by default.
	 */
	void enableSharedGLContext();

	/** Disable the Shared GL Context
	**	@see enableSharedGLContext()
	*/
	void disableSharedGLContext();

	/** @return If the Shared GL Context is enabled and ready to use. */
	bool isSharedGLContextEnabled();

	/** @return Indicates if the current running platform/OS supports threads (always true except
	 * for emscripten) */
	bool isThreaded();

	/** @return The id of the thread that was used to initialize the OpenGL Context. */
	Uint32 getMainThreadId();

	/** @returns True if the current thread is the main thread. */
	bool isMainThread() const;

	/** @return The instance of platform class that provides some helpers for some platforms */
	PlatformHelper* getPlatformHelper();

	/** @return The display manager. Holds the physical displays information. */
	DisplayManager* getDisplayManager();

	/** Open a URL in a separate, system-provided application.
	 * @return true if success
	 */
	bool openURI( const std::string& url );

  protected:
	friend class Window;

	Backend::WindowBackendLibrary* mBackend;
	std::list<Window*> mWindows;
	EE::Window::Window* mWindow;
	bool mSharedGLContext;
	Uint32 mMainThreadId;
	PlatformHelper* mPlatformHelper;
	Pack* mZip;
	DisplayManager* mDisplayManager;

	Engine();

	void destroy();

	Backend::WindowBackendLibrary* createSDL2Backend( const WindowSettings& Settings );

	EE::Window::Window* createSDL2Window( const WindowSettings& Settings,
										  const ContextSettings& Context );

	EE::Window::Window* createDefaultWindow( const WindowSettings& Settings,
											 const ContextSettings& Context );

	WindowBackend getDefaultBackend() const;
};

}} // namespace EE::Window

#endif
