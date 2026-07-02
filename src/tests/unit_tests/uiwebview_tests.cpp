#include "utest.hpp"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uilayout.hpp>
#include <eepp/ui/uiroot.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwebview.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;

UTEST( UIWebView, OwnedDocumentSceneScrollTarget ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Document Scene Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 400, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->setViewType( ScrollViewType::Overlay );

	const std::string path = Sys::getTempPath() + "eepp_uiwebview_owned_scene.html";
	FileSystem::fileWrite( path, R"html(
<!DOCTYPE html>
<html>
<body style="margin:0">
	<div id="tall" style="height:1200px;width:380px;background:#abcdef"></div>
</body>
</html>
)html" );
	webView->loadURI( URI( "file://" + path ) );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	ASSERT_TRUE( webView->getScrollView() == documentScene->getParent() );
	ASSERT_TRUE( documentScene->getParent()->getParent() == webView->getContainer() );
	EXPECT_EQ( SceneManager::instance()->count(), (size_t)1 );

	Node* htmlNode = nullptr;
	for ( int i = 0; i < 10; i++ ) {
		win->getInput()->update();
		SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		htmlNode = documentScene->getRoot()->findByType( UI_TYPE_HTML_HTML );
		if ( htmlNode && documentScene->getPixelsSize().getHeight() > 1000.f )
			break;
	}

	ASSERT_TRUE( htmlNode != nullptr );
	auto bodyNode = documentScene->getRoot()->findByType( UI_TYPE_HTML_BODY );
	ASSERT_TRUE( bodyNode != nullptr );
	auto tallNode = documentScene->getRoot()->find( "tall" );
	ASSERT_TRUE( tallNode != nullptr );

	EXPECT_TRUE( webView->getDocumentContainer()->getUISceneNode() == documentScene );
	EXPECT_TRUE( htmlNode->getSceneNode() == documentScene );
	EXPECT_TRUE( bodyNode->getSceneNode() == documentScene );
	EXPECT_TRUE( tallNode->getSceneNode() == documentScene );
	EXPECT_NEAR( documentScene->getViewportPixelsSize().getWidth(), 400.f, 1.f );
	EXPECT_NEAR( documentScene->getViewportPixelsSize().getHeight(), 300.f, 1.f );
	EXPECT_GT( documentScene->getPixelsSize().getHeight(), 1000.f );
	EXPECT_TRUE( webView->getVerticalScrollBar()->isVisible() );

	ASSERT_TRUE( documentScene->getParent() != nullptr && documentScene->getParent()->isUINode() );
	UINode* scrollTarget = documentScene->getParent()->asType<UINode>();
	webView->getVerticalScrollBar()->setValue( 1.f );
	EXPECT_LT( scrollTarget->getPixelsPosition().y, -500.f );

	Engine::destroySingleton();
}

UTEST( UIWebView, DocumentRootHitTestingTraversesScrollableExtent ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Document Hit Test Bounds", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 400, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 300 );
	webView->getHorizontalScrollBar()->setPixelsSize( 400, 15 );

	const std::string path = Sys::getTempPath() + "eepp_uiwebview_hit_test_extent.html";
	FileSystem::fileWrite( path, R"html(
<!DOCTYPE html>
<html>
<head><style>html, body { margin: 0; padding: 0; }</style></head>
<body>
	<div style="height: 900px"></div>
	<div id="target" style="width: 220px; height: 100px; background: #abcdef"></div>
</body>
</html>
)html" );
	webView->loadURI( URI( "file://" + path ) );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );

	auto pump = [&]() {
		for ( int i = 0; i < 30; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};
	pump();

	Node* targetNode = documentScene->getRoot()->find( "target" );
	ASSERT_TRUE( targetNode != nullptr && targetNode->isWidget() );
	UIWidget* target = targetNode->asType<UIWidget>();
	ASSERT_TRUE( webView->getVerticalScrollBar()->isVisible() );
	UIRoot* documentRoot = documentScene->getRoot()->asType<UIRoot>();
	ASSERT_TRUE( documentRoot != nullptr );
	ASSERT_TRUE( documentRoot->hasChildHitTestTraversalPixelsSize() );
	EXPECT_NEAR( documentScene->getRoot()->getPixelsSize().getHeight(),
				 documentScene->getViewportPixelsSize().getHeight(), 0.5f );
	EXPECT_NEAR( documentRoot->getChildHitTestTraversalPixelsSize().getHeight(),
				 documentScene->getPixelsSize().getHeight(), 0.5f );

	webView->getVerticalScrollBar()->setValue( 1.f );
	pump();

	const Rectf targetRect = target->getScreenRect();
	const Rectf rootRect = documentScene->getRoot()->getScreenRect();
	const Rectf containerRect = webView->getContainer()->getScreenRect();
	const Vector2f hitPoint( targetRect.Left + targetRect.getWidth() * 0.5f,
							 targetRect.Top + targetRect.getHeight() * 0.5f );
	ASSERT_TRUE( containerRect.contains( hitPoint ) );
	ASSERT_FALSE( rootRect.contains( hitPoint ) );

	Node* hitNode = sceneNode->overFind( hitPoint );
	ASSERT_TRUE( hitNode != nullptr );
	EXPECT_TRUE( hitNode == target || target->isParentOf( hitNode ) );

	Engine::destroySingleton();
}

UTEST( UIWebView, VerticalScrollbarViewportDoesNotCreateHorizontalScroll ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Scrollbar Viewport Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 380, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 300 );
	webView->getHorizontalScrollBar()->setPixelsSize( 380, 15 );

	const std::string path = Sys::getTempPath() + "eepp_uiwebview_scrollbar_viewport.html";
	FileSystem::fileWrite( path, R"html(
<!DOCTYPE html>
<html>
<head>
<style>
	html, body { margin: 0; padding: 0; }
	#wide { width: 100vw; height: 1200px; background: #abcdef; }
</style>
</head>
<body><div id="wide"></div></body>
</html>
)html" );
	webView->loadURI( URI( "file://" + path ) );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );

	auto pump = [&]() {
		for ( int i = 0; i < 20; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};
	pump();

	auto wide = documentScene->getRoot()->find( "wide" );
	auto html = documentScene->getRoot()->findByType( UI_TYPE_HTML_HTML )->asType<UIWidget>();
	ASSERT_TRUE( wide != nullptr );
	ASSERT_TRUE( html != nullptr );

	EXPECT_TRUE( webView->getVerticalScrollBar()->isVisible() );
	EXPECT_FALSE( webView->getHorizontalScrollBar()->isVisible() );
	EXPECT_NEAR( documentScene->getViewportPixelsSize().getWidth(),
				 webView->getContainer()->getPixelsSize().getWidth(), 0.5f );
	EXPECT_LT( documentScene->getViewportPixelsSize().getWidth(),
			   webView->getPixelsSize().getWidth() );
	EXPECT_NEAR( wide->asType<UIWidget>()->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_GT( documentScene->getPixelsSize().getHeight(), 1000.f );
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(),
				 html->fitMinMaxSizePx( html->getPixelsSize() ).getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(),
				 html->fitMinMaxSizePx( html->getPixelsSize() ).getHeight(), 0.5f );

	Sizef stableExtent = documentScene->getPixelsSize();
	pump();
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(), stableExtent.getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(), stableExtent.getHeight(), 0.5f );

	ASSERT_TRUE( documentScene->getParent() != nullptr && documentScene->getParent()->isUINode() );
	UINode* scrollTarget = documentScene->getParent()->asType<UINode>();
	webView->getVerticalScrollBar()->setValue( 1.f );
	EXPECT_NEAR( -scrollTarget->getPixelsPosition().y +
					 webView->getContainer()->getPixelsSize().getHeight(),
				 scrollTarget->getPixelsSize().getHeight(), 1.f );

	Engine::destroySingleton();
}

UTEST( UIWebView, ExplicitWideDocumentStillCreatesHorizontalScroll ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Explicit Wide Document Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 380, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 300 );
	webView->getHorizontalScrollBar()->setPixelsSize( 380, 15 );

	const std::string path = Sys::getTempPath() + "eepp_uiwebview_explicit_wide.html";
	FileSystem::fileWrite( path, R"html(
<!DOCTYPE html>
<html>
<head><style>html, body { margin: 0; padding: 0; }</style></head>
<body><div id="wide" style="width: 600px; height: 1200px; background: #abcdef"></div></body>
</html>
)html" );
	webView->loadURI( URI( "file://" + path ) );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );

	for ( int i = 0; i < 20; i++ ) {
		win->getInput()->update();
		SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
	}

	auto wide = documentScene->getRoot()->find( "wide" );
	ASSERT_TRUE( wide != nullptr );
	EXPECT_TRUE( webView->getVerticalScrollBar()->isVisible() );
	EXPECT_TRUE( webView->getHorizontalScrollBar()->isVisible() );
	EXPECT_NEAR( wide->asType<UIWidget>()->getPixelsSize().getWidth(), 600.f, 0.5f );
	EXPECT_GE( documentScene->getPixelsSize().getWidth(), 600.f );
	EXPECT_GT( documentScene->getPixelsSize().getWidth(),
			   documentScene->getViewportPixelsSize().getWidth() );

	Engine::destroySingleton();
}

UTEST( UIWebView, ExplicitWideDocumentHorizontalScrollReachesRightEdgeAtPixelDensity2 ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Explicit Wide Document PD2 Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 2.f, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 380, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 300 );
	webView->getHorizontalScrollBar()->setPixelsSize( 380, 15 );

	const std::string path = Sys::getTempPath() + "eepp_uiwebview_explicit_wide_pd2.html";
	FileSystem::fileWrite( path, R"html(
<!DOCTYPE html>
<html>
<head><style>html, body { margin: 0; padding: 0; }</style></head>
<body><div id="wide" style="width: 600px; height: 1200px; background: #abcdef"></div></body>
</html>
)html" );
	webView->loadURI( URI( "file://" + path ) );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );

	auto pump = [&]() {
		for ( int i = 0; i < 20; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};
	pump();

	auto wide = documentScene->getRoot()->find( "wide" );
	ASSERT_TRUE( wide != nullptr );
	EXPECT_TRUE( webView->getVerticalScrollBar()->isVisible() );
	EXPECT_TRUE( webView->getHorizontalScrollBar()->isVisible() );
	EXPECT_GT( documentScene->getPixelsSize().getWidth(),
			   webView->getContainer()->getPixelsSize().getWidth() );

	webView->getHorizontalScrollBar()->setValue( 1.f );
	pump();

	ASSERT_TRUE( documentScene->getParent() != nullptr && documentScene->getParent()->isUINode() );
	UINode* scrollTarget = documentScene->getParent()->asType<UINode>();
	EXPECT_NEAR( -scrollTarget->getPixelsPosition().x +
					 webView->getContainer()->getPixelsSize().getWidth(),
				 scrollTarget->getPixelsSize().getWidth(), 1.f );

	Engine::destroySingleton();
	EE::Graphics::PixelDensity::setPixelDensity( 1.0f );
}

UTEST( UIWebView, ResponsiveDocumentShrinksAfterGrowResize ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "UIWebView Responsive Shrink Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 2.f, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 400, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 300 );
	webView->getHorizontalScrollBar()->setPixelsSize( 400, 15 );

	const std::string path = Sys::getTempPath() + "eepp_uiwebview_responsive_shrink.html";
	FileSystem::fileWrite( path, R"html(
<!DOCTYPE html>
<html>
<head><style>html, body { margin: 0; padding: 0; } #fill { width: 100vw; height: 1200px; }</style></head>
<body><div id="fill"></div></body>
</html>
)html" );
	webView->loadURI( URI( "file://" + path ) );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );

	auto pump = [&]() {
		for ( int i = 0; i < 20; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};
	pump();

	auto fill = documentScene->getRoot()->find( "fill" )->asType<UIWidget>();
	auto html = documentScene->getRoot()->findByType( UI_TYPE_HTML_HTML )->asType<UIWidget>();
	auto body = documentScene->getRoot()->findByType( UI_TYPE_HTML_BODY )->asType<UIWidget>();
	ASSERT_TRUE( fill != nullptr );
	ASSERT_TRUE( html != nullptr );
	ASSERT_TRUE( body != nullptr );
	EXPECT_NEAR( fill->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(),
				 html->fitMinMaxSizePx( html->getPixelsSize() ).getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(),
				 html->fitMinMaxSizePx( html->getPixelsSize() ).getHeight(), 0.5f );

	webView->setPixelsSize( 700, 500 );
	pump();
	const Float grownFillWidth = fill->getPixelsSize().getWidth();
	EXPECT_NEAR( grownFillWidth, documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( html->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( body->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_FALSE( webView->getHorizontalScrollBar()->isVisible() );
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(),
				 html->fitMinMaxSizePx( html->getPixelsSize() ).getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(),
				 html->fitMinMaxSizePx( html->getPixelsSize() ).getHeight(), 0.5f );

	webView->setPixelsSize( 400, 300 );
	pump();
	EXPECT_LT( fill->getPixelsSize().getWidth(), grownFillWidth );
	EXPECT_NEAR( fill->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( html->getPixelsSize().getWidth(), documentScene->getPixelsSize().getWidth(),
				 0.5f );
	EXPECT_NEAR( body->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_FALSE( webView->getHorizontalScrollBar()->isVisible() );
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(),
				 html->fitMinMaxSizePx( html->getPixelsSize() ).getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(),
				 html->fitMinMaxSizePx( html->getPixelsSize() ).getHeight(), 0.5f );

	Engine::destroySingleton();
	EE::Graphics::PixelDensity::setPixelDensity( 1.0f );
}

UTEST( UIWebView, HorizontalScrollDisappearsAfterResponsiveShrink ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Horizontal Scroll Clears Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1.f, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 400, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 300 );
	webView->getHorizontalScrollBar()->setPixelsSize( 400, 15 );

	const std::string widePath = Sys::getTempPath() + "eepp_uiwebview_wide_then_fit_wide.html";
	FileSystem::fileWrite( widePath, R"html(
<!DOCTYPE html>
<html>
<head><style>html, body { margin: 0; padding: 0; }</style></head>
<body><div id="wide" style="width: 900px; height: 1200px"></div></body>
</html>
)html" );
	const std::string fitPath = Sys::getTempPath() + "eepp_uiwebview_wide_then_fit_fit.html";
	FileSystem::fileWrite( fitPath, R"html(
<!DOCTYPE html>
<html>
<head><style>html, body { margin: 0; padding: 0; } #fit { width: 100vw; height: 1200px; }</style></head>
<body><div id="fit"></div></body>
</html>
)html" );

	auto pump = [&]() {
		for ( int i = 0; i < 30; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI( URI( "file://" + widePath ) );
	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	pump();
	ASSERT_TRUE( webView->getHorizontalScrollBar()->isVisible() );

	webView->getHorizontalScrollBar()->setValue( 1.f );
	pump();
	ASSERT_TRUE( documentScene->getParent() != nullptr && documentScene->getParent()->isUINode() );
	UINode* scrollTarget = documentScene->getParent()->asType<UINode>();
	ASSERT_LT( scrollTarget->getPixelsPosition().x, -100.f );

	webView->loadURI( URI( "file://" + fitPath ) );
	pump();

	UIWidget* fit = documentScene->getRoot()->find( "fit" )->asType<UIWidget>();
	UIWidget* html = documentScene->getRoot()->findByType( UI_TYPE_HTML_HTML )->asType<UIWidget>();
	UIWidget* body = documentScene->getRoot()->findByType( UI_TYPE_HTML_BODY )->asType<UIWidget>();
	ASSERT_TRUE( fit != nullptr );
	ASSERT_TRUE( html != nullptr );
	ASSERT_TRUE( body != nullptr );
	EXPECT_NEAR( documentScene->getRoot()->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( fit->getPixelsSize().getWidth(), documentScene->getViewportPixelsSize().getWidth(),
				 0.5f );
	EXPECT_NEAR( html->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( body->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_FALSE( webView->getHorizontalScrollBar()->isVisible() );
	EXPECT_NEAR( scrollTarget->getPixelsPosition().x, 0.f, 0.5f );

	Engine::destroySingleton();
}

UTEST( UIWebView, HiddenAndClippedWideDescendantsDoNotCreateHorizontalScroll ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Hidden Wide Extent Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1.f, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 400, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 300 );
	webView->getHorizontalScrollBar()->setPixelsSize( 400, 15 );

	const std::string path = Sys::getTempPath() + "eepp_uiwebview_hidden_wide_extent.html";
	FileSystem::fileWrite( path, R"html(
<!DOCTYPE html>
<html>
<head><style>
	html, body { margin: 0; padding: 0; }
	#hidden { display: none; width: 2000px; height: 20px; }
	#clip { width: 100%; height: 20px; overflow: hidden; }
	#clipped-wide { width: 2000px; height: 20px; }
	#tall { height: 1200px; }
</style></head>
<body>
	<div id="hidden"></div>
	<div id="clip"><div id="clipped-wide"></div></div>
	<div id="tall"></div>
</body>
</html>
)html" );
	webView->loadURI( URI( "file://" + path ) );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );

	auto pump = [&]() {
		for ( int i = 0; i < 30; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};
	pump();

	UIWidget* hidden = documentScene->getRoot()->find( "hidden" )->asType<UIWidget>();
	UIWidget* clippedWide = documentScene->getRoot()->find( "clipped-wide" )->asType<UIWidget>();
	ASSERT_TRUE( hidden != nullptr );
	ASSERT_TRUE( clippedWide != nullptr );
	EXPECT_FALSE( hidden->isVisible() );
	EXPECT_GT( hidden->getPixelsSize().getWidth(), 1000.f );
	EXPECT_GT( clippedWide->getPixelsSize().getWidth(), 1000.f );
	EXPECT_TRUE( webView->getVerticalScrollBar()->isVisible() );
	EXPECT_FALSE( webView->getHorizontalScrollBar()->isVisible() );
	EXPECT_LE( documentScene->getPixelsSize().getWidth(), webView->getPixelsSize().getWidth() );

	Engine::destroySingleton();
}

UTEST( UIWebView, VerticalScrollbarDisappearsWithoutViewportWidthOscillation ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Scrollbar Width Oscillation Test",
						WindowStyle::Default, WindowBackend::Default, 32, {}, 1.f, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 380, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 300 );
	webView->getHorizontalScrollBar()->setPixelsSize( 380, 15 );

	const std::string path = Sys::getTempPath() + "eepp_uiwebview_vscroll_oscillation.html";
	FileSystem::fileWrite( path, R"html(
<!DOCTYPE html>
<html>
<head><style>
	html, body { margin: 0; padding: 0; }
	#content { height: 520px; }
	@media only screen and (min-width: 391px) {
		#content { height: 100px; }
	}
</style></head>
<body><div id="content"></div></body>
</html>
)html" );
	webView->loadURI( URI( "file://" + path ) );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );

	auto pump = [&]() {
		for ( int i = 0; i < 30; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};
	pump();
	ASSERT_TRUE( webView->getVerticalScrollBar()->isVisible() );

	webView->setPixelsSize( 500, 500 );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 500 );
	webView->getHorizontalScrollBar()->setPixelsSize( 500, 15 );
	pump();

	bool vscrollVisible = webView->getVerticalScrollBar()->isVisible();
	Sizef viewport = documentScene->getViewportPixelsSize();
	Sizef sceneExtent = documentScene->getPixelsSize();
	pump();

	EXPECT_FALSE( webView->getVerticalScrollBar()->isVisible() );
	EXPECT_EQ( webView->getVerticalScrollBar()->isVisible(), vscrollVisible );
	EXPECT_NEAR( documentScene->getViewportPixelsSize().getWidth(), viewport.getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(), sceneExtent.getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getViewportPixelsSize().getWidth(), 500.f, 0.5f );
	EXPECT_FALSE( webView->getHorizontalScrollBar()->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIWebView, HackerNewsGrowUntilVerticalScrollbarDisappearsSettlesViewport ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "UIWebView Hacker News Scrollbar Disappear Test",
						WindowStyle::Default, WindowBackend::Default, 32, {}, 1.f, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 800, 500 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 500 );
	webView->getHorizontalScrollBar()->setPixelsSize( 800, 15 );
	webView->loadURI( URI( "file://" + Sys::getProcessPath() + "assets/html/hn_frontpage.html" ) );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );

	auto pump = [&]() {
		for ( int i = 0; i < 30; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};
	pump();
	ASSERT_TRUE( webView->getVerticalScrollBar()->isVisible() );
	ASSERT_GE( documentScene->getRoot()->querySelectorAll( ".athing" ).size(), (size_t)30 );

	webView->setPixelsSize( 2345, 5000 );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 5000 );
	webView->getHorizontalScrollBar()->setPixelsSize( 2345, 15 );
	pump();

	bool vscrollVisible = webView->getVerticalScrollBar()->isVisible();
	Sizef viewport = documentScene->getViewportPixelsSize();
	Sizef extent = documentScene->getPixelsSize();
	pump();

	EXPECT_FALSE( webView->getVerticalScrollBar()->isVisible() );
	EXPECT_EQ( webView->getVerticalScrollBar()->isVisible(), vscrollVisible );
	EXPECT_NEAR( documentScene->getViewportPixelsSize().getWidth(), viewport.getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(), extent.getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getViewportPixelsSize().getWidth(), 2345.f, 0.5f );
	EXPECT_FALSE( webView->getHorizontalScrollBar()->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIWebView, HackerNewsFrontPageBottomIsReachable ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "UIWebView Hacker News Document Extent Test",
						WindowStyle::Default, WindowBackend::Default, 32, {}, 2.f, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 800, 500 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 500 );
	webView->getHorizontalScrollBar()->setPixelsSize( 800, 15 );
	webView->loadURI( URI( "file://" + Sys::getProcessPath() + "assets/html/hn_frontpage.html" ) );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );

	auto pump = [&]() {
		for ( int i = 0; i < 30; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};
	pump();

	auto assertStoriesAreVerticallyOrdered = [&]() {
		auto stories = documentScene->getRoot()->querySelectorAll( ".athing" );
		ASSERT_GE( stories.size(), (size_t)30 );
		Float lastY = stories.front()->getPixelsPosition().y;
		for ( size_t i = 1; i < stories.size(); i++ ) {
			Float curY = stories[i]->getPixelsPosition().y;
			EXPECT_GT( curY, lastY );
			lastY = curY;
		}
	};
	assertStoriesAreVerticallyOrdered();

	UIWidget* searchInput = documentScene->getRoot()->querySelector( "form input" );
	ASSERT_TRUE( searchInput != nullptr );
	UIWidget* html = documentScene->getRoot()->findByType( UI_TYPE_HTML_HTML )->asType<UIWidget>();
	UIWidget* body = documentScene->getRoot()->findByType( UI_TYPE_HTML_BODY )->asType<UIWidget>();
	ASSERT_TRUE( html != nullptr );
	ASSERT_TRUE( body != nullptr );
	ASSERT_TRUE( webView->getVerticalScrollBar()->isVisible() );
	ASSERT_TRUE( documentScene->getParent() != nullptr && documentScene->getParent()->isUINode() );
	UINode* scrollTarget = documentScene->getParent()->asType<UINode>();

	Float viewportHeight = webView->getContainer()->getPixelsSize().getHeight();
	const Float inputBottomInDocument =
		searchInput->getScreenRect().Bottom - documentScene->getScreenRect().Top;
	EXPECT_GE( documentScene->getPixelsSize().getHeight(), inputBottomInDocument - 1.f );
	EXPECT_GT( documentScene->getPixelsSize().getHeight(), viewportHeight );
	EXPECT_GE( documentScene->getPixelsSize().getWidth(),
			   documentScene->getViewportPixelsSize().getWidth() );
	EXPECT_NEAR( documentScene->getRoot()->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( html->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(),
				 html->fitMinMaxSizePx( html->getPixelsSize() ).getHeight(), 0.5f );

	Sizef stableExtent = documentScene->getPixelsSize();
	pump();
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(), stableExtent.getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(), stableExtent.getHeight(), 0.5f );

	webView->setPixelsSize( 1000, 600 );
	pump();
	assertStoriesAreVerticallyOrdered();

	stableExtent = documentScene->getPixelsSize();
	pump();
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(), stableExtent.getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(), stableExtent.getHeight(), 0.5f );
	EXPECT_GE( documentScene->getPixelsSize().getWidth(),
			   documentScene->getViewportPixelsSize().getWidth() );
	EXPECT_NEAR( documentScene->getRoot()->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( html->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(),
				 html->fitMinMaxSizePx( html->getPixelsSize() ).getHeight(), 0.5f );
	viewportHeight = webView->getContainer()->getPixelsSize().getHeight();

	webView->setPixelsSize( 800, 500 );
	pump();
	assertStoriesAreVerticallyOrdered();

	stableExtent = documentScene->getPixelsSize();
	pump();
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(), stableExtent.getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(), stableExtent.getHeight(), 0.5f );
	EXPECT_GE( documentScene->getPixelsSize().getWidth(),
			   documentScene->getViewportPixelsSize().getWidth() );
	EXPECT_NEAR( documentScene->getRoot()->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( html->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(),
				 html->fitMinMaxSizePx( html->getPixelsSize() ).getHeight(), 0.5f );
	viewportHeight = webView->getContainer()->getPixelsSize().getHeight();

	webView->getVerticalScrollBar()->setValue( 1.f );
	pump();

	const Rectf inputRect = searchInput->getScreenRect();
	const Rectf containerRect = webView->getContainer()->getScreenRect();
	EXPECT_LE( inputRect.Bottom, containerRect.Bottom + 1.f );
	EXPECT_GT( inputRect.Bottom, containerRect.Top );
	EXPECT_NEAR( -scrollTarget->getPixelsPosition().y + viewportHeight,
				 scrollTarget->getPixelsSize().getHeight(), 1.f );
	EXPECT_NEAR( documentScene->getRoot()->getPixelsSize().getHeight(),
				 documentScene->getViewportPixelsSize().getHeight(), 0.5f );

	const Vector2f inputCenter( inputRect.Left + inputRect.getWidth() * 0.5f,
								inputRect.Top + inputRect.getHeight() * 0.5f );
	Node* hitNode = sceneNode->overFind( inputCenter );
	ASSERT_TRUE( hitNode != nullptr );
	EXPECT_TRUE( hitNode == documentScene->getRoot() ||
				 documentScene->getRoot()->isParentOf( hitNode ) );

	Engine::destroySingleton();
	EE::Graphics::PixelDensity::setPixelDensity( 1.0f );
}

UTEST( UIWebView, NavigationAfterGrowDoesNotKeepMaximizedWidthOnShrink ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "UIWebView Navigation Shrink Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1.f, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 800, 500 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 500 );
	webView->getHorizontalScrollBar()->setPixelsSize( 800, 15 );

	auto pump = [&]() {
		for ( int i = 0; i < 30; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI(
		URI( "file://" + Sys::getProcessPath() + "assets/html/hn_thread_test.html" ) );
	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	pump();

	webView->setPixelsSize( 2345, 900 );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 900 );
	webView->getHorizontalScrollBar()->setPixelsSize( 2345, 15 );
	pump();
	EXPECT_GT( documentScene->getViewportPixelsSize().getWidth(), 2000.f );

	webView->loadURI( URI( "file://" + Sys::getProcessPath() + "assets/html/hn_frontpage.html" ) );
	pump();
	ASSERT_GE( documentScene->getRoot()->querySelectorAll( ".athing" ).size(), (size_t)30 );

	webView->setPixelsSize( 800, 500 );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 500 );
	webView->getHorizontalScrollBar()->setPixelsSize( 800, 15 );
	pump();

	UIWidget* html = documentScene->getRoot()->findByType( UI_TYPE_HTML_HTML )->asType<UIWidget>();
	UIWidget* body = documentScene->getRoot()->findByType( UI_TYPE_HTML_BODY )->asType<UIWidget>();
	ASSERT_TRUE( html != nullptr );
	ASSERT_TRUE( body != nullptr );
	EXPECT_NEAR( documentScene->getViewportPixelsSize().getWidth(),
				 webView->getContainer()->getPixelsSize().getWidth(), 0.5f );
	EXPECT_LT( documentScene->getViewportPixelsSize().getWidth(), 900.f );
	EXPECT_NEAR( documentScene->getRoot()->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_LT( html->fitMinMaxSizePx( html->getPixelsSize() ).getWidth(), 900.f );
	EXPECT_LT( body->fitMinMaxSizePx( body->getPixelsSize() ).getWidth(), 900.f );
	EXPECT_LT( documentScene->getPixelsSize().getWidth(), 900.f );
	EXPECT_FALSE( webView->getHorizontalScrollBar()->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIWebView, NavigationFromTallToShortShrinksDocumentExtent ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Navigation Extent Shrink Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 400, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string tallPath = Sys::getTempPath() + "eepp_uiwebview_tall.html";
	FileSystem::fileWrite( tallPath, R"html(
<!DOCTYPE html>
<html>
<head><style>html, body { margin: 0; padding: 0; }</style></head>
<body><div id="content" style="height: 1800px; width: 100%;"></div></body>
</html>
)html" );
	const std::string shortPath = Sys::getTempPath() + "eepp_uiwebview_short.html";
	FileSystem::fileWrite( shortPath, R"html(
<!DOCTYPE html>
<html>
<head><style>html, body { margin: 0; padding: 0; }</style></head>
<body><div id="content" style="height: 120px; width: 100%;"></div></body>
</html>
)html" );

	auto pump = [&]() {
		for ( int i = 0; i < 20; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI( URI( "file://" + tallPath ) );
	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	pump();
	ASSERT_TRUE( documentScene->getParent() != nullptr && documentScene->getParent()->isUINode() );
	UINode* scrollTarget = documentScene->getParent()->asType<UINode>();
	EXPECT_GT( scrollTarget->getPixelsSize().getHeight(), 1700.f );

	webView->loadURI( URI( "file://" + shortPath ) );
	pump();
	EXPECT_NEAR( scrollTarget->getPixelsSize().getHeight(),
				 webView->getContainer()->getPixelsSize().getHeight(), 1.f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(),
				 webView->getContainer()->getPixelsSize().getHeight(), 1.f );
	EXPECT_FALSE( webView->getVerticalScrollBar()->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIWebView, AsyncCSSCanShrinkDocumentExtent ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView CSS Extent Shrink Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 400, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string path = Sys::getTempPath() + "eepp_uiwebview_css_shrink.html";
	FileSystem::fileWrite( path, R"html(
<!DOCTYPE html>
<html>
<head><style>html, body { margin: 0; padding: 0; } #content { height: 1800px; }</style></head>
<body><div id="content"></div></body>
</html>
)html" );

	auto pump = [&]() {
		for ( int i = 0; i < 20; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI( URI( "file://" + path ) );
	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	pump();
	ASSERT_TRUE( documentScene->getParent() != nullptr && documentScene->getParent()->isUINode() );
	UINode* scrollTarget = documentScene->getParent()->asType<UINode>();
	EXPECT_GT( scrollTarget->getPixelsSize().getHeight(), 1700.f );

	documentScene->combineStyleSheet( "#content { height: 120px; }", true );
	pump();
	EXPECT_NEAR( scrollTarget->getPixelsSize().getHeight(),
				 webView->getContainer()->getPixelsSize().getHeight(), 1.f );
	EXPECT_NEAR( documentScene->getPixelsSize().getHeight(),
				 webView->getContainer()->getPixelsSize().getHeight(), 1.f );
	EXPECT_FALSE( webView->getVerticalScrollBar()->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIWebView, CoalescesViewportResizeDocumentMetrics ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "UIWebView Resize Metrics Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 800, 500 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->loadURI( URI( "file://" + Sys::getProcessPath() + "assets/html/hn_frontpage.html" ) );

	auto pump = [&]() {
		for ( int i = 0; i < 30; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};
	pump();
	ASSERT_GE( webView->getDocumentSceneNode()->getRoot()->querySelectorAll( ".athing" ).size(),
			   (size_t)30 );

	UILayout::resetMetrics();
	webView->setPixelsSize( 900, 530 );
	webView->setPixelsSize( 700, 480 );
	webView->setPixelsSize( 1000, 600 );
	webView->setPixelsSize( 800, 500 );
	for ( int frame = 0; frame < 6; frame++ ) {
		win->getInput()->update();
		SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
	}

	auto metrics = UILayout::getMetrics();
	UILayout::setMetricsEnabled( false );
	EXPECT_LE( metrics.treeUpdates, (Uint64)4 );
	EXPECT_EQ( metrics.synchronousUpdates, (Uint64)0 );
	EXPECT_EQ( metrics.richTextRebuilds, (Uint64)0 );

	Engine::destroySingleton();
}

UTEST( UIWebView, HackerNewsSingleStepRestoreSettlesViewportInOneFrame ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "UIWebView Single Step Restore Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1.f, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 800, 500 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 500 );
	webView->getHorizontalScrollBar()->setPixelsSize( 800, 15 );
	webView->loadURI( URI( "file://" + Sys::getProcessPath() + "assets/html/hn_frontpage.html" ) );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );

	auto pump = [&]() {
		for ( int i = 0; i < 30; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};
	auto updateOnce = [&]() {
		win->getInput()->update();
		SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
	};

	pump();
	ASSERT_GE( documentScene->getRoot()->querySelectorAll( ".athing" ).size(), (size_t)30 );

	webView->setPixelsSize( 2345, 900 );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 900 );
	webView->getHorizontalScrollBar()->setPixelsSize( 2345, 15 );
	updateOnce();

	EXPECT_NEAR( documentScene->getViewportPixelsSize().getWidth(),
				 webView->getContainer()->getPixelsSize().getWidth(), 0.5f );
	EXPECT_GT( documentScene->getViewportPixelsSize().getWidth(), 2000.f );

	webView->setPixelsSize( 800, 500 );
	webView->getVerticalScrollBar()->setPixelsSize( 15, 500 );
	webView->getHorizontalScrollBar()->setPixelsSize( 800, 15 );
	updateOnce();

	UIWidget* html = documentScene->getRoot()->findByType( UI_TYPE_HTML_HTML )->asType<UIWidget>();
	UIWidget* body = documentScene->getRoot()->findByType( UI_TYPE_HTML_BODY )->asType<UIWidget>();
	UIWidget* center = documentScene->getRoot()->querySelector( "center" )->asType<UIWidget>();
	UIWidget* table = documentScene->getRoot()->find( "hnmain" )->asType<UIWidget>();
	ASSERT_TRUE( html != nullptr );
	ASSERT_TRUE( body != nullptr );
	ASSERT_TRUE( center != nullptr );
	ASSERT_TRUE( table != nullptr );
	EXPECT_NEAR( documentScene->getViewportPixelsSize().getWidth(),
				 webView->getContainer()->getPixelsSize().getWidth(), 0.5f );
	EXPECT_LT( documentScene->getViewportPixelsSize().getWidth(), 900.f );
	EXPECT_LT( html->getPixelsSize().getWidth(), 900.f );
	EXPECT_LT( body->getPixelsSize().getWidth(), 900.f );
	EXPECT_LT( center->getPixelsSize().getWidth(), 900.f );
	EXPECT_LT( table->getPixelsSize().getWidth(), 900.f );
	EXPECT_LT( documentScene->getPixelsSize().getWidth(), webView->getPixelsSize().getWidth() );
	EXPECT_FALSE( webView->getHorizontalScrollBar()->isVisible() );

	Engine::destroySingleton();
}

UTEST( UIWebView, LayoutDrivenResizeKeepsDocumentRootAtViewport ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "UIWebView Layout Driven Resize Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1.f, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWidget* vbox = sceneNode->loadLayoutFromString( R"xml(
	<vbox layout_width="match_parent" layout_height="match_parent">
		<hbox layout_width="match_parent" layout_height="wrap_content">
			<PushButton id="backbtn" text="Back" />
			<PushButton id="fwdbtn" text="Forward" />
			<TextInput id="url_bar" layout_width="0" layout_weight="1" />
		</hbox>
		<WebView id="webview" layout_width="match_parent" layout_height="0" layout_weight="1" />
	</vbox>
	)xml" );
	ASSERT_TRUE( vbox != nullptr );

	UIWebView* webView = vbox->find( "webview" )->asType<UIWebView>();
	ASSERT_TRUE( webView != nullptr );

	auto pump = [&]() {
		for ( int i = 0; i < 30; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	sceneNode->getRoot()->setPixelsSize( 800, 600 );
	pump();

	webView->loadURI(
		URI( "file://" + Sys::getProcessPath() + "assets/html/hn_thread_test.html" ) );
	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	pump();

	sceneNode->getRoot()->setPixelsSize( 2345, 900 );
	pump();
	EXPECT_GT( documentScene->getViewportPixelsSize().getWidth(), 2000.f );
	EXPECT_NEAR( documentScene->getRoot()->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );

	webView->loadURI( URI( "file://" + Sys::getProcessPath() + "assets/html/hn_frontpage.html" ) );
	pump();
	ASSERT_GE( documentScene->getRoot()->querySelectorAll( ".athing" ).size(), (size_t)30 );

	sceneNode->getRoot()->setPixelsSize( 800, 600 );
	pump();

	UIWidget* html = documentScene->getRoot()->findByType( UI_TYPE_HTML_HTML )->asType<UIWidget>();
	UIWidget* body = documentScene->getRoot()->findByType( UI_TYPE_HTML_BODY )->asType<UIWidget>();
	ASSERT_TRUE( html != nullptr );
	ASSERT_TRUE( body != nullptr );
	EXPECT_LT( documentScene->getViewportPixelsSize().getWidth(), 900.f );
	EXPECT_NEAR( documentScene->getRoot()->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_NEAR( html->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_LE( body->getPixelsSize().getWidth(),
			   documentScene->getViewportPixelsSize().getWidth() + 0.5f );
	EXPECT_GT( body->getPixelsSize().getWidth(),
			   documentScene->getViewportPixelsSize().getWidth() - 40.f );
	EXPECT_GE( documentScene->getPixelsSize().getWidth(),
			   documentScene->getViewportPixelsSize().getWidth() );
	EXPECT_NEAR( documentScene->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 0.5f );
	EXPECT_FALSE( webView->getHorizontalScrollBar()->isVisible() );
	EXPECT_GT( documentScene->getPixelsSize().getHeight(),
			   documentScene->getViewportPixelsSize().getHeight() );

	webView->getVerticalScrollBar()->setValue( 1.f );
	webView->getHorizontalScrollBar()->setValue( 1.f );
	pump();
	ASSERT_TRUE( documentScene->getParent() != nullptr && documentScene->getParent()->isUINode() );
	UINode* scrollTarget = documentScene->getParent()->asType<UINode>();
	EXPECT_NEAR( -scrollTarget->getPixelsPosition().y +
					 webView->getContainer()->getPixelsSize().getHeight(),
				 scrollTarget->getPixelsSize().getHeight(), 1.f );
	if ( webView->getHorizontalScrollBar()->isVisible() ) {
		EXPECT_NEAR( -scrollTarget->getPixelsPosition().x +
						 webView->getContainer()->getPixelsSize().getWidth(),
					 scrollTarget->getPixelsSize().getWidth(), 1.f );
	}

	Engine::destroySingleton();
}

UTEST( UIWebView, DocumentScenesIsolateStylesUriAndLookup ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "UIWebView Isolation Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWidget* hostWidget = UIWidget::New();
	hostWidget->setClass( "shared" );
	hostWidget->setParent( sceneNode->getRoot() );
	sceneNode->setStyleSheet( ".shared { background-color: #00ff00; }" );
	EXPECT_TRUE( hostWidget->getBackgroundColor() == Color( "#00ff00" ) );

	UIWebView* webViewA = UIWebView::New();
	webViewA->setParent( sceneNode->getRoot() );
	webViewA->setPixelsSize( 300, 200 );
	webViewA->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	UIWebView* webViewB = UIWebView::New();
	webViewB->setParent( sceneNode->getRoot() );
	webViewB->setPixelsSize( 300, 200 );
	webViewB->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string pathA = Sys::getTempPath() + "eepp_uiwebview_doc_a.html";
	const std::string pathB = Sys::getTempPath() + "eepp_uiwebview_doc_b.html";
	const std::string pathA2 = Sys::getTempPath() + "eepp_uiwebview_doc_a2.html";
	FileSystem::fileWrite( pathA, R"html(
<html><head><style>.shared { background-color: #ff0000; }</style></head>
<body><div id="target-a" class="shared" style="height:20px"></div></body></html>
)html" );
	FileSystem::fileWrite( pathB, R"html(
<html><head><style>.shared { background-color: #0000ff; }</style></head>
<body><div id="target-b" class="shared" style="height:20px"></div></body></html>
)html" );
	FileSystem::fileWrite( pathA2, R"html(
<html><head><style>.shared { background-color: #ffff00; }</style></head>
<body><div id="target-a2" class="shared" style="height:20px"></div></body></html>
)html" );

	webViewA->loadURI( URI( "file://" + pathA ) );
	webViewB->loadURI( URI( "file://" + pathB ) );

	auto pump = [&]() {
		for ( int i = 0; i < 10; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};
	pump();

	UISceneNode* docA = webViewA->getDocumentSceneNode();
	UISceneNode* docB = webViewB->getDocumentSceneNode();
	ASSERT_TRUE( docA != nullptr );
	ASSERT_TRUE( docB != nullptr );
	ASSERT_TRUE( docA != docB );

	auto targetA = docA->getRoot()->find( "target-a" );
	auto targetB = docB->getRoot()->find( "target-b" );
	ASSERT_TRUE( targetA != nullptr );
	ASSERT_TRUE( targetB != nullptr );
	EXPECT_TRUE( targetA->asType<UIWidget>()->getBackgroundColor() == Color( "#ff0000" ) );
	EXPECT_TRUE( targetB->asType<UIWidget>()->getBackgroundColor() == Color( "#0000ff" ) );
	EXPECT_TRUE( sceneNode->getRoot()->findByType( UI_TYPE_HTML_HTML ) == nullptr );
	EXPECT_TRUE( sceneNode->getRoot()->find( "target-a" ) == nullptr );
	EXPECT_TRUE( docA->getReferer().toString().find( "eepp_uiwebview_doc_a" ) !=
				 std::string::npos );
	EXPECT_TRUE( docB->getReferer().toString().find( "eepp_uiwebview_doc_b" ) !=
				 std::string::npos );

	webViewA->loadURI( URI( "file://" + pathA2 ) );
	pump();

	auto targetA2 = docA->getRoot()->find( "target-a2" );
	ASSERT_TRUE( targetA2 != nullptr );
	EXPECT_TRUE( targetA2->asType<UIWidget>()->getBackgroundColor() == Color( "#ffff00" ) );
	targetB = docB->getRoot()->find( "target-b" );
	ASSERT_TRUE( targetB != nullptr );
	EXPECT_TRUE( targetB->asType<UIWidget>()->getBackgroundColor() == Color( "#0000ff" ) );
	EXPECT_TRUE( docB->getReferer().toString().find( "eepp_uiwebview_doc_b" ) !=
				 std::string::npos );

	Engine::destroySingleton();
}

UTEST( UIWebView, DocumentScenesIsolateAuthorFontFaces ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "UIWebView Font Face Isolation Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webViewA = UIWebView::New();
	webViewA->setParent( sceneNode->getRoot() );
	webViewA->setPixelsSize( 300, 200 );
	webViewA->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	UIWebView* webViewB = UIWebView::New();
	webViewB->setParent( sceneNode->getRoot() );
	webViewB->setPixelsSize( 300, 200 );
	webViewB->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string processPath( Sys::getProcessPath() );
	const std::string pathA = Sys::getTempPath() + "eepp_uiwebview_font_doc_a.html";
	const std::string pathB = Sys::getTempPath() + "eepp_uiwebview_font_doc_b.html";
	const std::string pathA2 = Sys::getTempPath() + "eepp_uiwebview_font_doc_a2.html";
	FileSystem::fileWrite( pathA,
						   "<html><head><style>"
						   "@font-face { font-family: 'SharedDocFace'; src: url('file://" +
							   processPath +
							   "../assets/fonts/NotoSans-Regular.ttf'); }"
							   "#target-a { font-family: 'SharedDocFace'; }"
							   "</style></head><body><span id='target-a'>A</span></body></html>" );
	FileSystem::fileWrite( pathB,
						   "<html><head><style>"
						   "@font-face { font-family: 'SharedDocFace'; src: url('file://" +
							   processPath +
							   "../assets/fonts/DejaVuSansMono.ttf'); }"
							   "#target-b { font-family: 'SharedDocFace'; }"
							   "</style></head><body><span id='target-b'>B</span></body></html>" );
	FileSystem::fileWrite(
		pathA2, "<html><head><style>"
				"@font-face { font-family: 'SharedDocFace'; src: url('file://" +
					processPath +
					"../assets/fonts/DejaVuSansMono.ttf'); }"
					"#target-a2 { font-family: 'SharedDocFace'; }"
					"</style></head><body><span id='target-a2'>A2</span></body></html>" );

	webViewA->loadURI( URI( "file://" + pathA ) );
	webViewB->loadURI( URI( "file://" + pathB ) );

	auto pump = [&]() {
		for ( int i = 0; i < 10; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};
	pump();

	UISceneNode* docA = webViewA->getDocumentSceneNode();
	UISceneNode* docB = webViewB->getDocumentSceneNode();
	ASSERT_TRUE( docA != nullptr );
	ASSERT_TRUE( docB != nullptr );
	Font* fontA = docA->getFontFromNamesList( "SharedDocFace" );
	Font* fontB = docB->getFontFromNamesList( "SharedDocFace" );
	ASSERT_TRUE( fontA != nullptr );
	ASSERT_TRUE( fontB != nullptr );
	EXPECT_TRUE( fontA->loaded() );
	EXPECT_TRUE( fontB->loaded() );
	EXPECT_NE( fontA, fontB );
	EXPECT_EQ( nullptr, FontManager::instance()->getByName( "SharedDocFace" ) );

	webViewA->loadURI( URI( "file://" + pathA2 ) );
	pump();

	Font* reloadedFontA = docA->getFontFromNamesList( "SharedDocFace" );
	Font* stableFontB = docB->getFontFromNamesList( "SharedDocFace" );
	ASSERT_TRUE( reloadedFontA != nullptr );
	ASSERT_TRUE( stableFontB != nullptr );
	EXPECT_NE( reloadedFontA, fontA );
	EXPECT_EQ( stableFontB, fontB );
	EXPECT_NE( reloadedFontA, stableFontB );

	Engine::destroySingleton();
}

UTEST( UIWebView, NewerNavigationSupersedesStartedLoad ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "UIWebView Stale Navigation Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 300, 200 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string oldPath = Sys::getTempPath() + "eepp_uiwebview_stale_old.html";
	const std::string newPath = Sys::getTempPath() + "eepp_uiwebview_stale_new.html";
	FileSystem::fileWrite( oldPath, "<html><body><div id='old-doc'></div></body></html>" );
	FileSystem::fileWrite( newPath, "<html><body><div id='new-doc'></div></body></html>" );

	bool redirected = false;
	webView->onNavigationStarted( [&]( const URI& uri ) {
		if ( !redirected && uri.toString().find( "stale_old" ) != std::string::npos ) {
			redirected = true;
			webView->loadURI( URI( "file://" + newPath ) );
		}
	} );

	webView->loadURI( URI( "file://" + oldPath ) );
	for ( int i = 0; i < 10; i++ ) {
		win->getInput()->update();
		SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
	}

	UISceneNode* doc = webView->getDocumentSceneNode();
	ASSERT_TRUE( doc != nullptr );
	EXPECT_TRUE( doc->getRoot()->find( "old-doc" ) == nullptr );
	EXPECT_TRUE( doc->getRoot()->find( "new-doc" ) != nullptr );
	EXPECT_TRUE( webView->getCurrentURI().toString().find( "stale_new" ) != std::string::npos );

	Engine::destroySingleton();
}
