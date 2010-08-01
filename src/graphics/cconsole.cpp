#include "cconsole.hpp"

#include "../audio/caudiolistener.hpp"
#include "../window/cinput.hpp"
#include "../window/cengine.hpp"

using namespace EE::Window;

namespace EE { namespace Graphics {

cConsole::cConsole() : 	mLastLogPos(0), mEnabled(false), mVisible(false), mFadeIn(false), mFadeOut(false), mExpand(false), mFading(false), mShowFps(false),
						mConColor(35, 47, 73, 230), mConLineColor(55, 67, 93, 230), mFontColor(153, 153, 179, 230), mFontLineColor(255, 255, 255, 230),
						mWidth(0), mHeight(0), mHeightMin(0), mY(0.0f), mA(0.0f), mFadeSpeed(250.f),
						mMaxLogLines(1024), mFont(NULL), mTexId(0), mCurAlpha(0), mCurSide(false)
{
}

cConsole::~cConsole() {
	mCallbacks.clear();
	mCmdLog.clear();
	mLastCommands.clear();

	if ( mMyCallback && NULL != cInput::ExistsSingleton() )
		cInput::instance()->PopCallback( mMyCallback );
}

void cConsole::Create( cFont* Font, const bool& MakeDefaultCommands, const eeUint& MaxLogLines, const Uint32& TextureId, const eeRGBA& ConsoleColor, const eeRGBA& ConsoleLineColor, const eeRGBA& FontColor, const eeRGBA& FontLineColor ) {
	mFont = Font;

	mFontSize = (eeFloat)mFont->GetFontSize();

	if ( !FontColor.voidRGB )
		mFontColor = FontColor;

	if ( !FontLineColor.voidRGB )
		mFontLineColor = FontLineColor;

	if ( !ConsoleColor.voidRGB )
		mConColor = ConsoleColor;

	if ( !ConsoleLineColor.voidRGB )
		mConLineColor = ConsoleLineColor;

	if ( TextureId > 0 )
		mTexId = TextureId;

	mMaxLogLines = MaxLogLines;
	mMaxAlpha = (eeFloat)mConColor.A();

	mEnabled = true;
	mFontSize *= 1.25f;

	if ( MakeDefaultCommands )
		CreateDefaultCommands();

	mWidth = (eeFloat) cEngine::instance()->GetWidth();
	mHeight = (eeFloat) cEngine::instance()->GetHeight();
	mHeightMin = (eeFloat) cEngine::instance()->GetHeight() * 0.4f;

	mMyCallback = cInput::instance()->PushCallback( boost::bind( &cConsole::PrivInputCallback, this, _1) );

	mTBuf.SetReturnCallback( boost::bind( &cConsole::ProcessLine, this ) );
	mTBuf.Start();
	mTBuf.SupportNewLine( false );
	mTBuf.Active( false );
	IgnoreCharOnPrompt( 9 );

	mCon.ConModif = 0;

	CmdGetLog();
}

void cConsole::AddCommand( const std::wstring& Command, ConsoleCallback CB ) {
	if ( !(mCallbacks.count( Command ) > 0) )
		mCallbacks[Command] = CB;
}

void cConsole::AddCommand( const std::string& Command, ConsoleCallback CB ) {
	AddCommand ( toWStr( Command ), CB );
}

void cConsole::Draw() {
	if ( mEnabled && NULL != mFont ) {
		Fade();

		if ( mY > 0.0f ) {
			if ( mTexId == 0 ) {
				mPri.SetColor( eeColorA( mConColor.R(), mConColor.G(), mConColor.B(), static_cast<Uint8>(mA) ) );
				mPri.DrawRectangle( 0.0f, 0.0f, mWidth, mY );
			} else {
				eeRGBA C( mConColor.R(), mConColor.G(), mConColor.B(), static_cast<Uint8>(mA) );

				cTexture * Tex = cTextureFactory::instance()->GetTexture( mTexId );

				if ( NULL != Tex )
					Tex->DrawEx( 0.0f, 0.0f, mWidth, mY, 0.0f, 1.0f, C, C, C, C );
			}
			mPri.SetColor( eeColorA( mConLineColor.R(), mConLineColor.G(), mConLineColor.B(), static_cast<Uint8>(mA) ) );
			mPri.DrawRectangle( 0.0f, mY, mWidth, 4.0f );

			Int16 LinesInScreen = (Int16) ( (mCurHeight / mFontSize) - 1 );

			if ( static_cast<Int16>( mCmdLog.size() ) > LinesInScreen )
				mEx = mCmdLog.size() - LinesInScreen;
			else
				mEx = 0;
			mTempY = -mCurHeight;

			Uint16 Pos = 0;
			eeFloat CurY;

			mCon.ConMin = mEx;
			mCon.ConMax = mCmdLog.size() - 1;

			mFont->Color( eeColorA ( mFontColor.R(), mFontColor.G(), mFontColor.B(), static_cast<Uint8>(mA) ) );

			for (eeInt i = mCon.ConMax - mCon.ConModif; i >= mCon.ConMin - mCon.ConModif; i-- ) {
				if ( i < static_cast<Int16>( mCmdLog.size() ) && i >= 0 ) {
					CurY = mTempY + mY + mCurHeight - Pos * mFontSize - mFontSize * 2;

					mFont->Draw( mCmdLog[i], mFontSize, CurY );

					Pos++;
				}
			}

			CurY = mTempY + mY + mCurHeight - mFontSize;

			mFont->Color( eeColorA ( mFontLineColor.R(), mFontLineColor.G(), mFontLineColor.B(), static_cast<Uint8>(mA) ) );
			mFont->SetText( L"> " + mTBuf.Buffer() );
			mFont->Draw( mFontSize, CurY );

			mFont->Color( eeColorA ( mFontLineColor.R(), mFontLineColor.G(), mFontLineColor.B(), static_cast<Uint8>(mCurAlpha) ) );
			if ( (eeUint)mTBuf.CurPos() == mTBuf.Buffer().size() ) {
				mFont->Draw( L"_", mFontSize + mFont->GetTextWidth() , CurY );
			} else {
				mFont->SetText( L"> " + mTBuf.Buffer().substr( 0, mTBuf.CurPos() ) );
				mFont->Draw( L"_", mFontSize + mFont->GetTextWidth() , CurY );
			}
		}
	}

	if ( mShowFps ) {
		mFont->Color( eeColorA () );
		mFont->SetText( L"FPS: " + toWStr( cEngine::instance()->FPS() ) );
		mFont->Draw( cEngine::instance()->GetWidth() - mFont->GetTextWidth() - 15, 6 );
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
	std::wstring wstr = mTBuf.Buffer();;
	std::vector < std::wstring > params = SplitString( wstr, L' ' );

	mLastCommands.push_back( wstr );
	mLastLogPos = mLastCommands.size();

	if ( mLastCommands.size() > 20 )
		mLastCommands.pop_front();

	if ( wstr.size() > 0 ) {
		PushText( L"> " + params[0] );

		if ( mCallbacks.find( params[0] ) != mCallbacks.end() ) {
			mCallbacks[ params[0] ]( params );
		} else {
			PushText( L"Unknown Command: '" + params[0] + L"'" );
		}
	}
	mTBuf.Clear();
}

void cConsole::PrivPushText( const std::wstring& str ) {
	mCmdLog.push_back( str );

	if ( mCmdLog.size() >= mMaxLogLines )
		mCmdLog.pop_front();
}

void cConsole::PushText( const std::wstring& str ) {
	PrivPushText( str );
}

void cConsole::PushText( const std::string& str ) {
	PrivPushText( toWStr( str ) );
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
		mCurAlpha -= 255.f * cEngine::instance()->Elapsed() / mFadeSpeed;
		if ( mCurAlpha <= 0.0f ) {
			mCurAlpha = 0.0f;
			mCurSide = !mCurSide;
		}
	} else {
		mCurAlpha += 255.f * cEngine::instance()->Elapsed() / mFadeSpeed;
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
		mY += mCurHeight * cEngine::instance()->Elapsed() / mFadeSpeed;

		mA = ( mY * mMaxAlpha / mCurHeight ) ;
		if ( mY > mCurHeight ) {
			mY = mCurHeight;
			mFadeIn = false;
			mFading = false;
		}
	}

	if ( mFadeOut ) {
		mFadeIn = false;
		mY -= mCurHeight * cEngine::instance()->Elapsed() / mFadeSpeed;

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

void cConsole::PrintCommandsStartingWith( const std::wstring& start ) {
	std::list<std::wstring> cmds;
	std::map < std::wstring, ConsoleCallback >::iterator it;

	for ( it = mCallbacks.begin(); it != mCallbacks.end(); it++ ) {
		if ( -1 != StrStartsWith( start, it->first ) ) {
			cmds.push_back( it->first );
		}
	}

	if ( cmds.size() > 1 ) {
		PushText( L"> " + mTBuf.Buffer() );

		std::list<std::wstring>::iterator ite;

		for ( ite = cmds.begin(); ite != cmds.end(); ite++ )
			PushText( (*ite) );

	} else if ( cmds.size() ) {
		mTBuf.Buffer( cmds.front() );
		mTBuf.CurPos( cmds.front().size() );
	}
}

void cConsole::PrivInputCallback( EE_Event* Event ) {
	switch( Event->type ) {
		case SDL_KEYUP:
			if ( mVisible ) {
				if ( ( ( Event->key.keysym.mod & KMOD_SHIFT ) && Event->key.keysym.sym == SDLK_INSERT ) || ( ( Event->key.keysym.mod & KMOD_CTRL ) && Event->key.keysym.sym == SDLK_v ) ) {
					std::wstring tStr = cEngine::instance()->GetClipboardTextWStr();
					if ( tStr.size() ) {
						std::vector<std::wstring> tVec = SplitString( tStr );
						if ( tVec.size() > 1 ) {
							for (eeUint i = 0; i < tVec.size() - 1; i++)
								PushText( tVec[i] );
						}
						tStr = tVec[ tVec.size() - 1 ];

						if ( (eeUint)mTBuf.CurPos() != mTBuf.Buffer().size() ) {
							std::wstring part1 = mTBuf.Buffer().substr( 0, mTBuf.CurPos() );
							std::wstring part2 = mTBuf.Buffer().substr( mTBuf.CurPos(), mTBuf.Buffer().size()-mTBuf.CurPos() );
							mTBuf.Buffer( part1 + tStr + part2 );
							mTBuf.CurPos( part1.size() );
						} else {
							std::wstring Str = mTBuf.Buffer();
							mTBuf.Buffer( Str + tStr );
						}
					}
				} else if ( ( Event->key.keysym.sym == SDLK_TAB ) && (eeUint)mTBuf.CurPos() == mTBuf.Buffer().size() ) {
					PrintCommandsStartingWith( mTBuf.Buffer() );
				}
			}
			break;
		case SDL_KEYDOWN:
			if ( mVisible ) {
				if ( mLastCommands.size() > 0 ) {
					if ( Event->key.keysym.sym == SDLK_UP && mLastLogPos > 0 )
						mLastLogPos--;

					if ( Event->key.keysym.sym == SDLK_DOWN && mLastLogPos < static_cast<Int16>( mLastCommands.size() ) )
						mLastLogPos++;

					if ( Event->key.keysym.sym == SDLK_UP ||  Event->key.keysym.sym == SDLK_DOWN ) {
						if ( mLastLogPos == static_cast<Int16>( mLastCommands.size() ) ) {
							mTBuf.Buffer( L"" );
						} else {
							mTBuf.Buffer( mLastCommands[mLastLogPos] );
						}
					}
				}

				if ( Event->key.keysym.sym == SDLK_PAGEUP ) {
					if ( mCon.ConMin - mCon.ConModif > 0 )
						mCon.ConModif++;
				}

				if ( Event->key.keysym.sym == SDLK_PAGEDOWN ) {
					if ( mCon.ConModif > 0 )
						mCon.ConModif--;
				}

				if ( Event->key.keysym.sym == SDLK_HOME ) {
					Int16 LinesInScreen = static_cast<Int16> ( (mCurHeight / mFontSize) - 1 );
					if ( static_cast<Int16>( mCmdLog.size() ) > LinesInScreen )
						mCon.ConModif = mCon.ConMin;
				}

				if ( Event->key.keysym.sym == SDLK_END ) {
					mCon.ConModif = 0;
				}
			}
			break;
		case SDL_VIDEOEXPOSE:
		case SDL_VIDEORESIZE: // Fix the size of the console if the resolution change
			mWidth = (eeFloat) cEngine::instance()->GetWidth();
			mHeight = (eeFloat) cEngine::instance()->GetHeight();
			mHeightMin = (eeFloat) cEngine::instance()->GetHeight() * 0.4f;

			if (mVisible) {
				if ( mExpand )
					mCurHeight = mHeight;
				else
					mCurHeight = mHeightMin;

				mY = mCurHeight;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if ( mVisible ) {
				if ( Event->button.button == SDL_BUTTON_WHEELUP ) {
					if ( mCon.ConMin - mCon.ConModif > 0 )
						mCon.ConModif++;
				}

				if ( Event->button.button == SDL_BUTTON_WHEELDOWN ) {
					if ( mCon.ConModif > 0 )
						mCon.ConModif--;
				}
			}
			break;
	}
}

void cConsole::CreateDefaultCommands() {
	AddCommand( L"clear", boost::bind( &cConsole::CmdClear, this, _1) );
	AddCommand( L"quit", boost::bind( &cConsole::CmdQuit, this, _1) );
	AddCommand( L"maximize", boost::bind( &cConsole::CmdMaximize, this, _1) );
	AddCommand( L"minimize", boost::bind( &cConsole::CmdMinimize, this, _1) );
	AddCommand( L"cmdlist", boost::bind( &cConsole::CmdCmdList, this, _1) );
	AddCommand( L"help", boost::bind( &cConsole::CmdCmdList, this, _1) );
	AddCommand( L"showcursor", boost::bind( &cConsole::CmdShowCursor, this, _1) );
	AddCommand( L"setframelimit", boost::bind( &cConsole::CmdFrameLimit, this, _1) );
	AddCommand( L"getlog", boost::bind( &cConsole::CmdGetLog, this, _1) );
	AddCommand( L"setgamma", boost::bind( &cConsole::CmdSetGamma, this, _1) );
	AddCommand( L"setvolume", boost::bind( &cConsole::CmdSetVolume, this, _1) );
	AddCommand( L"getgpuextensions", boost::bind( &cConsole::CmdGetGpuExtensions, this, _1) );
	AddCommand( L"dir", boost::bind( &cConsole::CmdDir, this, _1) );
	AddCommand( L"ls", boost::bind( &cConsole::CmdDir, this, _1) );
	AddCommand( L"showfps", boost::bind( &cConsole::CmdShowFps, this, _1) );
	AddCommand( L"gettexturememory", boost::bind( &cConsole::CmdGetTextureMemory, this, _1) );
}

void cConsole::CmdClear	() {
	Uint16 CutLines;
	if ( mExpand ) {
		CutLines = (Uint16)( mHeight / mFontSize );
	} else {
		CutLines = (Uint16)( mHeightMin / mFontSize );
	}

	for (Uint16 i = 0; i < CutLines; i++ )
		PushText( L"" );
}

void cConsole::CmdClear	( const std::vector < std::wstring >& params ) {
	CmdClear();
}

void cConsole::CmdMaximize ( const std::vector < std::wstring >& params ) {
	mExpand = true;
	mY = mHeight;
	PushText( L"Console Maximized" );
}

void cConsole::CmdMinimize ( const std::vector < std::wstring >& params ) {
	mExpand = false;
	mY = mHeightMin;
	PushText( L"Console Minimized" );
}

void cConsole::CmdQuit ( const std::vector < std::wstring >& params ) {
	cEngine::instance()->Running(false);
}

void cConsole::CmdGetTextureMemory ( const std::vector < std::wstring >& params ) {
	std::wstring size = L" bytes";
	eeDouble mem = static_cast<eeDouble>( cTextureFactory::instance()->MemorySize() );
	Uint8 c = 0;

	while ( mem > 1024 ) {
		c++;
		mem = mem / 1024;
	}

	switch (c) {
		case 1: size = L" KB"; break;
		case 2: size = L" MB"; break;
		case 3: size = L" GB"; break;
		case 4: size = L" TB"; break;
		default: size = L" WTF";
	}

	PushText( L"Total texture memory used: "+ toWStr( mem ) + size );
}

void cConsole::CmdCmdList ( const std::vector < std::wstring >& params ) {
	std::map < std::wstring, ConsoleCallback >::iterator itr;
	for (itr = mCallbacks.begin(); itr != mCallbacks.end(); itr++) {
		PushText( itr->first );
	}
}

void cConsole::CmdShowCursor ( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			Int32 tInt = boost::lexical_cast<Int32>( wstringTostring( params[1] ) );
			if ( tInt == 0 || tInt == 1 ) {
				cEngine::instance()->ShowCursor( 0 != tInt );
				PushText( L"showcursor " + toWStr(tInt) );
			} else
				PushText( L"Valid parameters are 0 or 1." );
		} catch (boost::bad_lexical_cast&) {
			PushText( L"Invalid Parameter. Expected int value from '" + params[1] + L"'." );
		}
	}
}

void cConsole::CmdFrameLimit ( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			Int32 tInt = boost::lexical_cast<Int32>( wstringTostring( params[1] ) );
			if ( tInt >= 0 && tInt <= 10000 ) {
				cEngine::instance()->SetFrameRateLimit( tInt );
				PushText( L"setframelimit " + toWStr(tInt) );
			} else
				PushText( L"Valid parameters are between 0 and 10000 (0 = no limit)." );
		} catch (boost::bad_lexical_cast&) {
			PushText( L"Invalid Parameter. Expected int value from '" + params[1] + L"'." );
		}
	}
}

void cConsole::CmdGetLog() {
	std::vector < std::wstring > tvec = SplitString( toWStr( cLog::instance()->Buffer() ) );
	if ( tvec.size() > 0 ) {
		for ( eeUint i = 0; i < tvec.size(); i++ )
			PushText( tvec[i] );
	}
}

void cConsole::CmdGetLog( const std::vector < std::wstring >& params ) {
	CmdGetLog();
}

void cConsole::CmdGetGpuExtensions() {
    char *Exts = (char *)glGetString(GL_EXTENSIONS);
    std::vector < std::wstring > tvec = SplitString( toWStr( std::string( Exts ) ), L' ' );
	if ( tvec.size() > 0 ) {
		for ( eeUint i = 0; i < tvec.size(); i++ )
			PushText( tvec[i] );
	}
}

void cConsole::CmdGetGpuExtensions( const std::vector < std::wstring >& params ) {
    CmdGetGpuExtensions();
}

void cConsole::CmdSetGamma( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			eeFloat tFloat = boost::lexical_cast<eeFloat>( wstringTostring( params[1] ) );
			if ( tFloat > 0.1f && tFloat <= 10.0f ) {
				cEngine::instance()->SetGamma( tFloat, tFloat, tFloat );
				PushText( L"setgamma " + toWStr(tFloat) );
			} else
				PushText( L"Valid parameters are between 0.1 and 10." );
		} catch (boost::bad_lexical_cast&) {
			PushText( L"Invalid Parameter. Expected float value." );
		}
	}
}

void cConsole::CmdSetVolume( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			eeFloat tFloat = boost::lexical_cast<eeFloat>( wstringTostring( params[1] ) );
			if ( tFloat >= 0.0f && tFloat <= 100.0f ) {
				EE::Audio::cAudioListener::instance()->SetGlobalVolume( tFloat );
				PushText( L"setvolume " + toWStr(tFloat) );
			} else
				PushText( L"Valid parameters are between 0 and 100." );
		} catch (boost::bad_lexical_cast&) {
			PushText( L"Invalid Parameter. Expected eeFloat value." );
		}
	}

}

void cConsole::CmdDir( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			#if EE_PLATFORM == EE_PLATFORM_WIN32
			std::string Slash( "/\\" );
			#else
			std::string Slash( "/" );
			#endif
			std::string myPath = wstringTostring( params[1] );
			std::string myOrder;

			if ( params.size() > 2 ) {
				for ( eeUint i = 2; i < params.size(); i++ ) {
					if ( i + 1 == params.size() ) {
						if ( params[i] == L"ff" )
							myOrder = wstringTostring( params[i] );
						else
							myPath += " " + wstringTostring( params[i] );
					} else {
						myPath += " " + wstringTostring( params[i] );
					}
				}
			}

			if ( IsDirectory( myPath ) ) {
				eeUint i;

				std::vector<std::string> mFiles = FilesGetInPath( myPath );
				std::sort( mFiles.begin(), mFiles.end() );

				PushText( "Directory: " + myPath );

				if ( myOrder == "ff" ) {
					std::vector<std::string> mFolders;
					std::vector<std::string> mFile;

					for ( i = 0; i < mFiles.size(); i++ ) {
						if ( IsDirectory( myPath + Slash + mFiles[i] ) ) {
							mFolders.push_back( mFiles[i] );
						} else {
							mFile.push_back( mFiles[i] );
						}
					}

					if ( mFolders.size() )
						PushText( L"Folders: " );

					for ( i = 0; i < mFolders.size(); i++ )
						PushText( "	" + mFolders[i] );

					if ( mFolders.size() )
						PushText( L"Files: " );

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
		} catch (...) {
			PushText( L"Invalid Parameter." );
		}
	}
}

void cConsole::CmdShowFps( const std::vector < std::wstring >& params ) {
	if ( params.size() >= 2 ) {
		try {
			Int32 tInt = boost::lexical_cast<Int32>( wstringTostring( params[1] ) );
			if ( tInt == 0 || tInt == 1 ) {
				mShowFps = 0 != tInt;
				PushText( L"showfps " + toWStr(tInt) );
			} else
				PushText( L"Valid parameters are 0 or 1." );
		} catch (boost::bad_lexical_cast&) {
			PushText( L"Invalid Parameter. Expected int value from '" + params[1] + L"'." );
		}
	}
}

void cConsole::IgnoreCharOnPrompt( const Uint32& ch ) {
	mTBuf.PushIgnoredChar( ch );
}

}}
