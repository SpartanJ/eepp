#include "eeiv.hpp"

#include <eepp/network/http.hpp>

using namespace EE::Network;
using namespace EE::Window;

bool App::isImage( const std::string& inputPath ) {
	std::string path( inputPath );
	if ( String::startsWith( path, "file://" ) )
		path.erase( 0, 7 );
	return !FileSystem::isDirectory( path ) && FileSystem::fileSize( path ) &&
		   Image::isImage( path );
}

bool App::isHttpUrl( const std::string& path ) {
	return String::startsWith( path, "http://" ) || String::startsWith( path, "https://" );
}

App::App( int argc, char* argv[] ) :
	mStorePath( Sys::getConfigPath( "eeiv" ) + FileSystem::getOSSlash() ),
	mTmpPath( mStorePath + "tmp" + FileSystem::getOSSlash() ) {
	if ( argc > 1 ) {
		mInitialPath = argv[1];
		if ( FileSystem::isRelativePath( mInitialPath ) && !isHttpUrl( mInitialPath ) ) {
			std::string cwd( FileSystem::getCurrentWorkingDirectory() );
			FileSystem::dirAddSlashAtEnd( cwd );
			mInitialPath.insert( 0, cwd );
		}
	}
}

App::~App() {
	if ( mUIApplication ) {
		saveConfig();
		Http::Pool::getGlobal().clear();
		Http::setThreadPool( nullptr );
	}
}

EE::Window::Window* App::getWindow() const {
	return mUIApplication ? mUIApplication->getWindow() : nullptr;
}

void App::setBackgroundColor( const Color& color ) {
	if ( getWindow() )
		getWindow()->setClearColor( color.toRGB() );
	if ( mMainLayout )
		mMainLayout->setBackgroundColor( color );
}

void App::loadConfig() {
	const std::string configPath( mStorePath + "eeiv.ini" );
	mIni.loadFromFile( configPath );
	if ( FileSystem::fileExists( configPath ) )
		mIni.readFile();
	else
		FileSystem::makeDir( mStorePath );

	mConfig.Fade = mIni.getValueB( "viewer", "fade", true );
	mConfig.LateLoading = mIni.getValueB( "viewer", "lateloading", true );
	mConfig.BlockWheelSpeed = mIni.getValueB( "viewer", "blockwheelspeed", true );
	mConfig.ShowInfo = mIni.getValueB( "viewer", "showinfo", true );
	mConfig.MaximizeAtStart = mIni.getValueB( "window", "maximizeatstart", true );
	mConfig.VSync = mIni.getValueB( "window", "vsync", false );
	const std::string frameRateLimit( mIni.getValue( "window", "frameratelimit" ) );
	mConfig.FrameLimit = frameRateLimit.empty()
							 ? mIni.getValueI( "window", "framelimit",
											   ContextSettings::FrameRateLimitScreenRefreshRate )
							 : mIni.getValueI( "window", "frameratelimit",
											   ContextSettings::FrameRateLimitScreenRefreshRate );
	mConfig.TransitionTime = mIni.getValueF( "viewer", "transitiontime", 200 );
	mConfig.ConsoleFontSize = mIni.getValueI( "viewer", "consolefontsize", 12 );
	mConfig.AppFontSize = mIni.getValueI( "viewer", "appfontsize", 12 );
	mConfig.DefaultImageZoom = mIni.getValueF( "viewer", "defaultimagezoom", 1 );
	mConfig.WheelBlockTime = mIni.getValueI( "viewer", "wheelblocktime", 200 );
}

void App::updateConfig() {
	if ( !getWindow() || !getWindow()->isOpen() )
		return;
	mIni.setValueI( "window", "width", getWindow()->getWidth() );
	mIni.setValueI( "window", "height", getWindow()->getHeight() );
	mConfig.MaximizeAtStart = getWindow()->isMaximized();
}

void App::saveConfig() {
	updateConfig();
	mIni.setValueI( "viewer", "fade", mConfig.Fade );
	mIni.setValueI( "viewer", "lateloading", mConfig.LateLoading );
	mIni.setValueI( "viewer", "blockwheelspeed", mConfig.BlockWheelSpeed );
	mIni.setValueI( "viewer", "showinfo", mConfig.ShowInfo );
	mIni.setValueI( "window", "vsync", mConfig.VSync );
	mIni.setValueI( "window", "frameratelimit", mConfig.FrameLimit );
	mIni.setValueI( "window", "maximizeatstart", mConfig.MaximizeAtStart );
	mIni.setValueF( "viewer", "transitiontime", mConfig.TransitionTime );
	mIni.setValueI( "viewer", "consolefontsize", mConfig.ConsoleFontSize );
	mIni.setValueI( "viewer", "appfontsize", mConfig.AppFontSize );
	mIni.setValueF( "viewer", "defaultimagezoom", mConfig.DefaultImageZoom );
	mIni.setValueI( "viewer", "wheelblocktime", mConfig.WheelBlockTime );
	mIni.writeFile();
}

bool App::init() {
	loadConfig();
	WindowSettings windowSettings = Engine::instance()->createWindowSettings( &mIni, "window" );
	ContextSettings contextSettings =
		Engine::instance()->createContextSettings( &mIni, "window", false );
	contextSettings.VSync = mConfig.VSync;
	contextSettings.FrameRateLimit = mConfig.FrameLimit;
	contextSettings.SharedGLContext = true;
	windowSettings.Title = "eeiv";
	windowSettings.Icon = Sys::getProcessPath() + "assets/eeiv.png";
	if ( !FileSystem::fileExists( windowSettings.Icon ) )
		windowSettings.Icon = Sys::getProcessPath() + "assets/icon/ee.png";

	mUIApplication = std::make_unique<UIApplication>( windowSettings, UIApplication::Settings(),
													  contextSettings );
	if ( !getWindow() || !getWindow()->isOpen() )
		return false;
	setBackgroundColor( Color::Black );

	mThreadPool = ThreadPool::createShared( 2 );
	mUIApplication->getUI()->setThreadPool( mThreadPool );
	Http::setThreadPool( mThreadPool );
	if ( mConfig.MaximizeAtStart )
		getWindow()->maximize();
	getWindow()->setCloseRequestCallback( [this]( EE::Window::Window* ) {
		saveConfig();
		return true;
	} );

	mMainLayout = mUIApplication->getUI()->loadLayoutFromString( R"xml(
		<RelativeLayout id="eeiv_layout" lw="mp" lh="mp" background-color="#000">
			<ImageViewer id="image_viewer" lw="mp" lh="mp" />
			<TextView id="key_help" lw="wc" lh="wc" lg="bottom|left" margin="8dp"
				padding="8dp" background-color="#141414DC" visible="false" enabled="false" />
		</RelativeLayout>
	)xml" );
	if ( !mMainLayout ) {
		Log::error( "eeiv: failed to create the main UI layout" );
		return false;
	}
	mImageViewer = mMainLayout->find<UIImageViewer>( "image_viewer" );
	mHelp = mMainLayout->find<UITextView>( "key_help" );
	if ( !mImageViewer || !mHelp ) {
		Log::error( "eeiv: one or more widgets are missing from the main UI layout" );
		return false;
	}

	mImageViewer->setMinScale( 0.0125f );
	mImageViewer->setMaxScale( 50.f );
	mImageViewer->setAutoFitOnSizeChange( true );
	mImageViewer->setUseNativeImageSize( true );
	mImageViewer->setDisplayOptions( mConfig.ShowInfo ? UIImageViewer::DisplayName |
															UIImageViewer::DisplayDimensions |
															UIImageViewer::DisplayGalleryPosition
													  : 0 );
	mImageViewer->on( Event::OnResourceLoaded, [this]( auto ) { syncLoadedImage(); } );

	mHelp->setText( "H: Toggle this help\n"
					"F3: Toggle console\n"
					"F11: Open UI inspector\n"
					"F12: Take screenshot\n"
					"Mouse wheel: Previous / next image\n"
					"Page Up: Previous image\n"
					"Page Down: Next image\n"
					"Home: First image\n"
					"End: Last image\n"
					"Arrow keys: Move image\n"
					"Keypad + / -: Zoom image\n"
					"Z: Fit image\n"
					"Keypad *: Fit image\n"
					"Keypad /: Reset zoom\n"
					"X: Flip image\n"
					"C: Mirror image\n"
					"R: Rotate image\n"
					"T: Reset view\n"
					"A: Toggle filtering\n"
					"E: Start slideshow\n"
					"D: Stop slideshow\n"
					"L: Lock view\n"
					"F5: Reload image\n"
					"F: Toggle fullscreen\n"
					"S: Toggle cursor\n"
					"I: Toggle image info\n"
					"Q: Quit\n"
					"Escape: Quit" );

	// Quake-mode consoles are root overlays by design. Keeping it outside the content layout also
	// preserves UIConsole's native show/hide animation.
	mConsole = UIConsole::NewOpt( nullptr, true, true, 1024000 );
	mConsole->setParent( mUIApplication->getUI()->getRoot() );
	mConsole->setQuakeMode( true );
	mConsole->setVisible( false );
	mConsole->setMaxLogLines( 1024000 );
	mConsole->setFontSize( mConfig.ConsoleFontSize );
	mConsoleCommands = std::make_unique<ConsoleCommands>( this, mConsole );
	registerConsoleCommands();
	registerKeyBindings();

	if ( !mInitialPath.empty() )
		loadDir( mInitialPath, true );
	if ( mFiles.empty() )
		mConsole->toggle();
	return true;
}

void App::registerConsoleCommands() {
	const auto add = [this]( const char* name, auto command ) {
		mConsole->addCommand( name, [this, command]( const auto& event ) {
			( mConsoleCommands.get()->*command )( event );
		} );
	};
	add( "loaddir", &ConsoleCommands::cmdLoadDir );
	add( "loadimg", &ConsoleCommands::cmdLoadImg );
	add( "setbackcolor", &ConsoleCommands::cmdSetBackColor );
	add( "setimgfade", &ConsoleCommands::cmdSetImgFade );
	add( "setlateloading", &ConsoleCommands::cmdSetLateLoading );
	add( "setblockwheel", &ConsoleCommands::cmdSetBlockWheel );
	add( "moveto", &ConsoleCommands::cmdMoveTo );
	add( "batchimgscale", &ConsoleCommands::cmdBatchImgScale );
	add( "batchimgchangeformat", &ConsoleCommands::cmdBatchImgChangeFormat );
	add( "batchimgthumbnail", &ConsoleCommands::cmdBatchImgThumbnail );
	add( "imgchangeformat", &ConsoleCommands::cmdImgChangeFormat );
	add( "imgresize", &ConsoleCommands::cmdImgResize );
	add( "imgscale", &ConsoleCommands::cmdImgScale );
	add( "imgthumbnail", &ConsoleCommands::cmdImgThumbnail );
	add( "imgcentercrop", &ConsoleCommands::cmdImgCenterCrop );
	add( "slideshow", &ConsoleCommands::cmdSlideShow );
	add( "setzoom", &ConsoleCommands::cmdSetZoom );
}

void App::registerKeyBindings() {
	mUIApplication->getUI()->on( Event::KeyUp, [this]( const Event* event ) {
		const auto* keyEvent = event->asKeyEvent();
		const Keycode key = keyEvent->getKeyCode();
		if ( key == KEY_F11 ) {
			UIWidgetInspector::create( mUIApplication->getUI() );
			return;
		}
		if ( key == KEY_F3 ) {
			mHelp->setVisible( false );
			mConsole->toggle();
			return;
		}
		if ( mConsole->isActive() )
			return;
		if ( key == KEY_ESCAPE || key == KEY_Q )
			getWindow()->close();
		else if ( key == KEY_TAB && keyEvent->getMod() & KEYMOD_ALT )
			getWindow()->minimize();
		else if ( key == KEY_F || ( key == KEY_RETURN && keyEvent->getMod() & KEYMOD_ALT ) )
			getWindow()->toggleFullscreen();
		else if ( key == KEY_F12 )
			getWindow()->takeScreenshot();
		else if ( key == KEY_H )
			toggleHelp();
		else if ( key == KEY_S || key == KEY_F4 ) {
			mCursorVisible = !mCursorVisible;
			getWindow()->getCursorManager()->setVisible( mCursorVisible );
		} else if ( ( key == KEY_V && keyEvent->getMod() & KEYMOD_CTRL ) ||
					( key == KEY_INSERT && keyEvent->getMod() & KEYMOD_SHIFT ) ) {
			const std::string path( getWindow()->getClipboard()->getText() );
			if ( isImage( path ) || isHttpUrl( path ) || FileSystem::isDirectory( path ) )
				loadDir( path );
		} else if ( key == KEY_HOME && !mFiles.empty() )
			fastLoadImage( 0 );
		else if ( key == KEY_END && !mFiles.empty() )
			fastLoadImage( mFiles.size() - 1 );
		else if ( key == KEY_PAGEUP && mCurImg > 0 ) {
			mSlideShow = false;
			fastLoadImage( mCurImg - 1 );
		} else if ( key == KEY_PAGEDOWN && mCurImg + 1 < static_cast<Int32>( mFiles.size() ) ) {
			mSlideShow = false;
			fastLoadImage( mCurImg + 1 );
		} else if ( key == KEY_I ) {
			mConfig.ShowInfo = !mConfig.ShowInfo;
			mImageViewer->setDisplayOptions(
				mConfig.ShowInfo ? UIImageViewer::DisplayName | UIImageViewer::DisplayDimensions |
									   UIImageViewer::DisplayGalleryPosition
								 : 0 );
		} else if ( key == KEY_KP_MULTIPLY || key == KEY_Z ) {
			fitImage();
		} else if ( key == KEY_KP_DIVIDE ) {
			setImgScale( mConfig.DefaultImageZoom );
		} else if ( key == KEY_N && mImageViewer->getImage()->getDrawable() ) {
			const Sizef size( mImageViewer->getImage()->getDrawable()->getPixelsSize() );
			getWindow()->setSize( size.getWidth(), size.getHeight() );
		} else if ( key == KEY_A ) {
			if ( Sprite* sprite = getImageSprite() ) {
				mTextureFilter = mTextureFilter == Texture::Filter::Linear
									 ? Texture::Filter::Nearest
									 : Texture::Filter::Linear;
				for ( size_t i = 0; i < sprite->getNumFrames(); ++i ) {
					Texture* texture = sprite->getTextureRegion( i )->getTexture().get();
					if ( texture )
						texture->setFilter( mTextureFilter );
				}
			}
		} else if ( key == KEY_L ) {
			mImageViewer->setPreserveImageView( !mImageViewer->getPreserveImageView() );
		} else if ( key == KEY_M ) {
			fitImage();
			if ( getWindow()->isMaximized() && mImageViewer->getImage()->getDrawable() ) {
				const Sizef size( mImageViewer->getImage()->getDrawable()->getPixelsSize() );
				getWindow()->setSize( size.getWidth(), size.getHeight() );
			}
		} else if ( key == KEY_K ) {
			if ( Sprite* sprite = getImageSprite() ) {
				if ( TextureRegion* region = sprite->getCurrentTextureRegion() ) {
					if ( Texture* texture = region->getTexture().get() ) {
						texture->setMipmap( !texture->getMipmap() );
						texture->reload();
					}
				}
			}
		} else if ( key == KEY_F5 && !getFilePath().empty() ) {
			loadImagePath( getFilePath() );
		} else if ( key == KEY_E ) {
			createSlideShow( mSlideTime );
		} else if ( key == KEY_D ) {
			mSlideShow = false;
		}
	} );
	mUIApplication->getUI()->on( Event::KeyDown, [this]( const Event* event ) {
		if ( mConsole->isActive() || !mImageViewer->hasImage() )
			return;
		const Keycode key = event->asKeyEvent()->getKeyCode();
		Vector2f position( mImageViewer->getImage()->getPixelsPosition() );
		if ( key == KEY_LEFT )
			position.x += 20.f;
		else if ( key == KEY_RIGHT )
			position.x -= 20.f;
		else if ( key == KEY_UP )
			position.y += 20.f;
		else if ( key == KEY_DOWN )
			position.y -= 20.f;
		else
			return;
		mImageViewer->getImage()->setPixelsPosition( position );
	} );
}

void App::toggleHelp() {
	mHelp->setVisible( !mHelp->isVisible() );
	if ( mHelp->isVisible() )
		mHelp->toFront();
}

void App::fitImage() {
	if ( mImageViewer && mImageViewer->hasImage() )
		mImageViewer->resetImageView();
}

Sprite* App::getImageSprite() const {
	if ( !mImageViewer || !mImageViewer->getImage() || !mImageViewer->getImage()->getDrawable() )
		return nullptr;
	return static_cast<Sprite*>( mImageViewer->getImage()->getDrawable().get() );
}

void App::getImages() {
	mFiles.clear();
	if ( !FileSystem::isDirectory( mFilePath ) )
		return;
	for ( const std::string& file : FileSystem::filesGetInPath( mFilePath, true ) ) {
		if ( isImage( mFilePath + file ) )
			mFiles.push_back( file );
	}
}

void App::loadDir( const std::string& inputPath, const bool& getImagesList ) {
	std::string path( inputPath );
	String::replaceAll( path, "%20", " " );
	if ( String::startsWith( path, "file://" ) )
		path.erase( 0, 7 );

	if ( isHttpUrl( path ) ) {
		loadImageUrl( path );
		return;
	}

	if ( FileSystem::isDirectory( path ) ) {
		mFilePath = path;
		FileSystem::dirAddSlashAtEnd( mFilePath );
		mFile.clear();
	} else {
		if ( !isImage( path ) )
			return;
		mFilePath = FileSystem::fileRemoveFileName( path );
		FileSystem::dirAddSlashAtEnd( mFilePath );
		mFile = FileSystem::fileNameFromPath( path );
	}

	if ( getImagesList )
		getImages();
	if ( mFiles.empty() )
		return;

	mCurImg = 0;
	if ( !mFile.empty() ) {
		auto it = std::find( mFiles.begin(), mFiles.end(), mFile );
		if ( it != mFiles.end() )
			mCurImg = std::distance( mFiles.begin(), it );
	}
	fastLoadImage( mCurImg );
}

void App::loadImagePath( const std::string& path, bool loadGallery ) {
	Log::info( "eeiv: loading image: %s", path.c_str() );
	mImageViewer->loadImageAsync( path, false, loadGallery );
}

void App::loadImageUrl( const std::string& url ) {
	FileSystem::makeDir( mTmpPath );
	const std::string outputPath( mTmpPath + "download" );
	mConsole->pushText( "Downloading \"" + url + "\"..." );
	Http::getAsync(
		[this, outputPath]( const Http&, Http::Request&, Http::Response& response ) {
			if ( !response.isOK() ) {
				mConsole->runOnMainThread( [this, status = response.getStatus()] {
					mConsole->pushText( "Couldn't download the image (HTTP %d).", status );
				} );
				return;
			}
			const std::string& body = response.getBody();
			if ( !FileSystem::fileWrite( outputPath, reinterpret_cast<const Uint8*>( body.data() ),
										 body.size() ) ) {
				mConsole->runOnMainThread( [this] {
					mConsole->pushText( "Couldn't write the downloaded image to disk." );
				} );
				return;
			}
			mConsole->runOnMainThread( [this, outputPath] { loadDir( outputPath ); } );
		},
		URI( url ) );
}

void App::fastLoadImage( const Uint32& imageNum ) {
	if ( imageNum >= mFiles.size() )
		return;
	mCurImg = imageNum;
	mFile = mFiles[mCurImg];
	loadImagePath( mFilePath + mFile );
}

void App::syncLoadedImage() {
	const std::string path( mImageViewer->getImagePath() );
	if ( path.empty() )
		return;
	mFilePath = FileSystem::fileRemoveFileName( path );
	FileSystem::dirAddSlashAtEnd( mFilePath );
	mFile = FileSystem::fileNameFromPath( path );
	auto it = std::find( mFiles.begin(), mFiles.end(), mFile );
	if ( it != mFiles.end() )
		mCurImg = std::distance( mFiles.begin(), it );
	if ( Sprite* sprite = getImageSprite() ) {
		for ( size_t i = 0; i < sprite->getNumFrames(); ++i ) {
			if ( Texture* texture = sprite->getTextureRegion( i )->getTexture().get() )
				texture->setFilter( mTextureFilter );
		}
	}
	getWindow()->setTitle( "eeiv - " + mFile );
	mImageViewer->getImage()->setFocus();
	Log::info( "eeiv: displaying image %d/%d: %s", mCurImg + 1, mFiles.size(), path.c_str() );
}

void App::setImgScale( Float scale ) {
	if ( mImageViewer && mImageViewer->getImage() )
		mImageViewer->getImage()->setScale( scale );
}

void App::createSlideShow( Uint32 time ) {
	mSlideTime = eemax<Uint32>( 250, time );
	mSlideTicks = Sys::getTicks();
	mSlideShow = true;
}

void App::updateSlideShow() {
	if ( !mSlideShow || Sys::getTicks() - mSlideTicks < mSlideTime )
		return;
	mSlideTicks = Sys::getTicks();
	if ( mCurImg + 1 < static_cast<Int32>( mFiles.size() ) )
		fastLoadImage( mCurImg + 1 );
	else
		mSlideShow = false;
}

void App::process() {
	if ( !init() )
		return;
	getWindow()->runMainLoop( [this] {
		getWindow()->getInput()->update();
		updateSlideShow();
		SceneManager::instance()->update();
		if ( mUIApplication->getUI()->invalidated() ) {
			getWindow()->clear();
			SceneManager::instance()->draw();
			getWindow()->display();
		} else {
			getWindow()->getInput()->waitEvent(
				Milliseconds( getWindow()->hasFocus() ? 16 : 100 ) );
		}
	} );
}

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	App app( argc, argv );
	app.process();
	return EXIT_SUCCESS;
}
