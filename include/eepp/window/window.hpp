#ifndef EE_WINDOWCWINDOW_HPP
#define EE_WINDOWCWINDOW_HPP

#include <eepp/window/base.hpp>
#include <eepp/window/view.hpp>

namespace EE { namespace Window {

namespace Platform { class PlatformImpl; }

class Clipboard;
class Input;
class CursorManager;


/** @namespace EE::Window::WindowStyle Define the Windows Styles */
namespace WindowStyle {
	enum
	{
		NoBorder 					= ( 1 << 0 ),
		Titlebar					= ( 1 << 1 ),
		Resize						= ( 1 << 2 ),
		Fullscreen					= ( 1 << 3 ),
		UseDesktopResolution		= ( 1 << 4 ),
		#if EE_PLATFORM == EE_PLATFORM_IOS || EE_PLATFORM == EE_PLATFORM_ANDROID
		Default						= NoBorder
		#else
		Default						= Titlebar | Resize
		#endif
	};
}

/** @namespace EE::Window::WindowBackend Define the window backends supported by eepp */
namespace WindowBackend {
	enum
	{
		SDL2,
		SFML,
		Default
	};
}

/** @brief WindowSettings A small class that contains the window settings */
class WindowSettings {
	public:

	inline WindowSettings( Uint32 width, Uint32 height, const std::string& caption = std::string(), Uint32 style = WindowStyle::Default, Uint32 backend = WindowBackend::Default, Uint32 bpp = 32, const std::string& icon = std::string(), const Float& pixelDensity = 1 ) :
		Style( style ),
		Width( width ),
		Height( height ),
		BitsPerPixel( bpp ),
		Icon( icon ),
		Caption( caption ),
		Backend( backend ),
		PixelDensity( pixelDensity )
	{}

	inline WindowSettings() :
		Style( WindowStyle::Default ),
		Width( 800 ),
		Height( 600 ),
		BitsPerPixel( 32 ),
		Backend( WindowBackend::Default ),
		PixelDensity( 1 )
	{}

	Uint32			Style;
	Uint32			Width;
	Uint32			Height;
	Uint32			BitsPerPixel;
	std::string		Icon;
	std::string		Caption;
	Uint32			Backend;
	Float			PixelDensity;
};

/** @brief ContextSettings Small class that contains the renderer context information */
class ContextSettings {
	public:

	inline ContextSettings( bool vsync, EEGL_version version = GLv_default, bool doubleBuffering = true, Uint32 depthBufferSize = 24, Uint32 stencilBufferSize = 1 ) :
		Version( version ),
		DepthBufferSize( depthBufferSize ),
		StencilBufferSize( stencilBufferSize ),
		VSync( vsync ),
		DoubleBuffering( doubleBuffering )
	{}

	inline ContextSettings() :
		Version( GLv_default ),
		DepthBufferSize( 24 ),
		StencilBufferSize( 1 ),
		VSync( false ),
		DoubleBuffering( true )
	{}

	EEGL_version	Version;
	Uint32			DepthBufferSize;
	Uint32			StencilBufferSize;
	bool			VSync;
	bool			DoubleBuffering;
};

/** @brief WindowInfo Contains the window state information */
class WindowInfo {
	public:

	inline WindowInfo() :
		BackgroundColor(0,0,0),
		Created( false ),
		Maximized( false )
	{}

	WindowSettings		WindowConfig;
	ContextSettings		ContextConfig;
	Uint32				Backend;
	Sizei				DesktopResolution;
	Sizei				WindowSize;
	Uint32				Flags;
	RGB				BackgroundColor;
	bool				Created;
	bool				Maximized;
	eeWindowContex		Context;
};

/** @brief DisplayMode contains a display mode available to use */
class DisplayMode {
	public:

	inline DisplayMode( int width, int height, int refreshRate, int screenIndex ) :
		Width( width ),
		Height( height ),
		RefreshRate( refreshRate ),
		ScreenIndex( screenIndex )
	{
	}

	Uint32 Width;
	Uint32 Height;
	Uint32 RefreshRate;
	Uint32 ScreenIndex;
};


/* See the official Android developer guide for more information:
   http://developer.android.com/guide/topics/data/data-storage.html
*/
#define EE_ANDROID_EXTERNAL_STORAGE_READ   0x01
#define EE_ANDROID_EXTERNAL_STORAGE_WRITE  0x02

class EE_API Window {
	public:
		typedef cb::Callback1<void, Window*>			WindowResizeCallback;

		Window( WindowSettings Settings, ContextSettings Context, Clipboard * Clipboard, Input * Input, CursorManager * CursorManager );
		
		virtual ~Window();
		
		/** Creates a new window and GL context */
		virtual bool create( WindowSettings Settings, ContextSettings Context ) = 0;
		
		/** Toogle the screen to Fullscreen, if it's in fullscreen toogle to windowed mode. */
		virtual void toggleFullscreen() = 0;
		
		/** Set the window caption */
		virtual void setCaption( const std::string& setCaption ) = 0;

		/** @return The caption of the titlebar */
		virtual std::string getCaption();

		/** Set the Window icon */
		virtual bool setIcon( const std::string& Path ) = 0;

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

		/** This will attempt to show the window */
		virtual void show();

		/** This will attemp to move the window over the desktop to the position */
		virtual void setPosition( Int16 Left, Int16 Top );

		/** @return The Current Window Position */
		virtual Vector2i getPosition();

		/** Set as current context the default context ( the context used for the window creation ) */
		virtual void setDefaultContext();

		/** @return If the current window is active */
		virtual bool isActive() = 0;

		/** @return If the current window is visible */
		virtual bool isVisible() = 0;

		/** Set the size of the window for a windowed window */
		virtual void setSize( Uint32 Width, Uint32 Height );

		/** Change the window size or the screen resolution
		* @param Width New screen width
		* @param Height New screen height
		* @param Windowed Windowed or Fullscreen
		*/
		virtual void setSize( Uint32 Width, Uint32 Height, bool isWindowed ) = 0;

		/** @return The window size */
		virtual Sizei getSize();

		/** @return The resolutions that support the video card */
		virtual std::vector<DisplayMode> getDisplayModes() const = 0;

		/** Set the Screen Gamma. Default is (1,1,1). Accept values between 0.1 and 10. */
		virtual void setGamma( Float Red, Float Green, Float Blue ) = 0;

		/** The the OpenGL context as the current context */
		virtual void setCurrentContext( eeWindowContex Context );

		/** @return The current OpenGL context */
		virtual eeWindowContex getContext() const;

		/** @return The window handler */
		virtual eeWindowHandle	getWindowHandler() = 0;

		/** @brief Clear the window back buffer
		This function is usually called once every frame, to clear the previous frame content.
		*/
		virtual void clear();

		/** Render the Scene to Screen
		@param clear Clear after swapping buffers? It will not work if the target platform is Emscripten. Since there's no swap buffers.
		*/
		virtual void display( bool clear = false );

		/** @return The elapsed time for the last frame rendered */
		virtual System::Time getElapsed() const;

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
		virtual void centerToScreen();

		/** @return If the aplication is running returns true ( If you Init correctly the window and is running ). */
		bool isRunning() const;

		/** @return If the window was created */
		bool isOpen() const;

		/** Close the window if is running */
		virtual void close();

		/** Set the current active view
		@param View New view to use (pass GetDefaultView() to set the default view)
		*/
		void setView( const View& View );

		/** Get the current view */
		const View& getView() const;

		/** Get the default view of the window */
		const View& getDefaultView() const;

		/** This will set the default rendering states and view to render in 2D mode */
		void setup2D( const bool& KeepView = false );

		/** Set a new 2D projection matrix */
		void set2DProjection( const Uint32& Width, const Uint32& Height );

		/** Set the current Viewport ( and creates a new ortho proyection if needed ) */
		void setViewport( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height, const bool& UpdateProjectionMatrix = true );

		/** Set the window background color */
		void setBackColor( const RGB& Color );

		/** @return The background clear color */
		const RGB& getBackColor() const;

		/** Captures the window front buffer and saves it to disk. \n
		* You have to call it before Display, and after render all the objects. \n
		* If the file path is empty will save the files like 0001.bmp, and will check if the file exists, otherwise will create 0002.bmp, and so on... \n
		* You can set only the path to save the files, like "screenshots/"
		* @return False if failed, otherwise returns True
		*/
		bool takeScreenshot( std::string filepath = "", const EE_SAVE_TYPE& Format = SAVE_TYPE_PNG );

		/** @return The pointer to the Window Info ( read only ) */
		const WindowInfo * getWindowInfo() const;

		/** Set a frame per second limit. It's not 100 % accurate. */
		void setFrameRateLimit( const Uint32& setFrameRateLimit );

		/** Get a frame per second limit. */
		Uint32 getFrameRateLimit();

		/** Set the current Clipping area ( default the entire window, SCISSOR TEST ). */
		void clipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height );

		/** Disable the Clipping area */
		void clipDisable();

		/** Clip the area with a plane. */
		void clipPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height );

		/** Disable the clip plane area. */
		void clipPlaneDisable();

		/** @return The clipboard manager */
		Clipboard * getClipboard() const;
		
		/** @return The input manager */
		Input * getInput() const;

		/** @return The cursor manager */
		CursorManager * getCursorManager() const;

		/** Push a new window resize callback.
		* @return The Callback Id
		*/
		Uint32 pushResizeCallback( const WindowResizeCallback& cb );

		/** Pop the callback id indicated. */
		void popResizeCallback( const Uint32& CallbackId );

		/** @return The pointer to the instance of the platform implementation functions. */
		Platform::PlatformImpl * getPlatform() const;

		/**
		* @brief Start accepting Unicode text input events.
		*        This function will show the on-screen keyboard if supported.
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
		virtual bool isTextInputActive();

		/**
		* @brief Stop receiving any text input events.
		*        This function will hide the on-screen keyboard if supported.
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
		virtual void setTextInputRect( Recti& rect );

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

#if EE_PLATFORM == EE_PLATFORM_ANDROID
		/** @return The JNI environment for the current thread
		* This returns JNIEnv*, but the prototype is void* so we don't need jni.h
		*/
		virtual void * getJNIEnv();

		/** @return The SDL Activity object for the application
		* This returns jobject, but the prototype is void* so we don't need jni.h
		*/
		virtual void * getActivity();

		/** @return The current state of external storage, a bitmask of these values:
		* EE_ANDROID_EXTERNAL_STORAGE_READ
		* EE_ANDROID_EXTERNAL_STORAGE_WRITE
		* If external storage is currently unavailable, this will return 0.
		*/
		virtual int getExternalStorageState();

		/** @return The path used for internal storage for this application.
		* This path is unique to your application and cannot be written to
		* by other applications.
		*/
		virtual std::string getInternalStoragePath();

		/** @return The path used for external storage for this application.
		* This path is unique to your application, but is public and can be
		* written to by other applications.
		*/
		virtual std::string getExternalStoragePath();

		/** @return The application APK file path */
		virtual std::string getApkPath();
#endif

		/** @return True if the current window support a threaded GL Context. This means that supports OpenGL Shared Contexts ( multithreaded opengl contexts ).
		**	Only supported with SDL2 backend.*/
		virtual bool isThreadedGLContext();

		/** Activates the shared GL context in the current thread. */
		virtual void setGLContextThread();

		/** Deactviates the shared GL context in the current thread. */
		virtual void unsetGLContextThread();

		/** Runs the main loop function passed as parameter
		**	@param func The main loop function
		**	@param fps The desired FPS ( 0 = infinite ) */
		void runMainLoop( void (*func)(), int fps = 0 );
	protected:
		friend class Engine;

		WindowInfo mWindow;
		Clipboard *	mClipboard;
		Input *	mInput;
		CursorManager *	mCursorManager;
		Platform::PlatformImpl * mPlatform;
		View mDefaultView;
		const View * mCurrentView;
		Uint32 mNumCallBacks;
		std::map<Uint32, WindowResizeCallback> mCallbacks;
		
		class FrameData {
			public:
				class cFPSData {
					public:
					cFPSData() :
						LastCheck(0),
						Current(0),
						Count(0),
						Limit(0),
						Error(0)
					{}

					Uint32 LastCheck;
					Uint32 Current;
					Uint32 Count;
					Float Limit;
					Int32 Error;
				};

				cFPSData		FPS;
				Clock *			FrameElapsed;
				System::Time	ElapsedTime;

				FrameData();

				~FrameData();
		};

		FrameData mFrameData;
		std::list<Rectf> mScissorsClipped;
		bool mPushScissorClip;

		/** Set the flag state to be the current window */
		virtual void setCurrent();

		/** Swap Buffers call */
		virtual void swapBuffers() = 0;

		/** Obtain the Main Context, this is called after the OpenGL context creation. */
		virtual void getMainContext();

		virtual void createPlatform();

		void sendVideoResizeCb();

		void createView();

		void calculateFps();

		void limitFps();

		void getElapsedTime();

		void viewCheckUpdate();

		void logSuccessfulInit( const std::string& BackendName, const std::string& ProcessPath = "" );

		void logFailureInit( const std::string& ClassName, const std::string& BackendName );
};

}}

#endif
