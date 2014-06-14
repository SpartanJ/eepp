#include <eepp/graphics/cconsole.hpp>
#include <eepp/window/cengine.hpp>
#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/audio/audiolistener.hpp>
#include <eepp/window/cinput.hpp>
#include <eepp/window/ccursormanager.hpp>
#include <eepp/window/cwindow.hpp>
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
	mFadeSpeed( Milliseconds( 250.f ) ),
	mMyCallback(0),
	mVidCb(0),
	mMaxLogLines(1024),
	mLastLogPos(0),
	mTBuf( eeNew( cInputTextBuffer, () ) ),
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

cConsole::cConsole( cFont* Font, const bool& MakeDefaultCommands, const bool& AttachToLog, const unsigned int& MaxLogLines, const Uint32& TextureId, Window::cWindow * window ) :
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
	mFadeSpeed( Milliseconds( 250.f) ),
	mMyCallback(0),
	mVidCb(0),
	mMaxLogLines(1024),
	mLastLogPos(0),
	mTBuf( eeNew( cInputTextBuffer, () ) ),
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

	Create( Font, MakeDefaultCommands, AttachToLog, MaxLogLines, TextureId );
}

cConsole::~cConsole() {
	if ( mMyCallback &&
		NULL != cEngine::ExistsSingleton() &&
		cEngine::instance()->ExistsWindow( mWindow )
	)
	{
		mWindow->GetInput()->PopCallback( mMyCallback );
		mWindow->PopResizeCallback( mVidCb );
	}

	eeSAFE_DELETE( mTBuf );

	if ( cLog::ExistsSingleton() ) {
		cLog::instance()->RemoveLogReader( this );
	}
}

void cConsole::Create( cFont* Font, const bool& MakeDefaultCommands, const bool& AttachToLog, const unsigned int& MaxLogLines, const Uint32& TextureId ) {
	if ( NULL == mWindow ) {
		mWindow = cEngine::instance()->GetCurrentWindow();
	}

	mFont = Font;

	mFontSize = (Float)( mFont->GetFontSize() * 1.25 );

	if ( mFont->GetFontHeight() < mFontSize && ( mFont->GetFontHeight() != mFont->GetFontSize() || mFont->GetLineSkip() != (Int32)mFont->GetFontHeight() ) )
		mFontSize = mFont->GetFontHeight();

	if ( TextureId > 0 )
		mTexId = TextureId;

	mMaxLogLines = MaxLogLines;
	mMaxAlpha = (Float)mConColor.A();

	mEnabled = true;

	if ( MakeDefaultCommands )
		CreateDefaultCommands();

	mWidth = (Float) mWindow->GetWidth();
	mHeight = (Float) mWindow->GetHeight();
	mHeightMin = (Float) ( mWindow->GetHeight() / 2 );

	if ( NULL != cEngine::ExistsSingleton() &&
		cEngine::instance()->ExistsWindow( mWindow ) )
	{
		mMyCallback = mWindow->GetInput()->PushCallback( cb::Make1( this, &cConsole::PrivInputCallback ) );
		mVidCb = mWindow->PushResizeCallback( cb::Make1( this, &cConsole::PrivVideoResize )  );
	}

	mTBuf->SetReturnCallback( cb::Make0( this, &cConsole::ProcessLine ) );
	mTBuf->Start();
	mTBuf->SupportNewLine( false );
	mTBuf->Active( false );
	IgnoreCharOnPrompt( KEY_TAB );

	mCon.ConModif = 0;

	CmdGetLog();

	if ( AttachToLog ) {
		cLog::instance()->AddLogReader( this );
	}
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
				mPri.DrawRectangle( eeRectf( eeVector2f( 0.0f, 0.0f ), eeSizef( mWidth, mY ) ) );
			} else {
				eeColorA C( mConColor.R(), mConColor.G(), mConColor.B(), static_cast<Uint8>(mA) );

				cTexture * Tex = cTextureFactory::instance()->GetTexture( mTexId );

				if ( NULL != Tex )
					Tex->DrawEx( 0.0f, 0.0f, mWidth, mY, 0.0f, eeVector2f::One, C, C, C, C );
			}
			mPri.SetColor( eeColorA( mConLineColor.R(), mConLineColor.G(), mConLineColor.B(), static_cast<Uint8>(mA) ) );
			mPri.DrawRectangle( eeRectf( eeVector2f( 0.0f, mY ), eeSizef( mWidth, 4.0f ) ) );

			Int32 linesInScreen = LinesInScreen();

			if ( static_cast<Int32>( mCmdLog.size() ) > linesInScreen )
				mEx = (Uint32) ( mCmdLog.size() - linesInScreen );
			else
				mEx = 0;
			mTempY = -mCurHeight;

			Uint16 Pos = 0;
			Float CurY;

			mCon.ConMin = mEx;
			mCon.ConMax = (int)mCmdLog.size() - 1;

			mFont->Color( eeColorA ( mFontColor.R(), mFontColor.G(), mFontColor.B(), static_cast<Uint8>(mA) ) );

			for (int i = mCon.ConMax - mCon.ConModif; i >= mCon.ConMin - mCon.ConModif; i-- ) {
				if ( i < static_cast<Int16>( mCmdLog.size() ) && i >= 0 ) {
					CurY = mTempY + mY + mCurHeight - Pos * mFontSize - mFontSize * 2;

					mFont->Draw( mCmdLog[i], mFontSize, CurY );

					Pos++;
				}
			}

			CurY = mTempY + mY + mCurHeight - mFontSize - 1;

			mFont->Color( eeColorA ( mFontLineColor.R(), mFontLineColor.G(), mFontLineColor.B(), static_cast<Uint8>(mA) ) );
			mFont->SetText( "> " + mTBuf->Buffer() );
			mFont->Draw( mFontSize, CurY );

			mFont->Color( eeColorA ( mFontLineColor.R(), mFontLineColor.G(), mFontLineColor.B(), static_cast<Uint8>(mCurAlpha) ) );

			if ( (unsigned int)mTBuf->CurPos() == mTBuf->Buffer().size() ) {
				mFont->Draw( "_", mFontSize + mFont->GetTextWidth() , CurY );
			} else {
				mFont->SetText( "> " + mTBuf->Buffer().substr( 0, mTBuf->CurPos() ) );
				mFont->Draw( "_", mFontSize + mFont->GetTextWidth() , CurY );
			}

			mFont->Color( OldColor );
		}
	}

	if ( mShowFps && NULL != mFont ) {
		eeColorA OldColor1( mFont->Color() );
		mFont->Color( eeColorA () );
		mFont->SetText( "FPS: " + String::ToStr( mWindow->FPS() ) );
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
		mTBuf->Active( true );
	}
}

void cConsole::FadeOut() {
	if (!mFading) {
		mFading = true;
		mFadeOut = true;
		mVisible = false;
		mTBuf->Active( false );
	}
}

void cConsole::ProcessLine() {
	String str = mTBuf->Buffer();
	std::vector < String > params = String::Split( str, ' ' );

	mLastCommands.push_back( str );
	mLastLogPos = (int)mLastCommands.size();

	if ( mLastCommands.size() > 20 )
		mLastCommands.pop_front();

	if ( str.size() > 0 ) {
		PrivPushText( "> " + params[0] );

		if ( mCallbacks.find( params[0] ) != mCallbacks.end() ) {
			mCallbacks[ params[0] ]( params );
		} else {
			PrivPushText( "Unknown Command: '" + params[0] + "'" );
		}
	}
	mTBuf->Clear();
}

void cConsole::PrivPushText( const String& str ) {
	mCmdLog.push_back( str );

	if ( mCmdLog.size() >= mMaxLogLines )
		mCmdLog.pop_front();
}

void cConsole::PushText( const String& str ) {
	if ( std::string::npos != str.find_first_of( '\n' ) ) {
		std::vector<String> Strings = String::Split( String( str ) );

		for ( Uint32 i = 0; i < Strings.size(); i++ ) {
			PrivPushText( Strings[i] );
		}
	} else {
		PrivPushText( str );
	}
}

void cConsole::PushText( const char * format, ... ) {
	int n, size = 256;
	std::string tstr( size, '\0' );

	va_list args;

	while (1) {
		va_start( args, format );

		n = vsnprintf( &tstr[0], size, format, args );

		if ( n > -1 && n < size ) {
			tstr.resize( n );

			PushText( tstr );

			va_end( args );

			return;
		}

		if ( n > -1 )	// glibc 2.1
			size = n+1; // precisely what is needed
		else			// glibc 2.0
			size *= 2;	// twice the old size

		tstr.resize( size );
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
		mCurAlpha -= 255.f * mWindow->Elapsed().AsMilliseconds() / mFadeSpeed.AsMilliseconds();
		if ( mCurAlpha <= 0.0f ) {
			mCurAlpha = 0.0f;
			mCurSide = !mCurSide;
		}
	} else {
		mCurAlpha += 255.f * mWindow->Elapsed().AsMilliseconds() / mFadeSpeed.AsMilliseconds();
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
		mY += mCurHeight * mWindow->Elapsed().AsMilliseconds() / mFadeSpeed.AsMilliseconds();

		mA = ( mY * mMaxAlpha / mCurHeight ) ;
		if ( mY > mCurHeight ) {
			mY = mCurHeight;
			mFadeIn = false;
			mFading = false;
		}
	}

	if ( mFadeOut ) {
		mFadeIn = false;
		mY -= mCurHeight * mWindow->Elapsed().AsMilliseconds() / mFadeSpeed.AsMilliseconds();

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

String cConsole::GetLastCommonSubStr( std::list<String>& cmds ) {
	String lastCommon( mTBuf->Buffer() );
	String strTry( lastCommon );

	std::list<String>::iterator ite;

	bool found = false;

	do {
		found = false;

		bool allEqual = true;

		String strBeg( (*cmds.begin()) );

		if ( strTry.size() + 1 <= strBeg.size() ) {
			strTry = String( strBeg.substr( 0, strTry.size() + 1 ) );

			for ( ite = ++cmds.begin(); ite != cmds.end(); ite++ ) {
				String& strCur = (*ite);

				if ( !( strTry.size() <= strCur.size() && strTry == strCur.substr( 0, strTry.size() ) ) ) {
					allEqual = false;
				}
			}

			if ( allEqual ) {
				lastCommon = strTry;

				found = true;
			}
		}
	} while ( found );

	return lastCommon;
}

void cConsole::PrintCommandsStartingWith( const String& start ) {
	std::list<String> cmds;
	std::map < String, ConsoleCallback >::iterator it;

	for ( it = mCallbacks.begin(); it != mCallbacks.end(); it++ ) {
		if ( -1 != String::StartsWith( start, it->first ) ) {
			cmds.push_back( it->first );
		}
	}

	if ( cmds.size() > 1 ) {
		PrivPushText( "> " + mTBuf->Buffer() );

		std::list<String>::iterator ite;

		for ( ite = cmds.begin(); ite != cmds.end(); ite++ )
			PrivPushText( (*ite) );

		String newStr( GetLastCommonSubStr( cmds ) );

		if ( newStr != mTBuf->Buffer() ) {
			mTBuf->Buffer( newStr );
			mTBuf->CursorToEnd();
		}
	} else if ( cmds.size() ) {
		mTBuf->Buffer( cmds.front() );
		mTBuf->CursorToEnd();
	}
}

void cConsole::PrivVideoResize( cWindow * win ) {
	mWidth		= (Float) mWindow->GetWidth();
	mHeight		= (Float) mWindow->GetHeight();

	if ( mVisible ) {
		if ( mExpand )
			mCurHeight = mHeight;
		else
			mCurHeight = mHeightMin;

		mY = mCurHeight;
	}
}

void cConsole::GetFilesFrom( std::string txt, const Uint32& curPos ) {
	static char OSSlash = FileSystem::GetOSlash().at(0);
	size_t pos;

	if ( std::string::npos != ( pos = txt.find_last_of( OSSlash ) ) && pos <= curPos ) {
		size_t fpos = txt.find_first_of( OSSlash );

		std::string dir( txt.substr( fpos, pos - fpos + 1 ) );
		std::string file( txt.substr( pos + 1 ) );

		if ( FileSystem::IsDirectory( dir ) ) {
			size_t count = 0, lasti = 0;
			std::vector<std::string> files = FileSystem::FilesGetInPath( dir, true, true );
			String res;

			for ( size_t i = 0; i < files.size(); i++ ) {
				if ( !file.size() || -1 != String::StartsWith( file, files[i] ) ) {
					res += "\t" + files[i] + "\n";
					count++;
					lasti = i;
				}
			}

			if ( count == 1 ) {
				std::string slash = "";

				if ( FileSystem::IsDirectory( dir + files[lasti] ) ) {
					slash = FileSystem::GetOSlash();
				}

				mTBuf->Buffer( mTBuf->Buffer().substr( 0, pos + 1 ) + files[lasti] + slash );
				mTBuf->CursorToEnd();
			} else if ( count > 1 ) {
				PrivPushText( "Directory file list:" );
				PushText( res );
			}
		}
	}
}

Int32 cConsole::LinesInScreen() {
	return static_cast<Int32> ( (mCurHeight / mFontSize) - 1 );
}

void cConsole::PrivInputCallback( InputEvent * Event ) {
	Uint8 etype = Event->Type;

	if ( mVisible ) {
		Uint32 KeyCode	= (Uint32)Event->key.keysym.sym;
		Uint32 KeyMod	= (Uint32)Event->key.keysym.mod;
		Uint32 Button	= Event->button.button;

		if ( InputEvent::KeyDown == etype ) {
			if ( ( KeyCode == KEY_TAB ) && (unsigned int)mTBuf->CurPos() == mTBuf->Buffer().size() ) {
				PrintCommandsStartingWith( mTBuf->Buffer() );
				GetFilesFrom( mTBuf->Buffer().ToUtf8(), mTBuf->CurPos() );
			}

			if ( KeyMod & KEYMOD_SHIFT ) {
				if (  KeyCode == KEY_UP ) {
					if ( mCon.ConMin - mCon.ConModif > 0 )
						mCon.ConModif++;
				}

				if ( KeyCode == KEY_DOWN ) {
					if ( mCon.ConModif > 0 )
						mCon.ConModif--;
				}

				if ( KeyCode == KEY_HOME ) {
					if ( static_cast<Int32>( mCmdLog.size() ) > LinesInScreen() )
						mCon.ConModif = mCon.ConMin;
				}

				if ( KeyCode == KEY_END ) {
					mCon.ConModif = 0;
				}

				if ( KeyCode == KEY_PAGEUP ) {
					if ( mCon.ConMin - mCon.ConModif - LinesInScreen() / 2 > 0 )
						mCon.ConModif+=LinesInScreen() / 2;
					else
						mCon.ConModif = mCon.ConMin;
				}

				if ( KeyCode == KEY_PAGEDOWN ) {
					if ( mCon.ConModif - LinesInScreen() / 2 > 0 )
						mCon.ConModif-=LinesInScreen() / 2;
					else
						mCon.ConModif = 0;
				}
			} else {
				if ( mLastCommands.size() > 0 ) {
					if ( KeyCode == KEY_UP && mLastLogPos > 0 ) {
						mLastLogPos--;
					}

					if ( KeyCode == KEY_DOWN && mLastLogPos < static_cast<int>( mLastCommands.size() ) ) {
						mLastLogPos++;
					}

					if ( KeyCode == KEY_UP || KeyCode == KEY_DOWN ) {
						if ( mLastLogPos == static_cast<int>( mLastCommands.size() ) ) {
							mTBuf->Buffer( "" );
						} else {
							mTBuf->Buffer( mLastCommands[mLastLogPos] );
							mTBuf->CursorToEnd();
						}
					}
				}

			}
		} else if ( InputEvent::MouseButtonUp == etype ) {
			if ( Button == EE_BUTTON_WHEELUP ) {
				if ( mCon.ConMin - mCon.ConModif - 6 > 0 ) {
					mCon.ConModif += 6;
				} else {
					mCon.ConModif = mCon.ConMin;
				}
			}

			if ( Button == EE_BUTTON_WHEELDOWN ) {
				if ( mCon.ConModif - 6 > 0 ) {
					mCon.ConModif -= 6;
				} else {
					mCon.ConModif = 0;
				}
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
	AddCommand( "hide", cb::Make1( this, &cConsole::CmdHideConsole ) );
}

void cConsole::CmdClear	() {
	Uint16 CutLines;
	if ( mExpand ) {
		CutLines = (Uint16)( mHeight / mFontSize );
	} else {
		CutLines = (Uint16)( mHeightMin / mFontSize );
	}

	for (Uint16 i = 0; i < CutLines; i++ )
		PrivPushText( "" );
}

void cConsole::CmdClear	( const std::vector < String >& params ) {
	CmdClear();
}

void cConsole::CmdMaximize ( const std::vector < String >& params ) {
	mExpand = true;
	mY = mHeight;
	PrivPushText( "Console Maximized" );
}

void cConsole::CmdMinimize ( const std::vector < String >& params ) {
	mExpand = false;
	mY = mHeightMin;
	PrivPushText( "Console Minimized" );
}

void cConsole::CmdQuit ( const std::vector < String >& params ) {
	mWindow->Close();
}

void cConsole::CmdGetTextureMemory ( const std::vector < String >& params ) {
	PrivPushText( "Total texture memory used: " + FileSystem::SizeToString( cTextureFactory::instance()->MemorySize() ) );
}

void cConsole::CmdCmdList ( const std::vector < String >& params ) {
	std::map < String, ConsoleCallback >::iterator itr;
	for (itr = mCallbacks.begin(); itr != mCallbacks.end(); itr++) {
		PrivPushText( "\t" + itr->first );
	}
}

void cConsole::CmdShowCursor ( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::FromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mWindow->GetCursorManager()->Visible( 0 != tInt );
			PrivPushText( "showcursor " + String::ToStr( tInt ) );
		} else
			PrivPushText( "Valid parameters are 0 or 1." );
	} else {
		PrivPushText( "No parameters. Valid parameters are 0 ( hide ) or 1 ( show )." );
	}
}

void cConsole::CmdFrameLimit ( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::FromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt >= 0 && tInt <= 10000 ) ) {
			mWindow->FrameRateLimit( tInt );
			PrivPushText( "setfpslimit " + String::ToStr( tInt ) );
			return;
		}
	}

	PrivPushText( "Valid parameters are between 0 and 10000 (0 = no limit)." );
}

void cConsole::CmdGetLog() {
	std::vector < String > tvec = String::Split( String( String::ToStr( cLog::instance()->Buffer() ) ) );
	if ( tvec.size() > 0 ) {
		for ( unsigned int i = 0; i < tvec.size(); i++ )
			PrivPushText( tvec[i] );
	}
}

void cConsole::CmdGetLog( const std::vector < String >& params ) {
	CmdGetLog();
}

void cConsole::CmdGetGpuExtensions() {
	std::vector < String > tvec = String::Split( String( GLi->GetExtensions() ), ' ' );
	if ( tvec.size() > 0 ) {
		for ( unsigned int i = 0; i < tvec.size(); i++ )
			PrivPushText( tvec[i] );
	}
}

void cConsole::CmdGetGpuExtensions( const std::vector < String >& params ) {
	CmdGetGpuExtensions();
}

void cConsole::CmdSetGamma( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Float tFloat = 0.f;

		bool Res = String::FromString<Float>( tFloat, params[1] );

		if ( Res && ( tFloat > 0.1f && tFloat <= 10.0f ) ) {
			mWindow->SetGamma( tFloat, tFloat, tFloat );
			PrivPushText( "setgamma " + String::ToStr( tFloat ) );
			return;
		}
	}

	PrivPushText( "Valid parameters are between 0.1 and 10." );
}

void cConsole::CmdSetVolume( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Float tFloat = 0.f;

		bool Res = String::FromString<Float>( tFloat, params[1] );

		if ( Res && ( tFloat >= 0.0f && tFloat <= 100.0f ) ) {
			EE::Audio::AudioListener::GlobalVolume( tFloat );
			PrivPushText( "setvolume " + String::ToStr( tFloat ) );
			return;
		}
	}

	PrivPushText( "Valid parameters are between 0 and 100." );
}

void cConsole::CmdDir( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		String Slash( FileSystem::GetOSlash() );
		String myPath = params[1];
		String myOrder;

		if ( params.size() > 2 ) {
			for ( unsigned int i = 2; i < params.size(); i++ ) {
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

		if ( FileSystem::IsDirectory( myPath ) ) {
			unsigned int i;

			std::vector<String> mFiles = FileSystem::FilesGetInPath( myPath );
			std::sort( mFiles.begin(), mFiles.end() );

			PrivPushText( "Directory: " + myPath );

			if ( myOrder == "ff" ) {
				std::vector<String> mFolders;
				std::vector<String> mFile;

				for ( i = 0; i < mFiles.size(); i++ ) {
					if ( FileSystem::IsDirectory( myPath + Slash + mFiles[i] ) ) {
						mFolders.push_back( mFiles[i] );
					} else {
						mFile.push_back( mFiles[i] );
					}
				}

				if ( mFolders.size() )
					PrivPushText( "Folders: " );

				for ( i = 0; i < mFolders.size(); i++ )
					PrivPushText( "	" + mFolders[i] );

				if ( mFolders.size() )
					PrivPushText( "Files: " );

				for ( i = 0; i < mFile.size(); i++ )
					PrivPushText( "	" + mFile[i] );

			} else {
				for ( i = 0; i < mFiles.size(); i++ )
					PrivPushText( "	" + mFiles[i] );
			}
		} else {
			if ( myPath == "help" )
				PrivPushText( "You can use a third parameter to show folders first, the parameter is ff." );
			else
				PrivPushText( "Path is not a directory." );
		}
	} else {
		PrivPushText( "Expected a path to list. Example of usage: ls /home" );
	}
}

void cConsole::CmdShowFps( const std::vector < String >& params ) {
	if ( params.size() >= 2 ) {
		Int32 tInt = 0;

		bool Res = String::FromString<Int32>( tInt, params[1] );

		if ( Res && ( tInt == 0 || tInt == 1 ) ) {
			mShowFps = 0 != tInt;
			PrivPushText( "showfps " + String::ToStr( tInt ) );
			return;
		}
	}

	PrivPushText( "Valid parameters are 0 ( hide ) or 1 ( show )." );
}

void cConsole::CmdHideConsole( const std::vector < String >& params ) {
	FadeOut();
}

void cConsole::IgnoreCharOnPrompt( const Uint32& ch ) {
	mTBuf->PushIgnoredChar( ch );
}

const bool& cConsole::IsShowingFps() const {
	return mShowFps;
}

void cConsole::ShowFps( const bool& Show ) {
	mShowFps = Show;
}

void cConsole::WriteLog( const std::string& Text ) {
	std::vector<String> Strings = String::Split( String( Text ) );

	for ( Uint32 i = 0; i < Strings.size(); i++ ) {
		PrivPushText( Strings[i] );
	}
}

}}
