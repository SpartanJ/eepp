#include <eepp/graphics/cconsole.hpp>
#include <eepp/graphics/glhelper.hpp>
#include <eepp/audio/caudiolistener.hpp>
#include <eepp/window/cinput.hpp>
#include <eepp/window/cengine.hpp>
#include <eepp/window/ccursormanager.hpp>
#include <algorithm>
#include <cstdarg>

using namespace EE::Window;

namespace EE { namespace Graphics {

cConsole::cConsole( Window::cWindow * window ) :
	mWindow( window ),
	mConColor(35, 47, 73, 230),
	mConLineColor(55, 67, 93, 230),
	mFontColor(153, 153, 179, 230),
	mFontLineColor(255, 255, 255, 230),
	mWidth(0),
	mHeight(0),
	mHeightMin(0),
	mY(0.0f),
	mA(0.0f),
	mFadeSpeed(250.f),
	mMyCallback(0),
	mVidCb(0),
	mMaxLogLines(1024),
	mLastLogPos(0),
	mFont(NULL),
	mTexId(0),
	mCurAlpha(0),
	mEnabled(false),
	mVisible(false),
	mFadeIn(false),
	mFadeOut(false),
	mExpand(false),
	mFading(false),
	mShowFps(false),
	mCurSide(false)
{
	if ( NULL == mWindow ) {
		mWindow = cEngine::instance()->GetCurrentWindow();
	}
}

cConsole::~cConsole() {
	mCallbacks.clear();
	mCmdLog.clear();
	mLastCommands.clear();

	if ( mMyCallback &&
		NULL != cEngine::ExistsSingleton() &&
		cEngine::instance()->ExistsWindow( mWindow )
	)
	{
		mWindow->GetInput()->PopCallback( mMyCallback );
		mWindow->PopResizeCallback( mVidCb );
	}
}

void cConsole::Create( cFont* Font, const bool& MakeDefaultCommands, const eeUint& MaxLogLines, const Uint32& TextureId ) {
	if ( NULL == mWindow ) {
		mWindow = cEngine::instance()->GetCurrentWindow();
	}

	mFont = Font;

	mFontSize = (eeFloat)( mFont->GetFontSize() * 1.25 );

	if ( mFont->GetFontHeight() < mFontSize && ( mFont->GetFontHeight() != mFont->GetFontSize() || mFont->GetLineSkip() != (Int32)mFont->GetFontHeight() ) )
		mFontSize = mFont->GetFontHeight();

	if ( TextureId > 0 )
		mTexId = TextureId;

	mMaxLogLines = MaxLogLines;
	mMaxAlpha = (eeFloat)mConColor.A();

	mEnabled = true;

	if ( MakeDefaultCommands )
		CreateDefaultCommands();

	mWidth = (eeFloat) mWindow->GetWidth();
	mHeight = (eeFloat) mWindow->GetHeight();
	mHeightMin = (eeFloat) mWindow->GetHeight() * 0.4f;

	if ( NULL != cEngine::ExistsSingleton() &&
		cEngine::instance()->ExistsWindow( mWindow ) )
	{
		mMyCallback = mWindow->GetInput()->PushCallback( cb::Make1( this, &cConsole::PrivInputCallback ) );
		mVidCb = mWindow->PushResizeCallback( cb::Make0( this, &cConsole::PrivVideoResize )  );
	}

	mTBuf.SetReturnCallback( cb::Make0( this, &cConsole::ProcessLine ) );
	mTBuf.Start();
	mTBuf.SupportNewLine( false );
	mTBuf.Active( false );
	IgnoreCharOnPrompt( KEY_TAB );

	mCon.ConModif = 0;

	CmdGetLog();
}

void cConsole::AddCommand( const String& Command, ConsoleCallback CB ) {
	if ( !(mCallbacks.count( Command ) > 0) )
		mCallbacks[Command] = CB;
}

void cConsole::Draw() {
	if ( mEnabled && NULL != mFont ) {
		eeColorA OldColor( mFont->Color() );

		Fade();

		if ( mY > 0.0f ) {
			if ( mTexId == 0 ) {
				mPri.SetColor( eeColorA( mConColor.R(), mConColor.G(), mConColor.B(), static_cast<Uint8>(mA) ) );
				mPri.DrawRectangle( 0.0f, 0.0f, mWidth, mY );
			} else {
				eeColorA C( mConColor.R(), mConColor.G(), mConColor.B(), static_cast<Uint8>(mA) );

				cTexture * Tex = cTextureFactory::instance()->GetTexture( mTexId );

				if ( NULL != Tex )
					Tex->DrawEx( 0.0f, 0.0f, mWidth, mY, 0.0f, 1.0f, C, C, C, C );
			}
			mPri.SetColor( eeColorA( mConLineColor.R(), mConLineColor.G(), mConLineColor.B(), static_cast<Uint8>(mA) ) );
			mPri.DrawRectangle( 0.0f, mY, mWidth, 4.0f );

			Int16 LinesInScreen = (Int16) ( (mCurHeight / mFontSize) - 1 );

			if ( static_cast<Int16>( mCmdLog.size() ) > LinesInScreen )
				mEx = (Uint32) ( mCmdLog.size() - LinesInScreen );
			else
				mEx = 0;
			mTempY = -mCurHeight;

			Uint16 Pos = 0;
			eeFloat CurY;

			mCon.ConMin = mEx;
			mCon.ConMax = (eeInt)mCmdLog.size() - 1;

			mFont->Color( eeColorA ( mFontColor.R(), mFontColor.G(), mFontColor.B(), static_cast<Uint8>(mA) ) );

			for (eeInt i = mCon.ConMax - mCon.ConModif; i >= mCon.ConMin - mCon.ConModif; i-- ) {
				if ( i < static_cast<Int16>( mCmdLog.size() ) && i >= 0 ) {
					CurY = mTempY + mY + mCurHeight - Pos * mFontSize - mFontSize * 2;

					mFont->Draw( mCmdLog[i], mFontSize, CurY );

					Pos++;
				}
			}

			CurY = mTempY + mY + mCurHeight - mFontSize - 1;

			mFont->Color( eeColorA ( mFontLineColor.R(), mFontLineColor.G(), mFontLineColor.B(), static_cast<Uint8>(mA) ) );
			mFont->SetText( "> " + mTBuf.Buffer() );
			mFont->Draw( mFontSize, CurY );

			mFont->Color( eeColorA ( mFontLineColor.R(), mFontLineColor.G(), mFontLineColor.B(), static_cast<Uint8>(mCurAlpha) ) );

			mFont->Color( OldColor );

			if ( (eeUint)mTBuf.CurPos() == mTBuf.Buffer().size() ) {
				mFont->Draw( "_", mFontSize + mFont->GetTextWidth() , CurY );
			} else {
				mFont->SetText( "> " + mTBuf.Buffer().substr( 0, mTBuf.CurPos() ) );
				mFont->Draw( "_", mFontSize + mFont->GetTextWidth() , CurY );
			}
		}
	}

	if ( mShowFps ) {
		eeColorA OldColor1( mFont->Color() );
		mFont->Color( eeColorA () );
		mFont->SetText( "FPS: " + toStr( mWindow->FPS() ) );
		mFont->Draw( mWindow->GetWidth() - mFont->GetTextWidth() - 15, 6 );
		mFont->Color( OldColor1 );
	}
}

void cConsole::FadeIn() {
	if (!mFading) {
		mFading = true;
		mFadeIn = true;
		mVisible = true;
		mY = 0.0f;
		mTBuf.Active( true );
	}
}

void cConsole::FadeOut() {
	if (!mFading) {
		mFading = true;
		mFadeOut = true;
		mVisible = false;
		mTBuf.Active( false );
	}
}

void cConsole::ProcessLine() {
	String str = mTBuf.Buffer();
	std::vector < String > params = SplitString( str, ' ' );

	mLastCommands.push_back( str );
	mLastLogPos = (eeInt)mLastCommands.size();

	if ( mLastCommands.size() > 20 )
		mLastCommands.pop_front();

	if ( str.size() > 0 ) {
		PushText( "> " + params[0] );

		if ( mCallbacks.find( params[0] ) != mCallbacks.end() ) {
			mCallbacks[ params[0] ]( params );
		} else {
			PushText( "Unknown Command: '" + params[0] + "'" );
		}
	}
	mTBuf.Clear();
}

void cConsole::PrivPushText( const String& str ) {
	mCmdLog.push_back( str );

	if ( mCmdLog.size() >= mMaxLogLines )
		mCmdLog.pop_front();
}

void cConsole::PushText( const String& str ) {
	PrivPushText( str );
}

void cConsole::PushText( const char * format, ... ) {
	int n, size = 256;
	std::string tstr( size, '\0' );

	va_list args;

	while (1) {
		va_start( args, format );

		#ifdef EE_COMPILER_MSVC
			n = _vsnprintf_s( &tstr[0], size, size, format, args );
		#else
			n = vsnprintf( &tstr[0], size, format, args );
		#endif

		va_end( args );

		if ( n > -1 && n < size ) {
			tstr.resize( n );

			PushText( tstr );

			return;
		}

		if ( n > -1 )	// glibc 2.1
			size = n+1; // precisely what is needed
		else			// glibc 2.0
			size *= 2;	// twice the old size

		tstr.resize( size, '\0' );
	}
}

void cConsole::Toggle() {
	if ( mVisible )
		FadeOut();
	else
		FadeIn();
}

void cConsole::Fade() {
	if (mCurSide) {
		mCurAlpha -= 255.f * mWindow->Elapsed() / mFadeSpeed;
		if ( mCurAlpha <= 0.0f ) {
			mCurAlpha = 0.0f;
			mCurSide = !mCurSide;
		}
	} else {
		mCurAlpha += 255.f * mWindow->Elapsed() / mFadeSpeed;
		if ( mCurAlpha >= 255.f ) {
			mCurAlpha = 255.f;
			mCurSide = !mCurSide;
		}
	}

	if ( mExpand )
		mCurHeight = mHeight;
	else
		mCurHeight = mHeightMin;

	if ( mFadeIn ) {
		mFadeOut = false;
		mY += mCurHeight * mWindow->Elapsed() / mFadeSpeed;

		mA = ( mY * mMaxAlpha / mCurHeight ) ;
		if ( mY > mCurHeight ) {
			mY = mCurHeight;
			mFadeIn = false;
			mFading = false;
		}
	}

	if ( mFadeOut ) {
		mFadeIn = false;
		mY -= mCurHeight * mWindow->Elapsed() / mFadeSpeed;

		mA = ( mY * mMaxAlpha / mCurHeight ) ;
		if ( mY <= 0.0f ) {
			mY = 0.0f;
			mFadeOut = false;
			mFading = false;
		}
	}

	if ( mA > 255.0f ) mA = 255.0f;
	if ( mA < 0.0f ) mA = 0.0f;
}

void cConsole::PrintCommandsStartingWith( const String& start ) {
	std::list<String> cmds;
	std::map < String, ConsoleCallback >::iterator it;

	for ( it = mCallbacks.begin(); it != mCallbacks.end(); it++ ) {
		if ( -1 != StrStartsWith( start, it->first ) ) {
			cmds.push_back( it->first );
		}
	}

	if ( cmds.size() > 1 ) {
		PushText( "> " + mTBuf.Buffer() );

		std::list<String>::iterator ite;

		for ( ite = cmds.begin(); ite != cmds.end(); ite++ )
			PushText( (*ite) );

	} else if ( cmds.size() ) {
		mTBuf.Buffer( cmds.front() );
		mTBuf.CursorToEnd();
	}
}

void cConsole::PrivVideoResize() {
	mWidth		= (eeFloat) mWindow->GetWidth();
	mHeight		= (eeFloat) mWindow->GetHeight();

	if ( mVisible ) {
		if ( mExpand )
			mCurHeight = mHeight;
		else
			mCurHeight = mHeightMin;

		mY = mCurHeight;
	}
}

void cConsole::PrivInputCallback( InputEvent * Event ) {
	Uint8 etype = Event->Type;

	if ( mVisible ) {
		Uint32 KeyCode	= (Uint32)Event->key.keysym.sym;
		Uint32 Button	= Event->button.button;

		if ( InputEvent::KeyDown == etype ) {
			if ( ( KeyCode == KEY_TAB ) && (eeUint)mTBuf.CurPos() == mTBuf.Buffer().size() ) {
				PrintCommandsStartingWith( mTBuf.Buffer() );
			}

			if ( mLastCommands.size() > 0 ) {
				if ( KeyCode == KEY_UP && mLastLogPos > 0 ) {
					mLastLogPos--;
				}

				if ( KeyCode == KEY_DOWN && mLastLogPos < static_cast<eeInt>( mLastCommands.size() ) ) {
					mLastLogPos++;
				}

				if ( KeyCode == KEY_UP || KeyCode == KEY_DOWN ) {
					if ( mLastLogPos == static_cast<eeInt>( mLastCommands.size() ) ) {
						mTBuf.Buffer( "" );
					} else {
						mTBuf.Buffer( mLastCommands[mLastLogPos] );
						mTBuf.CursorToEnd();
					}
				}
			}

			if ( KeyCode == KEY_PAGEUP ) {
				if ( mCon.ConMin - mCon.ConModif > 0 )
					mCon.ConModif++;
			}

			if ( KeyCode == KEY_PAGEDOWN ) {
				if ( mCon.ConModif > 0 )
					mCon.ConModif--;
			}

			if ( KeyCode == KEY_HOME ) {
				Int16 LinesInScreen = static_cast<Int16> ( (mCurHeight / mFontSize) - 1 );
				if ( static_cast<Int16>( mCmdLog.size() ) > LinesInScreen )
					mCon.ConModif = mCon.ConMin;
			}

			if ( KeyCode == KEY_END ) {
				mCon.ConModif = 0;
			}
		} else if ( InputEvent::MouseButtonUp == etype ) {
			if ( Button == EE_BUTTON_WHEELUP ) {
				if ( mCon.ConMin - mCon.ConModif > 0 )
					mCon.ConModif++;
			}

			if ( Button == EE_BUTTON_WHEELDOWN ) {
				if ( mCon.ConModif > 0 )
					mCon.ConModif--;
			}
		}
	}
}

void cConsole::CreateDefaultCommands() {
	AddCommand( "clear", cb::Make1( this, &cConsole::CmdClear) );
	AddCommand( "quit", cb::Make1( this, &cConsole::CmdQuit) );
	AddCommand( "maximize", cb::Make1( this, &cConsole::CmdMaximize) );
	AddCommand( "minimize", cb::Make1( this, &cConsole::CmdMinimize) );
	AddCommand( "cmdlist", cb::Make1( this, &cConsole::CmdCmdList) );
	AddCommand( "help", cb::Make1( this, &cConsole::CmdCmdList) );
	AddCommand( "showcursor", cb::Make1( this, &cConsole::CmdShowCursor) );
	AddCommand( "setfpslimit", cb::Make1( this, &cConsole::CmdFrameLimit) );
	AddCommand( "getlog", cb::Make1( this, &cConsole::CmdGetLog) );
	AddCommand( "setgamma", cb::Make1( this, &cConsole::CmdSetGamma) );
	AddCommand( "setvolume", cb::Make1( this, &cConsole::CmdSetVolume) );
	AddCommand( "getgpuextensions", cb::Make1( this, &cConsole::CmdGetGpuExtensions) );
	AddCommand( "dir", cb::Make1( this, &cConsole::CmdDir) );
	AddCommand( "ls", cb::Make1( this, &cConsole::CmdDir) );
	AddCommand( "showfps", cb::Make1( this, &cConsole::CmdShowFps) );
	AddCommand( "gettexturememory", cb::Make1( this, &cConsole::CmdGetTextureMemory) );
}

void cConsole::CmdClear	() {
	Uint16 CutLines;
	if ( mExpand ) {
		CutLines = (Uint16)( mHeight / mFontSize );
	} else {
		CutLines = (Uint16)( mHeightMin / mFontSize );
	}

	for (Uint16 i = 0; i < CutLines; i++ )
		PushText( "" );
}

void cConsole::CmdClear	( const std::vector < String >& params ) {
	CmdClear();
}

void cConsole::CmdMaximize ( const std::vector < String >& params ) {
	mExpand = true;
	mY = mHeight;
	PushText( "Console Maximized" );
}

void cConsole::CmdMinimize ( const std::vector < String >& params ) {
	mExpand = false;
	mY = mHeightMin;
	PushText( "Console Minimized" );
}

void cConsole::CmdQuit ( const std::vector < String >& params ) {
	mWindow->Close();
}

void cConsole::CmdGetTextureMemory ( const std::vector < String >& params ) {
	PushText( "Total texture memory used: " + SizeToString( cTextureFactory::instance()->MemorySize() ) );
}

void cConsole::CmdCmdList ( const std::vector < String >& params ) {
	std::map < String, ConsoleCallback >::iterator itr;
	for (itr = mCallbacks.begin(); itr != mCallbacks.end(); itr++) {
		PushText( itr->first );
	}
}

void cConsole::CmdShowCursor ( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mWindow->GetCursorManager()->Visible( 0 != tInt );
			PushText( "showcursor " + toStr( tInt ) );
		} else
			PushText( "Valid parameters are 0 or 1." );
	} else {
		PushText( "No parameters. Valid parameters are 0 ( hide ) or 1 ( show )." );
	}
}

void cConsole::CmdFrameLimit ( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt >= 0 && tInt <= 10000 ) ) {
			mWindow->FrameRateLimit( tInt );
			PushText( "setfpslimit " + toStr( tInt ) );
			return;
		}
	}

	PushText( "Valid parameters are between 0 and 10000 (0 = no limit)." );
}

void cConsole::CmdGetLog() {
	std::vector < String > tvec = SplitString( String( toStr( cLog::instance()->Buffer() ) ) );
	if ( tvec.size() > 0 ) {
		for ( eeUint i = 0; i < tvec.size(); i++ )
			PushText( tvec[i] );
	}
}

void cConsole::CmdGetLog( const std::vector < String >& params ) {
	CmdGetLog();
}

void cConsole::CmdGetGpuExtensions() {
	char *Exts = GLi->GetExtensions();
	std::vector < String > tvec = SplitString( String( toStr( std::string( Exts ) ) ), ' ' );
	if ( tvec.size() > 0 ) {
		for ( eeUint i = 0; i < tvec.size(); i++ )
			PushText( tvec[i] );
	}
}

void cConsole::CmdGetGpuExtensions( const std::vector < String >& params ) {
    CmdGetGpuExtensions();
}

void cConsole::CmdSetGamma( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		eeFloat tFloat = 0.f;

		bool Res = fromString<eeFloat>( tFloat, params[1] );

		if ( Res && ( tFloat > 0.1f && tFloat <= 10.0f ) ) {
			mWindow->SetGamma( tFloat, tFloat, tFloat );
			PushText( "setgamma " + toStr( tFloat ) );
			return;
		}
	}

	PushText( "Valid parameters are between 0.1 and 10." );
}

void cConsole::CmdSetVolume( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		eeFloat tFloat = 0.f;

		bool Res = fromString<eeFloat>( tFloat, params[1] );

		if ( Res && ( tFloat >= 0.0f && tFloat <= 100.0f ) ) {
			EE::Audio::cAudioListener::GlobalVolume( tFloat );
			PushText( "setvolume " + toStr( tFloat ) );
			return;
		}
	}

	PushText( "Valid parameters are between 0 and 100." );
}

void cConsole::CmdDir( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		#if EE_PLATFORM == EE_PLATFORM_WIN
		String Slash( "/\\" );
		#else
		String Slash( "/" );
		#endif
		String myPath = params[1];
		String myOrder;

		if ( params.size() > 2 ) {
			for ( eeUint i = 2; i < params.size(); i++ ) {
				if ( i + 1 == params.size() ) {
					if ( params[i] == "ff" )
						myOrder = params[i];
					else
						myPath += " " + params[i];
				} else {
					myPath += " " + params[i];
				}
			}
		}

		if ( IsDirectory( myPath ) ) {
			eeUint i;

			std::vector<String> mFiles = FilesGetInPath( myPath );
			std::sort( mFiles.begin(), mFiles.end() );

			PushText( "Directory: " + myPath );

			if ( myOrder == "ff" ) {
				std::vector<String> mFolders;
				std::vector<String> mFile;

				for ( i = 0; i < mFiles.size(); i++ ) {
					if ( IsDirectory( myPath + Slash + mFiles[i] ) ) {
						mFolders.push_back( mFiles[i] );
					} else {
						mFile.push_back( mFiles[i] );
					}
				}

				if ( mFolders.size() )
					PushText( "Folders: " );

				for ( i = 0; i < mFolders.size(); i++ )
					PushText( "	" + mFolders[i] );

				if ( mFolders.size() )
					PushText( "Files: " );

				for ( i = 0; i < mFile.size(); i++ )
					PushText( "	" + mFile[i] );

			} else {
				for ( i = 0; i < mFiles.size(); i++ )
					PushText( "	" + mFiles[i] );
			}
		} else {
			if ( myPath == "help" )
				PushText( "You can use a third parameter to show folders first, the parameter is ff." );
			else
				PushText( "Path is not a directory." );
		}
	} else {
		PushText( "Expected a path to list. Example of usage: ls /home" );
	}
}

void cConsole::CmdShowFps( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = fromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mShowFps = 0 != tInt;
			PushText( "showfps " + toStr( tInt ) );
			return;
		}
	}

	PushText( "Valid parameters are 0 ( hide ) or 1 ( show )." );
}

void cConsole::IgnoreCharOnPrompt( const Uint32& ch ) {
	mTBuf.PushIgnoredChar( ch );
}

const bool& cConsole::IsShowingFps() const {
	return mShowFps;
}

void cConsole::ShowFps( const bool& Show ) {
	mShowFps = Show;
}

}}
