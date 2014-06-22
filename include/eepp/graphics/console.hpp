#ifndef EE_GRAPHICSCCONSOLE_H
#define EE_GRAPHICSCCONSOLE_H

#include <eepp/graphics/base.hpp>
#include <eepp/window/inputtextbuffer.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/font.hpp>
#include <deque>

namespace EE { namespace Window { class Window; class InputTextBuffer; class InputEvent; } }

using namespace EE::Window;

namespace EE { namespace Graphics {

class EE_API Console : protected LogReaderInterface {
	public:
		//! The Console Callback return a vector of parameters ( String )
		typedef cb::Callback1<void, const std::vector < String >& > ConsoleCallback;

		/** Instances the console but doesn't create it, you must call Create to initialize the console. */
		Console( EE::Window::Window * window = NULL );

		/** Creates the console */
		Console( Font* Font, const bool& MakeDefaultCommands = true, const bool& AttachToLog = true, const unsigned int& MaxLogLines = 1024, const Uint32& TextureId = 0, EE::Window::Window * window = NULL );

		~Console();

		/** Set the Console Height when it's Minimized ( Not Maximized ) */
		void ConsoleMinimizedHeight( const Float& MinHeight ) { mHeightMin = MinHeight; if (mVisible && !mExpand) mCurHeight = mHeightMin; }

		/** Get the Console Height when it's Minimized ( Not Maximized ) */
		Float ConsoleMinimizedHeight() const { return mHeightMin; }

		/** Set the Texture Id for the Background, 0 will disable texture background */
		void TextureId( const Uint32& TexId ) { mTexId = TexId; }

		/** Get the Background Texture Id */
		Uint32 TextureId() const { return mTexId; }

		/** Set the Console Background Color */
		void BackgroundColor( const ColorA& BackColor ) { mConColor = BackColor; }

		/** Get the Console Background Color */
		const ColorA& BackgroundColor() const { return mConColor; }

		/** Set the Console Border Line Background Color */
		void BackgroundLineColor( const ColorA& BackColor ) { mConLineColor = BackColor; }

		/** Get the Console Border Line Background Color */
		const ColorA& BackgroundLineColor() const { return mConLineColor; }

		/** Set the Console Font Color */
		void FontColor( const ColorA& FntColor ) { mFontColor = FntColor; }

		/** Get the Console Font Color */
		const ColorA& FontColor() const { return mFontColor; }

		/** Set the Console Client Input ( Writeable Line ) Font Color */
		void FontLineColor( const ColorA& FntColor ) { mFontLineColor = FntColor; }

		/** Get the Console Client Input ( Writeable Line ) Font Color */
		const ColorA& FontLineColor() const { return mFontLineColor; }

		/** Toogle the console between visible and hided with Fade In or Fade Out effect. */
		void Toggle();

		/** Make visible the console */
		void FadeIn();

		/** Hide the console */
		void FadeOut();

		/** @return If Console Active ( Visible ) */
		bool Active() const { return mVisible; }

		/** Maximize or Minimize the Console */
		void Expand(const bool& Exp) { mExpand = Exp; }

		/** @return If console is maximized */
		bool Expand() const { return mExpand; }

		/** Set the fade time */
		void FadeSpeed( const Time& fadespeed ) { mFadeSpeed = fadespeed; }

		/** @return The fading speed in ms */
		const Time& FadeSpeed() const { return mFadeSpeed; }

		/** @brief Creates the new console
		* @param Font The Font pointer to class
		* @param MakeDefaultCommands Register the default commands provided by the class?
		* @param AttachToLog Attach the console to the Log instance
		* @param MaxLogLines Maximun number of lines stored on the console
		* @param TextureId Background texture id ( 0 for no texture )
		*/
		void Create( Font* Font, const bool& MakeDefaultCommands = true, const bool& AttachToLog = true, const unsigned int& MaxLogLines = 1024, const Uint32& TextureId = 0 );

		/** Add Text to Console */
		void PushText( const String& str );

		/** Add formated Text to console */
		void PushText( const char* format, ... );

		/** Adds a new Command
		* @param Command The Command Name ( raise the event )
		* @param CB The Callback for the Command
		*/
		void AddCommand( const String& Command, ConsoleCallback CB );

		/** Draw the Console ( allways call it, visible or not ) */
		void Draw();

		/** Set the line height ( distance between lines ) */
		void SetLineHeight( const Float& LineHeight ) { mFontSize = LineHeight; }

		/** Use this if you need to ignore some char to activate the console, for example '~'. A common char to activate a console. */
		void IgnoreCharOnPrompt( const Uint32& ch );

		/** @return If the console is rendering the FPS count. */
		const bool& IsShowingFps() const;

		/** Activate/Deactive fps rendering */
		void ShowFps( const bool& Show );
	protected:
		std::map < String, ConsoleCallback > mCallbacks;
		std::deque < String > mCmdLog;
		std::deque < String > mLastCommands;

		EE::Window::Window * mWindow;

		ColorA mConColor;
		ColorA mConLineColor;
		ColorA mFontColor;
		ColorA mFontLineColor;

		Float mWidth;
		Float mHeight;
		Float mHeightMin;
		Float mCurHeight;
		Float mY;
		Float mA;
		Float mMaxAlpha;
		Float mTempY;
		Float mFontSize;
		Time	mFadeSpeed;

		Uint32 mMyCallback;
		Uint32 mVidCb;
		Uint32 mEx;
		Uint32 mMaxLogLines;
		int mLastLogPos;

		InputTextBuffer * mTBuf;

		Font * mFont;

		Primitives mPri;
		Uint32 mTexId;

		typedef struct {
			int ConMin;
			int ConMax;
			int ConModif;
		} sCon;
		sCon mCon;

		Float mCurAlpha;

		bool mEnabled;
		bool mVisible;
		bool mFadeIn;
		bool mFadeOut;
		bool mExpand;
		bool mFading;
		bool mShowFps;
		bool mCurSide;

		void CreateDefaultCommands();

		void Fade();

		/** Internal Callback for default command ( clear ) */
		void CmdClear		( const std::vector < String >& params );

		/** Internal Callback for default command ( maximize ) */
		void CmdMaximize	( const std::vector < String >& params );

		/** Internal Callback for default command ( minimize ) */
		void CmdMinimize	( const std::vector < String >& params );

		/** Internal Callback for default command ( quit ) */
		void CmdQuit		( const std::vector < String >& params );

		/** Internal Callback for default command ( cmdlist ) */
		void CmdCmdList		( const std::vector < String >& params );

		/** Internal Callback for default command ( showcursor ) */
		void CmdShowCursor	( const std::vector < String >& params );

		/** Internal Callback for default command ( setfpslimit ) */
		void CmdFrameLimit	( const std::vector < String >& params );

		/** Internal Callback for default command ( getlog ) */
		void CmdGetLog	( const std::vector < String >& params );

		/** Internal Callback for default command ( setgamma ) */
		void CmdSetGamma( const std::vector < String >& params );

		/** Internal Callback for default command ( setvolume ) */
		void CmdSetVolume( const std::vector < String >& params );

		/** Internal Callback for default command ( getgpuextensions ) */
		void CmdGetGpuExtensions( const std::vector < String >& params );

		/** Internal Callback for default command ( dir and ls ) */
		void CmdDir( const std::vector < String >& params );

		/** Internal Callback for default command ( showfps ) */
		void CmdShowFps( const std::vector < String >& params );

		/** Internal Callback for default command ( gettexturememory ) */
		void CmdGetTextureMemory( const std::vector < String >& params );

		/** Internal Callback for default command ( hide ) */
		void CmdHideConsole( const std::vector < String >& params );

		/** The Default Commands Callbacks for the Console ( don't call it ) */
		void PrivInputCallback( InputEvent * Event );

		/** Clear the Console */
		void CmdClear();

		/** Add the current log to the console */
		void CmdGetLog();

		/** Add the GPU Extensions supported to the console */
		void CmdGetGpuExtensions();

		/** Internal Callback to Process the new line ( when return pressed ) */
		void ProcessLine();

		void PrivPushText( const String& str );

		void PrintCommandsStartingWith( const String& start );

		void PrivVideoResize( EE::Window::Window * win );

		void WriteLog( const std::string& Text );

		void GetFilesFrom( std::string txt, const Uint32& curPos );

		Int32 LinesInScreen();

		String GetLastCommonSubStr( std::list<String>& cmds );
};

}}

#endif
