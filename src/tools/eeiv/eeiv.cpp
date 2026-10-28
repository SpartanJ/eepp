#include "eeiv.hpp"

#include <eepp/network/http.hpp>
#include <eepp/ui/tools/keybindingsconfig.hpp>
#include <eepp/ui/tools/uifontpickerdialog.hpp>
#include <eepp/ui/tools/uikeybindingseditor.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>
#include <eepp/ui/uifiledialog.hpp>
#include <eepp/ui/uiwindow.hpp>

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
	Log::instance()->setKeepLog( true );
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
		Http::Pool::getGlobal().clear();
		Http::setThreadPool( nullptr );
	}
}

EE::Window::Window* App::getWindow() const {
	return mUIApplication ? mUIApplication->getWindow() : nullptr;
}

void App::showWelcomeScreen() {
	if ( !mWelcomeScreen )
		return;
	mWelcomeScreen->setVisible( true );
	mWelcomeScreen->toFront();
	mWelcomeScreen->setFocus();
}

void App::hideWelcomeScreen() {
	if ( mWelcomeScreen )
		mWelcomeScreen->setVisible( false );
}

void App::openFileDialog() {
	const auto& extensions = Image::getImageExtensionsSupported();
	std::string fileTypes;
	fileTypes.reserve( extensions.size() * 6 );
	for ( const auto& ext : extensions ) {
		if ( !fileTypes.empty() )
			fileTypes += ";";
		fileTypes += "*." + ext;
	}

	UIFileDialog* dialog = UIFileDialog::New(
		UIFileDialog::DefaultFlags, fileTypes,
		mFilePath.empty() ? FileSystem::getCurrentWorkingDirectory() : mFilePath );
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( "Open Image" );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->on( Event::OpenFile, [this]( const Event* event ) {
		const std::string path( event->getNode()->asType<UIFileDialog>()->getFullPath() );
		if ( isImage( path ) || isHttpUrl( path ) || FileSystem::isDirectory( path ) )
			loadDir( path );
	} );
	dialog->on( Event::OnWindowClose, [this]( const Event* ) {
		if ( mWelcomeScreen && mWelcomeScreen->isVisible() )
			mWelcomeScreen->setFocus();
	} );
	dialog->center();
	dialog->show();
}

void App::openFolderDialog() {
	UIFileDialog* dialog = UIFileDialog::New(
		UIFileDialog::DefaultFlags | UIFileDialog::AllowFolderSelect |
			UIFileDialog::ShowOnlyFolders,
		"*", mFilePath.empty() ? FileSystem::getCurrentWorkingDirectory() : mFilePath );
	dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
	dialog->setTitle( "Open Folder" );
	dialog->setCloseShortcut( KEY_ESCAPE );
	dialog->on( Event::OpenFile, [this]( const Event* event ) {
		const std::string path( event->getNode()->asType<UIFileDialog>()->getFullPath() );
		if ( FileSystem::isDirectory( path ) )
			loadDir( path );
	} );
	dialog->on( Event::OnWindowClose, [this]( const Event* ) {
		if ( mWelcomeScreen && mWelcomeScreen->isVisible() )
			mWelcomeScreen->setFocus();
	} );
	dialog->center();
	dialog->show();
}

void App::showSettings() {
	if ( !mSettingsWindow )
		mSettingsWindow = createSettingsWindow();
	else {
		mSettingsWindow->show();
		mSettingsWindow->toFront();
		mSettingsWindow->find<UISettingsPanel>( "settings_panel_content" )->focusSearch();
	}
}

UIWindow* App::createSettingsWindow() {
	auto* scene = mUIApplication->getUI();
	UIWindow::StyleConfig style{ UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL };
	auto* window = UIWindow::NewOpt( UIWindow::SIMPLE_LAYOUT, style );
	window->setId( "settings_panel" );
	window->setTitle( "Settings" );
	const auto sceneSize = scene->getPixelsSize();
	window->setPixelsSize( { eeclamp( sceneSize.getWidth() * 0.82f, 720.f, 1200.f ),
							 eeclamp( sceneSize.getHeight() * 0.82f, 520.f, 850.f ) } );
	window->setMinWindowSize( 640, 440 );

	auto* panel = UISettingsPanel::New( window->getContainer() );
	panel->setId( "settings_panel_content" );
	panel->addCategory( "general.keybindings", "General", "Keyboard Shortcuts" );
	panel->addCustomWidget(
		{ "keybindingsEditor", "general.keybindings", "Keyboard Shortcuts", {} },
		[this]( UIWidget* parent ) { return createKeybindingsEditor( parent ); }, true );
	panel->addCategory( "appearance.theme", "Appearance", "Theme" );
	panel->addChoice(
		{ "uiColorScheme", "appearance.theme", "UI Prefers Color Scheme",
		  "Choose whether the interface follows the system, light, or dark appearance." },
		{ "System", "Light", "Dark" },
		[this] {
			return mConfig.ColorScheme == ColorSchemeExtPreference::System	? size_t{ 0 }
				   : mConfig.ColorScheme == ColorSchemeExtPreference::Light ? size_t{ 1 }
																			: size_t{ 2 };
		},
		[this, scene]( size_t selected ) {
			static constexpr ColorSchemeExtPreference values[] = { ColorSchemeExtPreference::System,
																   ColorSchemeExtPreference::Light,
																   ColorSchemeExtPreference::Dark };
			mConfig.ColorScheme = values[std::min( selected, size_t{ 2 } )];
			scene->setColorSchemePreference( mConfig.ColorScheme );
			scene->updateWindowTitleBarColor();
			saveConfig();
		} );
	panel->addCategory( "appearance.fonts", "Appearance", "Fonts & Scale" );
	panel->addAction( { "uiFont", "appearance.fonts", "UI Font & Size...",
						"Choose the proportional font used by the interface." },
					  "Choose Font...", [this] { openFontPicker(); } );
	panel->addAction( { "fallbackFont", "appearance.fonts", "Fallback Font...",
						"Choose the font used for missing glyphs." },
					  "Choose Font...", [this] { openFontPicker( true ); } );
	panel->addFloat(
		{ "uiFontSize", "appearance.fonts", "UI Font Size",
		  "Set the font size used by the application UI." },
		6, 72, 0.5, [this] { return mConfig.AppFontSize; },
		[this]( double value ) {
			setUIFontSize( value );
			saveConfig();
		} );
	panel->addInteger(
		{ "consoleFontSize", "appearance.fonts", "Console Font Size",
		  "Set the font size used by the console." },
		6, 72, [this] { return mConfig.ConsoleFontSize; },
		[this]( int value ) {
			mConfig.ConsoleFontSize = value;
			mConsole->setFontSize( value );
			saveConfig();
		} );
	panel->addFloat(
		{ "uiScaleFactor", "appearance.fonts", "UI Scale Factor",
		  "Scale the complete user interface. Restart required." },
		1, 6, 0.1,
		[this] { return mConfig.UIScale > 0 ? mConfig.UIScale : PixelDensity::getPixelDensity(); },
		[this]( double value ) {
			mConfig.UIScale = value;
			saveConfig();
		} );
	panel->addChoice(
		{ "fontHinting", "appearance.fonts", "Font Hinting",
		  "Control glyph alignment to the pixel grid." },
		{ "None", "Slight", "Full" }, [this] { return static_cast<size_t>( mConfig.Hinting ); },
		[this, scene]( size_t selected ) {
			mConfig.Hinting = static_cast<FontHinting>( std::min( selected, size_t{ 2 } ) );
			scene->setFontRendering( mConfig.Hinting, mConfig.Antialiasing );
			saveConfig();
		} );
	panel->addChoice(
		{ "fontAntialiasing", "appearance.fonts", "Font Anti-Aliasing",
		  "Choose how glyph edges are smoothed." },
		{ "None", "Grayscale", "Subpixel" },
		[this] { return static_cast<size_t>( mConfig.Antialiasing ); },
		[this, scene]( size_t selected ) {
			mConfig.Antialiasing =
				static_cast<FontAntialiasing>( std::min( selected, size_t{ 2 } ) );
			scene->setFontRendering( mConfig.Hinting, mConfig.Antialiasing );
			saveConfig();
		} );

	panel->addCategory( "window.renderer", "Window", "Renderer" );
	panel->addBool( { "vsync", "window.renderer", "VSync",
					  "Synchronize rendering with the display. Restart required." },
					&mConfig.VSync, [this]( bool ) { saveConfig(); } );
	panel->addEditableChoice(
		{ "frameRateLimit", "window.renderer", "Frame Rate Limit",
		  "Limit rendered frames per second, follow the monitor refresh rate, or disable the "
		  "limit." },
		{ "Monitor Refresh Rate", "Unlimited", "30", "60", "75", "120", "144", "165", "240" },
		[this] {
			return mConfig.FrameLimit == ContextSettings::FrameRateLimitScreenRefreshRate
					   ? String( "Monitor Refresh Rate" )
				   : mConfig.FrameLimit == 0 ? String( "Unlimited" )
											 : String( String::toString( mConfig.FrameLimit ) );
		},
		[this]( const String& selection ) {
			Uint32 value;
			if ( selection == "Monitor Refresh Rate" )
				value = ContextSettings::FrameRateLimitScreenRefreshRate;
			else if ( selection == "Unlimited" )
				value = 0;
			else if ( !String::fromString( value, selection ) || value > 1000 )
				return false;
			mConfig.FrameLimit = value;
			getWindow()->setFrameRateLimit( mConfig.BenchmarkMode ? 0 : value );
			saveConfig();
			return true;
		} );
	auto availableVersions = Renderer::getAvailableGraphicsLibraryVersions();
	SmallVector<GraphicsLibraryVersion, 6> rendererVersions{ GLv_default };
	rendererVersions.insert( rendererVersions.end(), availableVersions.begin(),
							 availableVersions.end() );
	std::vector<String> rendererNames{ "Automatic" };
	for ( auto version : availableVersions )
		rendererNames.emplace_back( Renderer::graphicsLibraryVersionToString( version ) );
	panel->addChoice(
		{ "rendererVersion", "window.renderer", "Renderer Version",
		  "Select the graphics API. Restart required." },
		std::move( rendererNames ),
		[this, rendererVersions] {
			auto found = std::find( rendererVersions.begin(), rendererVersions.end(),
									mConfig.RendererVersion );
			return found == rendererVersions.end()
					   ? size_t{ 0 }
					   : static_cast<size_t>( found - rendererVersions.begin() );
		},
		[this, rendererVersions]( size_t selected ) {
			mConfig.RendererVersion =
				rendererVersions[std::min( selected, rendererVersions.size() - 1 )];
			saveConfig();
		} );
	panel->addChoice(
		{ "multisamples", "window.renderer", "Multisample Anti-Aliasing Level",
		  "Set renderer multisampling. Restart required." },
		{ "0", "2", "4", "8", "16" },
		[this] {
			static constexpr Uint32 values[] = { 0, 2, 4, 8, 16 };
			auto found =
				std::find( std::begin( values ), std::end( values ), mConfig.Multisamples );
			return found == std::end( values )
					   ? size_t{ 0 }
					   : static_cast<size_t>( found - std::begin( values ) );
		},
		[this]( size_t selected ) {
			static constexpr Uint32 values[] = { 0, 2, 4, 8, 16 };
			mConfig.Multisamples = values[std::min( selected, size_t{ 4 } )];
			saveConfig();
		} );
	panel->addBool( { "benchmarkMode", "window.renderer", "Benchmark Mode",
					  "Render continuously with no frame rate limit to measure performance." },
					&mConfig.BenchmarkMode, [this]( bool enabled ) {
						getWindow()->setFrameRateLimit( enabled ? 0 : mConfig.FrameLimit );
						saveConfig();
					} );

	panel->addCategory( "viewer.display", "Image Viewer", "Display" );
	panel->addBool(
		{ "showInfo", "viewer.display", "Show Image Info",
		  "Show the image name, dimensions, and gallery position." },
		[this] { return mConfig.ShowInfo; },
		[this]( bool value ) {
			setShowInfo( value );
			saveConfig();
		} );
	panel->addChoice(
		{ "imageFilter", "viewer.display", "Image Filtering",
		  "Choose sharp pixels or smooth scaling when viewing images." },
		{ "Nearest", "Linear" },
		[this] { return mTextureFilter == Texture::Filter::Linear ? size_t{ 1 } : size_t{ 0 }; },
		[this]( size_t selected ) {
			setTextureFilter( selected == 1 ? Texture::Filter::Linear : Texture::Filter::Nearest );
			saveConfig();
		} );
	panel->addFloat(
		{ "defaultImageZoom", "viewer.display", "Default Image Zoom",
		  "Set the zoom factor used when resetting the image zoom." },
		0.0125, 50, 0.1, [this] { return mConfig.DefaultImageZoom; },
		[this]( double value ) {
			mConfig.DefaultImageZoom = value;
			saveConfig();
		} );

	panel->build();
	window->on( Event::OnWindowReady, [panel]( const Event* ) {
		panel->runOnMainThread( [panel] { panel->focusSearch(); } );
	} );
	// eeiv handles viewer shortcuts on key release, so dismiss settings on release too.
	window->on( Event::KeyUp, [window]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE )
			window->closeWindow();
	} );
	window->on( Event::OnWindowClose, [this, scene]( const Event* ) {
		mSettingsWindow = nullptr;
		if ( !scene->isClosing() ) {
			saveConfig();
			if ( mWelcomeScreen->isVisible() )
				mWelcomeScreen->setFocus();
			else
				mImageViewer->setFocus();
		}
	} );
	window->center();
	window->showWhenReady();
	return window;
}

void App::openFontPicker( bool fallbackFont ) {
	const Uint32 flags =
		UIFontPickerDialog::ShowStyle | ( fallbackFont ? 0 : UIFontPickerDialog::ShowSize );
	auto* dialog = UIFontPickerDialog::New( flags );
	dialog->setTitle( "Select Font" );
	dialog->setCloseShortcut( KEY_ESCAPE );
	std::string path = fallbackFont ? mConfig.FallbackFontPath : mConfig.UIFontPath;
	if ( path.empty() ) {
		if ( fallbackFont ) {
			const auto& fallbacks =
				mUIApplication->getUI()->getResourceScope()->getFontService().getFallbackFonts();
			if ( !fallbacks.empty() )
				path = FontTrueType::filePathOf( fallbacks.front().get() );
		} else {
			path = FontTrueType::filePathOf(
				mUIApplication->getUI()->getUIThemeManager()->getDefaultFont() );
		}
	}
	if ( !path.empty() )
		dialog->setSelectedFont( path );
	if ( !fallbackFont ) {
		auto selection = dialog->getSelection();
		selection.size = static_cast<Uint32>( mConfig.AppFontSize );
		dialog->setSelection( selection );
	}
	dialog->setOnFontPicked( [this, fallbackFont]( const UIFontSelection& selection ) {
		if ( selection.font.path.empty() )
			return;
		auto* scene = mUIApplication->getUI();
		auto& scope = *scene->getResourceScope();
		auto font =
			FontTrueType::New( fallbackFont ? "eeiv-fallback-font" : "eeiv-ui-font", scope );
		if ( !font->loadFromFile( selection.font.path ) )
			return;
		if ( fallbackFont ) {
			mConfig.FallbackFontPath = selection.font.path;
			setFallbackFont( std::move( font ) );
		} else {
			mConfig.UIFontPath = selection.font.path;
			mConfig.AppFontSize = selection.size;
			scene->applyDefaultFont( font.get(), PixelDensity::dpToPx( mConfig.AppFontSize ) );
		}
		saveConfig();
	} );
	dialog->center();
	dialog->show();
}

void App::setUIFontSize( Float size ) {
	mConfig.AppFontSize = size;
	auto* scene = mUIApplication->getUI();
	scene->applyDefaultFont( nullptr, PixelDensity::dpToPx( size ) );
}

void App::setFallbackFont( FontPtr font ) {
	// Base UI fonts live in the default scope; user-selected fonts live in the scene scope.
	for ( FontService* service :
		  { &defaultResourceScope().getFontService(),
			&mUIApplication->getUI()->getResourceScope()->getFontService() } ) {
		while ( !service->getFallbackFonts().empty() )
			service->removeFallbackFont( service->getFallbackFonts().back().get() );
		service->addFallbackFont( font );
	}
	mUIApplication->getUI()->invalidateDraw();
}

void App::setShowInfo( bool show ) {
	mConfig.ShowInfo = show;
	mImageViewer->setDisplayOptions( show ? UIImageViewer::DisplayName |
												UIImageViewer::DisplayDimensions |
												UIImageViewer::DisplayGalleryPosition
										  : 0 );
}

void App::setTextureFilter( Texture::Filter filter ) {
	mTextureFilter = filter;
	if ( Sprite* sprite = getImageSprite() ) {
		for ( size_t i = 0; i < sprite->getNumFrames(); ++i ) {
			if ( Texture* texture = sprite->getTextureRegion( i )->getTexture().get() )
				texture->setFilter( filter );
		}
		mImageViewer->invalidateDraw();
	}
}

bool App::isViewerFocused() const {
	for ( auto* node = mUIApplication->getUI()->getEventDispatcher()->getFocusNode(); node;
		  node = node->getParent() ) {
		if ( node->isType( UI_TYPE_WINDOW ) )
			return false;
	}
	return true;
}

void App::processDroppedFiles() {
	if ( mPathsToLoad.empty() )
		return;
	std::string path( std::move( mPathsToLoad.back() ) );
	mPathsToLoad.clear();
	if ( isImage( path ) || isHttpUrl( path ) || FileSystem::isDirectory( path ) )
		loadDir( path );
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
	IniFile state( mStorePath + "state.cfg" );
	mConfig.ShowInfo =
		state.getValueB( "viewer", "showinfo", mIni.getValueB( "viewer", "showinfo", true ) );
	mTextureFilter = state.getValue( "viewer", "filter", "nearest" ) == "linear"
						 ? Texture::Filter::Linear
						 : Texture::Filter::Nearest;
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
	mConfig.AppFontSize = mIni.getValueF( "viewer", "appfontsize", 12 );
	mConfig.UIFontPath = mIni.getValue( "ui", "font" );
	mConfig.FallbackFontPath = mIni.getValue( "ui", "fallbackfont" );
	mConfig.UIScale = mIni.getValueF( "ui", "scale", 0 );
	mConfig.Hinting = Font::fontHintingFromString( mIni.getValue( "ui", "fonthinting", "full" ) );
	mConfig.Antialiasing =
		Font::fontAntialiasingFromString( mIni.getValue( "ui", "fontantialiasing", "grayscale" ) );
	mConfig.ColorScheme =
		ColorSchemePreferences::fromStringExt( mIni.getValue( "ui", "colorscheme", "system" ) );
	mConfig.RendererVersion =
		Renderer::glVersionFromString( mIni.getValue( "window", "glversion", "0" ) );
	mConfig.Multisamples = mIni.getValueU( "window", "multisamples", 0 );
	mConfig.BenchmarkMode = mIni.getValueB( "window", "benchmarkmode", false );
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
	mIni.setValueI( "window", "vsync", mConfig.VSync );
	mIni.setValueI( "window", "frameratelimit", mConfig.FrameLimit );
	mIni.setValueI( "window", "maximizeatstart", mConfig.MaximizeAtStart );
	mIni.setValueF( "viewer", "transitiontime", mConfig.TransitionTime );
	mIni.setValueI( "viewer", "consolefontsize", mConfig.ConsoleFontSize );
	mIni.setValueF( "viewer", "appfontsize", mConfig.AppFontSize );
	mIni.setValue( "ui", "font", mConfig.UIFontPath );
	mIni.setValue( "ui", "fallbackfont", mConfig.FallbackFontPath );
	mIni.setValueF( "ui", "scale", mConfig.UIScale );
	mIni.setValue( "ui", "fonthinting", Font::fontHintingToString( mConfig.Hinting ) );
	mIni.setValue( "ui", "fontantialiasing",
				   Font::fontAntialiasingToString( mConfig.Antialiasing ) );
	mIni.setValue( "ui", "colorscheme", ColorSchemePreferences::toString( mConfig.ColorScheme ) );
	mIni.setValue( "window", "glversion",
				   mConfig.RendererVersion == GLv_default
					   ? "0"
					   : Renderer::graphicsLibraryVersionToString( mConfig.RendererVersion ) );
	mIni.setValueU( "window", "multisamples", mConfig.Multisamples );
	mIni.setValueB( "window", "benchmarkmode", mConfig.BenchmarkMode );
	mIni.setValueF( "viewer", "defaultimagezoom", mConfig.DefaultImageZoom );
	mIni.setValueI( "viewer", "wheelblocktime", mConfig.WheelBlockTime );
	mIni.writeFile();

	IniFile state( mStorePath + "state.cfg" );
	state.setValueB( "viewer", "showinfo", mConfig.ShowInfo );
	state.setValue( "viewer", "filter",
					mTextureFilter == Texture::Filter::Linear ? "linear" : "nearest" );
	state.writeFile();
}

bool App::init() {
	loadConfig();
	WindowSettings windowSettings = Engine::instance()->createWindowSettings( &mIni, "window" );
	ContextSettings contextSettings =
		Engine::instance()->createContextSettings( &mIni, "window", false );
	contextSettings.VSync = mConfig.VSync;
	contextSettings.FrameRateLimit = mConfig.BenchmarkMode ? 0 : mConfig.FrameLimit;
	contextSettings.SharedGLContext = true;
	windowSettings.Title = "eeiv";
#if EE_PLATFORM == EE_PLATFORM_MACOS
	const char* windowIcon = "eeiv-macos.png";
	const char* fallbackIcon = "ee-macos.png";
#else
	const char* windowIcon = "eeiv.png";
	const char* fallbackIcon = "ee.png";
#endif
	windowSettings.Icon = Sys::getResourcePath( "eeiv" ) + "assets/icon/" + windowIcon;
	if ( !FileSystem::fileExists( windowSettings.Icon ) )
		windowSettings.Icon = Sys::getResourcePath( "eeiv" ) + "assets/icon/" + fallbackIcon;
#if EE_PLATFORM == EE_PLATFORM_MACOS
	if ( !FileSystem::fileExists( windowSettings.Icon ) )
		windowSettings.Icon = Sys::getResourcePath( "eeiv" ) + "assets/icon/ee.png";
#endif

	FontTrueTypePtr uiFont;
	if ( !mConfig.UIFontPath.empty() ) {
		uiFont = FontTrueType::New( "eeiv-ui-font" );
		if ( !uiFont->loadFromFile( mConfig.UIFontPath ) )
			uiFont.reset();
	}
	FontTrueTypePtr fallbackFont;
	if ( !mConfig.FallbackFontPath.empty() ) {
		fallbackFont = FontTrueType::New( "eeiv-fallback-font" );
		if ( !fallbackFont->loadFromFile( mConfig.FallbackFontPath ) )
			fallbackFont.reset();
	}
	UIApplication::Settings appSettings;
	appSettings.basePath = Sys::getResourcePath( "eeiv" );
	appSettings.baseFont = uiFont.get();
	appSettings.fallbackFont = fallbackFont.get();
	appSettings.fontHinting = mConfig.Hinting;
	appSettings.fontAntialiasing = mConfig.Antialiasing;
	if ( mConfig.UIScale > 0 )
		appSettings.pixelDensity = mConfig.UIScale;
	mUIApplication =
		std::make_unique<UIApplication>( windowSettings, appSettings, contextSettings );
	if ( !getWindow() || !getWindow()->isOpen() )
		return false;

	mUIApplication->getUI()->setColorSchemePreference( mConfig.ColorScheme );
	mUIApplication->getUI()->updateWindowTitleBarColor();
	setUIFontSize( mConfig.AppFontSize );
	if ( fallbackFont )
		setFallbackFont( std::move( fallbackFont ) );
	else if ( auto fallback =
				  mUIApplication->getUI()->getResourceScope()->findFont( "DroidSansFallbackFull" ) )
		setFallbackFont( std::move( fallback ) );
	setBackgroundColor( Color::Black );

	ResourceScope& resourceScope = *mUIApplication->getUI()->getResourceScope();
	FontTrueTypePtr remixIconFont =
		FontTrueType::New( "icon", "assets/fonts/remixicon.ttf", resourceScope );
	FontTrueTypePtr noniconsFont =
		FontTrueType::New( "nonicons", "assets/fonts/nonicons.ttf", resourceScope );
	FontTrueTypePtr codIconFont =
		FontTrueType::New( "codicon", "assets/fonts/codicon.ttf", resourceScope );
	mUIApplication->getUI()->getUIIconThemeManager()->setCurrentTheme(
		IconManager::init( "icons", remixIconFont.get(), noniconsFont.get(), codIconFont.get() ) );

	mThreadPool = ThreadPool::createShared( 4 );
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
	setShowInfo( mConfig.ShowInfo );
	mImageViewer->on( Event::OnResourceLoaded, [this]( auto ) { syncLoadedImage(); } );

	// Quake-mode consoles are root overlays by design. Keeping it outside the content layout also
	// preserves UIConsole's native show/hide animation.
	mConsole = UIConsole::NewOpt( nullptr, true, true, 1024000 );
	mConsole->setParent( mUIApplication->getUI()->getRoot() );
	mConsole->setQuakeMode( true );
	mConsole->setVisible( false );
	mConsole->setMaxLogLines( 1024000 );
	mConsole->setFontSize( mConfig.ConsoleFontSize );
	mConsole->on( Event::KeyDown, [this]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE ) {
			saveConfig();
			mUIApplication->getWindow()->close();
		}
	} );
	mConsoleCommands = std::make_unique<ConsoleCommands>( this, mConsole );
	registerConsoleCommands();
	registerKeyBindings();

	mWelcomeScreen = UIWelcomeScreen::New( this );
	mWelcomeScreen->setParent( mMainLayout );
	mWelcomeScreen->setVisible( false );

	mUIApplication->getWindow()->getInput()->pushCallback( [this]( InputEvent* event ) {
		switch ( event->Type ) {
			case InputEvent::FileDropped: {
				mPathsToLoad.emplace_back( event->file.file );
				break;
			}
			case InputEvent::EventsSent: {
				processDroppedFiles();
				break;
			}
			default:
				break;
		}
	} );

	if ( !mInitialPath.empty() )
		loadDir( mInitialPath, true );

	if ( mFiles.empty() )
		showWelcomeScreen();

	Log::instance()->setKeepLog( false );
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
	auto* ui = mUIApplication->getUI();
	// Viewer shortcuts never act while a dialog has focus, and most are off while the console is
	// open (it takes printable keys first).
	const auto command = [this, ui]( const char* name, bool withConsole,
									 std::function<void()> fn ) {
		ui->setKeyBindingCommand( name, [this, withConsole, fn = std::move( fn )] {
			if ( isViewerFocused() && ( withConsole || !mConsole->isActive() ) )
				fn();
		} );
	};
	const auto pan = [this]( Float x, Float y ) {
		return [this, x, y] {
			if ( mImageViewer->hasImage() ) {
				auto* image = mImageViewer->getImage();
				image->setPixelsPosition( image->getPixelsPosition() + Vector2f( x, y ) );
			}
		};
	};
	command( "open-settings", true, [this] { showSettings(); } );
	command( "debug-widget-tree-view", true, [ui] { UIWidgetInspector::create( ui ); } );
	command( "toggle-console", true, [this] {
		mHelp->setVisible( false );
		mConsole->toggle();
	} );
	command( "quit", false, [this] {
		saveConfig();
		getWindow()->close();
	} );
	command( "minimize", false, [this] { getWindow()->minimize(); } );
	command( "toggle-fullscreen", false, [this] { getWindow()->toggleFullscreen(); } );
	command( "screenshot", false, [this] { getWindow()->takeScreenshot(); } );
	command( "toggle-help", false, [this] { toggleHelp(); } );
	command( "toggle-cursor", false, [this] {
		mCursorVisible = !mCursorVisible;
		getWindow()->getCursorManager()->setVisible( mCursorVisible );
	} );
	command( "paste-path", false, [this] {
		const std::string path( getWindow()->getClipboard()->getText() );
		if ( isImage( path ) || isHttpUrl( path ) || FileSystem::isDirectory( path ) )
			loadDir( path );
	} );
	command( "first-image", false, [this] {
		if ( !mFiles.empty() )
			fastLoadImage( 0 );
	} );
	command( "last-image", false, [this] {
		if ( !mFiles.empty() )
			fastLoadImage( mFiles.size() - 1 );
	} );
	command( "previous-image", false, [this] {
		if ( mCurImg > 0 ) {
			mSlideShow = false;
			fastLoadImage( mCurImg - 1 );
		}
	} );
	command( "next-image", false, [this] {
		if ( mCurImg + 1 < static_cast<Int32>( mFiles.size() ) ) {
			mSlideShow = false;
			fastLoadImage( mCurImg + 1 );
		}
	} );
	command( "toggle-info", false, [this] { setShowInfo( !mConfig.ShowInfo ); } );
	command( "fit-image", false, [this] { fitImage(); } );
	command( "reset-zoom", false, [this] { resetImageZoom(); } );
	command( "native-size", false, [this] {
		if ( mImageViewer->getImage()->getDrawable() ) {
			const Sizef size( mImageViewer->getOrientedImageSize() );
			getWindow()->setSize( size.getWidth(), size.getHeight() );
		}
	} );
	command( "toggle-filtering", false, [this] {
		setTextureFilter( mTextureFilter == Texture::Filter::Linear ? Texture::Filter::Nearest
																	: Texture::Filter::Linear );
	} );
	command( "lock-view", false, [this] {
		mImageViewer->setPreserveImageView( !mImageViewer->getPreserveImageView() );
	} );
	command( "fit-window", false, [this] {
		fitImage();
		if ( getWindow()->isMaximized() && mImageViewer->getImage()->getDrawable() ) {
			const Sizef size( mImageViewer->getOrientedImageSize() );
			getWindow()->setSize( size.getWidth(), size.getHeight() );
		}
	} );
	command( "toggle-mipmaps", false, [this] {
		if ( Sprite* sprite = getImageSprite() ) {
			if ( TextureRegion* region = sprite->getCurrentTextureRegion() ) {
				if ( Texture* texture = region->getTexture().get() ) {
					texture->setMipmap( !texture->getMipmap() );
					texture->reload();
				}
			}
		}
	} );
	command( "reload-image", false, [this] {
		if ( !getFilePath().empty() )
			loadImagePath( getFilePath() );
	} );
	command( "start-slideshow", false, [this] { createSlideShow( mSlideTime ); } );
	command( "stop-slideshow", false, [this] { mSlideShow = false; } );
	command( "pan-left", false, pan( 20.f, 0.f ) );
	command( "pan-right", false, pan( -20.f, 0.f ) );
	command( "pan-up", false, pan( 0.f, 20.f ) );
	command( "pan-down", false, pan( 0.f, -20.f ) );
	// A broken keybindings.cfg must not disable the keyboard: fall back to the defaults.
	if ( !loadKeybindings() )
		loadKeybindings( true );
}

KeyBindings::ShortcutMap App::getDefaultKeybindings() const {
	const Uint32 mod = KeyMod::getDefaultModifier();
	const Uint32 mod2 = KeyMod::getDefaultSecondaryModifier();
	auto defaults = UIImageViewer::getDefaultKeybindings();
	defaults.insert( { { { KEY_COMMA, mod }, "open-settings" },
					   { { KEY_F11, KEYMOD_NONE }, "debug-widget-tree-view" },
					   { { KEY_F3, KEYMOD_NONE }, "toggle-console" },
					   { { KEY_ESCAPE, KEYMOD_NONE }, "quit" },
					   { { KEY_Q, KEYMOD_NONE }, "quit" },
					   { { KEY_TAB, mod2 }, "minimize" },
					   { { KEY_F, KEYMOD_NONE }, "toggle-fullscreen" },
					   { { KEY_RETURN, mod2 }, "toggle-fullscreen" },
					   { { KEY_F12, KEYMOD_NONE }, "screenshot" },
					   { { KEY_H, KEYMOD_NONE }, "toggle-help" },
					   { { KEY_S, KEYMOD_NONE }, "toggle-cursor" },
					   { { KEY_F4, KEYMOD_NONE }, "toggle-cursor" },
					   { { KEY_V, KEYMOD_CTRL }, "paste-path" },
					   { { KEY_INSERT, KEYMOD_SHIFT }, "paste-path" },
					   { { KEY_HOME, KEYMOD_NONE }, "first-image" },
					   { { KEY_END, KEYMOD_NONE }, "last-image" },
					   { { KEY_PAGEUP, KEYMOD_NONE }, "previous-image" },
					   { { KEY_PAGEDOWN, KEYMOD_NONE }, "next-image" },
					   { { KEY_I, KEYMOD_NONE }, "toggle-info" },
					   { { KEY_Z, KEYMOD_NONE }, "fit-image" },
					   { { KEY_KP_MULTIPLY, KEYMOD_NONE }, "fit-image" },
					   { { KEY_KP_DIVIDE, KEYMOD_NONE }, "reset-zoom" },
					   { { KEY_N, KEYMOD_NONE }, "native-size" },
					   { { KEY_A, KEYMOD_NONE }, "toggle-filtering" },
					   { { KEY_L, KEYMOD_NONE }, "lock-view" },
					   { { KEY_M, KEYMOD_NONE }, "fit-window" },
					   { { KEY_K, KEYMOD_NONE }, "toggle-mipmaps" },
					   { { KEY_F5, KEYMOD_NONE }, "reload-image" },
					   { { KEY_E, KEYMOD_NONE }, "start-slideshow" },
					   { { KEY_D, KEYMOD_NONE }, "stop-slideshow" },
					   { { KEY_LEFT, KEYMOD_NONE }, "pan-left" },
					   { { KEY_RIGHT, KEYMOD_NONE }, "pan-right" },
					   { { KEY_UP, KEYMOD_NONE }, "pan-up" },
					   { { KEY_DOWN, KEYMOD_NONE }, "pan-down" } } );
	return defaults;
}

bool App::loadKeybindings( bool defaultsOnly, std::string* error ) {
	KeybindingsConfig::Section bindings;
	std::string message;
	if ( !KeybindingsConfig::loadContext(
			 defaultsOnly ? "" : mStorePath + "keybindings.cfg", "eeiv",
			 [this] { return getDefaultKeybindings(); }, getWindow()->getInput(), bindings,
			 message ) ) {
		Log::error( "%s", message.c_str() );
		if ( error )
			*error = message;
		return false;
	}
	// The image viewer runs its own commands; the scene runs the rest.
	auto& scene = mUIApplication->getUI()->getKeyBindings();
	auto& viewer = mImageViewer->getKeyBindings();
	const auto ownedByViewer = [this]( const std::string& command ) {
		return mImageViewer->hasCommand( command );
	};
	scene.reset();
	viewer.reset();
	bindings.apply( viewer, ownedByViewer );
	bindings.apply( scene, std::not_fn( ownedByViewer ) );
	updateHelp();
	return true;
}

UIWidget* App::createKeybindingsEditor( UIWidget* parent ) {
	static constexpr KeybindingsConfig::Context contexts[] = { { "eeiv" } };
	const auto path = mStorePath + "keybindings.cfg";
	return UIKeybindingsEditor::NewForFile(
		parent,
		{ .path = path,
		  .contexts = contexts,
		  .catalog =
			  [this]( KeybindingsConfig::Catalog& catalog ) {
				  catalog.add( "eeiv", getDefaultKeybindings(), {}, "Image Viewer" );
			  },
		  .apply =
			  [this] {
				  std::string error;
				  return loadKeybindings( false, &error ) ? String() : String( error );
			  },
		  .manualEdit =
			  [path] { Engine::instance()->openURI( URI::fromFSPath( path ).toString() ); } } );
}

void App::updateHelp() {
	static constexpr std::pair<const char*, const char*> commands[] = {
		{ "toggle-help", "Toggle this help" },
		{ "toggle-console", "Toggle console" },
		{ "debug-widget-tree-view", "Open UI inspector" },
		{ "screenshot", "Take screenshot" },
		{ "previous-image", "Previous image" },
		{ "next-image", "Next image" },
		{ "first-image", "First image" },
		{ "last-image", "Last image" },
		{ "pan-left", "Move image left" },
		{ "pan-right", "Move image right" },
		{ "pan-up", "Move image up" },
		{ "pan-down", "Move image down" },
		{ "image-zoom-in", "Zoom in" },
		{ "image-zoom-out", "Zoom out" },
		{ "fit-image", "Fit image" },
		{ "reset-zoom", "Reset zoom" },
		{ "image-flip", "Flip image" },
		{ "image-mirror", "Mirror image" },
		{ "image-rotate", "Rotate image" },
		{ "image-reset-view", "Reset view" },
		{ "toggle-filtering", "Toggle filtering" },
		{ "start-slideshow", "Start slideshow" },
		{ "stop-slideshow", "Stop slideshow" },
		{ "lock-view", "Lock view" },
		{ "reload-image", "Reload image" },
		{ "toggle-fullscreen", "Toggle fullscreen" },
		{ "toggle-cursor", "Toggle cursor" },
		{ "toggle-info", "Toggle image info" },
		{ "quit", "Quit" },
		{ "open-settings", "Open settings" },
	};
	auto& scene = mUIApplication->getUI()->getKeyBindings();
	auto& viewer = mImageViewer->getKeyBindings();
	String text( "Mouse wheel: Previous / next image" );
	for ( const auto& [command, label] : commands ) {
		auto shortcut = scene.getCommandKeybindString( command );
		if ( shortcut.empty() )
			shortcut = viewer.getCommandKeybindString( command );
		if ( !shortcut.empty() )
			text += "\n" + shortcut + ": " + label;
	}
	mHelp->setText( text );
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

void App::resetImageZoom() {
	if ( !mImageViewer || !mImageViewer->getImage() || !mImageViewer->getImage()->getDrawable() )
		return;

	UIImage* image = mImageViewer->getImage();
	const Sizef displayedSize( image->getPixelsSize() );
	const Sizef nativeSize( image->getDrawable()->getPixelsSize() );
	if ( displayedSize.getWidth() == 0.f || displayedSize.getHeight() == 0.f )
		return;

	const Float pixelDensity = PixelDensity::getPixelDensity();
	image->setScale( { mConfig.DefaultImageZoom * nativeSize.getWidth() /
						   ( displayedSize.getWidth() * pixelDensity ),
					   mConfig.DefaultImageZoom * nativeSize.getHeight() /
						   ( displayedSize.getHeight() * pixelDensity ) } );
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
	hideWelcomeScreen();
	mFilePath = FileSystem::fileRemoveFileName( path );
	FileSystem::dirAddSlashAtEnd( mFilePath );
	mFile = FileSystem::fileNameFromPath( path );
	auto it = std::find( mFiles.begin(), mFiles.end(), mFile );
	if ( it != mFiles.end() )
		mCurImg = std::distance( mFiles.begin(), it );
	setTextureFilter( mTextureFilter );
	getWindow()->setTitle( "eeiv - " + mFile );
	// An asynchronous image load must not take keyboard focus from the settings window.
	if ( !mSettingsWindow )
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
		if ( mConfig.BenchmarkMode || mUIApplication->getUI()->invalidated() ) {
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
