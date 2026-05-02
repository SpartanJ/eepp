#include "utest.h"
#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::Window;
using namespace EE::Graphics;

static void init_ui_test() {
	Engine::instance()->createWindow(
		WindowSettings( 1024, 650, "UIHTMLWidget Position Out Of Flow Test", WindowStyle::Default,
						WindowBackend::Default, 32, {}, 1, false, true ) );
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );

	UI::UISceneNode* sceneNode = UI::UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	UI::UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
}

UTEST( UIHTMLWidget, positionOutOfFlow_AbsoluteRelToRoot ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* rootContainer = UIHTMLWidget::New();
	rootContainer->setParent( sceneNode->getRoot() );
	rootContainer->setPixelsSize( 500, 500 );
	rootContainer->setPixelsPosition( 100, 100 );

	UIHTMLWidget* staticChild = UIHTMLWidget::New();
	staticChild->setParent( rootContainer );
	staticChild->setPixelsSize( 100, 100 );
	staticChild->setPixelsPosition( 50, 50 );

	UIHTMLWidget* absoluteChild = UIHTMLWidget::New();
	absoluteChild->setParent( staticChild );
	absoluteChild->setCSSPosition( CSSPosition::Absolute );
	absoluteChild->setOffsets( Rectf( 25, 15, 0, 0 ) ); // L, T, R, B
	absoluteChild->setPixelsSize( 50, 50 );

	sceneNode->updateDirtyLayouts();

	UIWidget* cb = absoluteChild->getContainingBlock();
	EXPECT_EQ( cb, sceneNode->getRoot() ); // Because no relative ancestor

	Vector2f worldPos = absoluteChild->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( 25.f, worldPos.x, 1.f );
	EXPECT_NEAR( 15.f, worldPos.y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLWidget, positionOutOfFlow_AbsoluteRelToRelative ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* rootContainer = UIHTMLWidget::New();
	rootContainer->setParent( sceneNode->getRoot() );
	rootContainer->setCSSPosition( CSSPosition::Relative );
	rootContainer->setPixelsSize( 500, 500 );
	rootContainer->setPixelsPosition( 100, 100 );
	rootContainer->setPadding( Rectf( 10, 20, 30, 40 ) ); // L, T, R, B

	UIHTMLWidget* staticChild = UIHTMLWidget::New();
	staticChild->setParent( rootContainer );
	staticChild->setPixelsSize( 100, 100 );
	staticChild->setPixelsPosition( 50, 50 );

	UIHTMLWidget* absoluteChild = UIHTMLWidget::New();
	absoluteChild->setParent( staticChild );
	absoluteChild->setCSSPosition( CSSPosition::Absolute );
	absoluteChild->setOffsets( Rectf( 25, 15, 0, 0 ) ); // L, T, R, B
	absoluteChild->setPixelsSize( 50, 50 );

	sceneNode->updateDirtyLayouts();

	UIWidget* cb = absoluteChild->getContainingBlock();
	EXPECT_EQ( cb, rootContainer );

	Vector2f worldPos = absoluteChild->convertToWorldSpace( { 0, 0 } );
	// rootContainer world pos is 100, 100
	// cb padding left = 10, top = 20
	// absoluteChild offset left = 25, top = 15
	// worldPos should be 100 + 10 + 25 = 135
	// worldPos y should be 100 + 20 + 15 = 135
	EXPECT_NEAR( 135.f, worldPos.x, 1.f );
	EXPECT_NEAR( 135.f, worldPos.y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLWidget, positionOutOfFlow_NestedAbsolute ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* relContainer = UIHTMLWidget::New();
	relContainer->setParent( sceneNode->getRoot() );
	relContainer->setCSSPosition( CSSPosition::Relative );
	relContainer->setPixelsSize( 500, 500 );
	relContainer->setPixelsPosition( 100, 100 );

	UIHTMLWidget* absContainer = UIHTMLWidget::New();
	absContainer->setParent( relContainer );
	absContainer->setCSSPosition( CSSPosition::Absolute );
	absContainer->setOffsets( Rectf( 50, 50, 0, 0 ) );
	absContainer->setPixelsSize( 200, 200 );

	UIHTMLWidget* absChild = UIHTMLWidget::New();
	absChild->setParent( absContainer );
	absChild->setCSSPosition( CSSPosition::Absolute );
	absChild->setOffsets( Rectf( 20, 20, 0, 0 ) );
	absChild->setPixelsSize( 50, 50 );

	sceneNode->updateDirtyLayouts();

	UIWidget* cb1 = absContainer->getContainingBlock();
	EXPECT_EQ( cb1, relContainer );

	UIWidget* cb2 = absChild->getContainingBlock();
	EXPECT_EQ( cb2, absContainer );

	Vector2f worldPos1 = absContainer->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( 150.f, worldPos1.x, 1.f );
	EXPECT_NEAR( 150.f, worldPos1.y, 1.f );

	Vector2f worldPos2 = absChild->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( 170.f, worldPos2.x, 1.f );
	EXPECT_NEAR( 170.f, worldPos2.y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLWidget, positionOutOfFlow_Fixed ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* relContainer = UIHTMLWidget::New();
	relContainer->setParent( sceneNode->getRoot() );
	relContainer->setCSSPosition( CSSPosition::Relative );
	relContainer->setPixelsSize( 500, 500 );
	relContainer->setPixelsPosition( 100, 100 );

	UIHTMLWidget* staticChild = UIHTMLWidget::New();
	staticChild->setParent( relContainer );
	staticChild->setPixelsSize( 100, 100 );
	staticChild->setPixelsPosition( 50, 50 );

	UIHTMLWidget* fixedChild = UIHTMLWidget::New();
	fixedChild->setParent( staticChild );
	fixedChild->setCSSPosition( CSSPosition::Fixed );
	fixedChild->setOffsets( Rectf( 30, 40, 0, 0 ) ); // L, T, R, B
	fixedChild->setPixelsSize( 50, 50 );

	sceneNode->updateDirtyLayouts();

	UIWidget* cbFixed = fixedChild->getContainingBlock();
	EXPECT_EQ( cbFixed, sceneNode->getRoot() );

	Vector2f worldPos = fixedChild->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( 30.f, worldPos.x, 1.f );
	EXPECT_NEAR( 40.f, worldPos.y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLWidget, positionOutOfFlow_DoesNotAffectParentSize ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* autoSizedParent = UIRichText::New();
	autoSizedParent->setParent( sceneNode->getRoot() );
	autoSizedParent->setCSSPosition( CSSPosition::Relative );
	autoSizedParent->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );

	UIWidget* normalChild = UIWidget::New();
	normalChild->setParent( autoSizedParent );
	normalChild->setPixelsSize( 100, 100 );
	normalChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* absoluteChild = UIHTMLWidget::New();
	absoluteChild->setParent( autoSizedParent );
	absoluteChild->setCSSPosition( CSSPosition::Absolute );
	absoluteChild->setPixelsSize( 5000, 5000 ); // Very large
	absoluteChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// The parent's size should only encompass the normal child
	EXPECT_NEAR( 100.f, autoSizedParent->getPixelsSize().getWidth(), 1.f );
	EXPECT_NEAR( 100.f, autoSizedParent->getPixelsSize().getHeight(), 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLWidget, positionOutOfFlow_PercentageAndMargin ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* rootContainer = UIHTMLWidget::New();
	rootContainer->setParent( sceneNode->getRoot() );
	rootContainer->setCSSPosition( CSSPosition::Relative );
	rootContainer->setPixelsSize( 800, 600 );
	rootContainer->setPixelsPosition( 0, 0 );

	UIHTMLWidget* absoluteChild = UIHTMLWidget::New();
	absoluteChild->setParent( rootContainer );
	absoluteChild->setPixelsSize( 400, 400 );

	// Emulate the CSS parsing via applyProperty
	absoluteChild->applyProperty( StyleSheetProperty( "position", "absolute" ) );
	absoluteChild->applyProperty( StyleSheetProperty( "left", "50%" ) );
	absoluteChild->applyProperty( StyleSheetProperty( "margin-left", "-200px" ) );
	absoluteChild->applyProperty( StyleSheetProperty( "margin-top", "120px" ) );

	sceneNode->updateDirtyLayouts();

	UIWidget* cb = absoluteChild->getContainingBlock();
	EXPECT_EQ( cb, rootContainer );

	Vector2f worldPos = absoluteChild->convertToWorldSpace( { 0, 0 } );
	// left should be 50% of 800 (400) plus margin-left (-200) = 200
	// top should be 0 + margin-top (120) = 120
	EXPECT_NEAR( 200.f, worldPos.x, 1.f );
	EXPECT_NEAR( 120.f, worldPos.y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLWidget, positionOutOfFlow_ComplexHTML ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	sceneNode->setURI( "file://" + Sys::getProcessPath() + "assets/html/" );
	std::string html;
	FileSystem::fileGet( "assets/html/absolute_position.html", html );
	std::string xml = UI::Tools::HTMLFormatter::HTMLtoXML( html );
	sceneNode->loadLayoutFromString( xml );

	sceneNode->update( Milliseconds( 16 ) );
	sceneNode->updateDirtyLayouts();

	UIWidget* mainWidget = sceneNode->getRoot()->find<UIWidget>( "main" );
	ASSERT_TRUE( mainWidget != nullptr );
	EXPECT_GT( mainWidget->getPixelsSize().getHeight(), 0.f ); // This is not standard in HTML!

	Vector2f worldPos = mainWidget->convertToWorldSpace( { 0, 0 } );
	// Window size is 1024x650
	// left: 50% of 1024 = 512
	// margin-left: -200px
	// 512 - 200 = 312
	EXPECT_NEAR( 312.f, worldPos.x, 1.f );

	// top should just be margin-top
	EXPECT_NEAR( 120.f, worldPos.y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLWidget, positionOutOfFlow_ShrinkToFit ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* relContainer = UIRichText::New();
	relContainer->setParent( sceneNode->getRoot() );
	relContainer->setCSSPosition( CSSPosition::Relative );
	relContainer->setPixelsSize( 800, 600 );
	relContainer->setPixelsPosition( 0, 0 );

	UIRichText* absoluteChild = UIRichText::New();
	absoluteChild->setParent( relContainer );
	absoluteChild->applyProperty( StyleSheetProperty( "position", "absolute" ) );
	absoluteChild->applyProperty( StyleSheetProperty( "display", "block" ) );
	absoluteChild->applyProperty( StyleSheetProperty( "padding", "4px" ) );

	sceneNode->updateDirtyLayouts();

	EXPECT_EQ( SizePolicy::WrapContent, absoluteChild->getLayoutWidthPolicy() );

	// With no text content, the element should shrink to just padding
	Float childWidth = absoluteChild->getPixelsSize().getWidth();
	EXPECT_LT( childWidth, 20.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLWidget, positionOutOfFlow_RightBottomPositioning ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* relContainer = UIHTMLWidget::New();
	relContainer->setParent( sceneNode->getRoot() );
	relContainer->setCSSPosition( CSSPosition::Relative );
	relContainer->setPixelsSize( 800, 600 );
	relContainer->setPixelsPosition( 0, 0 );

	UIHTMLWidget* absoluteChild = UIHTMLWidget::New();
	absoluteChild->setParent( relContainer );
	absoluteChild->setPixelsSize( 100, 50 );
	absoluteChild->applyProperty( StyleSheetProperty( "position", "absolute" ) );
	absoluteChild->applyProperty( StyleSheetProperty( "right", "0px" ) );
	absoluteChild->applyProperty( StyleSheetProperty( "bottom", "0px" ) );

	sceneNode->updateDirtyLayouts();

	Vector2f worldPos = absoluteChild->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( 700.f, worldPos.x, 1.f );
	EXPECT_NEAR( 550.f, worldPos.y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLWidget, positionOutOfFlow_RightBottomWithMargin ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* relContainer = UIHTMLWidget::New();
	relContainer->setParent( sceneNode->getRoot() );
	relContainer->setCSSPosition( CSSPosition::Relative );
	relContainer->setPixelsSize( 800, 600 );
	relContainer->setPixelsPosition( 0, 0 );

	UIHTMLWidget* absoluteChild = UIHTMLWidget::New();
	absoluteChild->setParent( relContainer );
	absoluteChild->setPixelsSize( 100, 50 );
	absoluteChild->applyProperty( StyleSheetProperty( "position", "absolute" ) );
	absoluteChild->applyProperty( StyleSheetProperty( "right", "10px" ) );
	absoluteChild->applyProperty( StyleSheetProperty( "bottom", "20px" ) );
	absoluteChild->applyProperty( StyleSheetProperty( "margin-right", "5px" ) );
	absoluteChild->applyProperty( StyleSheetProperty( "margin-bottom", "5px" ) );

	sceneNode->updateDirtyLayouts();

	Vector2f worldPos = absoluteChild->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( 685.f, worldPos.x, 1.f );
	EXPECT_NEAR( 525.f, worldPos.y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLWidget, positionOutOfFlow_FixedScroll ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIScrollView* scrollView = UIScrollView::New();
	scrollView->setParent( sceneNode->getRoot() );
	scrollView->setPixelsSize( 800, 600 );

	UIHTMLBody* body = UIHTMLBody::New( "body" );
	body->setParent( scrollView );
	body->applyProperty( StyleSheetProperty( "position", "relative" ) );

	UIWidget* dummyChild = UIWidget::New();
	dummyChild->setParent( body );
	dummyChild->setPixelsSize( 800, 2000 );
	dummyChild->setPixelsPosition( 0, 0 );

	UIHTMLWidget* fixedChild = UIHTMLWidget::New();
	fixedChild->setParent( body );
	fixedChild->setPixelsSize( 100, 50 );
	fixedChild->applyProperty( StyleSheetProperty( "position", "fixed" ) );
	fixedChild->applyProperty( StyleSheetProperty( "top", "50px" ) );
	fixedChild->applyProperty( StyleSheetProperty( "left", "50px" ) );

	sceneNode->updateDirtyLayouts();

	// Scroll down by 200px
	scrollView->getScrollView()->setPosition( { 0, -200 } );

	Vector2f worldPos = fixedChild->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( 50.f, worldPos.x, 1.f );
	EXPECT_NEAR( 50.f, worldPos.y, 1.f );

	Engine::destroySingleton();
}

UTEST( UIHTMLWidget, positionOutOfFlow_StickyScroll ) {
	init_ui_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIScrollView* scrollView = UIScrollView::New();
	scrollView->setParent( sceneNode->getRoot() );
	scrollView->setPixelsSize( 800, 600 );

	UIHTMLBody* body = UIHTMLBody::New( "body" );
	body->setParent( scrollView );
	body->applyProperty( StyleSheetProperty( "position", "relative" ) );

	UIHTMLWidget* stickyChild = UIHTMLWidget::New();
	stickyChild->setParent( body );
	stickyChild->setPixelsSize( 100, 50 );
	stickyChild->applyProperty( StyleSheetProperty( "margin-top", "100px" ) );
	stickyChild->applyProperty( StyleSheetProperty( "position", "sticky" ) );
	stickyChild->applyProperty( StyleSheetProperty( "top", "20px" ) );

	UIWidget* dummyChild = UIWidget::New();
	dummyChild->setParent( body );
	dummyChild->setPixelsSize( 800, 2000 );
	dummyChild->setPixelsPosition( 0, 0 );

	sceneNode->updateDirtyLayouts();

	// Force base pos (as if layouter did it)
	stickyChild->setPixelsPosition( 0, 100 );

	// Ensure base pos was captured correctly
	EXPECT_NEAR( 100.f, stickyChild->getPixelsPosition().y, 1.f );

	// Scroll down by 50px
	Float actualMaxScrollY =
		body->getPixelsSize().getHeight() - scrollView->getContainer()->getPixelsSize().getHeight();
	scrollView->getVerticalScrollBar()->setValue( 50.f / actualMaxScrollY );

	Vector2f worldPos1 = stickyChild->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( 50.f, worldPos1.y, 1.f );

	// Scroll down by 150px
	scrollView->getVerticalScrollBar()->setValue( 150.f / actualMaxScrollY );

	Vector2f worldPos2 = stickyChild->convertToWorldSpace( { 0, 0 } );
	EXPECT_NEAR( 20.f, worldPos2.y, 1.f );

	Engine::destroySingleton();
}
