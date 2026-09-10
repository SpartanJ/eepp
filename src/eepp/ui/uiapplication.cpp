#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fontservice.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/systemfontresolver.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/thread.hpp>
#include <eepp/ui/iconmanager.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiiconthememanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/runtime.hpp>

#include <atomic>
#include <iostream>

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::Scene;

namespace EE { namespace UI {

namespace Private {

class UIApplicationSystemFontState {
  public:
	UIApplicationSystemFontState() :
		warmUpThread( [] { SystemFontResolver::instance()->warmUp(); } ) {
		warmUpThread.launch();
	}

	Thread warmUpThread;
};

} // namespace Private

static std::atomic<bool> sSystemFontsEnabledByDefault{ true };

UIApplication::UIApplication( const WindowSettings& windowSettings, const Settings& appSettings,
							  const ContextSettings& contextSettings ) :
	mSettings( appSettings ) {
	const bool enableSystemFonts = appSettings.enableSystemFonts.value_or(
		sSystemFontsEnabledByDefault.load( std::memory_order_acquire ) );
	if ( enableSystemFonts ) {
		SystemFontResolver::setEnabled( true );
		mSystemFontState = std::make_unique<Private::UIApplicationSystemFontState>();
	}

	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	const bool offscreen = Runtime::isOffscreen();
	if ( !offscreen ) {
		displayManager->enableScreenSaver();
		displayManager->enableMouseFocusClickThrough();
		displayManager->disableBypassCompositor();
	}

	if ( !offscreen && displayManager->getDisplayIndex( 0 ) == nullptr ) {
		std::cerr << "Display not found, exiting" << std::endl;
		return;
	}

	mWindow = Engine::instance()->createWindow( windowSettings, contextSettings );

	if ( nullptr == mWindow || !mWindow->isOpen() ) {
		std::cerr << "Could not create window, exiting" << std::endl;
		return;
	}
	mWindow->setDeferNativeResourceDestructionOnClose( true );

	mDidRun = true;

	if ( appSettings.pixelDensity && *appSettings.pixelDensity > 0 ) {
		PixelDensity::setPixelDensity( *appSettings.pixelDensity );
	} else if ( offscreen ) {
		PixelDensity::setPixelDensity( 1.f );
	} else {
		PixelDensity::setPixelDensity(
			eemax( mWindow->getScale(),
				   displayManager->getDisplayIndex( mWindow->getCurrentDisplayIndex() )
					   ->getPixelDensity() ) );
	}

	if ( !appSettings.basePath || appSettings.basePath->empty() ) {
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	} else {
		std::string path( *appSettings.basePath );
		FileSystem::dirAddSlashAtEnd( path );
		FileSystem::changeWorkingDirectory( path );
	}

	FontService& defaultFontService = defaultResourceScope().getFontService();
	defaultFontService.setHinting( appSettings.fontHinting );
	defaultFontService.setAntialiasing( appSettings.fontAntialiasing );

	mUISceneNode = UISceneNode::New( mWindow );
	FontService& uiFontService = mUISceneNode->getResourceScope()->getFontService();
	uiFontService.setHinting( appSettings.fontHinting );
	uiFontService.setAntialiasing( appSettings.fontAntialiasing );
	SceneManager::instance()->add( mUISceneNode );
	mWindows.push_back( { mWindow, mUISceneNode, true, false } );

	if ( !appSettings.loadBaseResources )
		return;
	FontTrueTypePtr loadedBaseFont;
	if ( !appSettings.baseFont )
		loadedBaseFont =
			FontTrueType::New( "NotoSans-Regular", "assets/fonts/NotoSans-Regular.ttf" );
	Font* font = appSettings.baseFont ? appSettings.baseFont : loadedBaseFont.get();

	if ( font && font->getType() == FontType::TTF )
		FontFamily::loadFromRegular( static_cast<FontTrueType*>( font ) );

	FontTrueTypePtr loadedMonospaceFont;
	if ( !appSettings.monospaceFont )
		loadedMonospaceFont = FontTrueType::New( "monospace", "assets/fonts/DejaVuSansMono.ttf" );
	Font* monospaceFont =
		appSettings.monospaceFont ? appSettings.monospaceFont : loadedMonospaceFont.get();

	if ( monospaceFont && monospaceFont->getType() == FontType::TTF ) {
		static_cast<FontTrueType*>( monospaceFont )->setEnableDynamicMonospace( true );
		FontFamily::loadFromRegular( static_cast<FontTrueType*>( monospaceFont ) );
	}

	if ( appSettings.emojiFont == nullptr ) {
		if ( FileSystem::fileExists( "assets/fonts/NotoColorEmoji.ttf" ) )
			FontTrueType::New( "NotoColorEmoji", "assets/fonts/NotoColorEmoji.ttf" );
		else if ( FileSystem::fileExists( "assets/fonts/NotoEmoji-Regular.ttf" ) )
			FontTrueType::New( "NotoEmoji-Regular", "assets/fonts/NotoEmoji-Regular.ttf" );
	}

	if ( appSettings.fallbackFont == nullptr &&
		 FileSystem::fileExists( "assets/fonts/DroidSansFallbackFull.ttf" ) )
		FontTrueType::New( "DroidSansFallbackFull", "assets/fonts/DroidSansFallbackFull.ttf" );

	mUISceneNode->getUIThemeManager()->setDefaultFont( font );
	mUISceneNode->getRoot()->addClass( "appbackground" );
	mUISceneNode->getUIThemeManager()->setDefaultEffectsEnabled( true )->setDefaultFont( font );

	UIThemePtr theme = UITheme::load(
		"uitheme", "uitheme", "", font,
		appSettings.baseStyleSheetPath ? *appSettings.baseStyleSheetPath : "assets/ui/breeze.css" );

	mStyleSheetMarker = String::hash( "uitheme" );
	mUISceneNode->setStyleSheet( theme->getStyleSheet() );
	mUISceneNode->getStyleSheet().setMarker( mStyleSheetMarker );
	mUISceneNode->getUIThemeManager()->setDefaultTheme( std::move( theme ) );

	if ( appSettings.loadIconResources ) {
		auto loadIconFont = []( const std::string& name,
								const std::string& path ) -> FontTrueType* {
			if ( auto font = defaultResourceScope().findFont( name ) )
				return font->getType() == FontType::TTF ? static_cast<FontTrueType*>( font.get() )
														: nullptr;
			if ( !FileSystem::fileExists( path ) )
				return nullptr;
			return FontTrueType::New( name, path ).get();
		};
		auto* remixIconFont = loadIconFont( "icon", "assets/fonts/remixicon.ttf" );
		auto* noniconsFont = loadIconFont( "nonicons", "assets/fonts/nonicons.ttf" );
		auto* codIconFont = loadIconFont( "codicon", "assets/fonts/codicon.ttf" );
		if ( remixIconFont || noniconsFont || codIconFont )
			mUISceneNode->getUIIconThemeManager()->setCurrentTheme(
				IconManager::init( "uiapplication", remixIconFont, noniconsFont, codIconFont ) );
	}
}

UIApplication::~UIApplication() {
	mSystemFontState.reset();
	Engine::destroySingleton();
	if ( mShowMemoryManagerResult )
		MemoryManager::showResults();
}

EE::Window::Window* UIApplication::getWindow() const {
	return mWindow;
}

UISceneNode* UIApplication::getUI() const {
	return mUISceneNode;
}

UIApplication::WindowEntry*
UIApplication::createWindowInternal( const WindowSettings& windowSettings,
									 const ContextSettings& contextSettings, bool primary ) {
	auto context = Engine::instance()->makeWindowCurrent( Engine::instance()->getCurrentWindow() );
	auto* window = Engine::instance()->createWindow( windowSettings, contextSettings );
	if ( !window || !window->isOpen() )
		return nullptr;
	window->setDeferNativeResourceDestructionOnClose( true );

	auto* ui = UISceneNode::New( window );
	SceneManager::instance()->add( ui );
	mWindows.push_back( { window, ui, primary, false } );
	configureUIScene( ui );
	return &mWindows.back();
}

void UIApplication::configureUIScene( UISceneNode* ui ) {
	FontService& uiFontService = ui->getResourceScope()->getFontService();
	uiFontService.setHinting( mSettings.fontHinting );
	uiFontService.setAntialiasing( mSettings.fontAntialiasing );
	if ( !mUISceneNode || ui == mUISceneNode || !mSettings.loadBaseResources )
		return;

	auto* sourceThemeManager = mUISceneNode->getUIThemeManager();
	ui->getUIThemeManager()->setDefaultFont( sourceThemeManager->getDefaultFont() );
	ui->getUIThemeManager()->setDefaultEffectsEnabled(
		sourceThemeManager->getDefaultEffectsEnabled() );
	ui->getUIThemeManager()->setDefaultTheme( sourceThemeManager->getDefaultThemeHandle() );
	ui->getUIIconThemeManager()->setCurrentTheme(
		mUISceneNode->getUIIconThemeManager()->getCurrentThemeHandle() );
	ui->setStyleSheet( mUISceneNode->getStyleSheet() );
	ui->getStyleSheet().setMarker( mStyleSheetMarker );
	ui->getRoot()->addClass( "appbackground" );
}

UISceneNode* UIApplication::createWindow( const WindowSettings& windowSettings,
										  const ContextSettings& contextSettings ) {
	auto* entry = createWindowInternal( windowSettings, contextSettings, false );
	return entry ? entry->ui : nullptr;
}

UISceneNode* UIApplication::getUI( EE::Window::Window* window ) const {
	for ( const auto& entry : mWindows ) {
		if ( entry.window == window )
			return entry.ui;
	}
	return nullptr;
}

size_t UIApplication::getWindowCount() const {
	return mWindows.size();
}

void UIApplication::closeWindow( EE::Window::Window* window ) {
	if ( nullptr == window )
		return;

	bool closeAllWindows = false;
	for ( auto& entry : mWindows ) {
		if ( entry.window != window )
			continue;
		entry.window->hide();
		entry.window->close();
		entry.pendingDestroy = true;
		closeAllWindows = entry.primary && mQuitPolicy == QuitPolicy::OnPrimaryWindowClosed;
		break;
	}

	if ( closeAllWindows ) {
		for ( auto& entry : mWindows ) {
			if ( entry.window ) {
				entry.window->hide();
				entry.window->close();
			}
			entry.pendingDestroy = true;
		}
	}
}

void UIApplication::requestQuit() {
	mRunning = false;
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	emscripten_cancel_main_loop();
#endif
}

bool UIApplication::isRunning() const {
	return mRunning;
}

void UIApplication::setQuitPolicy( QuitPolicy policy ) {
	mQuitPolicy = policy;
}

UIApplication::QuitPolicy UIApplication::getQuitPolicy() const {
	return mQuitPolicy;
}

int UIApplication::run() {
	if ( !mDidRun )
		return EXIT_FAILURE;
	// Offscreen SDL windows do not receive an initial expose event. Ensure the first logical
	// framebuffer is rendered even when the scene was fully laid out before entering the loop.
	if ( Runtime::isOffscreen() ) {
		for ( auto& entry : mWindows )
			entry.ui->invalidate( nullptr );
	}

	mRunning = true;
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	emscripten_set_main_loop_arg(
		[]( void* application ) { static_cast<UIApplication*>( application )->tick(); }, this,
		mWindow->getFrameRateLimit(), 1 );
#else
	while ( mRunning )
		tick();
#endif

	return mDidRun ? EXIT_SUCCESS : EXIT_FAILURE;
}

void UIApplication::processClosedWindows() {
	bool primaryClosed = false;
	for ( auto& entry : mWindows ) {
		if ( entry.window && !entry.window->isOpen() ) {
			entry.window->hide();
			entry.pendingDestroy = true;
			primaryClosed |= entry.primary;
		}
	}

	if ( primaryClosed && mQuitPolicy == QuitPolicy::OnPrimaryWindowClosed ) {
		for ( auto& entry : mWindows ) {
			if ( entry.window && entry.window->isOpen() ) {
				entry.window->hide();
				entry.window->close();
			}
			entry.pendingDestroy = true;
		}
	}
}

void UIApplication::processPendingWindowDestruction() {
	for ( auto it = mWindows.begin(); it != mWindows.end(); ) {
		if ( !it->pendingDestroy ) {
			++it;
			continue;
		}
		auto* window = it->window;
		const bool wasPrimary = it->primary;
		const bool retainFinalContext = mWindows.size() == 1;
		Engine::instance()->setCurrentWindow( window );
		SceneManager::instance()->destroyScenes( window );
		it = mWindows.erase( it );
		// The engine needs one GL context alive while its shared GPU resources are released during
		// shutdown. A closed final window no longer participates in the application loop, but its
		// native resources remain engine-owned until Engine destruction.
		if ( !retainFinalContext )
			Engine::instance()->destroyWindow( window );
		if ( wasPrimary ) {
			mWindow = nullptr;
			mUISceneNode = nullptr;
			if ( mQuitPolicy == QuitPolicy::OnPrimaryWindowClosed )
				mRunning = false;
		}
	}
	if ( mWindows.empty() && mQuitPolicy == QuitPolicy::OnLastWindowClosed )
		mRunning = false;
}

void UIApplication::tick() {
	Engine::instance()->updateInput();
	processClosedWindows();
	SceneManager::instance()->update();

	bool presented = false;
	for ( auto& entry : mWindows ) {
		if ( entry.pendingDestroy || !entry.window->isOpen() || !entry.ui->invalidated() )
			continue;
		auto context = entry.ui->makeCurrent();
		entry.window->clear();
		SceneManager::instance()->draw( entry.window );
		entry.window->display( false, false );
		presented = true;
	}

	processPendingWindowDestruction();
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	if ( !mRunning )
		emscripten_cancel_main_loop();
#endif
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( mRunning && !mWindows.empty() ) {
		auto* window = mWindows.front().window;
		Engine::instance()->setCurrentWindow( window );
		Int64 waitMilliseconds = window->hasFocus() ? 16 : 100;
		if ( presented && window->getFrameRateLimit() > 0 ) {
			waitMilliseconds = eemax<Int64>( 0, 1000 / window->getFrameRateLimit() -
													mFrameClock.getElapsedTime().asMilliseconds() );
		}
		if ( waitMilliseconds > 0 )
			window->getInput()->waitEvent( Milliseconds( waitMilliseconds ) );
		mFrameClock.restart();
	} else if ( mRunning ) {
		Sys::sleep( Milliseconds( 16 ) );
		mFrameClock.restart();
	}
#endif
}

UIApplication::Settings::Settings( std::optional<std::string> basePath,
								   std::optional<Float> pixelDensity, bool loadBaseResources,
								   Font* baseFont, std::optional<std::string> baseStyleSheetPath,
								   Font* emojiFont, Font* fallbackFont ) :
	basePath( basePath ),
	pixelDensity( pixelDensity ),
	loadBaseResources( loadBaseResources ),
	baseFont( baseFont ),
	baseStyleSheetPath( baseStyleSheetPath ),
	emojiFont( emojiFont ),
	fallbackFont( fallbackFont ) {}

void UIApplication::setShowMemoryManagerResult( bool show ) {
	mShowMemoryManagerResult = show;
}

bool UIApplication::showMemoryManagerResult() const {
	return mShowMemoryManagerResult;
}

void UIApplication::setSystemFontsEnabledByDefault( bool enabled ) {
	sSystemFontsEnabledByDefault.store( enabled, std::memory_order_release );
}

bool UIApplication::systemFontsEnabledByDefault() {
	return sSystemFontsEnabledByDefault.load( std::memory_order_acquire );
}

}} // namespace EE::UI
