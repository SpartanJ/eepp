#include "utest.hpp"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;

static UISceneNode* init_test_scene_node() {
	FontTrueType* font = nullptr;
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );
	FontTrueType* monoFont = FontTrueType::New( "monospace" );
	monoFont->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	SceneManager::instance()->setCurrentUISceneNode( sceneNode );
	UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
	themeManager->applyDefaultTheme( sceneNode->getRoot() );
	return sceneNode;
}

UTEST( UISceneNode, ViewportMetricsAreIndependentFromSceneExtent ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Scene Viewport Metrics Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_scene_node();
	sceneNode->setPixelsSize( 800, 3000 );

	EXPECT_EQ( sceneNode->getPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getPixelsSize().getHeight(), 3000 );
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getHeight(), 3000 );
	EXPECT_EQ( sceneNode->getMediaFeatures().height, 3000 );

	UIWidget* widget = UIWidget::New();
	widget->setId( "viewport-metric-target" );
	widget->setParent( sceneNode->getRoot() );
	sceneNode->setStyleSheet( R"css(
#viewport-metric-target { background-color: #ff0000; height: 100vh; }
@media screen and (max-height: 700px) {
	#viewport-metric-target { background-color: #00ff00; }
}
)css" );
	EXPECT_TRUE( widget->getBackgroundColor() == Color( "#ff0000" ) );
	EXPECT_NEAR( widget->getPixelsSize().getHeight(), 3000.f, 0.01f );

	sceneNode->setViewportPixelsSize( Sizef( 800, 600 ) );

	EXPECT_EQ( sceneNode->getPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getPixelsSize().getHeight(), 3000 );
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getHeight(), 600 );
	EXPECT_EQ( sceneNode->getMediaFeatures().width, 800 );
	EXPECT_EQ( sceneNode->getMediaFeatures().height, 600 );
	EXPECT_TRUE( widget->getBackgroundColor() == Color( "#00ff00" ) );
	EXPECT_NEAR( widget->getPixelsSize().getHeight(), 600.f, 0.01f );
	EXPECT_NEAR( widget->convertLength( StyleSheetLength( "100vh" ), 0 ), 600.f, 0.01f );
	EXPECT_NEAR( widget->convertLength( StyleSheetLength( "50vw" ), 0 ), 400.f, 0.01f );

	sceneNode->clearViewportPixelsSize();
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getHeight(), 3000 );
	EXPECT_TRUE( widget->getBackgroundColor() == Color( "#ff0000" ) );
	EXPECT_NEAR( widget->getPixelsSize().getHeight(), 3000.f, 0.01f );
	EXPECT_NEAR( widget->convertLength( StyleSheetLength( "100vh" ), 0 ), 3000.f, 0.01f );

	Engine::destroySingleton();
}

UTEST( UISceneNode, MediaQueriesUseCssViewportPixelsAtPixelDensity2 ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Scene Media CSS Pixels Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 2.f, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_scene_node();
	sceneNode->setViewportPixelsSize( Sizef( 800, 600 ) );

	EXPECT_EQ( sceneNode->getViewportPixelsSize().getWidth(), 800 );
	EXPECT_EQ( sceneNode->getViewportPixelsSize().getHeight(), 600 );
	EXPECT_EQ( sceneNode->getMediaFeatures().width, 400 );
	EXPECT_EQ( sceneNode->getMediaFeatures().height, 300 );

	UIWidget* widget = UIWidget::New();
	widget->setId( "media-css-pixels-target" );
	widget->setParent( sceneNode->getRoot() );
	sceneNode->setStyleSheet( R"css(
#media-css-pixels-target { background-color: #ff0000; }
@media only screen and (min-width: 300px) and (max-width: 500px) {
	#media-css-pixels-target { background-color: #00ff00; }
}
)css" );

	EXPECT_TRUE( widget->getBackgroundColor() == Color( "#00ff00" ) );

	Engine::destroySingleton();
	EE::Graphics::PixelDensity::setPixelDensity( 1.0f );
}

UTEST( UISceneNode, EmbeddedSceneCanKeepContentDrivenExtent ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Embedded Scene Extent Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* hostScene = init_test_scene_node();
	UIWidget* host = UIWidget::New();
	host->setPixelsSize( 800, 600 );
	host->setParent( hostScene->getRoot() );

	UISceneNode* embeddedScene = UISceneNode::New();
	embeddedScene->setFollowParentSize( false );
	embeddedScene->setPixelsSize( 800, 3000 );
	embeddedScene->setParent( host );

	EXPECT_FALSE( embeddedScene->followsParentSize() );
	EXPECT_EQ( embeddedScene->getPixelsSize().getWidth(), 800 );
	EXPECT_EQ( embeddedScene->getPixelsSize().getHeight(), 3000 );

	host->setPixelsSize( 700, 500 );
	EXPECT_EQ( embeddedScene->getPixelsSize().getWidth(), 800 );
	EXPECT_EQ( embeddedScene->getPixelsSize().getHeight(), 3000 );

	embeddedScene->setFollowParentSize( true );
	EXPECT_TRUE( embeddedScene->followsParentSize() );
	EXPECT_EQ( embeddedScene->getPixelsSize().getWidth(), 700 );
	EXPECT_EQ( embeddedScene->getPixelsSize().getHeight(), 500 );

	host->setPixelsSize( 640, 480 );
	EXPECT_EQ( embeddedScene->getPixelsSize().getWidth(), 640 );
	EXPECT_EQ( embeddedScene->getPixelsSize().getHeight(), 480 );

	Engine::destroySingleton();
}

UTEST( UISceneNode, NestedSceneOwnsDescendantSceneAndStyles ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Nested Scene Isolation Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* hostScene = init_test_scene_node();
	UIWidget* hostWidget = UIWidget::New();
	hostWidget->setParent( hostScene->getRoot() );

	UISceneNode* nestedScene = UISceneNode::New();
	nestedScene->setParent( hostWidget );
	UIWidget* nestedWidget = UIWidget::New();
	nestedWidget->setClass( "shared-name" );
	nestedWidget->setParent( nestedScene->getRoot() );
	UIWidget* hostStyledWidget = UIWidget::New();
	hostStyledWidget->setClass( "shared-name" );
	hostStyledWidget->setParent( hostScene->getRoot() );

	EXPECT_TRUE( nestedWidget->getUISceneNode() == nestedScene );
	EXPECT_TRUE( nestedWidget->getSceneNode() == nestedScene );

	hostScene->setStyleSheet( ".shared-name { background-color: #ff0000; }" );
	nestedScene->setStyleSheet( ".shared-name { background-color: #0000ff; }" );

	EXPECT_TRUE( hostStyledWidget->getBackgroundColor() == Color( "#ff0000" ) );
	EXPECT_TRUE( nestedWidget->getBackgroundColor() == Color( "#0000ff" ) );

	Engine::destroySingleton();
}

UTEST( UISceneNode, NestedSceneRebindsHostServicesOnSceneChange ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Nested Scene Rebind Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* hostSceneA = init_test_scene_node();
	UISceneNode* hostSceneB = UISceneNode::New();
	SceneManager::instance()->add( hostSceneB );
	hostSceneA->setColorSchemePreference( ColorSchemePreference::Light );
	hostSceneB->setColorSchemePreference( ColorSchemePreference::Dark );
	hostSceneB->getUIThemeManager()->setDefaultFontSize( 42.f );

	UIWidget* hostWidget = UIWidget::New();
	hostWidget->setParent( hostSceneA->getRoot() );
	UISceneNode* nestedScene = UISceneNode::New();
	nestedScene->setParent( hostWidget );

	EXPECT_TRUE( nestedScene->getEventDispatcher() == hostSceneA->getEventDispatcher() );
	EXPECT_EQ( nestedScene->getColorSchemePreference(), ColorSchemePreference::Light );

	hostWidget->setParent( hostSceneB->getRoot() );

	EXPECT_TRUE( nestedScene->getEventDispatcher() == hostSceneB->getEventDispatcher() );
	EXPECT_EQ( nestedScene->getColorSchemePreference(), ColorSchemePreference::Dark );
	EXPECT_EQ( nestedScene->getUIThemeManager()->getDefaultFontSize(), 42.f );
	EXPECT_TRUE( nestedScene->getRoot()->getUISceneNode() == nestedScene );

	Engine::destroySingleton();
}

UTEST( UISceneNode, NestedSceneMediaUsesExplicitViewport ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Nested Scene Media Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* hostScene = init_test_scene_node();
	UIWidget* hostWidget = UIWidget::New();
	hostWidget->setParent( hostScene->getRoot() );

	UISceneNode* nestedScene = UISceneNode::New();
	nestedScene->setFollowParentSize( false );
	nestedScene->setPixelsSize( 800, 3000 );
	nestedScene->setViewportPixelsSize( Sizef( 800, 600 ) );
	nestedScene->setParent( hostWidget );

	UIWidget* nestedWidget = UIWidget::New();
	nestedWidget->setId( "media-target" );
	nestedWidget->setParent( nestedScene->getRoot() );
	nestedScene->setStyleSheet( R"css(
#media-target { background-color: #ff0000; }
@media screen and (max-height: 700px) {
	#media-target { background-color: #00ff00; }
}
)css" );

	EXPECT_EQ( nestedScene->getMediaFeatures().height, 600 );
	EXPECT_TRUE( nestedWidget->getBackgroundColor() == Color( "#00ff00" ) );

	Engine::destroySingleton();
}

UTEST( UISceneNode, NestedSceneInvalidationPropagatesToHostScene ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 768, "Nested Scene Invalidation Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* hostScene = init_test_scene_node();
	hostScene->enableDrawInvalidation();

	UIWidget* hostWidget = UIWidget::New();
	hostWidget->setParent( hostScene->getRoot() );
	UISceneNode* nestedScene = UISceneNode::New();
	nestedScene->setParent( hostWidget );
	UIWidget* nestedWidget = UIWidget::New();
	nestedWidget->setParent( nestedScene->getRoot() );

	win->getInput()->update();
	SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
	win->clear();
	SceneManager::instance()->draw();
	win->display();

	ASSERT_FALSE( hostScene->invalidated() );
	ASSERT_FALSE( nestedScene->invalidated() );

	nestedWidget->invalidateDraw();

	EXPECT_TRUE( nestedScene->invalidated() );
	EXPECT_TRUE( hostScene->invalidated() );

	Engine::destroySingleton();
}
