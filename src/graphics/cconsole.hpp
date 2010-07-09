#ifndef EE_GRAPHICSCCONSOLE_H
#define EE_GRAPHICSCCONSOLE_H

#include "base.hpp"
#include "../window/cinputtextbuffer.hpp"
#include "cprimitives.hpp"
#include "cfont.hpp"
#include "ctexturefont.hpp"
#include "cttffont.hpp"

using namespace EE::Window;

namespace EE { namespace Graphics {

class EE_API cConsole{
	public:
		//! The Console Callback return a vector of parameters ( wstring )
		typedef boost::function1<void, std::vector < std::wstring > > ConsoleCallback;

		cConsole();

		~cConsole();

		/** Set the Console Height when it's Minimized ( Not Maximized ) */
		void ConsoleMinimizedHeight( const eeFloat& MinHeight ) { mHeightMin = MinHeight; if (mVisible && !mExpand) mCurHeight = mHeightMin; }

		/** Get the Console Height when it's Minimized ( Not Maximized ) */
		eeFloat ConsoleMinimizedHeight() const { return mHeightMin; }

		/** Set the Texture Id for the Background, 0 will disable texture background */
		void TextureId( const Uint32& TexId ) { mTexId = TexId; }

		/** Get the Background Texture Id */
		Uint32 TextureId() const { return mTexId; }

		/** Set the Console Background Color */
		void BackgroundColor( const eeRGBA& BackColor ) { mConColor = BackColor; }

		/** Get the Console Background Color */
		eeRGBA BackgroundColor() const { return mConColor; }

		/** Set the Console Border Line Background Color */
		void BackgroundLineColor( const eeRGBA& BackColor ) { mConLineColor = BackColor; }

		/** Get the Console Border Line Background Color */
		eeRGBA BackgroundLineColor() const { return mConLineColor; }

		/** Set the Console Font Color */
		void FontColor( const eeRGBA& FntColor ) { mFontColor = FntColor; }

		/** Get the Console Font Color */
		eeRGBA FontColor() const { return mFontColor; }

		/** Set the Console Client Input ( Writeable Line ) Font Color */
		void FontLineColor( const eeRGBA& FntColor ) { mFontLineColor = FntColor; }

		/** Get the Console Client Input ( Writeable Line ) Font Color */
		eeRGBA FontLineColor() const { return mFontLineColor; }

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

		/** Set the fade speed in ms */
		void FadeSpeed( const eeFloat& fadespeed ) { mFadeSpeed = fadespeed; }

		/** @return The fading speed in ms */
		eeFloat FadeSpeed() const { return mFadeSpeed; }

		/** Creates the new console
		* @param Font The cFont pointer to class
		* @param MakeDefaultCommands Register the default commands provided by the class?
		* @param ConsoleColor The Console Background Color
		* @param ConsoleLineColor The Console Line Background Color
		* @param FontColor The Console Font Color
		* @param FontLineColor The Console Line Font Color ( The Client Input )
		*/
		void Create( cFont* Font, const bool& MakeDefaultCommands = true, const eeRGBA& ConsoleColor = eeRGBA(true), const eeRGBA& ConsoleLineColor = eeRGBA(true), const eeRGBA& FontColor = eeRGBA(true), const eeRGBA& FontLineColor = eeRGBA(true), const Uint32& TextureId = 0, const eeUint& MaxLogLines = 1024 );

		/** Creates the new console
		* @param TTFFont The cTTFFont pointer to class
		* @param MakeDefaultCommands Register the default commands provided by the class?
		* @param ConsoleColor The Console Background Color
		* @param ConsoleLineColor The Console Line Background Color
		* @param FontColor The Console Font Color
		* @param FontLineColor The Console Line Font Color ( The Client Input )
		*/
		void Create( cTTFFont* TTFFont, const bool& MakeDefaultCommands = true, const eeRGBA& ConsoleColor = eeRGBA(true), const eeRGBA& ConsoleLineColor = eeRGBA(true), const eeRGBA& FontColor = eeRGBA(true), const eeRGBA& FontLineColor = eeRGBA(true), const Uint32& TextureId = 0, const eeUint& MaxLogLines = 1024 );

		/** Creates the new console
		* @param TexFont The cTextureFont pointer to class
		* @param MakeDefaultCommands Register the default commands provided by the class?
		* @param ConsoleColor The Console Background Color
		* @param ConsoleLineColor The Console Line Background Color
		* @param FontColor The Console Font Color
		* @param FontLineColor The Console Line Font Color ( The Client Input )
		*/
		void Create( cTextureFont* TexFont, const bool& MakeDefaultCommands = true, const eeRGBA& ConsoleColor = eeRGBA(true), const eeRGBA& ConsoleLineColor = eeRGBA(true), const eeRGBA& FontColor = eeRGBA(true), const eeRGBA& FontLineColor = eeRGBA(true), const Uint32& TextureId = 0, const eeUint& MaxLogLines = 1024 );

		/** Add Text to Console */
		void PushText( const std::wstring& str );

		/** Add Text to Console */
		void PushText( const std::string& str );

		/** Add formated Text to console */
		void PushText( const char* format, ... );

		/** Adds a new Command
		* @param Command The Command Name ( raise the event )
		* @param CB The Callback for the Command
		*/
		void AddCommand( const std::wstring& Command, ConsoleCallback CB );

		/** Adds a new Command
		* @param Command The Command Name ( raise the event )
		* @param CB The Callback for the Command
		*/
		void AddCommand( const std::string& Command, ConsoleCallback CB );

		/** Draw the Console ( allways call it, visible or not ) */
		void Draw();

		/** Set the line height ( distance between lines ) */
		void SetLineHeight( const eeFloat& LineHeight ) { mFontSize = LineHeight; }

		/** Use this if you need to ignore some char to activate the console, for example '~'. A common char to activate a console. */
		void IgnoreCharOnPrompt( const Uint32& ch );
	protected:
		std::map < std::wstring, ConsoleCallback > mCallbacks;
		std::deque < std::wstring > mCmdLog;
		std::deque < std::wstring > mLastCommands;

		Int16 mLastLogPos;

		bool mEnabled, mVisible, mFadeIn, mFadeOut, mExpand, mFading, mShowFps;
		eeRGBA mConColor, mConLineColor, mFontColor, mFontLineColor;
		eeFloat mWidth, mHeight, mHeightMin, mCurHeight, mY, mA, mMaxAlpha, mTempY, mFontSize, mFadeSpeed;
		Uint32 mMyCallback, mEx, mMaxLogLines;

		cInputTextBuffer mTBuf;

		cFont* mFont;

		cPrimitives mPri;
		Uint32 mTexId;

		typedef struct {
			eeInt ConMin;
			eeInt ConMax;
			eeInt ConModif;
		} sCon;
		sCon mCon;

		eeFloat mCurAlpha;
		bool mCurSide;

		void CreateDefaultCommands();

		void PrivCreate( const bool& MakeDefaultCommands, const eeRGBA& ConsoleColor, const eeRGBA& ConsoleLineColor, const eeRGBA& FontColor, const eeRGBA& FontLineColor, const Uint32& TextureId, const eeUint& MaxLogLines );

		void Fade();

		/** Internal Callback for default command ( clear ) */
		void CmdClear		( const std::vector < std::wstring >& params );

		/** Internal Callback for default command ( maximize ) */
		void CmdMaximize	( const std::vector < std::wstring >& params );

		/** Internal Callback for default command ( minimize ) */
		void CmdMinimize	( const std::vector < std::wstring >& params );

		/** Internal Callback for default command ( quit ) */
		void CmdQuit		( const std::vector < std::wstring >& params );

		/** Internal Callback for default command ( cmdlist ) */
		void CmdCmdList		( const std::vector < std::wstring >& params );

		/** Internal Callback for default command ( showcursor ) */
		void CmdShowCursor	( const std::vector < std::wstring >& params );

		/** Internal Callback for default command ( setframelimit ) */
		void CmdFrameLimit	( const std::vector < std::wstring >& params );

		/** Internal Callback for default command ( getlog ) */
		void CmdGetLog	( const std::vector < std::wstring >& params );

		/** Internal Callback for default command ( setgamma ) */
		void CmdSetGamma( const std::vector < std::wstring >& params );

		/** Internal Callback for default command ( setvolume ) */
		void CmdSetVolume( const std::vector < std::wstring >& params );

        /** Internal Callback for default command ( getgpuextensions ) */
		void CmdGetGpuExtensions( const std::vector < std::wstring >& params );

		/** Internal Callback for default command ( dir and ls ) */
		void CmdDir( const std::vector < std::wstring >& params );

		/** Internal Callback for default command ( showfps ) */
		void CmdShowFps( const std::vector < std::wstring >& params );

		/** Internal Callback for default command ( gettexturememory ) */
		void CmdGetTextureMemory ( const std::vector < std::wstring >& params );

		/** The Default Commands Callbacks for the Console ( don't call it ) */
		void PrivInputCallback( EE_Event* Event );

		/** Clear the Console */
		void CmdClear();

		/** Add the current log to the console */
		void CmdGetLog();

        /** Add the GPU Extensions supported to the console */
        void CmdGetGpuExtensions();

		/** Internal Callback to Process the new line ( when return pressed ) */
		void ProcessLine();

		void PrivPushText( const std::wstring& str );
};

}}

#endif
