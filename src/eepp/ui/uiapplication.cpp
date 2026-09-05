#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fontservice.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/systemfontresolver.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/thread.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/runtime.hpp>

#include <atomic>
#include <iostream>

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
							  const ContextSettings& contextSettings ) {
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

	mUISceneNode = UISceneNode::New();
	FontService& uiFontService = mUISceneNode->getResourceScope()->getFontService();
	uiFontService.setHinting( appSettings.fontHinting );
	uiFontService.setAntialiasing( appSettings.fontAntialiasing );
	SceneManager::instance()->add( mUISceneNode );

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

int UIApplication::run() {
	// Offscreen SDL windows do not receive an initial expose event. Ensure the first logical
	// framebuffer is rendered even when the scene was fully laid out before entering the loop.
	if ( Runtime::isOffscreen() )
		mUISceneNode->invalidate( nullptr );

	mWindow->runMainLoop( [this]() {
		mWindow->getInput()->update();
		SceneManager::instance()->update();
		if ( mUISceneNode->invalidated() ) {
			mWindow->clear();

			SceneManager::instance()->draw();

			mWindow->display();
		} else {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
			mWindow->getInput()->waitEvent( Milliseconds( mWindow->hasFocus() ? 16 : 100 ) );
#endif
		}
	} );

	return mDidRun ? EXIT_SUCCESS : EXIT_FAILURE;
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
