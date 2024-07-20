#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::Scene;

namespace EE { namespace UI {

UIApplication::UIApplication( const WindowSettings& windowSettings, const Settings& appSettings,
							  const ContextSettings& contextSettings ) {
	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();

	mWindow = Engine::instance()->createWindow( windowSettings, contextSettings );

	if ( !mWindow->isOpen() )
		return;

	mDidRun = true;

	PixelDensity::setPixelDensity(
		appSettings.pixelDensity
			? *appSettings.pixelDensity
			: eemax( mWindow->getScale(),
					 displayManager->getDisplayIndex( mWindow->getCurrentDisplayIndex() )
						 ->getPixelDensity() ) );

	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	mUISceneNode = UISceneNode::New();
	SceneManager::instance()->add( mUISceneNode );

	if ( !appSettings.loadBaseResources )
		return;

	Font* font = appSettings.baseFont
					 ? appSettings.baseFont
					 : FontTrueType::New( "NotoSans-Regular", "assets/fonts/NotoSans-Regular.ttf" );

	if ( font && font->getType() == FontType::TTF )
		FontFamily::loadFromRegular( static_cast<FontTrueType*>( font ) );

	if ( appSettings.emojiFont == nullptr )
		FontTrueType::New( "NotoEmoji-Regular", "assets/fonts/NotoEmoji-Regular.ttf" );

	mUISceneNode->getUIThemeManager()->setDefaultFont( font );
	mUISceneNode->getRoot()->addClass( "appbackground" );
	mUISceneNode->getUIThemeManager()->setDefaultEffectsEnabled( true )->setDefaultFont( font );

	UITheme* theme = UITheme::load( "uitheme", "uitheme", "", font,
									appSettings.baseStyleSheetPath ? *appSettings.baseStyleSheetPath
																   : "assets/ui/breeze.css" );

	mUISceneNode->setStyleSheet( theme->getStyleSheet() );
	mUISceneNode->getUIThemeManager()->setDefaultTheme( theme )->add( theme );
}

UIApplication::~UIApplication() {
	Engine::destroySingleton();
	MemoryManager::showResults();
}

EE::Window::Window* UIApplication::getWindow() const {
	return mWindow;
}

UISceneNode* UIApplication::getUI() const {
	return mUISceneNode;
}

int UIApplication::run() {
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

UIApplication::Settings::Settings( std::optional<Float> pixelDensity, bool loadBaseResources,
								   Font* baseFont, std::optional<std::string> baseStyleSheetPath,
								   Font* emojiFont ) :
	pixelDensity( pixelDensity ),
	loadBaseResources( loadBaseResources ),
	baseFont( baseFont ),
	baseStyleSheetPath( baseStyleSheetPath ),
	emojiFont( emojiFont ) {}

}} // namespace EE::UI
