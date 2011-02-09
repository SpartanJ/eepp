#ifndef EE_WINDOWCWINDOW_HPP
#define EE_WINDOWCWINDOW_HPP

#include "base.hpp"
#include "cview.hpp"

namespace EE { namespace Window {

class cClipboard;
class cInput;

namespace WindowStyle {
	enum
	{
		NoBorder 					= ( 0 << 0 ),
		Titlebar					= ( 1 << 0 ),
		Resize						= ( 1 << 1 ),
		Fullscreen					= ( 1 << 2 ),
		UseDesktopResolution		= ( 1 << 3 ),
		Default						= Titlebar | Resize
	};
}

class WindowSettings {
	public:

	inline WindowSettings( Uint32 width, Uint32 height, Uint32 bpp, Uint32 style = WindowStyle::Default, const std::string& icon = std::string(), const std::string& caption = std::string() ) :
		Style( style ),
		Width( width ),
		Height( height ),
		BitsPerPixel( bpp ),
		Icon( icon ),
		Caption( caption )
	{}

	inline WindowSettings() :
		Style( WindowStyle::Default ),
		Width( 800 ),
		Height( 600 ),
		BitsPerPixel( 32 )
	{}

	Uint32			Style;
	Uint32			Width;
	Uint32			Height;
	Uint32			BitsPerPixel;
	std::string		Icon;
	std::string		Caption;
};

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

class WindowInfo {
	public:

	inline WindowInfo() :
		BackgroundColor(0,0,0),
		Created( false ),
		Maximized( false ),
		CursorVisible( true )
	{}

	WindowSettings		WindowConfig;
	ContextSettings		ContextConfig;
	Uint32				Backend;
	eeSize				DesktopResolution;
	eeSize				WindowSize;
	Uint32				Flags;
	eeColor				BackgroundColor;
	bool				Created;
	bool				Maximized;
	bool				CursorVisible;
	eeWindowContex		Context;
};

class EE_API cWindow {
	public:
		typedef cb::Callback0<void>					WindowResizeCallback;

		cWindow( WindowSettings Settings, ContextSettings Context, cClipboard * Clipboard, cInput * Input );
		
		virtual ~cWindow();
		
		/** Creates a new window and GL context */
		virtual bool Create( WindowSettings Settings, ContextSettings Context ) = 0;
		
		/** Toogle the screen to Fullscreen, if it's in fullscreen toogle to windowed mode. */
		virtual void ToggleFullscreen() = 0;
		
		/** Set the window caption */
		virtual void Caption( const std::string& Caption ) = 0;
		
		/** @return The caption of the titlebar */
		virtual std::string Caption() = 0;

		/** Set the Window icon */
		virtual bool Icon( const std::string& Path ) = 0;

		/** This will attempt to iconify/minimize the window. */
		virtual void Minimize() = 0;

		/** Maximize the Window */
		virtual void Maximize() = 0;

		/** This will attempt to hide the window */
		virtual void Hide() = 0;

		/** This will attempt to raise the window */
		virtual void Raise() = 0;

		/** This will attempt to show the window */
		virtual void Show() = 0;

		/** This will attemp to move the window over the desktop to the position */
		virtual void Position( Int16 Left, Int16 Top ) = 0;

		/** @return If the current window is active */
		virtual bool Active() = 0;

		/** @return If the current window is visible */
		virtual bool Visible() = 0;

		/** @return The Current Window Position */
		virtual eeVector2i Position() = 0;

		/** Set to show or not the curson on the main screen */
		virtual void ShowCursor( const bool& showcursor ) = 0;
		
		/** Set the size of the window for a windowed window */
		virtual void Size( const Uint32& Width, const Uint32& Height ) = 0;

		/** Change the window size or the screen resolution
		* @param Width New screen width
		* @param Height New screen height
		* @param Windowed Windowed or Fullscreen
		*/
		virtual void Size( const Uint16& Width, const Uint16& Height, const bool& Windowed ) = 0;

		/** @return The resolutions that support the video card */
		virtual std::vector< std::pair<unsigned int, unsigned int> > GetPossibleResolutions() const = 0;

		/** Set the Screen Gamma. Default is (1,1,1). Accept values between 0.1 and 10. */
		virtual void SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue ) = 0;

		/** The the OpenGL context as the current context */
		virtual void SetCurrentContext( eeWindowContex Context ) = 0;

		/** @return The current OpenGL context */
		virtual eeWindowContex GetContext() const = 0;

		/** @return The window handler */
		virtual eeWindowHandler	GetWindowHandler() = 0;

		/** Set as current context the default context ( the context used for the window creation ) */
		virtual void SetDefaultContext() = 0;

		/** Render the Scene to Screen */
		virtual void Display();

		/** @return The elapsed time for the last frame rendered */
		virtual eeFloat Elapsed() const;

		/** @return The current frames per second of the screen */
		virtual Uint32 FPS() const;

		/** @return If the screen is windowed */
		virtual bool Windowed() const;

		/** @return If the main window is resizeable */
		virtual bool Resizeable() const;

		/** @return The window size */
		virtual eeSize Size();

		/** @return The Window Width */
		virtual const Uint32& GetWidth() const;

		/** @return The Window Height */
		virtual const Uint32& GetHeight() const;

		/** @return The current desktop resolution */
		virtual const eeSize& GetDesktopResolution() const;

		/** @return If the aplication is running returns true ( If you Init correctly the window and is running ). */
		bool Running() const;

		/** @return If the window was created */
		bool Created() const;

		/** Close the window if is running */
		virtual void Close();

		/** Set the current active view
		@param View New view to use (pass GetDefaultView() to set the default view)
		*/
		void SetView( const cView& View );

		/** Get the current view */
		const cView& GetView() const;

		/** Get the default view of the window */
		const cView& GetDefaultView() const;

		/** This will set the default rendering states and view to render in 2D mode */
		void Setup2D( const bool& KeepView = false );

		/**  Set the current Viewport ( and creates a new ortho proyection if needed ) */
		void SetViewport( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height, const bool& UpdateProjectionMatrix = true );

		/** Set the window background color */
		void BackColor( const eeColor& Color );

		/** @return The background clear color */
		const eeColor& BackColor() const;

		/** Captures the window front buffer and saves it to disk. \n
		* You have to call it before Display, and after render all the objects. \n
		* If the file path is empty will save the files like 0001.bmp, and will check if the file exists, otherwise will create 0002.bmp, and so on... \n
		* You can set only the path to save the files, like "screenshots/"
		* @return False if failed, otherwise returns True
		*/
		bool TakeScreenshot( std::string filepath = "", const EE_SAVE_TYPE& Format = EE_SAVE_TYPE_PNG );

		/** @return The pointer to the Window Info ( read only ) */
		const WindowInfo * GetWindowInfo() const;

		/** Set a frame per second limit. It's not 100 % accurate. */
		void FrameRateLimit( const Uint32& FrameRateLimit );

		/** Get a frame per second limit. */
		Uint32 FrameRateLimit();

		/** Set the current Clipping area ( default the entire window, SCISSOR TEST ). */
		void ClipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height );

		/** Disable the Clipping area */
		void ClipDisable();

		/** Clip the area with a plane. */
		void ClipPlaneEnable( const Int32& x, const Int32& y, const Int32& Width, const Int32& Height );

		/** Disable the clip plane area. */
		void ClipPlaneDisable();

		/** @return The clipboard manager */
		cClipboard * GetClipboard() const;
		
		/** @return The input manager */
		cInput * GetInput() const;

		/** Push a new window resize callback.
		* @return The Callback Id
		*/
		Uint32 PushResizeCallback( const WindowResizeCallback& cb );

		/** Pop the callback id indicated. */
		void PopResizeCallback( const Uint32& CallbackId );
	protected:
		WindowInfo			mWindow;
		cClipboard *		mClipboard;
		cInput *			mInput;
		cView				mDefaultView;
		const cView *		mCurrentView;
		Uint32				mNumCallBacks;
		std::map<Uint32, WindowResizeCallback> mCallbacks;
		
		struct _FrameData {
			public:
			struct FPSData {
				Uint32 LastCheck;
				Uint32 Current;
				Uint32 Count;
				eeFloat Limit;
				Int32 Error;
			}				FPS;
			cTimeElapsed	FrameElapsed;
			eeFloat			ElapsedTime;
		} mFrameData;

		/** Swap Buffers call */
		virtual void SwapBuffers() = 0;

		/** Obtain the Main Context, this is called after the OpenGL context creation. */
		virtual void GetMainContext() = 0;

		void SendVideoResizeCb();

		void CreateView();

		void CalculateFps();

		void LimitFps();

		void GetElapsedTime();

		void ViewCheckUpdate();

		void LogSuccessfulInit( const std::string& BackendName );

		void LogFailureInit( const std::string& ClassName, const std::string& BackendName );
};

}}

#endif
