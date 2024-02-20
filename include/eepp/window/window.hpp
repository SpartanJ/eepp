#ifndef EE_WINDOWCWINDOW_HPP
#define EE_WINDOWCWINDOW_HPP

#include <eepp/graphics/image.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/view.hpp>
#include <eepp/window/base.hpp>
#include <eepp/window/inputmethod.hpp>

namespace EE { namespace Window {

namespace Platform {
class PlatformImpl;
}

class Clipboard;
class Input;
class CursorManager;

enum WindowStyle {
	Borderless = ( 1 << 0 ),
	Titlebar = ( 1 << 1 ),
	Resize = ( 1 << 2 ),
	Fullscreen = ( 1 << 3 ),
	UseDesktopResolution = ( 1 << 4 ),
#if EE_PLATFORM == EE_PLATFORM_IOS || EE_PLATFORM == EE_PLATFORM_ANDROID
	Default = Borderless
#else
	Default = Titlebar | Resize
#endif
};

enum class WindowFlashOperation {
	Cancel,
	Briefly,
	UntilFocused,
};

enum class WindowBackend : Uint32 { SDL2, Default };

#ifndef EE_SCREEN_KEYBOARD_ENABLED
#define EE_SCREEN_KEYBOARD_ENABLED false
#endif

/** @brief WindowSettings A small class that contains the window settings */
class WindowSettings {
  public:
	inline WindowSettings( Uint32 width, Uint32 height, const std::string& title = std::string(),
						   Uint32 style = WindowStyle::Default,
						   WindowBackend backend = WindowBackend::Default, Uint32 bpp = 32,
						   const std::string& icon = std::string(), const Float& pixelDensity = 1,
						   const bool& useScreenKeyboard = EE_SCREEN_KEYBOARD_ENABLED ) :
		Style( style ),
		Width( width ),
		Height( height ),
		BitsPerPixel( bpp ),
		Icon( icon ),
		Title( title ),
		Backend( backend ),
		PixelDensity( pixelDensity ),
		UseScreenKeyboard( useScreenKeyboard ) {}

	inline WindowSettings() :
		Style( WindowStyle::Default ),
		Width( 800 ),
		Height( 600 ),
		BitsPerPixel( 32 ),
		Backend( WindowBackend::Default ),
		PixelDensity( 1 ),
		UseScreenKeyboard( EE_SCREEN_KEYBOARD_ENABLED ) {}

	Uint32 Style;
	Uint32 Width;
	Uint32 Height;
	Uint32 BitsPerPixel;
	std::string Icon;
	std::string Title;
	WindowBackend Backend;
	Float PixelDensity;
	bool UseScreenKeyboard;
};

/** @brief ContextSettings Small class that contains the renderer context information */
class ContextSettings {
  public:
	inline ContextSettings( bool vsync, GraphicsLibraryVersion version = GLv_default,
							bool doubleBuffering = true, Uint32 depthBufferSize = 24,
							Uint32 stencilBufferSize = 1, Uint32 multisamples = 0,
							bool sharedGLContext = true, Int32 frameRateLimit = 0 ) :
		Version( version ),
		DepthBufferSize( depthBufferSize ),
		StencilBufferSize( stencilBufferSize ),
		Multisamples( multisamples ),
		FrameRateLimit( frameRateLimit ),
		VSync( vsync ),
		DoubleBuffering( doubleBuffering ),
		SharedGLContext( sharedGLContext ) {}

	inline ContextSettings() :
		Version( GLv_default ),
		DepthBufferSize( 24 ),
		StencilBufferSize( 1 ),
		Multisamples( 0 ),
		FrameRateLimit( 0 ),
		VSync( false ),
		DoubleBuffering( true ),
		SharedGLContext( true ) {}

	GraphicsLibraryVersion Version;
	Uint32 DepthBufferSize;
	Uint32 StencilBufferSize;
	Uint32 Multisamples;
	Int32 FrameRateLimit;
	bool VSync;
	bool DoubleBuffering;
	bool SharedGLContext;
};

/** @brief WindowInfo Contains the window state information */
class WindowInfo {
  public:
	inline WindowInfo() :
		Backend( WindowBackend::Default ),
		Flags( 0 ),
		ClearColor( 0, 0, 0 ),
		Created( false ),
		Maximized( false ),
		Context( 0 ) {}

	Sizei getWindowSize() const { return Sizei( WindowConfig.Width, WindowConfig.Height ); }

	WindowSettings WindowConfig;
	ContextSettings ContextConfig;
	WindowBackend Backend;
	Sizei DesktopResolution;
	Sizei WindowSize;
	Uint32 Flags;
	RGB ClearColor;
	bool Created;
	bool Maximized;
	eeWindowContex Context;
};

/** @brief DisplayMode contains a display mode available to use */
class DisplayMode {
  public:
	inline DisplayMode( int width, int height, int refreshRate, int screenIndex ) :
		Width( width ), Height( height ), RefreshRate( refreshRate ), ScreenIndex( screenIndex ) {}

	Uint32 Width;
	Uint32 Height;
	Uint32 RefreshRate;
	Uint32 ScreenIndex;
};

class EE_API Window {
  public:
	typedef std::function<void( Window* )> WindowResizeCallback;
	typedef std::function<bool( Window* )> WindowRequestCloseCallback;
	typedef std::function<void( Window* )> WindowQuitCallback;

	Window( WindowSettings Settings, ContextSettings Context, Clipboard* Clipboard, Input* Input,
			CursorManager* CursorManager );

	virtual ~Window();

	/** Creates a new window and GL context */
	virtual bool create( WindowSettings Settings, ContextSettings Context ) = 0;

	/** Bind the OpenGL context to the current window */
	virtual void makeCurrent() = 0;

	virtual Uint32 getWindowID() = 0;

	/** Toogle the screen to Fullscreen, if it's in fullscreen toogle to windowed mode. */
	virtual void toggleFullscreen() = 0;

	/** Set the window title */
	virtual void setTitle( const std::string& title ) = 0;

	/** @return The window title*/
	virtual std::string getTitle();

	/** Set the Window icon */
	virtual bool setIcon( const std::string& path ) = 0;

	/** This will attempt to iconify/minimize the window. */
	virtual void minimize();

	/** Maximize the Window */
	virtual void maximize();

	/** @return true if the window is maximized */
	virtual bool isMaximized();

	/** This will attempt to hide the window */
	virtual void hide();

	/** This will attempt to raise the window */
	virtual void raise();

	/** Request a window to demand attention from the user. */
	virtual void flash( WindowFlashOperation op );

	/** This will attempt to show the window */
	virtual void show();

	/** This will attemp to move the window over the desktop to the position */
	virtual void setPosition( int Left, int Top );

	/** @return The Current Window Position */
	virtual Vector2i getPosition();

	/** Set as current context the default context ( the context used for the window creation ) */
	virtual void setDefaultContext();

	/** @return If the current window is active. This means that the window hasInputFocus() and
	 * hasMouseFocus(). */
	virtual bool isActive() = 0;

	/** @return If the current window is visible */
	virtual bool isVisible() = 0;

	/** @return If the current window has focus (same as hasInputFocus() or(||) hasMouseFocus()) */
	virtual bool hasFocus() = 0;

	/** @return If the current window has input focus */
	virtual bool hasInputFocus() = 0;

	/** @return If the current window has input focus */
	virtual bool hasMouseFocus() = 0;

	/** Set the size of the window for a windowed window */
	virtual void setSize( Uint32 Width, Uint32 Height );

	/** Change the window size or the screen resolution
	 * @param Width New screen width
	 * @param Height New screen height
	 * @param isWindowed Windowed or Fullscreen
	 */
	virtual void setSize( Uint32 Width, Uint32 Height, bool isWindowed ) = 0;

	/** @return The window size */
	virtual Sizei getSize();

	/** @return The window center point */
	Vector2f getCenter();

	/** @return The resolutions that support the video card */
	virtual std::vector<DisplayMode> getDisplayModes() const = 0;

	/** Set the Screen Gamma. Default is (1,1,1). Accept values between 0.1 and 10. */
	virtual void setGamma( Float Red, Float Green, Float Blue ) = 0;

	/** The the OpenGL context as the current context */
	virtual void setCurrentContext( eeWindowContex Context );

	/** @return The current OpenGL context */
	virtual eeWindowContex getContext() const;

	/** @return The window handler */
	virtual eeWindowHandle getWindowHandler() = 0;

	/** @brief Clear the window back buffer
	This function is usually called once every frame, to clear the previous frame content.
	*/
	virtual void clear();

	/** Render the Scene to Screen
	@param clear Clear after swapping buffers? It will not work if the target platform is
	Emscripten. Since there's no swap buffers.
	*/
	virtual void display( bool clear = false );

	/** @return The elapsed time for the last frame rendered */
	virtual const System::Time& getElapsed() const;

	/** @return The current frames per second of the screen */
	virtual Uint32 getFPS() const;

	/** @return If the screen is windowed */
	virtual bool isWindowed() const;

	/** @return If the main window is resizeable */
	virtual bool isResizeable() const;

	/** @return The Window Width */
	virtual const Uint32& getWidth() const;

	/** @return The Window Height */
	virtual const Uint32& getHeight() const;

	/** @return The current desktop resolution */
	virtual const Sizei& getDesktopResolution();

	/** Center the window to the desktop ( if windowed ) */
	virtual void centerToDisplay();

	/** @return The window borders size */
	virtual Rect getBorderSize();

	/** @return The size of the pixel in screen coordinates. This is the device scale factor. */
	virtual Float getScale();

	/** @return If the aplication is running returns true ( If you Init correctly the window and is
	 * running ). */
	bool isRunning() const;

	/** @return If the window was created */
	bool isOpen() const;

	/** Close the window if is running */
	virtual void close();

	/** Set the current active view
	 * @param view New view to use (pass GetDefaultView() to set the default view)
	 * @param forceRefresh Forces the view refresh even if is the same as the last one.
	 */
	void setView( const View& view, bool forceRefresh = false );

	/** Get the current view */
	const View& getView() const;

	/** Get the default view of the window */
	const View& getDefaultView() const;

	/** This will set the default rendering states and view to render in 2D mode */
	void setup2D( const bool& KeepView = true );

	/** Set a new 2D projection matrix */
	void set2DProjection( const Uint32& Width, const Uint32& Height );

	/** Set a new projection matrix */
	void setProjection( const Transform& transform );

	/** Set the current Viewport ( and creates a new ortho proyection if needed ) */
	void setViewport( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height );

	/** @return The viewport in pixels of the view */
	Rect getViewport( const View& view );

	/** Set the window background color */
	void setClearColor( const RGB& Color );

	/** @return The background clear color */
	RGB getClearColor() const;

	/** Captures the window front buffer and saves it to disk. \n
	 * You have to call it before Display, and after render all the objects. \n
	 * If the file path is empty will save the files like 0001.bmp, and will check if the file
	 * exists, otherwise will create 0002.bmp, and so on... \n You can set only the path to save the
	 * files, like "screenshots/"
	 * @return False if failed, otherwise returns True
	 */
	bool takeScreenshot( std::string filepath = "",
						 const Image::SaveType& Format = Image::SaveType::SAVE_TYPE_PNG );

	/** @return The pointer to the Window Info ( read only ) */
	const WindowInfo* getWindowInfo() const;

	/** Set a frame per second limit. It's not 100 % accurate. */
	void setFrameRateLimit( const Uint32& setFrameRateLimit );

	/** Get a frame per second limit. */
	Uint32 getFrameRateLimit();

	/** @return The clipboard manager */
	Clipboard* getClipboard() const;

	/** @return The input manager */
	Input* getInput() const;

	/** @return The cursor manager */
	CursorManager* getCursorManager() const;

	/** Push a new window resize callback.
	 * @return The Callback Id
	 */
	Uint32 pushResizeCallback( const WindowResizeCallback& cb );

	/** Pop the callback id indicated. */
	void popResizeCallback( const Uint32& CallbackId );

	/** @brief Show the on-screen keyboard if supported. */
	virtual void startOnScreenKeyboard();

	/** @brief Hide the on-screen keyboard if supported. */
	virtual void stopOnScreenKeyboard();

	/** @return True if on-screen keyboard is active. */
	virtual bool isOnScreenKeyboardActive() const;

	/**
	 * @brief Start accepting Unicode text input events.
	 *
	 * @sa stopTextInput()
	 * @sa setTextInputRect()
	 * @sa hasScreenKeyboardSupport()
	 */
	virtual void startTextInput();

	/**
	 * @brief Return whether or not Unicode text input events are enabled.
	 *
	 * @sa startTextInput()
	 * @sa stopTextInput()
	 */
	virtual bool isTextInputActive() const;

	/**
	 * @brief Stop receiving any text input events.
	 *
	 * @sa startTextInput()
	 * @sa hasScreenKeyboardSupport()
	 */
	virtual void stopTextInput();

	/**
	 * @brief Set the rectangle used to type Unicode text inputs.
	 *        This is used as a hint for IME and on-screen keyboard placement.
	 *
	 * @sa startTextInput()
	 */
	virtual void setTextInputRect( const Rect& rect );

	/**
	 * Dismiss the composition window/IME without disabling the subsystem.
	 *
	 * @sa startTextInput
	 * @sa stopTextInput
	 */
	virtual void clearComposition();

	/**
	 * @brief Returns whether the platform has some screen keyboard support.
	 * @return true if some keyboard support is available else false.
	 * @note Not all screen keyboard functions are supported on all platforms.
	 *
	 * @sa isScreenKeyboardShown()
	 */
	virtual bool hasScreenKeyboardSupport();

	/**
	 * @brief Returns whether the screen keyboard is shown for given window.
	 * @return true if screen keyboard is shown else false.
	 *
	 * @sa hasScreenKeyboardSupport()
	 */
	virtual bool isScreenKeyboardShown();

	/** @return True if the current window support a threaded GL Context. This means that supports
	 *OpenGL Shared Contexts ( multithreaded opengl contexts ). *	Only supported with SDL2
	 *backend.*/
	virtual bool isThreadedGLContext();

	/** Activates the shared GL context in the current thread. */
	virtual void setGLContextThread();

	/** Deactviates the shared GL context in the current thread. */
	virtual void unsetGLContextThread();

	/** Runs the main loop function passed as parameter
	**	@param func The main loop function
	**	@param fps The desired FPS ( 0 = infinite, < 0 keep current setting ) */
	void runMainLoop( std::function<void()> func, int fps = -1 );

	/** @return The current display index. */
	virtual int getCurrentDisplayIndex();

	Vector2f mapPixelToCoords( const Vector2i& point );

	Vector2f mapPixelToCoords( const Vector2i& point, const View& view );

	Vector2i mapCoordsToPixel( const Vector2f& point );

	Vector2i mapCoordsToPixel( const Vector2f& point, const View& view );

	void setCloseRequestCallback( const WindowRequestCloseCallback& closeRequestCallback );

	void setQuitCallback( const WindowQuitCallback& quitCallback );

	/** In case of a frame rate limit is set, this will return the time spent sleeping per second.
	 */
	const System::Time& getSleepTimePerSecond() const;

	/** In case of a frame rate limit is set, this will return the time spent doing work/rendering
	 * per second. */
	const System::Time& getRenderTimePerSecond() const;

	/** @return The last windowed size of the window */
	const Sizei& getLastWindowedSize() const;

	/** @return True if implements native message boxes */
	virtual bool hasNativeMessageBox() const { return false; };

	/** Native message box types */
	enum class MessageBoxType { Error, Warning, Information };

	/** Shows a native message box.
	 *  @return True if message box was shown
	 */
	virtual bool showMessageBox( const MessageBoxType& type, const std::string& title,
								 const std::string& message );

	InputMethod& getIME();

	const std::function<void()>& getMainLoop() { return mMainLoop; }

  protected:
	friend class Engine;
	friend class Input;

	WindowInfo mWindow;
	Clipboard* mClipboard;
	Input* mInput;
	CursorManager* mCursorManager;
	View mDefaultView;
	const View* mCurrentView;
	Uint32 mNumCallBacks;
	std::map<Uint32, WindowResizeCallback> mCallbacks;
	WindowRequestCloseCallback mCloseRequestCallback;
	WindowQuitCallback mQuitCallback;
	Sizei mLastWindowedSize;
	InputMethod mIME;
	std::function<void()> mMainLoop;

	class FrameData {
	  public:
		class FPSData {
		  public:
			FPSData() :
				LastCheck(), Current( 0 ), Count( 0 ), Limit( 0 ), Error( System::Time::Zero ) {}

			Clock LastCheck;
			Uint32 Current;
			Uint32 Count;
			Float Limit;
			System::Time Error;
			System::Time SleepTime;
			System::Time CurSleepTime;
			System::Time FrameTime;
			System::Time RenderTime;
			System::Time CurRenderTime;
			Clock RenderClock;
		};

		FPSData FPS;
		Clock* FrameElapsed;
		System::Time ElapsedTime;

		FrameData();

		~FrameData();
	};

	FrameData mFrameData;

	/** Set the flag state to be the current window */
	virtual void setCurrent();

	/** Swap Buffers call */
	virtual void swapBuffers() = 0;

	/** Obtain the Main Context, this is called after the OpenGL context creation. */
	virtual void getMainContext();

	virtual void onWindowResize( Uint32 Width, Uint32 Height ) = 0;

	void sendVideoResizeCb();

	void createView();

	void calculateFps();

	void limitFps();

	void updateElapsedTime();

	void logSuccessfulInit( const std::string& BackendName );

	void logFailureInit( const std::string& ClassName, const std::string& BackendName );

	void onCloseRequest();

	void onQuit();
};

}} // namespace EE::Window

#endif
