#include "utest.hpp"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/network/tcplistener.hpp>
#include <eepp/network/tcpsocket.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilayout.hpp>
#include <eepp/ui/uinodedrawable.hpp>
#include <eepp/ui/uiroot.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitextspan.hpp>
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

static bool readHttpRequestHeaders( TcpSocket& client ) {
	std::string request;
	char buffer[1024];
	std::size_t received = 0;
	while ( request.find( "\r\n\r\n" ) == std::string::npos ) {
		Socket::Status st = client.receive( buffer, sizeof( buffer ), received );
		if ( st != Socket::Done )
			return false;
		request.append( buffer, received );
	}
	return true;
}

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

	const Rectf targetRect = target->getWorldBounds();
	const Rectf rootRect = documentRoot->getWorldBounds();
	const Rectf containerRect = webView->getContainer()->getScreenRect();
	const Vector2f hitPoint( targetRect.Left + targetRect.getWidth() * 0.5f,
							 targetRect.Top + targetRect.getHeight() * 0.5f );
	const Vector2f rootLocalHitPoint = documentRoot->convertToNodeSpace( hitPoint );
	ASSERT_GT( rootLocalHitPoint.y, documentRoot->getPixelsSize().getHeight() );
	ASSERT_TRUE( containerRect.contains( hitPoint ) );
	ASSERT_FALSE( rootRect.contains( hitPoint ) );

	Node* hitNode = sceneNode->overFind( hitPoint );
	ASSERT_TRUE( hitNode != nullptr );
	EXPECT_TRUE( hitNode == target || target->isParentOf( hitNode ) );

	Engine::destroySingleton();
}

UTEST( UIWebView, FontSizeEmDoesNotCompoundOnViewportRelayout ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Font Size Em Relayout", WindowStyle::Default,
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
	webView->setPixelsSize( 420, 260 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string path = Sys::getTempPath() + "eepp_uiwebview_font_size_em.html";
	FileSystem::fileWrite( path, R"html(
<!DOCTYPE html>
<html>
<head>
<style>
	html, body { margin: 0; padding: 0; }
	body { font-size: 16px; }
	h1 { font-size: 2.5em; margin: 0.5em 0; }
	h2 { font-size: 250%; margin: 0.5em 0; }
</style>
</head>
<body>
	<h1><span id="title-em">Title em</span></h1>
	<h2><span id="title-percent">Title percent</span></h2>
	<div style="height: 900px"></div>
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

	Node* titleEm = documentScene->getRoot()->find( "title-em" );
	Node* titlePercent = documentScene->getRoot()->find( "title-percent" );
	ASSERT_TRUE( titleEm != nullptr && titleEm->isType( UI_TYPE_TEXTSPAN ) );
	ASSERT_TRUE( titlePercent != nullptr && titlePercent->isType( UI_TYPE_TEXTSPAN ) );
	EXPECT_NEAR( titleEm->asType<UITextSpan>()->getFontSize(), 40.f, 1.f );
	EXPECT_NEAR( titlePercent->asType<UITextSpan>()->getFontSize(), 40.f, 1.f );

	for ( int i = 0; i < 4; i++ ) {
		webView->setPixelsSize( 520 + i * 20, 320 + i * 10 );
		pump();
		webView->setPixelsSize( 420, 260 );
		pump();
		EXPECT_NEAR( titleEm->asType<UITextSpan>()->getFontSize(), 40.f, 1.f );
		EXPECT_NEAR( titlePercent->asType<UITextSpan>()->getFontSize(), 40.f, 1.f );
	}

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

UTEST( UIWebView, FixedPositionStaysPinnedToViewportWhenScrolled ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Fixed Position Viewport Test", WindowStyle::Default,
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
	webView->setPixelsPosition( 50, 40 );
	webView->setPixelsSize( 320, 220 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->setViewType( ScrollViewType::Overlay );

	const std::string path = Sys::getTempPath() + "eepp_uiwebview_fixed_viewport.html";
	FileSystem::fileWrite( path, R"html(
<!DOCTYPE html>
<html>
<head><style>
html, body { margin: 0; padding: 0; }
#fixed { position: fixed; left: 30px; top: 40px; width: 80px; height: 30px; background: #f00; }
#spacer { height: 1400px; }
</style></head>
<body><div id="fixed"></div><div id="spacer"></div></body>
</html>
)html" );

	auto pump = [&]( int frames ) {
		for ( int i = 0; i < frames; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI( URI( "file://" + path ) );
	pump( 40 );
	ASSERT_TRUE( webView->getVerticalScrollBar()->isVisible() );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	Node* fixedNode = documentScene->getRoot()->find( "fixed" );
	ASSERT_TRUE( fixedNode != nullptr && fixedNode->isWidget() );
	UIWidget* fixed = fixedNode->asType<UIWidget>();

	Vector2f beforeScroll = fixed->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( 80.f, beforeScroll.x, 1.f );
	EXPECT_NEAR( 80.f, beforeScroll.y, 1.f );

	webView->getVerticalScrollBar()->setValue( 0.5f );
	pump( 4 );

	Vector2f afterScroll = fixed->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( beforeScroll.x, afterScroll.x, 1.f );
	EXPECT_NEAR( beforeScroll.y, afterScroll.y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIWebView, StickyPositionUsesWebViewViewportWhenScrolled ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Sticky Position Viewport Test", WindowStyle::Default,
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
	webView->setPixelsPosition( 50, 40 );
	webView->setPixelsSize( 320, 220 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	webView->setViewType( ScrollViewType::Overlay );

	const std::string path = Sys::getTempPath() + "eepp_uiwebview_sticky_viewport.html";
	FileSystem::fileWrite( path, R"html(
<!DOCTYPE html>
<html>
<head><style>
html, body { margin: 0; padding: 0; }
#sticky { position: sticky; top: 20px; margin-top: 100px; width: 80px; height: 30px; background: #0f0; }
#spacer { height: 1400px; }
</style></head>
<body><div id="sticky"></div><div id="spacer"></div></body>
</html>
)html" );

	auto pump = [&]( int frames ) {
		for ( int i = 0; i < frames; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI( URI( "file://" + path ) );
	pump( 40 );
	ASSERT_TRUE( webView->getVerticalScrollBar()->isVisible() );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	Node* stickyNode = documentScene->getRoot()->find( "sticky" );
	ASSERT_TRUE( stickyNode != nullptr && stickyNode->isWidget() );
	UIWidget* sticky = stickyNode->asType<UIWidget>();

	Float maxScrollY = webView->getScrollView()->getPixelsSize().getHeight() -
					   webView->getContainer()->getPixelsSize().getHeight();
	ASSERT_GT( maxScrollY, 200.f );

	webView->getVerticalScrollBar()->setValue( 50.f / maxScrollY );
	pump( 4 );
	Vector2f beforeStick = sticky->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( 90.f, beforeStick.y, 1.f );

	webView->getVerticalScrollBar()->setValue( 150.f / maxScrollY );
	pump( 4 );
	Vector2f afterStick = sticky->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( 60.f, afterStick.y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIWebView, DeferredLocalCSSIgnoredAfterNavigation ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Deferred CSS Stale Navigation Test",
						WindowStyle::Default, WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );
	auto cssThreadPool = ThreadPool::createShared( 1 );
	sceneNode->setThreadPool( cssThreadPool );

	std::mutex cssMutex;
	std::condition_variable cssCv;
	bool cssWorkerStarted = false;
	bool releaseCSSWorker = false;
	cssThreadPool->run( [&] {
		std::unique_lock<std::mutex> lock( cssMutex );
		cssWorkerStarted = true;
		cssCv.notify_one();
		cssCv.wait( lock, [&] { return releaseCSSWorker; } );
	} );
	{
		std::unique_lock<std::mutex> lock( cssMutex );
		cssCv.wait( lock, [&] { return cssWorkerStarted; } );
	}

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 400, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string staleCSSPath = Sys::getTempPath() + "eepp_uiwebview_stale_deferred.css";
	const std::string oldPath = Sys::getTempPath() + "eepp_uiwebview_stale_deferred_old.html";
	const std::string newPath = Sys::getTempPath() + "eepp_uiwebview_stale_deferred_new.html";
	FileSystem::fileWrite( staleCSSPath, "#probe { width: 700px; height: 40px; }\n" );
	FileSystem::fileWrite(
		oldPath, "<!DOCTYPE html><html><head><link rel='stylesheet' href='file://" + staleCSSPath +
					 "'><style>#probe { width: 80px; height: 40px; }</style></head>"
					 "<body><div id='old-doc'></div><div id='probe'></div></body></html>" );
	FileSystem::fileWrite(
		newPath,
		"<!DOCTYPE html><html><head><style>#probe { width: 120px; height: 40px; }</style></head>"
		"<body><div id='new-doc'></div><div id='probe'></div></body></html>" );

	auto pump = [&]( int frames ) {
		for ( int i = 0; i < frames; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI( URI( "file://" + oldPath ) );
	pump( 2 );
	webView->loadURI( URI( "file://" + newPath ) );
	{
		std::unique_lock<std::mutex> lock( cssMutex );
		releaseCSSWorker = true;
	}
	cssCv.notify_one();
	pump( 80 );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	EXPECT_TRUE( documentScene->getRoot()->find( "old-doc" ) == nullptr );
	EXPECT_TRUE( documentScene->getRoot()->find( "new-doc" ) != nullptr );
	Node* probeNode = documentScene->getRoot()->find( "probe" );
	ASSERT_TRUE( probeNode != nullptr && probeNode->isWidget() );
	EXPECT_NEAR( probeNode->asType<UIWidget>()->getPixelsSize().getWidth(), 120.f, 0.5f );

	Engine::destroySingleton();
}

UTEST( UIWebView, RemoteCSSIgnoredAfterNavigation ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Remote CSS Stale Navigation Test",
						WindowStyle::Default, WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );
	std::atomic<bool> serverOk{ false };
	std::mutex serverMutex;
	std::condition_variable serverCv;
	bool requestReceived = false;
	bool releaseResponse = false;
	std::thread server( [&] {
		TcpSocket client;
		if ( listener.accept( client ) != Socket::Done )
			return;

		if ( !readHttpRequestHeaders( client ) )
			return;

		{
			std::unique_lock<std::mutex> lock( serverMutex );
			requestReceived = true;
		}
		serverCv.notify_one();
		{
			std::unique_lock<std::mutex> lock( serverMutex );
			serverCv.wait( lock, [&] { return releaseResponse; } );
		}

		const std::string body = "#probe { width: 700px; height: 40px; }\n";
		const std::string response = "HTTP/1.1 200 OK\r\n"
									 "Content-Type: text/css\r\n"
									 "Content-Length: " +
									 String::toString( static_cast<Uint64>( body.size() ) ) +
									 "\r\nConnection: close\r\n"
									 "\r\n" +
									 body;
		serverOk = client.send( response.data(), response.size() ) == Socket::Done;
		client.disconnect();
	} );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 400, 300 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string oldPath = Sys::getTempPath() + "eepp_uiwebview_remote_css_old.html";
	const std::string newPath = Sys::getTempPath() + "eepp_uiwebview_remote_css_new.html";
	const std::string cssURL =
		String::format( "http://127.0.0.1:%u/stale.css", listener.getLocalPort() );
	FileSystem::fileWrite(
		oldPath, "<!DOCTYPE html><html><head><link rel='stylesheet' href='" + cssURL +
					 "'><style>#probe { width: 80px; height: 40px; }</style></head>"
					 "<body><div id='old-doc'></div><div id='probe'></div></body></html>" );
	FileSystem::fileWrite(
		newPath,
		"<!DOCTYPE html><html><head><style>#probe { width: 120px; height: 40px; }</style></head>"
		"<body><div id='new-doc'></div><div id='probe'></div></body></html>" );

	auto pump = [&]( int frames ) {
		for ( int i = 0; i < frames; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI( URI( "file://" + oldPath ) );
	{
		std::unique_lock<std::mutex> lock( serverMutex );
		serverCv.wait( lock, [&] { return requestReceived; } );
	}
	webView->loadURI( URI( "file://" + newPath ) );
	pump( 10 );
	{
		std::unique_lock<std::mutex> lock( serverMutex );
		releaseResponse = true;
	}
	serverCv.notify_one();

	server.join();
	listener.close();
	EXPECT_TRUE( serverOk );
	pump( 80 );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	EXPECT_TRUE( documentScene->getRoot()->find( "old-doc" ) == nullptr );
	EXPECT_TRUE( documentScene->getRoot()->find( "new-doc" ) != nullptr );
	Node* probeNode = documentScene->getRoot()->find( "probe" );
	ASSERT_TRUE( probeNode != nullptr && probeNode->isWidget() );
	EXPECT_NEAR( probeNode->asType<UIWidget>()->getPixelsSize().getWidth(), 120.f, 0.5f );

	Engine::destroySingleton();
}

UTEST( UIWebView, RemoteImageIgnoredAfterNavigation ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Remote Image Stale Navigation Test",
						WindowStyle::Default, WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	std::string imageData;
	const std::string imagePath =
		Sys::getProcessPath() + "assets/html/inline_block_wrap_files/88x31.png";
	ASSERT_TRUE( FileSystem::fileGet( imagePath, imageData ) );
	ASSERT_FALSE( imageData.empty() );

	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );
	std::atomic<bool> serverOk{ false };
	std::mutex serverMutex;
	std::condition_variable serverCv;
	bool requestReceived = false;
	bool releaseResponse = false;
	std::thread server( [&] {
		TcpSocket client;
		if ( listener.accept( client ) != Socket::Done )
			return;

		if ( !readHttpRequestHeaders( client ) )
			return;

		{
			std::unique_lock<std::mutex> lock( serverMutex );
			requestReceived = true;
		}
		serverCv.notify_one();
		{
			std::unique_lock<std::mutex> lock( serverMutex );
			serverCv.wait( lock, [&] { return releaseResponse; } );
		}

		const std::string response = "HTTP/1.1 200 OK\r\n"
									 "Content-Type: image/png\r\n"
									 "Content-Length: " +
									 String::toString( static_cast<Uint64>( imageData.size() ) ) +
									 "\r\nConnection: close\r\n"
									 "\r\n" +
									 imageData;
		serverOk = client.send( response.data(), response.size() ) == Socket::Done;
		client.disconnect();
	} );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 300, 200 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string oldPath = Sys::getTempPath() + "eepp_uiwebview_remote_image_old.html";
	const std::string newPath = Sys::getTempPath() + "eepp_uiwebview_remote_image_new.html";
	const std::string imageURL =
		String::format( "http://127.0.0.1:%u/stale.png", listener.getLocalPort() );
	FileSystem::fileWrite(
		oldPath, "<!DOCTYPE html><html><body><div id='old-doc'></div><img id='probe' src='" +
					 imageURL + "'></body></html>" );
	FileSystem::fileWrite(
		newPath, "<!DOCTYPE html><html><body><div id='new-doc'></div><img id='probe' src='file://" +
					 imagePath + "'></body></html>" );

	auto pump = [&]( int frames ) {
		for ( int i = 0; i < frames; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI( URI( "file://" + oldPath ) );
	{
		std::unique_lock<std::mutex> lock( serverMutex );
		serverCv.wait( lock, [&] { return requestReceived; } );
	}
	webView->loadURI( URI( "file://" + newPath ) );
	pump( 10 );
	{
		std::unique_lock<std::mutex> lock( serverMutex );
		releaseResponse = true;
	}
	serverCv.notify_one();

	server.join();
	listener.close();
	EXPECT_TRUE( serverOk );
	pump( 80 );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	EXPECT_TRUE( documentScene->getRoot()->find( "old-doc" ) == nullptr );
	EXPECT_TRUE( documentScene->getRoot()->find( "new-doc" ) != nullptr );
	Node* probeNode = documentScene->getRoot()->find( "probe" );
	ASSERT_TRUE( probeNode != nullptr && probeNode->isType( UI_TYPE_HTML_IMAGE ) );
	EXPECT_TRUE( probeNode->asType<UIImage>()->getDrawable() != nullptr );

	Engine::destroySingleton();
}

UTEST( UIWebView, RemoteBackgroundImageIgnoredAfterNavigation ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 640, 480, "UIWebView Remote Background Image Stale Navigation Test",
						WindowStyle::Default, WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	std::string imageData;
	const std::string imagePath =
		Sys::getProcessPath() + "assets/html/inline_block_wrap_files/88x31.png";
	ASSERT_TRUE( FileSystem::fileGet( imagePath, imageData ) );
	ASSERT_FALSE( imageData.empty() );

	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );
	std::atomic<bool> serverOk{ false };
	std::mutex serverMutex;
	std::condition_variable serverCv;
	bool requestReceived = false;
	bool releaseResponse = false;
	std::thread server( [&] {
		TcpSocket client;
		if ( listener.accept( client ) != Socket::Done )
			return;

		if ( !readHttpRequestHeaders( client ) )
			return;

		{
			std::unique_lock<std::mutex> lock( serverMutex );
			requestReceived = true;
		}
		serverCv.notify_one();
		{
			std::unique_lock<std::mutex> lock( serverMutex );
			serverCv.wait( lock, [&] { return releaseResponse; } );
		}

		const std::string response = "HTTP/1.1 200 OK\r\n"
									 "Content-Type: image/png\r\n"
									 "Content-Length: " +
									 String::toString( static_cast<Uint64>( imageData.size() ) ) +
									 "\r\nConnection: close\r\n"
									 "\r\n" +
									 imageData;
		serverOk = client.send( response.data(), response.size() ) == Socket::Done;
		client.disconnect();
	} );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 300, 200 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string oldPath = Sys::getTempPath() + "eepp_uiwebview_remote_bg_old.html";
	const std::string newPath = Sys::getTempPath() + "eepp_uiwebview_remote_bg_new.html";
	const std::string imageURL =
		String::format( "http://127.0.0.1:%u/stale-bg.png", listener.getLocalPort() );
	FileSystem::fileWrite(
		oldPath,
		"<!DOCTYPE html><html><head><style>"
		"#probe { width: 100px; height: 80px; background-image: url('" +
			imageURL +
			"'); }"
			"</style></head><body><div id='old-doc'></div><div id='probe'></div></body></html>" );
	FileSystem::fileWrite(
		newPath,
		"<!DOCTYPE html><html><head><style>"
		"#probe { width: 100px; height: 80px; background-image: url('file://" +
			imagePath +
			"'); }"
			"</style></head><body><div id='new-doc'></div><div id='probe'></div></body></html>" );

	auto pump = [&]( int frames ) {
		for ( int i = 0; i < frames; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI( URI( "file://" + oldPath ) );
	{
		std::unique_lock<std::mutex> lock( serverMutex );
		serverCv.wait( lock, [&] { return requestReceived; } );
	}
	webView->loadURI( URI( "file://" + newPath ) );
	pump( 10 );
	{
		std::unique_lock<std::mutex> lock( serverMutex );
		releaseResponse = true;
	}
	serverCv.notify_one();

	server.join();
	listener.close();
	EXPECT_TRUE( serverOk );
	pump( 80 );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	EXPECT_TRUE( documentScene->getRoot()->find( "old-doc" ) == nullptr );
	EXPECT_TRUE( documentScene->getRoot()->find( "new-doc" ) != nullptr );
	Node* probeNode = documentScene->getRoot()->find( "probe" );
	ASSERT_TRUE( probeNode != nullptr && probeNode->isUINode() );
	UINode* probe = probeNode->asType<UINode>();
	ASSERT_TRUE( probe->getBackground() != nullptr );
	ASSERT_TRUE( probe->getBackground()->getLayer( 0 ) != nullptr );
	EXPECT_TRUE( probe->getBackground()->getLayer( 0 )->getDrawable() != nullptr );

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

UTEST( UIWebView, DocumentContainerHeightStaysSyncedAfterBottomGrow ) {
	UIApplication app( WindowSettings{ 1280, 720, "eepp - UI HTML Example", WindowStyle::Default,
									   WindowBackend::Default, 32 },
					   UIApplication::Settings( {}, 1.f ), ContextSettings( false, 0, 0 ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	auto win = app.getWindow();
	ASSERT_TRUE( win != nullptr );

	auto ui = app.getUI();
	ASSERT_TRUE( ui != nullptr );
	ui->setColorSchemePreference( ColorSchemeExtPreference::Light );

	UIWidget* vbox = ui->loadLayoutFromString( R"xml(
	<style>
		PushButton.webview_ui {
			border-top-color: transparent;
			border-right-color: transparent;
			border-bottom-color: transparent;
			border-left-color: transparent;
		}
		PushButton.webview_ui:hover {
			border-top-color: var(--primary);
			border-right-color: var(--primary);
			border-bottom-color: var(--primary);
			border-left-color: var(--primary);
		}
	</style>
	<vbox layout_width="match_parent" layout_height="match_parent">
		<hbox layout_width="match_parent" layout_height="wrap_content">
			<PushButton lw="26dp" id="backbtn" class="webview_ui" text="@string(back, Back)"
				icon="icon(arrow-left-s, 22dp)"
				text-as-fallback="true" />
			<PushButton lw="26dp" id="fwdbtn"  class="webview_ui" text="@string(forward, Forward)"
				icon="icon(arrow-right-s, 22dp)"
				text-as-fallback="true" />
			<PushButton lw="26dp" id="refreshbtn"  class="webview_ui" text="@string(refresh, Refresh)"
				icon="icon(refresh, 18dp)"
				text-as-fallback="true" />
			<TextInput id="url_bar" layout_width="0" layout_weight="1" />
		</hbox>
		<WebView id="webview" layout_width="match_parent" layout_height="0" layout_weight="1" />
	</vbox>
	)xml",
											   nullptr, app.getStyleSheetDefaultMarker() );
	ASSERT_TRUE( vbox != nullptr );

	UIWebView* webView = vbox->find( "webview" )->asType<UIWebView>();
	ASSERT_TRUE( webView != nullptr );
	webView->setStyleSheetDefaultMarker( app.getStyleSheetDefaultMarker() );
	bool navigationCompleted = false;
	webView->onNavigationCompleted(
		[&navigationCompleted]( const URI& ) { navigationCompleted = true; } );
	webView->loadURI(
		URI( "file://" + Sys::getProcessPath() + "assets/html/body_height_miscalculation.html" ) );

	UISceneNode* documentScene = webView->getDocumentSceneNode();
	ASSERT_TRUE( documentScene != nullptr );
	ASSERT_TRUE( documentScene->getParent() != nullptr && documentScene->getParent()->isUINode() );
	UINode* documentLayout = documentScene->getParent()->asType<UINode>();
	UIWidget* documentContainer = webView->getDocumentContainer();
	ASSERT_TRUE( documentContainer != nullptr );

	auto pump = [&]() {
		for ( int i = 0; i < 40; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
			win->clear();
			SceneManager::instance()->draw();
			win->display();
		}
	};
	auto updateOnce = [&]() {
		win->getInput()->update();
		SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		win->clear();
		SceneManager::instance()->draw();
		win->display();
	};
	for ( int i = 0; i < 120 && !navigationCompleted; i++ )
		updateOnce();
	ASSERT_TRUE( navigationCompleted );
	pump();
	ASSERT_TRUE( webView->getVerticalScrollBar()->isVisible() );
	const Sizef initialRootSize = ui->getRoot()->getPixelsSize();
	const Sizef initialWebViewSize = webView->getPixelsSize();

	webView->getVerticalScrollBar()->setValue( 1.f );
	pump();

	win->setSize( 2048, 1150 );
	ui->setPixelsSize( win->getSize().asFloat() );
	ui->setViewportPixelsSize( win->getSize().asFloat() );
	pump();

	UIWidget* html = documentScene->getRoot()->findByType( UI_TYPE_HTML_HTML )->asType<UIWidget>();
	UIWidget* body = documentScene->getRoot()->findByType( UI_TYPE_HTML_BODY )->asType<UIWidget>();
	ASSERT_TRUE( html != nullptr );
	ASSERT_TRUE( body != nullptr );
	EXPECT_GT( ui->getRoot()->getPixelsSize().getWidth(), initialRootSize.getWidth() );
	EXPECT_GT( ui->getRoot()->getPixelsSize().getHeight(), initialRootSize.getHeight() );
	EXPECT_GT( webView->getPixelsSize().getWidth(), initialWebViewSize.getWidth() );
	EXPECT_GT( webView->getPixelsSize().getHeight(), initialWebViewSize.getHeight() );
	EXPECT_NEAR( webView->getContainer()->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 1.f );
	EXPECT_NEAR( webView->getContainer()->getPixelsSize().getHeight(),
				 documentScene->getViewportPixelsSize().getHeight(), 1.f );
	EXPECT_NEAR( documentLayout->getPixelsSize().getWidth(),
				 documentScene->getPixelsSize().getWidth(), 1.f );
	EXPECT_NEAR( documentLayout->getPixelsSize().getWidth(),
				 documentContainer->getPixelsSize().getWidth(), 1.f );
	EXPECT_NEAR( documentLayout->getPixelsSize().getWidth(), html->getPixelsSize().getWidth(),
				 1.f );
	EXPECT_NEAR( body->getPixelsSize().getWidth(),
				 documentScene->getViewportPixelsSize().getWidth(), 1.f );
	EXPECT_NEAR( documentLayout->getPixelsSize().getHeight(),
				 documentScene->getPixelsSize().getHeight(), 1.f );
	EXPECT_NEAR( documentLayout->getPixelsSize().getHeight(),
				 documentContainer->getPixelsSize().getHeight(), 1.f );
	EXPECT_NEAR( documentLayout->getPixelsSize().getHeight(), html->getPixelsSize().getHeight(),
				 1.f );
	EXPECT_LE( body->getPixelsPosition().y + body->getPixelsSize().getHeight(),
			   documentLayout->getPixelsSize().getHeight() + 1.f );
	EXPECT_GE( body->getPixelsSize().getHeight(),
			   documentScene->getViewportPixelsSize().getHeight() - 1.f );
	EXPECT_NEAR( -documentLayout->getPixelsPosition().y +
					 webView->getContainer()->getPixelsSize().getHeight(),
				 documentLayout->getPixelsSize().getHeight(), 1.f );

	app.getUI()->getRoot()->closeAllChildren();
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

	const std::string testDir =
		Sys::getTempPath() + String::format( "eepp_uiwebview_doc_isolation_%llu_%llu/",
											 static_cast<unsigned long long>( Sys::getProcessID() ),
											 static_cast<unsigned long long>( Sys::getTicks() ) );
	const std::string dirA = testDir + "a/";
	const std::string dirB = testDir + "b/";
	ASSERT_TRUE( FileSystem::makeDir( dirA + "styles/", true ) );
	ASSERT_TRUE( FileSystem::makeDir( dirB + "styles/", true ) );

	const std::string pathA = dirA + "index.html";
	const std::string pathB = dirB + "index.html";
	const std::string pathA2 = Sys::getTempPath() + "eepp_uiwebview_doc_a2.html";
	FileSystem::fileWrite(
		dirA + "styles/site.css",
		".shared { background-color: #ff0000; } #target-a { width: 111px; height: 20px; }" );
	FileSystem::fileWrite(
		dirB + "styles/site.css",
		".shared { background-color: #0000ff; } #target-b { width: 222px; height: 20px; }" );
	FileSystem::fileWrite( pathA, R"html(
<html><head><link rel="stylesheet" href="styles/site.css"></head>
<body><div id="target-a" class="shared"></div></body></html>
)html" );
	FileSystem::fileWrite( pathB, R"html(
<html><head><link rel="stylesheet" href="styles/site.css"></head>
<body><div id="target-b" class="shared"></div></body></html>
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
	EXPECT_NEAR( 111.f, targetA->asType<UIWidget>()->getPixelsSize().getWidth(), 1.f );
	EXPECT_NEAR( 222.f, targetB->asType<UIWidget>()->getPixelsSize().getWidth(), 1.f );
	EXPECT_TRUE( sceneNode->getRoot()->findByType( UI_TYPE_HTML_HTML ) == nullptr );
	EXPECT_TRUE( sceneNode->getRoot()->find( "target-a" ) == nullptr );
	EXPECT_TRUE( docA->getReferer() == URI( "file://" + pathA ) );
	EXPECT_TRUE( docB->getReferer() == URI( "file://" + pathB ) );

	webViewA->loadURI( URI( "file://" + pathA2 ) );
	pump();

	auto targetA2 = docA->getRoot()->find( "target-a2" );
	ASSERT_TRUE( targetA2 != nullptr );
	EXPECT_TRUE( targetA2->asType<UIWidget>()->getBackgroundColor() == Color( "#ffff00" ) );
	targetB = docB->getRoot()->find( "target-b" );
	ASSERT_TRUE( targetB != nullptr );
	EXPECT_TRUE( targetB->asType<UIWidget>()->getBackgroundColor() == Color( "#0000ff" ) );
	EXPECT_TRUE( docB->getReferer() == URI( "file://" + pathB ) );

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
	const std::string pathAWithoutFont =
		Sys::getTempPath() + "eepp_uiwebview_font_doc_a_without_font.html";
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
	FileSystem::fileWrite( pathAWithoutFont,
						   "<html><head><style>"
						   "#target-a-empty { font-family: 'SharedDocFace'; }"
						   "</style></head><body><span id='target-a-empty'>A empty</span></body>"
						   "</html>" );
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
	const std::string fontAResourceName = fontA->getName();
	EXPECT_EQ( fontA, FontManager::instance()->getByName( fontAResourceName ) );

	webViewA->loadURI( URI( "file://" + pathAWithoutFont ) );
	pump();

	auto targetAWithoutFont = docA->getRoot()->find( "target-a-empty" );
	ASSERT_TRUE( targetAWithoutFont != nullptr );
	EXPECT_EQ( nullptr, docA->getFontFromNamesList( "SharedDocFace" ) );
	EXPECT_EQ( nullptr, FontManager::instance()->getByName( fontAResourceName ) );

	webViewA->loadURI( URI( "file://" + pathA2 ) );
	pump();

	Font* reloadedFontA = docA->getFontFromNamesList( "SharedDocFace" );
	Font* stableFontB = docB->getFontFromNamesList( "SharedDocFace" );
	ASSERT_TRUE( reloadedFontA != nullptr );
	ASSERT_TRUE( stableFontB != nullptr );
	EXPECT_EQ( stableFontB, fontB );
	EXPECT_NE( reloadedFontA, stableFontB );

	Engine::destroySingleton();
}

UTEST( UIWebView, DocumentSceneAuthorFontFacesCleanUpOnDestruction ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "UIWebView Font Face Cleanup Test", WindowStyle::Default,
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
	const std::string pathA = Sys::getTempPath() + "eepp_uiwebview_font_doc_destroy_a.html";
	const std::string pathB = Sys::getTempPath() + "eepp_uiwebview_font_doc_destroy_b.html";
	FileSystem::fileWrite( pathA,
						   "<html><head><style>"
						   "@font-face { font-family: 'DestroyDocFace'; src: url('file://" +
							   processPath +
							   "../assets/fonts/DejaVuSansMono.ttf'); }"
							   "#target-a { font-family: 'DestroyDocFace'; }"
							   "</style></head><body><span id='target-a'>A</span></body></html>" );
	FileSystem::fileWrite( pathB,
						   "<html><head><style>"
						   "@font-face { font-family: 'DestroyDocFace'; src: url('file://" +
							   processPath +
							   "../assets/fonts/NotoSans-Regular.ttf'); }"
							   "#target-b { font-family: 'DestroyDocFace'; }"
							   "</style></head><body><span id='target-b'>B</span></body></html>" );

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
	Font* loadedFontA = docA->getFontFromNamesList( "DestroyDocFace" );
	Font* loadedFontB = docB->getFontFromNamesList( "DestroyDocFace" );
	ASSERT_TRUE( loadedFontA != nullptr );
	ASSERT_TRUE( loadedFontB != nullptr );
	EXPECT_TRUE( loadedFontA->loaded() );
	EXPECT_TRUE( loadedFontB->loaded() );
	EXPECT_NE( loadedFontA, loadedFontB );
	const std::string loadedFontAName = loadedFontA->getName();
	const std::string loadedFontBName = loadedFontB->getName();
	EXPECT_EQ( loadedFontA, FontManager::instance()->getByName( loadedFontAName ) );
	EXPECT_EQ( loadedFontB, FontManager::instance()->getByName( loadedFontBName ) );

	webViewA->close();
	pump();

	EXPECT_EQ( nullptr, FontManager::instance()->getByName( loadedFontAName ) );
	EXPECT_EQ( loadedFontB, FontManager::instance()->getByName( loadedFontBName ) );
	EXPECT_EQ( loadedFontB, docB->getFontFromNamesList( "DestroyDocFace" ) );

	Engine::destroySingleton();
}

UTEST( UIWebView, RemoteFontFaceIgnoredAfterNavigation ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "UIWebView Remote Font Stale Navigation Test",
						WindowStyle::Default, WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	const std::string fontPath = Sys::getProcessPath() + "../assets/fonts/NotoSans-Regular.ttf";
	font->loadFromFile( fontPath );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	std::string fontData;
	ASSERT_TRUE( FileSystem::fileGet( fontPath, fontData ) );
	ASSERT_FALSE( fontData.empty() );

	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );
	std::atomic<bool> serverOk{ false };
	std::mutex serverMutex;
	std::condition_variable serverCv;
	bool requestReceived = false;
	bool releaseResponse = false;
	std::thread server( [&] {
		TcpSocket client;
		if ( listener.accept( client ) != Socket::Done )
			return;

		if ( !readHttpRequestHeaders( client ) )
			return;

		{
			std::unique_lock<std::mutex> lock( serverMutex );
			requestReceived = true;
		}
		serverCv.notify_one();
		{
			std::unique_lock<std::mutex> lock( serverMutex );
			serverCv.wait( lock, [&] { return releaseResponse; } );
		}

		const std::string response = "HTTP/1.1 200 OK\r\n"
									 "Content-Type: font/ttf\r\n"
									 "Content-Length: " +
									 String::toString( static_cast<Uint64>( fontData.size() ) ) +
									 "\r\nConnection: close\r\n"
									 "\r\n" +
									 fontData;
		serverOk = client.send( response.data(), response.size() ) == Socket::Done;
		client.disconnect();
	} );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 300, 200 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string oldPath = Sys::getTempPath() + "eepp_uiwebview_remote_font_old.html";
	const std::string newPath = Sys::getTempPath() + "eepp_uiwebview_remote_font_new.html";
	const std::string fontURL =
		String::format( "http://127.0.0.1:%u/stale.ttf", listener.getLocalPort() );
	FileSystem::fileWrite(
		oldPath,
		"<!DOCTYPE html><html><head><style>"
		"@font-face { font-family: 'StaleRemoteFace'; src: url('" +
			fontURL +
			"'); }"
			"#probe { font-family: 'StaleRemoteFace'; font-size: 20px; }"
			"</style></head><body><div id='old-doc'></div><span id='probe'>old</span></body>"
			"</html>" );
	FileSystem::fileWrite(
		newPath,
		"<!DOCTYPE html><html><body><div id='new-doc'></div><span id='probe'>new</span></body>"
		"</html>" );

	auto pump = [&]( int frames ) {
		for ( int i = 0; i < frames; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI( URI( "file://" + oldPath ) );
	{
		std::unique_lock<std::mutex> lock( serverMutex );
		serverCv.wait( lock, [&] { return requestReceived; } );
	}
	webView->loadURI( URI( "file://" + newPath ) );
	pump( 10 );
	{
		std::unique_lock<std::mutex> lock( serverMutex );
		releaseResponse = true;
	}
	serverCv.notify_one();

	server.join();
	listener.close();
	EXPECT_TRUE( serverOk );
	pump( 80 );

	UISceneNode* doc = webView->getDocumentSceneNode();
	ASSERT_TRUE( doc != nullptr );
	EXPECT_TRUE( doc->getRoot()->find( "old-doc" ) == nullptr );
	EXPECT_TRUE( doc->getRoot()->find( "new-doc" ) != nullptr );
	EXPECT_EQ( nullptr, doc->getFontFromNamesList( "StaleRemoteFace" ) );

	Engine::destroySingleton();
}

UTEST( UIWebView, StaleRedirectCookieIgnoredAfterNavigation ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "UIWebView Stale Redirect Cookie Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );
	std::mutex serverMutex;
	std::condition_variable serverCv;
	bool requestReceived = false;
	bool releaseResponse = false;
	std::thread server( [&] {
		TcpSocket client;
		if ( listener.accept( client ) != Socket::Done )
			return;

		if ( !readHttpRequestHeaders( client ) )
			return;

		{
			std::unique_lock<std::mutex> lock( serverMutex );
			requestReceived = true;
		}
		serverCv.notify_one();
		{
			std::unique_lock<std::mutex> lock( serverMutex );
			serverCv.wait( lock, [&] { return releaseResponse; } );
		}

		const std::string response = "HTTP/1.1 302 Found\r\n"
									 "Location: /new-location\r\n"
									 "Set-Cookie: stale_cookie=1; Path=/\r\n"
									 "Content-Length: 0\r\n"
									 "Connection: close\r\n"
									 "\r\n";
		client.send( response.data(), response.size() );
		client.disconnect();
	} );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 300, 200 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string newPath = Sys::getTempPath() + "eepp_uiwebview_stale_cookie_new.html";
	FileSystem::fileWrite( newPath, "<html><body><div id='new-doc'></div></body></html>" );
	const URI staleURI( String::format( "http://127.0.0.1:%u/old", listener.getLocalPort() ) );

	auto pump = [&]( int frames ) {
		for ( int i = 0; i < frames; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI( staleURI );
	{
		std::unique_lock<std::mutex> lock( serverMutex );
		serverCv.wait( lock, [&] { return requestReceived; } );
	}
	webView->loadURI( URI( "file://" + newPath ) );
	pump( 10 );
	{
		std::unique_lock<std::mutex> lock( serverMutex );
		releaseResponse = true;
	}
	serverCv.notify_one();

	listener.close();
	if ( server.joinable() )
		server.join();
	pump( 80 );

	UISceneNode* doc = webView->getDocumentSceneNode();
	ASSERT_TRUE( doc != nullptr );
	EXPECT_TRUE( doc->getRoot()->find( "new-doc" ) != nullptr );
	EXPECT_FALSE( doc->getCookieManager().hasCookie( staleURI.getAuthority() ) );

	Engine::destroySingleton();
}

UTEST( UIWebView, DestroyWithPendingSubresourcesIsSafe ) {
	auto win = Engine::instance()->createWindow(
		WindowSettings( 800, 600, "UIWebView Pending Subresource Destruction Test",
						WindowStyle::Default, WindowBackend::Default, 32, {}, 1, false, true ),
		ContextSettings( false, 0, 0, GLv_default, true, false ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	const std::string fontPath = Sys::getProcessPath() + "../assets/fonts/NotoSans-Regular.ttf";
	font->loadFromFile( fontPath );
	ASSERT_TRUE( font != nullptr && font->loaded() );
	FontFamily::loadFromRegular( font );

	std::string imageData;
	const std::string imagePath =
		Sys::getProcessPath() + "assets/html/inline_block_wrap_files/88x31.png";
	ASSERT_TRUE( FileSystem::fileGet( imagePath, imageData ) );
	ASSERT_FALSE( imageData.empty() );

	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );

	std::mutex serverMutex;
	std::condition_variable serverCv;
	bool requestReceived = false;
	bool releaseResponses = false;
	std::thread server( [&] {
		TcpSocket client;
		if ( listener.accept( client ) != Socket::Done )
			return;

		if ( !readHttpRequestHeaders( client ) )
			return;

		{
			std::unique_lock<std::mutex> lock( serverMutex );
			requestReceived = true;
		}
		serverCv.notify_one();
		{
			std::unique_lock<std::mutex> lock( serverMutex );
			serverCv.wait( lock, [&] { return releaseResponses; } );
		}

		const std::string response = "HTTP/1.1 200 OK\r\n"
									 "Content-Type: image/png\r\n"
									 "Content-Length: " +
									 String::toString( static_cast<Uint64>( imageData.size() ) ) +
									 "\r\nConnection: close\r\n"
									 "\r\n" +
									 imageData;
		client.send( response.data(), response.size() );
		client.disconnect();
	} );

	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	sceneNode->getUIThemeManager()->setDefaultFont( font );

	UIWebView* webView = UIWebView::New();
	webView->setParent( sceneNode->getRoot() );
	webView->setPixelsSize( 300, 200 );
	webView->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	const std::string baseURL = String::format( "http://127.0.0.1:%u", listener.getLocalPort() );
	const std::string path = Sys::getTempPath() + "eepp_uiwebview_destroy_pending.html";
	FileSystem::fileWrite( path, "<!DOCTYPE html><html><body><img id='img' src='" + baseURL +
									 "/img.png'></body></html>" );

	auto pump = [&]( int frames ) {
		for ( int i = 0; i < frames; i++ ) {
			win->getInput()->update();
			SceneManager::instance()->update( Seconds( 1.f / 60.f ) );
		}
	};

	webView->loadURI( URI( "file://" + path ) );
	{
		std::unique_lock<std::mutex> lock( serverMutex );
		serverCv.wait( lock, [&] { return requestReceived; } );
	}

	webView->close();
	pump( 20 );

	{
		std::unique_lock<std::mutex> lock( serverMutex );
		releaseResponses = true;
	}
	serverCv.notify_all();
	listener.close();
	if ( server.joinable() )
		server.join();

	pump( 20 );
	ASSERT_TRUE( sceneNode->getRoot()->findByType( UI_TYPE_WEBVIEW ) == nullptr );

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
