#ifndef EE_WINDOWCENGINE_H
#define EE_WINDOWCENGINE_H

#include "base.hpp"
#include "cview.hpp"

namespace EE { namespace Window {

#if EE_PLATFORM == EE_PLATFORM_LINUX
typedef Atom eeScrapType;
#elif EE_PLATFORM == EE_PLATFORM_WIN32
typedef UINT eeScrapType;
#else
typedef Uint32 eeScrapType;
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN32
inline BOOL WIN_ShowWindow( HWND hWnd, int nCmdShow ) {
    return ShowWindow( hWnd, nCmdShow );
}
#endif

/** @brief The basic Graphics class. Here Init the context and render to screen. (Singleton Class). */
class EE_API cEngine : public cSingleton<cEngine> {
	friend class cSingleton<cEngine>;
	public:
		/** Init the opengl context and the screen
		* @param Width Screen Width
		* @param Height Screen Height
		* @param BitColor Screen Color Depth
		* @param Windowed Windowed or Fullscreen
		* @param Resizeable If the Screen is resizeable
		* @param VSync Enable or Disable VSync
		* @param DoubleBuffering Enable or Disable the Double Buffering
		* @param UseDesktopResolution Instead of using Screen Width and Screen Height passed, will use the default desktop resolution
		* @param NoFrame If create the window without window manager frame.
		* @return True if success
		*/
		bool Init(const Uint32& Width = 640, const Uint32& Height = 480, const Uint8& BitColor = 32, const bool& Windowed = true, const bool& Resizeable = false, const bool& VSync = false, const bool& DoubleBuffering = true, const bool& UseDesktopResolution = false, const bool& NoFrame = false);

		/** Render the Scene to Screen */
		void Display();

		/** Set the window background color */
		void SetBackColor(const eeColor& Color);

		/** Set the window caption */
		void SetWindowCaption(const std::string& Caption);

		/** @return The window width */
		Uint32 GetWidth() const { return mVideoInfo.Width; }

		/** @return The window height */
		Uint32 GetHeight() const { return mVideoInfo.Height; }

		/** @return The desktop width */
		Uint32 GetDeskWidth() const { return mVideoInfo.DeskWidth; }

		/** @return The desktop height */
		Uint32 GetDeskHeight() const { return mVideoInfo.DeskHeight; }

		/** @return The elapsed time for the last frame rendered */
		eeFloat Elapsed() const;

		/** @return If the aplication is running returns true ( If you Init correctly the window and is running ). */
		bool Running() const { return mInit; }

		/** You can stop the aplication setting running as false */
		void Running(const bool& running) { mInit = running; }

		/** Toogle the screen to Fullscreen, if it's in fullscreen toogle to windowed mode. \n This it's not fully supported on Windows platform. Use ChangeRes instead.
		*/
		void ToggleFullscreen();

		/** Change the screen resolution
		 * @param width New screen width
		 * @param height New screen height
		 * @param Windowed Windowed or Fullscreen
		 */
		void ChangeRes( const Uint16& width, const Uint16& height, const bool& Windowed );

		/** @return If the main window is resizeable */
		bool Reziseable() const { return mVideoInfo.Resizeable; }

		/** Set a frame per second limit. It's not 100 % accurate. */
		void SetFrameRateLimit(const Uint32& FrameRateLimit);

		/** Get a frame per second limit. */
		Uint32 GetFrameRateLimit() { return static_cast<Uint32>( mFrames.FPS.Limit ); }

		/** Set to show or not the curson on the main screen */
		void ShowCursor(const bool& showcursor);

		/** @return The current frames per second of the screen */
		Uint32 FPS() const;

		/** Captures the OpenGL window and saves it to disk. \n
		* You have to call it before Display, and after render all the objects. \n
		* If the file path is empty will save the files like 0001.bmp, and will check if the file exists, otherwise will create 0002.bmp, and so on... \n
		* You can set only the path to save the files, like "screenshots/"
		* @return False if failed, otherwise returns True
		*/
		bool TakeScreenshot( std::string filepath = "", const EE_SAVETYPE& Format = EE_SAVE_TYPE_PNG );

		/** @return The resolutions that support the video card */
		std::vector< std::pair<unsigned int, unsigned int> > GetPossibleResolutions() const;

		/** Set the Screen Gamma. Default is (1,1,1). Accept values between 0.1 and 10. */
		void SetGamma( const eeFloat& Red, const eeFloat& Green, const eeFloat& Blue );

		/** Set the line smoothing ( default enabled ) */
		void SetLineSmooth( const bool& Enable );

		/** Set the polygon fill mode ( wireframe or filled ) */
		void SetPolygonMode( const EE_FILLMODE& Mode );

		/** @return If the screen is windowed */
		bool Windowed() const;

		/** @return If the extension passed is supported by the GPU */
		bool GetExtension( const std::string& Ext );

		/** @return If "GL_ARB_point_parameters" is supported (added for Particle System, faster way to get this. */
		bool PointSpriteSuppported() const { return mVideoInfo.SupARB_point; };

		/** @return True if shaders are supported by the GPU */
		bool ShadersSupported() const { return mVideoInfo.SupShaders; }

		/** Set a Cursor from a Texture (B/W Cursor) */
		void SetCursor( const Uint32& TexId, const eeVector2i& HotSpot = eeVector2i() );

		/** Set the Window icon */
		bool SetIcon( const Uint32& FromTexId );

		/** This will attempt to iconify/minimize the window. */
		void MinimizeWindow();

		/** Maximize the Window */
		void MaximizeWindow();

		/** This will attempt to hide the window */
		void HideWindow();

		/** This will attempt to raise the window */
		void RaiseWindow();

		/** This will attempt to show the window */
		void ShowWindow();

		/** This will attemp to move the window over the desktop to the position */
		void SetWindowPosition(Int16 Left, Int16 Top);

		/**  Set the current Viewport ( and create a new ortho proyection ) */
		void SetViewport( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height );

		/** The Video Info Struct */
		typedef struct {
			Uint32 Width;
			Uint32 Height;

			Uint8 ColorDepth;

			Uint32 DeskWidth;
			Uint32 DeskHeight;

			SDL_Surface * Screen;

			bool Resizeable;
			bool NoFrame;
			bool Windowed;
			bool DoubleBuffering;
			bool VSync;
			bool SupARB_point;
			bool LineSmooth;
			bool SupShaders;

			Int32 WWidth;
			Int32 WHeight;
			bool Maximized;

			Uint32 Flags;

			SDL_SysWMinfo info;
		} VideoInfo;

		/** @return The internal video info */
		const VideoInfo* GetVideoInfo() const { return &mVideoInfo; }

		/** @return The Clipboard Text if available */
		std::string GetClipboardText();

		/** @return The Clipboard Text if available ( as std::wstring ) */
		std::wstring GetClipboardTextWStr();

		/** Set the current Clipping area ( default the entire window ) */
		void ClipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height );

		/** Disable the Clipping area */
		void ClipDisable();

		/** @return If the current window is active */
		bool WindowActive();

		/** @return If the current window is visible */
		bool WindowVisible();

		/** This will set the default rendering states and view to render in 2D mode */
		void ResetGL2D();

		/** Set the current active view
		@param View New view to use (pass GetDefaultView() to set the default view)
		*/
		void SetView( const cView& View );

		/** Get the current view */
		const cView& GetView() const;

		/** Get the default view of the window */
		const cView& GetDefaultView() const;

		/** @return The company responsible for this GL implementation. */
		std::string GetVendor();

		/** @return The name of the renderer.\n This name is typically specific to a particular configuration of a hardware platform. */
		std::string GetRenderer();

		/** @return A GL version or release number. */
		std::string GetVersion();

		/** @return The Current Window Position */
		eeVector2i GetWindowPosition();

		/** Set the size of the window for a windowed window */
		void SetWindowSize( const Uint32& Width, const Uint32& Height );
	protected:
		cEngine();
		~cEngine();
	private:
		bool mInit;
		VideoInfo mVideoInfo;
		eeColor mBackColor;
		SDL_Cursor * mCursor;
		bool mShowCursor;
		eeVector2i mOldWinPos;

		struct _Frames {
			struct _FPS {
				Uint32 LastCheck;
				Uint32 Current;
				Uint32 Count;
				eeFloat Limit;
				Int32 Error;
			} FPS;
			cTimeElapsed FrameElapsed;
			eeFloat ElapsedTime;
		} mFrames;

		Uint32 mInitialWidth, mInitialHeight;

		cView mDefaultView;
		const cView* mCurrentView;

		GLenum mGLEWinit;

		void CalculateFps();
		void LimitFps();
		void GetElapsedTime();

		SDL_Cursor * CreateCursor( const Uint32& TexId, const eeVector2i& HotSpot );

		eeScrapType clipboard_convert_format(int type);

		int clipboard_convert_scrap(int type, char *dst, char *src, int srclen);

		void clipboard_get_scrap(int type, int *dstlen, char **dst);
};

}}

#endif
