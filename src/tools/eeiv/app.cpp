#include <eepp/config.hpp>
#include <eepp/core/string.hpp>

using namespace std::literals;

// This application is not meant to be used as an example of beautiful code,
// it's just old code that works fine and looks ugly. It was made exclusively
// for my personal use, and still manages to satisfy my very basic image viewing needs.
// Some day i'll make this look good.

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <string>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
static std::string getWindowsPath() {
#ifdef UNICODE
	wchar_t Buffer[1024];
	GetWindowsDirectory( Buffer, 1024 );
	return EE::String( Buffer ).toUtf8();
#else
	char Buffer[1024];
	GetWindowsDirectory( Buffer, 1024 );
	return std::string( Buffer );
#endif
}
#undef RGB
#undef KEY_EXECUTE
#endif

#include "app.hpp"

bool App::isRawImage( const std::string& path ) {
	std::string Ext = FileSystem::fileExtension( path );
	return Ext == "uint8" || Ext == "float32";
}

bool App::isImage( const std::string& _path ) {
	std::string path = _path;

	if ( path.size() >= 7 && path.substr( 0, 7 ) == "file://" )
		path = path.substr( 7 );

	if ( !FileSystem::isDirectory( path ) && FileSystem::fileSize( path ) ) {
		if ( Image::isImage( path ) ) {
			return true;
		} else {
			std::string Ext = FileSystem::fileExtension( path );
			if ( Ext == "uint8" || Ext == "float32" ) {
				return true;
			}
			return Image::isImage( path );
		}
	}

	return false;
}

bool App::isHttpUrl( const std::string& path ) {
	return path.substr( 0, 7 ) == "http://" || path.substr( 0, 8 ) == "https://";
}

App::App( int argc, char* argv[] ) :
	Fon( NULL ),
	Mon( NULL ),
	mCurImg( 0 ),
	mFading( false ),
	mAlpha( 255 ),
	mCurAlpha( 255 ),
	mLaterLoad( false ),
	mCursor( true ),
	mMouseLeftPressing( false ),
	mMouseMiddlePressing( false ),
	mImgRT( RENDER_NORMAL ),
	mFilter( Texture::Filter::Linear ),
	mShowHelp( false ),
	mFirstLoad( false ),
	mUsedTempDir( false ),
	mLockZoomAndPosition( false ),
	mHelpCache( NULL ),
	mSlideShow( false ),
	mSlideTime( 4000 ),
	mSlideTicks( Sys::getTicks() ) {
	mStorePath = Sys::getConfigPath( "eeiv" ) + FileSystem::getOSSlash();
	mTmpPath = mStorePath + "tmp" + FileSystem::getOSSlash();

	std::string nstr;

	if ( argc > 1 )
		nstr.assign( argv[1] );
	else
		nstr.assign( argv[0] );

	loadDir( nstr, false );
}

App::~App() {
	clearTempDir();
}

void App::loadConfigValues() {
	mConfig.Width = Ini.getValueI( "window", "width", 1024 );
	mConfig.Height = Ini.getValueI( "window", "height", 768 );
	mConfig.BitColor = Ini.getValueI( "window", "bitcolor", 32 );
	mConfig.Windowed = Ini.getValueB( "window", "windowed", true );
	mConfig.Resizeable = Ini.getValueB( "window", "resizeable", true );
	mConfig.VSync = Ini.getValueB( "window", "vsync", true );
	mConfig.DoubleBuffering = Ini.getValueB( "window", "doublebuffering", true );
	mConfig.UseDesktopResolution = Ini.getValueB( "window", "usedesktopresolution", false );
	mConfig.NoFrame = Ini.getValueB( "window", "borderless", false );
	mConfig.MaximizeAtStart = Ini.getValueB( "window", "maximizeatstart", true );
	mConfig.FrameLimit = Ini.getValueI( "window", "framelimit", 0 );
	mConfig.Fade = Ini.getValueB( "viewer", "fade", true );
	mConfig.LateLoading = Ini.getValueB( "viewer", "lateloading", true );
	mConfig.BlockWheelSpeed = Ini.getValueB( "viewer", "blockwheelspeed", true );
	mConfig.ShowInfo = Ini.getValueB( "viewer", "showinfo", true );
	mConfig.TransitionTime = Ini.getValueI( "viewer", "transitiontime", 200 );
	mConfig.ConsoleFontSize = Ini.getValueI( "viewer", "consolefontsize", 12 );
	mConfig.AppFontSize = Ini.getValueI( "viewer", "appfontsize", 12 );
	mConfig.DefaultImageZoom = Ini.getValueF( "viewer", "defaultimagezoom", 1 );
	mConfig.WheelBlockTime = Ini.getValueI( "viewer", "wheelblocktime", 200 );
}

void App::loadConfig() {
	std::string tPath = mStorePath + "eeiv.ini";
	Ini.loadFromFile( tPath );

	if ( FileSystem::fileExists( tPath ) ) {
		Ini.readFile();
		loadConfigValues();
	} else {
		Ini.setValueI( "window", "width", 1024 );
		Ini.setValueI( "window", "height", 768 );
		Ini.setValueI( "window", "bitcolor", 32 );
		Ini.setValueI( "window", "windowed", 1 );
		Ini.setValueI( "window", "resizeable", 1 );
		Ini.setValueI( "window", "vsync", 1 );
		Ini.setValueI( "window", "doublebuffering", 1 );
		Ini.setValueI( "window", "usedesktopresolution", 0 );
		Ini.setValueI( "window", "borderless", 0 );
		Ini.setValueI( "window", "maximizeatstart", 1 );
		Ini.setValueI( "window", "framelimit", 0 );
		Ini.setValueI( "viewer", "fade", 1 );
		Ini.setValueI( "viewer", "lateloading", 1 );
		Ini.setValueI( "viewer", "blockwheelspeed", 1 );
		Ini.setValueI( "viewer", "showinfo", 1 );
		Ini.setValueI( "viewer", "transitiontime", 200 );
		Ini.setValueI( "viewer", "consolefontsize", 12 );
		Ini.setValueI( "viewer", "appfontsize", 12 );
		Ini.setValueI( "viewer", "defaultimagezoom", 1 );
		Ini.setValueI( "viewer", "wheelblocktime", 200 );

		if ( !FileSystem::isDirectory( mStorePath ) )
			FileSystem::makeDir( mStorePath );

		Ini.writeFile();
		loadConfigValues();
	}
}

bool App::init() {
	loadConfig();

	EE = Engine::instance();
	MyPath = Sys::getProcessPath();

	std::string iconp( MyPath + "assets/eeiv.png" );

	if ( !FileSystem::fileExists( iconp ) ) {
		iconp = MyPath + "assets/icon/ee.png";
	}

	WindowSettings WinSettings = EE->createWindowSettings( &Ini, "window" );
	ContextSettings ConSettings = EE->createContextSettings( &Ini, "window" );

	WinSettings.Icon = iconp;
	WinSettings.Title = "eeiv";

	mWindow = EE->createWindow( WinSettings, ConSettings );

	if ( mWindow->isOpen() ) {
		Display* currentDisplay = Engine::instance()->getDisplayManager()->getDisplayIndex(
			mWindow->getCurrentDisplayIndex() );

		PixelDensity::setPixelDensity( PixelDensity::toFloat( currentDisplay->getPixelDensity() ) );

		formatConfiguration.svgScale( 2 /*PixelDensity::getPixelDensity()*/ );

		if ( mConfig.FrameLimit )
			mWindow->setFrameRateLimit( 60 );

		mWindow->setCloseRequestCallback( [&]( EE::Window::Window* ) -> bool {
			updateConfig();
			return true;
		} );

		TF = TextureFactory::instance();
		Log = Log::instance();
		KM = mWindow->getInput();

		if ( mConfig.MaximizeAtStart )
			mWindow->maximize();

		Clock TE;

		std::string MyFontPath = MyPath + "assets/fonts" + FileSystem::getOSSlash();

		TTF = FontTrueType::New( "DejaVuSans" );
		TTFMon = FontTrueType::New( "DejaVuSansMono" );

#if EE_PLATFORM == EE_PLATFORM_WIN
		std::string fontsPath( getWindowsPath() + "\\Fonts\\" );
#else
		std::string fontsPath( "/usr/share/fonts/truetype/" );
#endif

		if ( FileSystem::fileExists( fontsPath + "DejaVuSans.ttf" ) &&
			 FileSystem::fileExists( fontsPath + "DejaVuSansMono.ttf" ) ) {
			TTF->loadFromFile( fontsPath + "DejaVuSans.ttf" );
			TTFMon->loadFromFile( fontsPath + "DejaVuSansMono.ttf" );
		} else if ( FileSystem::fileExists( MyFontPath + "DejaVuSans.ttf" ) &&
					FileSystem::fileExists( MyFontPath + "DejaVuSansMono.ttf" ) ) {
			TTF->loadFromFile( MyFontPath + "DejaVuSans.ttf" );
			TTFMon->loadFromFile( MyFontPath + "DejaVuSansMono.ttf" );
		} else if ( FileSystem::fileExists( fontsPath + "Arial.ttf" ) &&
					FileSystem::fileExists( fontsPath + "cour.ttf" ) ) {
			TTF->loadFromFile( fontsPath + "Arial.ttf" );
			TTFMon->loadFromFile( fontsPath + "cour.ttf" );
		} else {
			Log::instance()->writef( "Fonts not found... closing." );
			return false;
		}

		Fon = TTF.get();
		Mon = TTFMon.get();

		FonCache.setFont( Fon );
		FonCache.setFontSize( mConfig.AppFontSize );
		FonCache.setOutlineThickness( PixelDensity::dpToPxI( 1 ) );

		Log::instance()->writef( "Fonts loading time: %f ms",
								 TE.getElapsedTimeAndReset().asMilliseconds() );

		if ( !Fon && !Mon )
			return false;

		UISceneNode* uiSceneNode = UISceneNode::New();
		uiSceneNode->getUIThemeManager()->setDefaultFont( Fon );
		SceneManager::instance()->add( uiSceneNode );

		Con = UIConsole::NewOpt( Mon, true, true, 1024000 );
		Con->setQuakeMode( true );
		Con->setVisible( false );
		Con->setFontSize( mConfig.ConsoleFontSize );

		mConCmds = std::make_unique<ConsoleCommands>( this, Con );

		Con->addCommand( "loaddir", [this]( auto event ) { mConCmds->cmdLoadDir( event ); } );
		Con->addCommand( "loadimg", [this]( auto event ) { mConCmds->cmdLoadImg( event ); } );
		Con->addCommand( "setbackcolor",
						 [this]( auto event ) { mConCmds->cmdSetBackColor( event ); } );
		Con->addCommand( "setimgfade", [this]( auto event ) { mConCmds->cmdSetImgFade( event ); } );
		Con->addCommand( "setlateloading",
						 [this]( auto event ) { mConCmds->cmdSetLateLoading( event ); } );
		Con->addCommand( "setblockwheel",
						 [this]( auto event ) { mConCmds->cmdSetBlockWheel( event ); } );
		Con->addCommand( "moveto", [this]( auto event ) { mConCmds->cmdMoveTo( event ); } );
		Con->addCommand( "batchimgscale",
						 [this]( auto event ) { mConCmds->cmdBatchImgScale( event ); } );
		Con->addCommand( "batchimgchangeformat",
						 [this]( auto event ) { mConCmds->cmdBatchImgChangeFormat( event ); } );
		Con->addCommand( "batchimgthumbnail",
						 [this]( auto event ) { mConCmds->cmdBatchImgThumbnail( event ); } );
		Con->addCommand( "imgchangeformat",
						 [this]( auto event ) { mConCmds->cmdImgChangeFormat( event ); } );
		Con->addCommand( "imgresize", [this]( auto event ) { mConCmds->cmdImgResize( event ); } );
		Con->addCommand( "imgscale", [this]( auto event ) { mConCmds->cmdImgScale( event ); } );
		Con->addCommand( "imgthumbnail",
						 [this]( auto event ) { mConCmds->cmdImgThumbnail( event ); } );
		Con->addCommand( "imgcentercrop",
						 [this]( auto event ) { mConCmds->cmdImgCenterCrop( event ); } );
		Con->addCommand( "slideshow", [this]( auto event ) { mConCmds->cmdSlideShow( event ); } );
		Con->addCommand( "setzoom", [this]( auto event ) { mConCmds->cmdSetZoom( event ); } );

		setWindowCaption();

		getImages();

		if ( mFile != "" ) {
			fastLoadImage( curImagePos( mFile ) );
		} else {
			if ( mFiles.size() )
				fastLoadImage( 0 );
		}

		if ( 0 == mFiles.size() && 0 == mFile.length() ) {
			Con->toggle();
		}

		return true;
	}

	return false;
}

void App::process() {
	if ( init() ) {
		do {
			ET = mWindow->getElapsed().asMilliseconds();

			input();

			TEP.restart();

			if ( mWindow->isVisible() ) {
				render();

				if ( KM->isKeyUp( KEY_F12 ) )
					mWindow->takeScreenshot();

				mWindow->display( true );
			} else {
				Sys::sleep( Milliseconds( 16 ) );
			}

			RET = TEP.getElapsedTimeAndReset().asMilliseconds();

			if ( mConfig.LateLoading && mLaterLoad ) {
				if ( Sys::getTicks() - mLastLaterTick > mConfig.TransitionTime ) {
					updateImages();
					mLaterLoad = false;
				}
			}

			if ( mFirstLoad ) {
				updateImages();
				mFirstLoad = false;
			}
		} while ( mWindow->isRunning() );
	}

	end();
}

void App::loadDir( const std::string& path, const bool& getimages ) {
	std::string tmpFile;

	if ( !FileSystem::isDirectory( path ) ) {
		if ( path.substr( 0, 7 ) == "file://" ) {
			mFilePath = path.substr( 7 );
			mFilePath = mFilePath.substr( 0, mFilePath.find_last_of( FileSystem::getOSSlash() ) );
			tmpFile = path.substr( path.find_last_of( FileSystem::getOSSlash() ) + 1 );
		} else if ( isHttpUrl( path ) ) {
			mUsedTempDir = true;

			if ( !FileSystem::isDirectory( mTmpPath ) )
				FileSystem::makeDir( mTmpPath );

			URI uri( path );
			Http http( uri.getHost(), uri.getPort() );
			Http::Request request( uri.getPathAndQuery() );
			Http::Response response = http.sendRequest( request );

			if ( response.getStatus() == Http::Response::Ok ) {
				if ( !FileSystem::fileWrite(
						 mTmpPath + "tmpfile",
						 reinterpret_cast<const Uint8*>( &response.getBody()[0] ),
						 response.getBody().size() ) ) {
					Con->pushText( "Couldn't write the downloaded image to disk." );

					return;
				}
			} else {
				Con->pushText( "Couldn't download the image from network." );

				return;
			}

			mFilePath = mTmpPath;
			tmpFile = "tmpfile";
		} else {
			mFilePath = path.substr( 0, path.find_last_of( FileSystem::getOSSlash() ) );
			tmpFile = path.substr( path.find_last_of( FileSystem::getOSSlash() ) + 1 );
		}

		String::replaceAll( mFilePath, "%20", " " );

		if ( mFilePath == "" ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
			mFilePath = "C:\\";
#else
			mFilePath = FileSystem::getOSSlash();
#endif
		}

		FileSystem::dirAddSlashAtEnd( mFilePath );

		if ( isImage( mFilePath + tmpFile ) )
			mFile = tmpFile;
		else
			return;
	} else {
		mFilePath = path;

		FileSystem::dirAddSlashAtEnd( mFilePath );
	}

	mCurImg = 0;

	if ( getimages )
		getImages();

	if ( mFiles.size() ) {
		if ( mFile.size() )
			mCurImg = curImagePos( mFile );

		if ( mWindow->isRunning() )
			updateImages();
	}
}

void App::updateConfig() {
	mConfig.Width = EE->getCurrentWindow()->getWidth();
	mConfig.Height = EE->getCurrentWindow()->getHeight();
	mConfig.MaximizeAtStart = EE->getCurrentWindow()->isMaximized();
}

void App::clearTempDir() {
	if ( mUsedTempDir ) {
		getImages();

		for ( Uint32 i = 0; i < mFiles.size(); i++ ) {
			std::string Delfile = mFilePath + mFiles[i].Path;
			remove( Delfile.c_str() );
		}
	}
}

void App::getImages() {
	Clock TE;

	Uint32 i;
	std::vector<std::string> tStr;
	mFiles.clear();

	std::vector<std::string> tmpFiles = FileSystem::filesGetInPath( mFilePath, true );
	for ( i = 0; i < tmpFiles.size(); i++ )
		if ( isImage( mFilePath + tmpFiles[i] ) )
			tStr.push_back( tmpFiles[i] );

	for ( i = 0; i < tStr.size(); i++ ) {
		ImageData tmpI;
		tmpI.Path = tStr[i];
		tmpI.Tex.clear();

		mFiles.push_back( tmpI );
	}

	Con->pushText( "Image list loaded in %f ms.", TE.getElapsedTimeAndReset().asMilliseconds() );

	Con->pushText( "Directory: \"" + String::fromUtf8( mFilePath ) + "\"" );
	for ( Uint32 i = 0; i < mFiles.size(); i++ )
		Con->pushText( "	" + String::fromUtf8( mFiles[i].Path ) );
}

Uint32 App::curImagePos( const std::string& path ) {
	for ( Uint32 i = 0; i < mFiles.size(); i++ ) {
		if ( mFiles[i].Path == path )
			return i;
	}
	return 0;
}

void App::fastLoadImage( const Uint32& ImgNum ) {
	mCurImg = ImgNum;
	auto res = loadImage( mFiles[mCurImg].Path, true );
	mFiles[mCurImg].Tex = std::move( res.first );
	mFiles[mCurImg].animFps = res.second;
	mFirstLoad = true;
}

void App::setImage( const std::vector<TexturePtr>& Tex, const std::string& path, Float animFps ) {
	if ( !Tex.empty() ) {
		mFiles[mCurImg].Tex = Tex;

		mImgRT = RENDER_NORMAL;

		Vector2f scale( mImg.getScale() );
		if ( Tex.size() == 1 ) {
			mImg.createStatic( Tex[0] );
		} else {
			mImg.reset();
			for ( const TexturePtr& texture : Tex )
				mImg.addFrame( texture );
			mImg.setAnimationSpeed( animFps );
		}
		mImg.setRenderMode( mImgRT );
		mImg.setScale( scale );

		if ( !mLockZoomAndPosition ) {
			mImg.setScale( mConfig.DefaultImageZoom );
			mImg.setPosition( Vector2f::Zero );
		}

		if ( path != mFiles[mCurImg].Path )
			mCurImg = curImagePos( path );

		mFile = mFiles[mCurImg].Path;

		if ( !mLockZoomAndPosition )
			scaleToScreen();

		Texture* pTex = nullptr;
		for ( const TexturePtr& texture : Tex ) {
			pTex = texture.get();
			if ( pTex )
				pTex->setFilter( mFilter );
		}

		if ( NULL != pTex ) {
			FonCache.setString( "File: " + String::fromUtf8( mFile ) +
								"\nWidth: " + String::toString( pTex->getWidth() ) +
								"\nHeight: " + String::toString( pTex->getHeight() ) + "\n" +
								String::toString( mCurImg + 1 ) + "/" +
								String::toString( (Uint64)mFiles.size() ) );
		}
	} else {
		FonCache.setString( "File: " + String::fromUtf8( path ) +
							" failed to load. \nReason: " + Image::getLastFailureReason() );
	}

	setWindowCaption();
}

static Sizei imageSizeFromName( const std::string& path ) {
	Sizei size( Sizei::Zero );
	size_t xPos = std::string::npos;
	for ( size_t i = 0; i < path.size(); i++ ) {
		if ( i > 0 && i < path.size() - 1 && path[i] == 'x' && String::isNumber( path[i - 1] ) &&
			 String::isNumber( path[i + 1] ) ) {
			xPos = i;
			break;
		}
	}
	if ( xPos != std::string::npos ) {
		std::string width;
		std::string height;
		for ( size_t i = xPos - 1; i >= 0; i-- ) {
			if ( String::isNumber( path[i] ) ) {
				width = path[i] + width;
			} else {
				break;
			}
		}
		for ( size_t i = xPos + 1; i < path.size(); i++ ) {
			if ( String::isNumber( path[i] ) ) {
				height += path[i];
			} else {
				break;
			}
		}
		int w;
		int h;
		if ( String::fromString( w, width ) && String::fromString( h, height ) ) {
			return Sizei( w, h );
		}
	}
	return size;
}

std::pair<std::vector<TexturePtr>, Float> App::loadImage( const std::string& path,
														  const bool& setAsCurrent ) {
	std::vector<TexturePtr> textures;
	std::string filePath( mFilePath + path );
	Float animFps = 60;

	if ( isRawImage( filePath ) ) {
		ScopedBuffer buffer;
		if ( FileSystem::fileGet( filePath, buffer ) ) {
			Sizei size( imageSizeFromName( path ) );
			int channels = 0;
			for ( size_t c = 1; c <= 4; c++ ) {
				if ( buffer.size() - 8 == size.getWidth() * size.getHeight() * c ) {
					channels = c;
					break;
				}
			}
			if ( channels > 0 ) {
				TexturePtr tex = TF->loadFromPixels( buffer.get() + 8, size.getWidth(),
													 size.getHeight(), channels );
				if ( tex )
					textures.push_back( std::move( tex ) );
			}
		}
	} else if ( Image::getFormat( filePath ) == Image::Format::GIF ) {
		IOStreamFile stream( filePath );
		auto [gif, delay] = Texture::loadGif( stream );
		textures = std::move( gif );
		delay = delay ? delay : 100;
		animFps = 1000.f / delay;
	} else {
		TexturePtr tex = TF->loadFromFile( filePath, false, Texture::ClampMode::ClampToEdge, false,
										   false, formatConfiguration );
		if ( tex )
			textures.push_back( std::move( tex ) );
	}

	if ( setAsCurrent )
		setImage( textures, path, animFps );

	return { textures, animFps };
}

void App::updateImages() {
	for ( Int32 i = 0; i < (Int32)mFiles.size(); i++ ) {
		if ( !( i == ( mCurImg - 1 ) || i == mCurImg || i == ( mCurImg + 1 ) ) ) {
			unloadImage( i );
		}

		if ( i == ( mCurImg - 1 ) || i == ( mCurImg + 1 ) ) {
			if ( mFiles[i].Tex.empty() ) {
				auto res = loadImage( mFiles[i].Path );
				mFiles[i].Tex = std::move( res.first );
				mFiles[i].animFps = res.second;
			}
		}

		if ( i == mCurImg ) {
			if ( mFiles[i].Tex.empty() ) {
				auto res = loadImage( mFiles[i].Path, true );
				mFiles[i].Tex = std::move( res.first );
				mFiles[i].animFps = res.second;
			} else
				setImage( mFiles[i].Tex, mFiles[i].Path, mFiles[i].animFps );
		}
	}
}

void App::unloadImage( const Uint32& img ) {
	mFiles[img].Tex.clear();
}

void App::optUpdate() {
	Vector2f scale( mImg.getScale() );
	if ( mFiles[mCurImg].Tex.size() == 1 )
		mImg.createStatic( mFiles[mCurImg].Tex[0] );
	else {
		mImg.reset();
		for ( const TexturePtr& texture : mFiles[mCurImg].Tex )
			mImg.addFrame( texture );
	}
	mImg.setScale( scale );

	if ( !mLockZoomAndPosition ) {
		mImg.setScale( mConfig.DefaultImageZoom );
		mImg.setPosition( Vector2f::Zero );
		scaleToScreen();
	}

	if ( mConfig.LateLoading ) {
		mLaterLoad = true;
		mLastLaterTick = Sys::getTicks();

		if ( !mFiles[mCurImg].Tex.empty() ) {
			Texture* Tex = mFiles[mCurImg].Tex[0].get();

			if ( Tex ) {
				FonCache.setString( "File: " + String::fromUtf8( mFiles[mCurImg].Path ) +
									"\nWidth: " + String::toString( Tex->getWidth() ) +
									"\nHeight: " + String::toString( Tex->getHeight() ) + "\n" +
									String::toString( mCurImg + 1 ) + "/" +
									String::toString( (Uint64)mFiles.size() ) );
			}
		}
	} else
		updateImages();
}

void App::loadFirstImage() {
	if ( mCurImg != 0 )
		fastLoadImage( 0 );
}

void App::loadLastImage() {
	if ( mCurImg != (Int32)( mFiles.size() - 1 ) )
		fastLoadImage( mFiles.size() - 1 );
}

void App::loadNextImage() {
	if ( ( mCurImg + 1 ) < (Int32)mFiles.size() ) {
		createFade();
		mCurImg++;
		optUpdate();
	}
}

void App::loadPrevImage() {
	if ( ( mCurImg - 1 ) >= 0 ) {
		createFade();
		mCurImg--;
		optUpdate();
	}
}

void App::switchFade() {
	if ( mConfig.Fade ) {
		mAlpha = 255.0f;
		mCurAlpha = 255;
		mFading = false;
	}

	mConfig.Fade = !mConfig.Fade;
	mConfig.LateLoading = !mConfig.LateLoading;
	mConfig.BlockWheelSpeed = !mConfig.BlockWheelSpeed;
}

void App::input() {
	KM->update();
	Mouse = KM->getMousePos();

	if ( KM->isKeyDown( KEY_TAB ) && KM->isAltPressed() ) {
		mWindow->minimize();
	}

	if ( KM->isKeyDown( KEY_ESCAPE ) || ( KM->isKeyDown( KEY_Q ) && !Con->isActive() ) ) {
		updateConfig();
		mWindow->close();
	}

	if ( ( KM->isAltPressed() && KM->isKeyUp( KEY_RETURN ) ) ||
		 ( KM->isKeyUp( KEY_F ) && !Con->isActive() ) ) {
		mWindow->toggleFullscreen();

		prepareFrame();
		scaleToScreen();
	}

	if ( KM->isKeyUp( KEY_F6 ) ) {
		switchFade();
	}

	if ( KM->isKeyUp( KEY_F3 ) ) {
		Con->toggle();
	}

	if ( ( KM->isKeyUp( KEY_S ) && !Con->isActive() ) || KM->isKeyUp( KEY_F4 ) ) {
		mCursor = !mCursor;
		mWindow->getCursorManager()->setVisible( mCursor );
	}

	if ( KM->isKeyUp( KEY_H ) && !Con->isActive() ) {
		mShowHelp = !mShowHelp;
	}

	if ( ( ( KM->isKeyUp( KEY_V ) && KM->isControlPressed() ) ||
		   ( KM->isKeyUp( KEY_INSERT ) && KM->isShiftPressed() ) ) &&
		 !Con->isActive() ) {
		std::string tPath = mWindow->getClipboard()->getText();

		if ( ( tPath.size() && isImage( tPath ) ) || FileSystem::isDirectory( tPath ) ) {
			loadDir( tPath );
		}
	}

	if ( !Con->isActive() ) {
		if ( KM->mouseWheelScrolledUp() || KM->isKeyUp( KEY_PAGEUP ) ) {
			if ( !mConfig.BlockWheelSpeed ||
				 Sys::getTicks() - mLastWheelUse > mConfig.WheelBlockTime ) {
				mLastWheelUse = Sys::getTicks();
				loadPrevImage();
				disableSlideShow();
			}
		}

		if ( KM->mouseWheelScrolledDown() || KM->isKeyUp( KEY_PAGEDOWN ) ) {
			if ( !mConfig.BlockWheelSpeed ||
				 Sys::getTicks() - mLastWheelUse > mConfig.WheelBlockTime ) {
				mLastWheelUse = Sys::getTicks();
				loadNextImage();
				disableSlideShow();
			}
		}

		if ( KM->isKeyUp( KEY_I ) ) {
			mConfig.ShowInfo = !mConfig.ShowInfo;
		}
	}

	if ( mFiles.size() && !mFiles[mCurImg].Tex.empty() && !Con->isActive() ) {
		if ( KM->isKeyUp( KEY_HOME ) ) {
			loadFirstImage();
			disableSlideShow();
		}

		if ( KM->isKeyUp( KEY_END ) ) {
			loadLastImage();
			disableSlideShow();
		}

		if ( KM->isKeyUp( KEY_KP_MULTIPLY ) ) {
			scaleToScreen();
		}

		if ( KM->isKeyUp( KEY_KP_DIVIDE ) ) {
			mImg.setScale( mConfig.DefaultImageZoom );
		}

		if ( KM->isKeyUp( KEY_Z ) ) {
			zoomImage();
		}

		if ( KM->isKeyUp( KEY_N ) ) {
			if ( mWindow->getSize().getWidth() != (Int32)mImg.getSize().getWidth() ||
				 mWindow->getSize().getHeight() != (Int32)mImg.getSize().getHeight() ) {
				mWindow->setSize( mImg.getSize().getWidth(), mImg.getSize().getHeight() );
			}
		}

		if ( Sys::getTicks() - mZoomTicks >= 15 ) {
			mZoomTicks = Sys::getTicks();

			if ( KM->isKeyDown( KEY_KP_MINUS ) )
				mImg.setScale( mImg.getScale() - 0.02f );

			if ( KM->isKeyDown( KEY_KP_PLUS ) )
				mImg.setScale( mImg.getScale() + 0.02f );

			if ( mImg.getScale().x < 0.0125f )
				mImg.setScale( 0.0125f );

			if ( mImg.getScale().x > 50.0f )
				mImg.setScale( 50.0f );
		}

		if ( KM->isKeyDown( KEY_LEFT ) ) {
			Vector2f nPos(
				(Float)( (Int32)( mImg.getPosition().x +
								  ( ( mWindow->getElapsed().asMilliseconds() * 0.4f ) ) ) ),
				mImg.getPosition().y );
			mImg.setPosition( nPos );
		}

		if ( KM->isKeyDown( KEY_RIGHT ) ) {
			Vector2f nPos(
				(Float)( (Int32)( mImg.getPosition().x +
								  ( -( mWindow->getElapsed().asMilliseconds() * 0.4f ) ) ) ),
				mImg.getPosition().y );
			mImg.setPosition( nPos );
		}

		if ( KM->isKeyDown( KEY_UP ) ) {
			Vector2f nPos(
				mImg.getPosition().x,
				(Float)( (Int32)( mImg.getPosition().y +
								  ( ( mWindow->getElapsed().asMilliseconds() * 0.4f ) ) ) ) );
			mImg.setPosition( nPos );
		}

		if ( KM->isKeyDown( KEY_DOWN ) ) {
			Vector2f nPos(
				mImg.getPosition().x,
				(Float)( (Int32)( mImg.getPosition().y +
								  ( -( mWindow->getElapsed().asMilliseconds() * 0.4f ) ) ) ) );
			mImg.setPosition( nPos );
		}

		if ( KM->mouseLeftClicked() ) {
			mMouseLeftPressing = false;
		}

		if ( KM->isMouseLeftPressed() ) {
			Vector2f mNewPos;
			if ( !mMouseLeftPressing ) {
				mMouseLeftStartClick = Mouse;
				mMouseLeftPressing = true;
			} else {
				mMouseLeftClick = Mouse;

				mNewPos.x = static_cast<Float>( mMouseLeftClick.x ) -
							static_cast<Float>( mMouseLeftStartClick.x );
				mNewPos.y = static_cast<Float>( mMouseLeftClick.y ) -
							static_cast<Float>( mMouseLeftStartClick.y );

				if ( mNewPos.x != 0 || mNewPos.y != 0 ) {
					mMouseLeftStartClick = Mouse;
					mImg.setPosition( Vector2f( mImg.getPosition().x + mNewPos.x,
												mImg.getPosition().y + mNewPos.y ) );
				}
			}
		}

		if ( KM->mouseMiddleClicked() ) {
			mMouseMiddlePressing = false;
		}

		if ( KM->isMouseMiddlePressed() ) {
			if ( !mMouseMiddlePressing ) {
				mMouseMiddleStartClick = Mouse;
				mMouseMiddlePressing = true;
			} else {
				mMouseMiddleClick = Mouse;

				Vector2f v1( (Float)mMouseMiddleStartClick.x, (Float)mMouseMiddleStartClick.y );
				Vector2f v2( Vector2f( (Float)mMouseMiddleClick.x, (Float)mMouseMiddleClick.y ) );
				Line2f l1( v1, v2 );
				Float Dist = v1.distance( v2 ) * 0.01f;
				Float Ang = l1.getAngle();

				if ( Dist ) {
					mMouseMiddleStartClick = Mouse;
					if ( Ang >= 0.0f && Ang <= 180.0f ) {
						mImg.setScale( mImg.getScale() - Dist );
						if ( mImg.getScale().x < 0.0125f )
							mImg.setScale( 0.0125f );
					} else {
						mImg.setScale( mImg.getScale() + Dist );
					}
				}
			}
		}

		if ( KM->isMouseRightPressed() ) {
			Line2f line( Vector2f( Mouse.x, Mouse.y ), Vector2f( HWidth, HHeight ) );
			mImg.setRotation( line.getAngle() );
		}

		if ( KM->isKeyUp( KEY_X ) ) {
			if ( mImgRT == RENDER_NORMAL )
				mImgRT = RENDER_FLIPPED;
			else if ( mImgRT == RENDER_MIRROR )
				mImgRT = RENDER_FLIPPED_MIRRORED;
			else if ( mImgRT == RENDER_FLIPPED_MIRRORED )
				mImgRT = RENDER_MIRROR;
			else
				mImgRT = RENDER_NORMAL;

			mImg.setRenderMode( mImgRT );
		}

		if ( KM->isKeyUp( KEY_C ) ) {
			if ( mImgRT == RENDER_NORMAL )
				mImgRT = RENDER_MIRROR;
			else if ( mImgRT == RENDER_FLIPPED )
				mImgRT = RENDER_FLIPPED_MIRRORED;
			else if ( mImgRT == RENDER_FLIPPED_MIRRORED )
				mImgRT = RENDER_FLIPPED;
			else
				mImgRT = RENDER_NORMAL;

			mImg.setRenderMode( mImgRT );
		}

		if ( KM->isKeyUp( KEY_R ) ) {
			mImg.setRotation( mImg.getRotation() + 90.0f );
			scaleToScreen();
		}

		if ( KM->isKeyUp( KEY_A ) ) {
			mFilter = mFilter == Texture::Filter::Linear ? Texture::Filter::Nearest
														 : Texture::Filter::Linear;
			size_t numFrames = mImg.getNumFrames();
			for ( size_t i = 0; i < numFrames; i++ ) {
				Texture* tex = mImg.getTextureRegion( i )->getTexture().get();
				if ( tex )
					tex->setFilter( mFilter );
			}
		}

		if ( KM->isKeyUp( KEY_M ) ) {
			mImg.setPosition( Vector2f::Zero );
			mImg.setScale( mConfig.DefaultImageZoom );
			mImg.setRotation( 0.f );
			scaleToScreen();

			if ( EE->getCurrentWindow()->isMaximized() ) {
				EE->getCurrentWindow()->setSize( mImg.getSize().getWidth(),
												 mImg.getSize().getHeight() );
			}
		}

		if ( KM->isKeyUp( KEY_T ) ) {
			mImg.setPosition( Vector2f::Zero );
			mImg.setScale( mConfig.DefaultImageZoom );
			mImg.setRotation( 0.f );
			scaleToScreen();
		}

		if ( KM->isKeyUp( KEY_E ) ) {
			createSlideShow( mSlideTime );
		}

		if ( KM->isKeyUp( KEY_D ) ) {
			disableSlideShow();
		}

		if ( KM->isKeyUp( KEY_K ) ) {
			Texture* curTex;

			if ( NULL != mImg.getCurrentTextureRegion() &&
				 NULL != ( curTex = mImg.getCurrentTextureRegion()->getTexture().get() ) ) {
				curTex->setMipmap( !curTex->getMipmap() );
				curTex->reload();
			}
		}

		if ( KM->isKeyUp( KEY_L ) ) {
			mLockZoomAndPosition = !mLockZoomAndPosition;
		}

		if ( KM->isKeyUp( KEY_F5 ) ) {
			Texture* curTex;

			if ( NULL != mImg.getCurrentTextureRegion() &&
				 NULL != ( curTex = mImg.getCurrentTextureRegion()->getTexture().get() ) ) {
				Image img( curTex->getFilepath(), 0, formatConfiguration );
				curTex->replace( &img );
			}
		}
	}
}

void App::createSlideShow( Uint32 time ) {
	if ( time < 250 )
		time = 250;

	mSlideShow = true;
	mSlideTime = time;
	mSlideTicks = Sys::getTicks();
}

void App::disableSlideShow() {
	mSlideShow = false;
}

void App::doSlideShow() {
	if ( mSlideShow ) {
		if ( Sys::getTicks() - mSlideTicks >= mSlideTime ) {
			mSlideTicks = Sys::getTicks();

			if ( (Uint32)( mCurImg + 1 ) < mFiles.size() ) {
				loadNextImage();
			} else {
				disableSlideShow();
			}
		}
	}
}

void App::scaleToScreen( const bool& force ) {
	if ( mFiles.size() && !mFiles[mCurImg].Tex.empty() ) {
		Texture* Tex = mFiles[mCurImg].Tex[0].get();

		if ( NULL == Tex )
			return;

		if ( Tex->getImageWidth() * mConfig.DefaultImageZoom >= Width ||
			 Tex->getImageHeight() * mConfig.DefaultImageZoom >= Height ) {
			zoomImage();
		} else if ( force ) {
			mImg.setScale( mConfig.DefaultImageZoom );
		}
	}
}

void App::zoomImage() {
	if ( mFiles.size() && !mFiles[mCurImg].Tex.empty() ) {
		Texture* Tex = mFiles[mCurImg].Tex[0].get();

		if ( NULL == Tex )
			return;

		Sizef boxSize = mImg.getSize();

		mImg.setScale( eemin( Width / boxSize.getWidth(), Height / boxSize.getHeight() ) );
	}
}

void App::setWindowCaption() {
	if ( mFiles.size() )
		mInfo = "EEiv - " + mFiles[mCurImg].Path;
	else
		mInfo = "EEiv";

	if ( mInfo != mWindow->getTitle() )
		mWindow->setTitle( mInfo );
}

void App::prepareFrame() {
	Width = mWindow->getWidth();
	Height = mWindow->getHeight();
	HWidth = Width * 0.5f;
	HHeight = Height * 0.5f;
}

void App::render() {
	prepareFrame();

	doSlideShow();

	if ( mFiles.size() && !mFiles[mCurImg].Tex.empty() ) {
		doFade();

		Texture* Tex = mImg.getCurrentTextureRegion()->getTexture().get();

		if ( Tex ) {
			Float X = static_cast<Float>(
				static_cast<Int32>( HWidth - mImg.getSize().getWidth() * 0.5f ) );
			Float Y = static_cast<Float>(
				static_cast<Int32>( HHeight - mImg.getSize().getHeight() * 0.5f ) );

			mImg.setAutoAnimate( false );
			mImg.update();
			mImg.setOffset( Vector2i( X, Y ) );
			mImg.setAlpha( mCurAlpha );
			mImg.draw();
		}
	}

	if ( mConfig.ShowInfo )
		FonCache.draw( 0, 0 );

	printHelp();

	SceneManager::instance()->update();
	SceneManager::instance()->draw();
}

void App::createFade() {
	if ( mConfig.Fade ) {
		mAlpha = 0.0f;
		mCurAlpha = 0;
		mFading = true;
		mOldImg = mImg;
	}
}

void App::doFade() {
	if ( mConfig.Fade && mFading ) {
		mAlpha += ( 255 * RET ) / mConfig.TransitionTime;
		mCurAlpha = static_cast<Uint8>( mAlpha );

		if ( mAlpha >= 255.0f ) {
			mAlpha = 255.0f;
			mCurAlpha = 255;
			mFading = false;
		}

		Texture* Tex = NULL;

		if ( NULL != mOldImg.getCurrentTextureRegion() &&
			 ( Tex = mOldImg.getCurrentTextureRegion()->getTexture().get() ) ) {
			Float X = static_cast<Float>(
				static_cast<Int32>( HWidth - mOldImg.getSize().getWidth() * 0.5f ) );
			Float Y = static_cast<Float>(
				static_cast<Int32>( HHeight - mOldImg.getSize().getHeight() * 0.5f ) );

			mOldImg.setOffset( Vector2i( X, Y ) );
			mOldImg.setAlpha( 255 - mCurAlpha );
			mOldImg.draw();
		}
	}
}

void App::setImgScale( Float scale ) {
	mImg.setScale( scale );
}

void App::saveConfig() {
	Ini.setValueI( "window", "width", mConfig.Width );
	Ini.setValueI( "window", "height", mConfig.Height );
	Ini.setValueI( "window", "bitcolor", mConfig.BitColor );
	Ini.setValueI( "window", "windowed", mConfig.Windowed );
	Ini.setValueI( "window", "resizeable", mConfig.Resizeable );
	Ini.setValueI( "window", "vsync", mConfig.VSync );
	Ini.setValueI( "window", "doublebuffering", mConfig.DoubleBuffering );
	Ini.setValueI( "window", "usedesktopresolution", mConfig.UseDesktopResolution );
	Ini.setValueI( "window", "noframe", mConfig.NoFrame );
	Ini.setValueI( "window", "maximizeatstart", mConfig.MaximizeAtStart );
	Ini.setValueI( "window", "framelimit", mConfig.FrameLimit );
	Ini.setValueI( "viewer", "fade", mConfig.Fade );
	Ini.setValueI( "viewer", "lateloading", mConfig.LateLoading );
	Ini.setValueI( "viewer", "blockwheelspeed", mConfig.BlockWheelSpeed );
	Ini.setValueI( "viewer", "showinfo", mConfig.ShowInfo );
	Ini.setValueI( "viewer", "transitiontime", mConfig.TransitionTime );
	Ini.setValueI( "viewer", "consolefontsize", mConfig.ConsoleFontSize );
	Ini.setValueI( "viewer", "appfontsize", mConfig.AppFontSize );
	Ini.setValueF( "viewer", "defaultimagezoom", mConfig.DefaultImageZoom );
	Ini.setValueI( "viewer", "wheelblocktime", mConfig.WheelBlockTime );

	Ini.writeFile();
}

void App::end() {
	saveConfig();
	TTF.reset();
	TTFMon.reset();
	Engine::destroySingleton();
}

void App::printHelp() {
	if ( mShowHelp ) {
		Uint32 Top = 6;
		Uint32 Left = 6;

		if ( NULL == mHelpCache ) {
			String HT = "Keys List:\n";
			HT += "Escape: Quit from EEiv\n";
			HT += "ALT + RETURN or F: Toogle Fullscreen - Windowed\n";
			HT += String::fromUtf8( "F3 or º: Toggle Console\n"sv );
			HT += "F4 or S: Show/Hide Cursor\n";
			HT += "Mouse Wheel Up or PageUp: Go to Previous Image\n";
			HT += "Mouse Wheel Down or PageDown: Go to Next Image\n";
			HT += "Key I: Show/Hide Image Info\n";
			HT += "Key *: Scale image to screen\n";
			HT += "Key /: Reset Scale to 1\n";
			HT += "Key Z: Fit Image to Screen\n";
			HT += "Key + and -, or mouse middle press up or down: Zoom in and Zoom out image\n";
			HT += "Key X: Flip Image\n";
			HT += "Key C: Mirror Image\n";
			HT += "Key R: Rotate 90º Image and scale it to screen\n";
			HT += "Key T: Reset the position and the scale of the image\n";
			HT += "Key A: Change the texture filter\n";
			HT += "Key M: Change the screen size to the size of the current image.\n";
			HT += "Key E: Play SlideShow\n";
			HT += "Key D: Pause/Disable SlideShow\n";
			HT +=
				"Key K: Reload the image switching the mipmap state ( with or without mipmaps )\n";
			HT += "Key L: Lock zoom and image position when switching images\n";
			HT += "Key Left - Right - Top - Down or left mouse press: Move the image\n";
			HT += "Key F5: Reload the image\n";
			HT += "Key F6: Switch fade\n";
			HT += "Key F12: Take a screenshot\n";
			HT += "Key HOME: Go to the first screenshot on the folder\n";
			HT += "Key END: Go to the last screenshot on the folder";

			mHelpCache = eeNew( Text, ( HT, Fon, mConfig.AppFontSize ) );
		}

		mHelpCache->draw( Left, Height - Top - mHelpCache->getTextHeight() );
	}
}
