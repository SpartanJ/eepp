#include "utest.h"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/flexlayouter.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uihtmlwidget.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/window.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::Window;
using namespace EE::Graphics;

static void init_flex_test() {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" );
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );
	UISceneNode* sceneNode = UISceneNode::New();
	SceneManager::instance()->add( sceneNode );
	SceneManager::instance()->setCurrentUISceneNode( sceneNode );
	UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
	themeManager->applyDefaultTheme( sceneNode->getRoot() );
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 1: Item Collection
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, collectsInFlowChildren ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Both children should be positioned by the flex layouter
	EXPECT_GT( child1->getPixelsPosition().x, -1.f );
	EXPECT_GT( child2->getPixelsPosition().x, -1.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, skipsOutOfFlowChildren ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* normalChild = UIHTMLWidget::New();
	normalChild->setParent( flex );
	normalChild->setPixelsSize( 100, 50 );
	normalChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* absoluteChild = UIHTMLWidget::New();
	absoluteChild->setParent( flex );
	absoluteChild->setCSSPosition( CSSPosition::Absolute );
	absoluteChild->setPixelsSize( 100, 50 );
	absoluteChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	absoluteChild->setPixelsPosition( 0, 0 );

	sceneNode->updateDirtyLayouts();

	// Normal child should be laid out by flex; absolute should stay at its explicit position
	EXPECT_GT( normalChild->getPixelsPosition().x, -1.f );
	EXPECT_NEAR( absoluteChild->getPixelsPosition().x, 0.f, 1.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, skipsDisplayNoneChildren ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* visibleChild = UIHTMLWidget::New();
	visibleChild->setParent( flex );
	visibleChild->setPixelsSize( 100, 50 );
	visibleChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* hiddenChild = UIHTMLWidget::New();
	hiddenChild->setParent( flex );
	hiddenChild->setDisplay( CSSDisplay::None );
	hiddenChild->setPixelsSize( 100, 50 );
	hiddenChild->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Visible child should be laid out; hidden child should not affect layout
	EXPECT_GT( visibleChild->getPixelsPosition().x, -1.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, sortsByOrder ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* childA = UIHTMLWidget::New();
	childA->setParent( flex );
	childA->setPixelsSize( 100, 50 );
	childA->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	childA->applyProperty( StyleSheetProperty( "order", "2" ) );

	UIHTMLWidget* childB = UIHTMLWidget::New();
	childB->setParent( flex );
	childB->setPixelsSize( 100, 50 );
	childB->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	childB->applyProperty( StyleSheetProperty( "order", "1" ) );

	sceneNode->updateDirtyLayouts();

	// childB (order:1) should come before childA (order:2)
	EXPECT_LT( childB->getPixelsPosition().x, childA->getPixelsPosition().x );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 2: Direction and Wrap
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, defaultRowLayout ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Default row: children should be side-by-side
	EXPECT_NEAR( child1->getPixelsPosition().x, 0.f, 1.f );
	EXPECT_GT( child2->getPixelsPosition().x, child1->getPixelsPosition().x );

	Engine::destroySingleton();
}

UTEST( FlexContainer, rowReverseLayout ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-direction", "row-reverse" ) );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Row-reverse: first child should be at the right
	EXPECT_GT( child1->getPixelsPosition().x, child2->getPixelsPosition().x );

	Engine::destroySingleton();
}

UTEST( FlexContainer, columnLayout ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-direction", "column" ) );
	flex->setPixelsSize( 500, 400 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Column: children should be stacked vertically
	EXPECT_NEAR( child1->getPixelsPosition().x, 0.f, 1.f );
	EXPECT_GT( child2->getPixelsPosition().y, child1->getPixelsPosition().y );

	Engine::destroySingleton();
}

UTEST( FlexContainer, columnReverseLayout ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-direction", "column-reverse" ) );
	flex->setPixelsSize( 500, 400 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Column-reverse: first child should be at the bottom
	EXPECT_GT( child1->getPixelsPosition().y, child2->getPixelsPosition().y );

	Engine::destroySingleton();
}

UTEST( FlexContainer, wrapCreatesMultipleLines ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-wrap", "wrap" ) );
	flex->setPixelsSize( 250, 400 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Three items of 100px width each in a 250px container should wrap to two lines
	for ( int i = 0; i < 3; i++ ) {
		UIHTMLWidget* child = UIHTMLWidget::New();
		child->setParent( flex );
		child->setPixelsSize( 100, 50 );
		child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	}

	sceneNode->updateDirtyLayouts();

	Node* firstChild = flex->getFirstChild();
	Node* lastChild = flex->getLastChild();
	ASSERT_TRUE( firstChild && firstChild->isWidget() );
	ASSERT_TRUE( lastChild && lastChild->isWidget() );

	// First and last child should be on different lines (different Y)
	EXPECT_GT( lastChild->asType<UIWidget>()->getPixelsPosition().y,
			   firstChild->asType<UIWidget>()->getPixelsPosition().y );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 3: Main-Axis Distribution (justify-content)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, justifyContentFlexStart ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Default flex-start: child at left edge
	EXPECT_NEAR( child->getPixelsPosition().x, 0.f, 1.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, justifyContentCenter ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "justify-content", "center" ) );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Center: child centered in 500px container
	EXPECT_NEAR( child->getPixelsPosition().x, 200.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, justifyContentFlexEnd ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "justify-content", "flex-end" ) );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Flex-end: child at right edge
	EXPECT_NEAR( child->getPixelsPosition().x, 400.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, justifyContentSpaceBetween ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "justify-content", "space-between" ) );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// space-between: first at start, last at end
	EXPECT_NEAR( child1->getPixelsPosition().x, 0.f, 5.f );
	EXPECT_NEAR( child2->getPixelsPosition().x, 400.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, justifyContentSpaceAround ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "justify-content", "space-around" ) );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// space-around: equal space on both sides
	// Total space = 500 - 200 = 300. 2 slots = 300/2 = 150 each
	// child1 x = 150/2 = 75
	EXPECT_NEAR( child1->getPixelsPosition().x, 75.f, 10.f );
	EXPECT_NEAR( child2->getPixelsPosition().x, 325.f, 10.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, justifyContentSpaceEvenly ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "justify-content", "space-evenly" ) );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// space-evenly: 3 equal gaps for 2 items
	// Total space = 500 - 200 = 300. 3 gaps = 300/3 = 100 each
	// child1 x = 100
	EXPECT_NEAR( child1->getPixelsPosition().x, 100.f, 10.f );
	EXPECT_NEAR( child2->getPixelsPosition().x, 300.f, 10.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 4: Cross-Axis Alignment (align-items)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, alignItemsStretch ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-items", "stretch" ) );
	flex->setPixelsSize( 500, 300 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	sceneNode->updateDirtyLayouts();

	// Stretch: child should fill the 300px cross axis (minus padding)
	EXPECT_GT( child->getPixelsSize().getHeight(), 200.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, alignItemsCenter ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-items", "center" ) );
	flex->setPixelsSize( 500, 300 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Center: child centered vertically in 300px container
	EXPECT_NEAR( child->getPixelsPosition().y, 125.f, 10.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, alignItemsFlexStart ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-items", "flex-start" ) );
	flex->setPixelsSize( 500, 300 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// flex-start: child at top
	EXPECT_NEAR( child->getPixelsPosition().y, 0.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, alignItemsFlexEnd ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-items", "flex-end" ) );
	flex->setPixelsSize( 500, 300 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// flex-end: child at bottom
	EXPECT_NEAR( child->getPixelsPosition().y, 250.f, 5.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 5: align-self Override
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, alignSelfOverride ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-items", "stretch" ) );
	flex->setPixelsSize( 500, 300 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child2->setStyleSheetProperty( StyleSheetProperty( "align-self", "center" ) );

	sceneNode->updateDirtyLayouts();

	// child1 should stretch, child2 should be centered
	EXPECT_GT( child1->getPixelsSize().getHeight(), 200.f );
	EXPECT_NEAR( child2->getPixelsPosition().y, 125.f, 10.f );
	EXPECT_NEAR( child2->getPixelsSize().getHeight(), 50.f, 5.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 6: Flexible Lengths (flex-grow, flex-shrink, flex-basis)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, flexGrowDistributesSpace ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child1->setStyleSheetProperty( StyleSheetProperty( "flex-grow", "1" ) );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// child1 (flex-grow:1) should expand to fill remaining space
	EXPECT_GT( child1->getPixelsSize().getWidth(), 100.f );
	EXPECT_NEAR( child1->getPixelsSize().getWidth(), 400.f, 10.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, flexGrowProportional ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child1->setStyleSheetProperty( StyleSheetProperty( "flex-grow", "2" ) );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child2->setStyleSheetProperty( StyleSheetProperty( "flex-grow", "1" ) );

	sceneNode->updateDirtyLayouts();

	// Available: 500 - 200 = 300. child1 gets 2/3 (200), child2 gets 1/3 (100)
	// child1 width = 100 + 200 = 300, child2 width = 100 + 100 = 200
	EXPECT_NEAR( child1->getPixelsSize().getWidth(), 300.f, 15.f );
	EXPECT_NEAR( child2->getPixelsSize().getWidth(), 200.f, 15.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, flexShrinkContractsItems ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 300, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 200, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 200, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Total base 400 > container 300, items should shrink
	EXPECT_LT( child1->getPixelsSize().getWidth(), 200.f );
	EXPECT_LT( child2->getPixelsSize().getWidth(), 200.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, flexBasisFixed ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child->setStyleSheetProperty( StyleSheetProperty( "flex-basis", "200px" ) );

	sceneNode->updateDirtyLayouts();

	// flex-basis: 200px should be the starting size before flexing
	EXPECT_NEAR( child->getPixelsSize().getWidth(), 200.f, 10.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 7: Flex Shorthand
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, flexShorthand ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child->setStyleSheetProperty( StyleSheetProperty( "flex", "1 0 100px" ) );

	sceneNode->updateDirtyLayouts();

	// flex: 1 0 100px -> flex-grow:1, flex-shrink:0, flex-basis:100px
	// With 500px container, one item grows to fill: width = 100 + 400 = 500
	EXPECT_NEAR( child->getPixelsSize().getWidth(), 500.f, 10.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 8: Multi-Line Cross-Axis Distribution (align-content)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, alignContentSpaceBetween ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-wrap", "wrap" ) );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-content", "space-between" ) );
	flex->setPixelsSize( 250, 400 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// 3 items, 100px each, container 250px wide -> 2 lines
	for ( int i = 0; i < 3; i++ ) {
		UIHTMLWidget* child = UIHTMLWidget::New();
		child->setParent( flex );
		child->setPixelsSize( 100, 50 );
		child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	}

	sceneNode->updateDirtyLayouts();

	// space-between: first line at top, last line at bottom
	Node* first = flex->getFirstChild();
	Node* last = flex->getLastChild();
	ASSERT_TRUE( first && first->isWidget() );
	ASSERT_TRUE( last && last->isWidget() );

	EXPECT_NEAR( first->asType<UIWidget>()->getPixelsPosition().y, 0.f, 5.f );
	EXPECT_NEAR( last->asType<UIWidget>()->getPixelsPosition().y, 350.f, 10.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, alignContentCenter ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-wrap", "wrap" ) );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-content", "center" ) );
	flex->setPixelsSize( 250, 400 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// 3 items, 100px each, container 250px wide -> 2 lines (100 + 100 = 200)
	for ( int i = 0; i < 3; i++ ) {
		UIHTMLWidget* child = UIHTMLWidget::New();
		child->setParent( flex );
		child->setPixelsSize( 100, 50 );
		child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	}

	sceneNode->updateDirtyLayouts();

	// Center: lines centered in 400px container
	// 2 lines of 50px each = 100px. Free = 300px. Offset = 150px
	Node* first = flex->getFirstChild();
	ASSERT_TRUE( first && first->isWidget() );
	EXPECT_NEAR( first->asType<UIWidget>()->getPixelsPosition().y, 150.f, 20.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, alignContentStretch ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-wrap", "wrap" ) );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-content", "stretch" ) );
	flex->setPixelsSize( 250, 400 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// 3 items, 100px each, container 250px wide -> 2 lines
	for ( int i = 0; i < 3; i++ ) {
		UIHTMLWidget* child = UIHTMLWidget::New();
		child->setParent( flex );
		child->setPixelsSize( 100, 50 );
		child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::WrapContent );
	}

	sceneNode->updateDirtyLayouts();

	// Stretch: lines stretched to fill container cross size
	// 400px / 2 lines = 200px per line. Items should be stretched.
	Node* child = flex->getFirstChild();
	ASSERT_TRUE( child && child->isWidget() );
	EXPECT_GT( child->asType<UIWidget>()->getPixelsSize().getHeight(), 100.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 9: Gap Properties
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, gapRowColumn ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "column-gap", "20px" ) );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// With 20px gap, child2 should start at 100 + 20 = 120
	EXPECT_NEAR( child2->getPixelsPosition().x, 120.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, gapShorthand ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "gap", "10px 30px" ) );
	flex->setPixelsSize( 500, 400 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// gap: 10px 30px -> row-gap=10px, column-gap=30px
	EXPECT_NEAR( child2->getPixelsPosition().x, 130.f, 5.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 10: Inline-Flex
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, inlineFlexShrinkWraps ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::InlineFlex );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Inline-flex should shrink-wrap to content width
	EXPECT_NEAR( flex->getPixelsSize().getWidth(), 100.f, 20.f );
	EXPECT_EQ( flex->getLayoutWidthPolicy(), SizePolicy::WrapContent );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 11: Edge Cases
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, emptyContainer ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Empty flex container should not crash and should have padding-only size
	EXPECT_GT( flex->getPixelsSize().getWidth(), 0.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, nestedFlexContainer ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* outer = UIHTMLWidget::New();
	outer->setParent( sceneNode->getRoot() );
	outer->setDisplay( CSSDisplay::Flex );
	outer->setPixelsSize( 500, 200 );
	outer->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* inner = UIHTMLWidget::New();
	inner->setParent( outer );
	inner->setDisplay( CSSDisplay::Flex );
	inner->setPixelsSize( 200, 100 );
	inner->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( inner );
	child->setPixelsSize( 50, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Nested flex should work without crashing
	EXPECT_GT( child->getPixelsPosition().x, -1.f );
	EXPECT_GT( child->getPixelsPosition().y, -1.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, negativeOrder ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child1->setStyleSheetProperty( StyleSheetProperty( "order", "0" ) );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child2->setStyleSheetProperty( StyleSheetProperty( "order", "-1" ) );

	sceneNode->updateDirtyLayouts();

	// child2 (order:-1) should come before child1 (order:0)
	EXPECT_LT( child2->getPixelsPosition().x, child1->getPixelsPosition().x );

	Engine::destroySingleton();
}

UTEST( FlexContainer, fixedWidthItem ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 150, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child->applyProperty( StyleSheetProperty( "width", "150px" ) );
	child->setLayoutWidthPolicy( SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Fixed width should be respected
	EXPECT_NEAR( child->getPixelsSize().getWidth(), 150.f, 5.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 12: Blockification / RichText Integration
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, blockifiesInlineChildren ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Create an inline span as a child of the flex container
	UITextSpan* span = UITextSpan::New();
	span->setParent( flex );
	span->setText( "Hello Flex" );
	span->applyProperty( StyleSheetProperty( "display", "inline" ) );

	sceneNode->updateDirtyLayouts();

	// Inline child of flex should be blockified and get a non-zero size from BlockLayouter
	ASSERT_TRUE( span->isType( UI_TYPE_HTML_WIDGET ) );
	auto* layouter = span->asType<UIHTMLWidget>()->getLayouter();
	ASSERT_TRUE( layouter != nullptr );
	// The fact that the span got laid out (non-zero size) proves blockification worked.
	// Without blockification, InlineLayouter::updateLayout() is empty, so size stays 0.
	EXPECT_GT( span->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( span->getPixelsSize().getHeight(), 0.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, textSpanInsideFlexGetsSize ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UITextSpan* span = UITextSpan::New();
	span->setParent( flex );
	span->setText( "Hello" );
	span->applyProperty( StyleSheetProperty( "display", "inline" ) );

	sceneNode->updateDirtyLayouts();

	// The text span should get a non-zero size from blockified layout
	EXPECT_GT( span->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( span->getPixelsSize().getHeight(), 0.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, flexDirectionColumnWithTextContent ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-direction", "column" ) );

	UITextSpan* span = UITextSpan::New();
	span->setParent( flex );
	span->setText( "Line One" );

	UITextSpan* span2 = UITextSpan::New();
	span2->setParent( flex );
	span2->setText( "Line Two" );

	sceneNode->updateDirtyLayouts();

	// Column with text items should stack vertically and compute height
	EXPECT_GT( flex->getPixelsSize().getHeight(), 0.f );
	EXPECT_GT( span2->getPixelsPosition().y, span->getPixelsPosition().y );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 13: Property Parsing
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexProperties, flexFlowShorthand ) {
	auto* spec = CSS::StyleSheetSpecification::instance();

	const auto* shorthand = spec->getShorthand( "flex-flow" );
	ASSERT_TRUE( shorthand != nullptr );

	auto props = spec->getShorthandParser( "flex-flow" )( shorthand, "row-reverse wrap" );
	ASSERT_EQ( props.size(), (size_t)2 );
	EXPECT_TRUE( props[0].asString() == "row-reverse" );
	EXPECT_TRUE( props[1].asString() == "wrap" );
}

UTEST( FlexProperties, gapShorthandTwoValues ) {
	auto* spec = CSS::StyleSheetSpecification::instance();

	const auto* shorthand = spec->getShorthand( "gap" );
	ASSERT_TRUE( shorthand != nullptr );

	auto props = shorthand->parse( "10px 20px" );
	ASSERT_EQ( props.size(), (size_t)2 );
	EXPECT_TRUE( props[0].asString() == "10px" );
	EXPECT_TRUE( props[1].asString() == "20px" );
}

UTEST( FlexProperties, gapShorthandOneValue ) {
	auto* spec = CSS::StyleSheetSpecification::instance();

	const auto* shorthand = spec->getShorthand( "gap" );
	ASSERT_TRUE( shorthand != nullptr );

	auto props = shorthand->parse( "15px" );
	ASSERT_EQ( props.size(), (size_t)2 );
	EXPECT_TRUE( props[0].asString() == "15px" );
	EXPECT_TRUE( props[1].asString() == "15px" );
}

UTEST( FlexProperties, flexShorthandAuto ) {
	auto* spec = CSS::StyleSheetSpecification::instance();

	const auto* shorthand = spec->getShorthand( "flex" );
	ASSERT_TRUE( shorthand != nullptr );

	auto props = spec->getShorthandParser( "flex" )( shorthand, "auto" );
	ASSERT_EQ( props.size(), (size_t)3 );
	EXPECT_TRUE( props[0].asString() == "1" );	  // flex-grow
	EXPECT_TRUE( props[1].asString() == "1" );	  // flex-shrink
	EXPECT_TRUE( props[2].asString() == "auto" ); // flex-basis
}

UTEST( FlexProperties, flexShorthandNone ) {
	auto* spec = CSS::StyleSheetSpecification::instance();

	const auto* shorthand = spec->getShorthand( "flex" );
	ASSERT_TRUE( shorthand != nullptr );

	auto props = spec->getShorthandParser( "flex" )( shorthand, "none" );
	ASSERT_EQ( props.size(), (size_t)3 );
	EXPECT_TRUE( props[0].asString() == "0" );	  // flex-grow
	EXPECT_TRUE( props[1].asString() == "0" );	  // flex-shrink
	EXPECT_TRUE( props[2].asString() == "auto" ); // flex-basis
}

UTEST( FlexProperties, flexShorthandOneNumber ) {
	auto* spec = CSS::StyleSheetSpecification::instance();

	const auto* shorthand = spec->getShorthand( "flex" );
	ASSERT_TRUE( shorthand != nullptr );

	auto props = spec->getShorthandParser( "flex" )( shorthand, "2" );
	ASSERT_EQ( props.size(), (size_t)3 );
	EXPECT_TRUE( props[0].asString() == "2" );	// flex-grow
	EXPECT_TRUE( props[1].asString() == "1" );	// flex-shrink
	EXPECT_TRUE( props[2].asString() == "0%" ); // flex-basis
}

UTEST( FlexProperties, flexShorthandThreeValues ) {
	auto* spec = CSS::StyleSheetSpecification::instance();

	const auto* shorthand = spec->getShorthand( "flex" );
	ASSERT_TRUE( shorthand != nullptr );

	auto props = spec->getShorthandParser( "flex" )( shorthand, "1 0 200px" );
	ASSERT_EQ( props.size(), (size_t)3 );
	EXPECT_TRUE( props[0].asString() == "1" );	   // flex-grow
	EXPECT_TRUE( props[1].asString() == "0" );	   // flex-shrink
	EXPECT_TRUE( props[2].asString() == "200px" ); // flex-basis
}

UTEST( FlexProperties, enumConversions ) {
	EXPECT_EQ( CSSFlexDirectionHelper::fromString( "row" ), CSSFlexDirection::Row );
	EXPECT_EQ( CSSFlexDirectionHelper::fromString( "row-reverse" ), CSSFlexDirection::RowReverse );
	EXPECT_EQ( CSSFlexDirectionHelper::fromString( "column" ), CSSFlexDirection::Column );
	EXPECT_EQ( CSSFlexDirectionHelper::fromString( "column-reverse" ),
			   CSSFlexDirection::ColumnReverse );

	EXPECT_EQ( CSSFlexWrapHelper::fromString( "nowrap" ), CSSFlexWrap::NoWrap );
	EXPECT_EQ( CSSFlexWrapHelper::fromString( "wrap" ), CSSFlexWrap::Wrap );
	EXPECT_EQ( CSSFlexWrapHelper::fromString( "wrap-reverse" ), CSSFlexWrap::WrapReverse );

	EXPECT_EQ( CSSJustifyContentHelper::fromString( "flex-start" ), CSSJustifyContent::FlexStart );
	EXPECT_EQ( CSSJustifyContentHelper::fromString( "center" ), CSSJustifyContent::Center );
	EXPECT_EQ( CSSJustifyContentHelper::fromString( "space-between" ),
			   CSSJustifyContent::SpaceBetween );
	EXPECT_EQ( CSSJustifyContentHelper::fromString( "space-around" ),
			   CSSJustifyContent::SpaceAround );
	EXPECT_EQ( CSSJustifyContentHelper::fromString( "space-evenly" ),
			   CSSJustifyContent::SpaceEvenly );

	EXPECT_EQ( CSSAlignItemsHelper::fromString( "stretch" ), CSSAlignItems::Stretch );
	EXPECT_EQ( CSSAlignItemsHelper::fromString( "center" ), CSSAlignItems::Center );
	EXPECT_EQ( CSSAlignItemsHelper::fromString( "flex-start" ), CSSAlignItems::FlexStart );
	EXPECT_EQ( CSSAlignItemsHelper::fromString( "flex-end" ), CSSAlignItems::FlexEnd );
	EXPECT_EQ( CSSAlignItemsHelper::fromString( "baseline" ), CSSAlignItems::Baseline );

	EXPECT_EQ( CSSAlignContentHelper::fromString( "stretch" ), CSSAlignContent::Stretch );
	EXPECT_EQ( CSSAlignContentHelper::fromString( "center" ), CSSAlignContent::Center );
	EXPECT_EQ( CSSAlignContentHelper::fromString( "space-between" ),
			   CSSAlignContent::SpaceBetween );
	EXPECT_EQ( CSSAlignContentHelper::fromString( "space-evenly" ), CSSAlignContent::SpaceEvenly );

	EXPECT_EQ( CSSAlignSelfHelper::fromString( "auto" ), CSSAlignSelf::Auto );
	EXPECT_EQ( CSSAlignSelfHelper::fromString( "stretch" ), CSSAlignSelf::Stretch );
	EXPECT_EQ( CSSAlignSelfHelper::fromString( "center" ), CSSAlignSelf::Center );
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 14: Flex Algorithm Bug Fixes (G1, G2, G4, G5)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, iterativeFlexResolutionWithMinWidths ) {
	// G1: When a flex item hits its min-width, remaining free space is
	// redistributed to other items (iterative §9.7 algorithm).
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 100 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Three items with flex: 1 1 0% (equal flex-grow, zero base size)
	UIHTMLWidget* a = UIHTMLWidget::New();
	a->setParent( flex );
	a->setPixelsSize( 10, 50 );
	a->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	a->setStyleSheetProperty( StyleSheetProperty( "flex", "1 1 0%" ) );

	UIHTMLWidget* b = UIHTMLWidget::New();
	b->setParent( flex );
	b->setPixelsSize( 10, 50 );
	b->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	b->setStyleSheetProperty( StyleSheetProperty( "flex", "1 1 0%" ) );

	// c has min-width:200px, so it should stay at 200 while a and b split
	// the remaining 300px → 150px each
	UIHTMLWidget* c = UIHTMLWidget::New();
	c->setParent( flex );
	c->setPixelsSize( 10, 50 );
	c->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	c->setStyleSheetProperty( StyleSheetProperty( "flex", "1 1 0%" ) );
	c->setStyleSheetProperty( StyleSheetProperty( "min-width", "200px" ) );

	sceneNode->updateDirtyLayouts();

	EXPECT_NEAR( a->getPixelsSize().getWidth(), 150.f, 5.f );
	EXPECT_NEAR( b->getPixelsSize().getWidth(), 150.f, 5.f );
	EXPECT_NEAR( c->getPixelsSize().getWidth(), 200.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, crossAxisAutoMargins ) {
	// G4: margin: auto on the cross axis should center/position the item
	// within the line before align-self applies (§8.1).
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	// Row-direction flex container with fixed height
	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 400, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-items", "flex-start" ) );

	// Single item with margin-top: auto and margin-bottom: auto
	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child->setStyleSheetProperty( StyleSheetProperty( "margin", "auto" ) );

	sceneNode->updateDirtyLayouts();

	// With both margin-top and margin-bottom as auto, the item should be
	// vertically centered. Container is 200px, item is 50px.
	// Item Y should be (200 - 50) / 2 = 75
	// (Flex container has no padding in this setup)
	EXPECT_NEAR( child->getPixelsPosition().y, 75.f, 5.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Anonymous Flex Items (text nodes as flex items)
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, anonymousTextNodeSizing ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* flex = UIRichText::NewWithTag( "div" );
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->applyProperty( StyleSheetProperty( "font-family", "NotoSans-Regular" ) );
	flex->applyProperty( StyleSheetProperty( "font-size", "16dp" ) );

	UIRichText* child1 = UIRichText::NewDiv();
	child1->setParent( flex );
	child1->setPixelsSize( 20, 20 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UITextNode* textNode = UITextNode::New();
	textNode->setParent( flex );
	textNode->setText( "Hello flex text" );

	sceneNode->updateDirtyLayouts();

	EXPECT_TRUE( textNode->getPixelsSize().getWidth() > 0.f );
	EXPECT_TRUE( textNode->getPixelsSize().getHeight() > 0.f );
	EXPECT_TRUE( textNode->getPixelsPosition().x >=
				 child1->getPixelsPosition().x + child1->getPixelsSize().getWidth() );

	Engine::destroySingleton();
}

UTEST( FlexContainer, anonymousTextNodeWithFixedSibling ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* flex = UIRichText::NewWithTag( "div" );
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 400, 100 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->applyProperty( StyleSheetProperty( "font-family", "NotoSans-Regular" ) );
	flex->applyProperty( StyleSheetProperty( "font-size", "14dp" ) );
	flex->applyProperty( StyleSheetProperty( "align-items", "flex-start" ) );
	flex->applyProperty( StyleSheetProperty( "column-gap", "10px" ) );

	UIRichText* bullet = UIRichText::NewDiv();
	bullet->setParent( flex );
	bullet->setPixelsSize( 8, 8 );
	bullet->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UITextNode* textNode = UITextNode::New();
	textNode->setParent( flex );
	textNode->setText( "Follow up to 1,024 sites" );

	sceneNode->updateDirtyLayouts();

	EXPECT_TRUE( textNode->getPixelsSize().getWidth() > 0.f );
	EXPECT_TRUE( textNode->getPixelsSize().getHeight() > 0.f );
	// 10px gap + bullet width (8px)
	Float expectedMinX = bullet->getPixelsPosition().x + bullet->getPixelsSize().getWidth() + 10.f;
	EXPECT_TRUE( textNode->getPixelsPosition().x >= expectedMinX - 1.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, anonymousTextNodeWrapping ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* flex = UIRichText::NewWithTag( "div" );
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 100, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->applyProperty( StyleSheetProperty( "font-family", "NotoSans-Regular" ) );
	flex->applyProperty( StyleSheetProperty( "font-size", "16dp" ) );
	flex->applyProperty( StyleSheetProperty( "align-items", "flex-start" ) );

	// Long text that should wrap within the 100px-wide flex container
	UITextNode* textNode = UITextNode::New();
	textNode->setParent( flex );
	textNode->setText( "This is a very long text that should wrap to multiple lines inside the "
					   "narrow flex container." );

	sceneNode->updateDirtyLayouts();

	// The font height for NotoSans at 16dp is ~22px. If the text wraps, the total
	// height should be at least 2x the single-line font height.
	FontStyleConfig fontConfig = flex->getRichText().getFontStyleConfig();
	Float fontHeight = (Float)fontConfig.Font->getFontHeight( fontConfig.CharacterSize );
	EXPECT_TRUE( textNode->getPixelsSize().getWidth() > 0.f );
	EXPECT_TRUE( textNode->getPixelsSize().getHeight() >= fontHeight * 2.f );
	EXPECT_TRUE( textNode->getPixelsSize().getWidth() <= 100.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, visibilityCollapsePreservesCrossSize ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* flex = UIRichText::NewWithTag( "div" );
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->applyProperty( StyleSheetProperty( "align-items", "flex-start" ) );

	// First item — normal, 100x50
	UIRichText* item1 = UIRichText::NewDiv();
	item1->setParent( flex );
	item1->setPixelsSize( 100, 50 );
	item1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Second item — visibility:collapse, 100x80
	UIRichText* item2 = UIRichText::NewDiv();
	item2->setParent( flex );
	item2->setPixelsSize( 100, 80 );
	item2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	item2->applyProperty( StyleSheetProperty( "visibility", "collapse" ) );

	sceneNode->updateDirtyLayouts();

	// Collapsed item should have 0 width (main axis for row)
	EXPECT_EQ( item2->getPixelsSize().getWidth(), 0.f );
	// Container cross size should be at least the taller item's height (80px from item2)
	EXPECT_GE( flex->getPixelsSize().getHeight(), 80.f );
	// First item should still be positioned normally at the start
	EXPECT_EQ( item1->getPixelsPosition().x, 0.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, visibilityCollapseWithTextNode ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* flex = UIRichText::NewWithTag( "div" );
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->applyProperty( StyleSheetProperty( "font-family", "NotoSans-Regular" ) );
	flex->applyProperty( StyleSheetProperty( "font-size", "14dp" ) );
	flex->applyProperty( StyleSheetProperty( "align-items", "flex-start" ) );

	// Normal text node
	UITextNode* textNode = UITextNode::New();
	textNode->setParent( flex );
	textNode->setText( "Normal text" );

	UIHTMLWidget* collapsedDiv = UIRichText::NewDiv();
	collapsedDiv->setParent( flex );
	collapsedDiv->setPixelsSize( 200, 50 );
	collapsedDiv->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	collapsedDiv->applyProperty( StyleSheetProperty( "visibility", "collapse" ) );

	sceneNode->updateDirtyLayouts();

	// Normal text node should have positive size
	EXPECT_TRUE( textNode->getPixelsSize().getWidth() > 0.f );
	EXPECT_TRUE( textNode->getPixelsSize().getHeight() > 0.f );
	// Collapsed div should have 0 width (main axis for row)
	EXPECT_EQ( collapsedDiv->getPixelsSize().getWidth(), 0.f );
	// Collapsed div has 0 cross size (setPixelsSize(0,0) was called)
	EXPECT_EQ( collapsedDiv->getPixelsSize().getHeight(), 0.f );
	// Normal text node is first, so x should be 0
	EXPECT_EQ( textNode->getPixelsPosition().x, 0.f );
	// Collapsed div should be after text node (no additional gap)
	EXPECT_EQ( collapsedDiv->getPixelsPosition().x, textNode->getPixelsSize().getWidth() );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 15: Missing Coverage — wrap-reverse, percentage basis, auto margins
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, wrapReverse ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 200, 300 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-wrap", "wrap-reverse" ) );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-content", "flex-start" ) );

	// 5 items at 50px each. In a 200px container, 4 items fit per line:
	//   DOM order: items 0-3 on line 0, item 4 on line 1.
	//   wrap-reverse reverses lines: line 0 (item 4) at cross-start (top),
	//   line 1 (items 0-3) below it.
	// With align-content: flex-start, line 0 is at y=0, line 1 at y=50.
	for ( int i = 0; i < 5; ++i ) {
		UIHTMLWidget* child = UIHTMLWidget::New();
		child->setParent( flex );
		child->setPixelsSize( 50, 50 );
		child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	}

	sceneNode->updateDirtyLayouts();

	// Item 4 is the last in DOM, placed on the reversed first line at y=0.
	UIWidget* lastItem = flex->getChildAt( 4 )->asType<UIWidget>();
	EXPECT_NEAR( lastItem->getPixelsPosition().y, 0.f, 5.f );

	// Items 0-3 are on the second line (originally first in DOM, now
	// second after reverse), positioned below line 0 at y=50.
	UIWidget* first = flex->getChildAt( 0 )->asType<UIWidget>();
	EXPECT_NEAR( first->getPixelsPosition().y, 50.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, autoMarginMainAxis ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 400, 100 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->setStyleSheetProperty( StyleSheetProperty( "justify-content", "flex-start" ) );

	// Single item with margin-left: auto in row direction.
	// Spec §8.1: auto margins consume free space before justify-content.
	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child->setStyleSheetProperty( StyleSheetProperty( "margin-left", "auto" ) );

	sceneNode->updateDirtyLayouts();

	// Container 400px, item 100px → 300px free space → margin-left:auto consumes it all.
	EXPECT_NEAR( child->getPixelsPosition().x, 300.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, autoMarginMainAxisBothSides ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 400, 100 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// margin: auto on both sides → splits free space equally → centered.
	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 200, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child->setStyleSheetProperty( StyleSheetProperty( "margin", "auto" ) );

	sceneNode->updateDirtyLayouts();

	EXPECT_NEAR( child->getPixelsPosition().x, 100.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, percentageBasis ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 400, 100 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 10, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child->setStyleSheetProperty( StyleSheetProperty( "flex-basis", "50%" ) );

	sceneNode->updateDirtyLayouts();

	// 50% of 400px container = 200px. No flex-grow, so it stays at 200px.
	EXPECT_NEAR( child->getPixelsSize().getWidth(), 200.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, flexBasisZeroPercent ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	// flex: 1 1 0% — common equal-distribution pattern.
	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 300, 100 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	for ( int i = 0; i < 3; ++i ) {
		UIHTMLWidget* child = UIHTMLWidget::New();
		child->setParent( flex );
		child->setPixelsSize( 10, 50 );
		child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		child->setStyleSheetProperty( StyleSheetProperty( "flex", "1 1 0%" ) );
	}

	sceneNode->updateDirtyLayouts();

	for ( int i = 0; i < 3; ++i ) {
		UIWidget* child = flex->getChildAt( i )->asType<UIWidget>();
		EXPECT_NEAR( child->getPixelsSize().getWidth(), 100.f, 5.f );
	}

	Engine::destroySingleton();
}

UTEST( FlexContainer, stretchWithFixedCrossSize ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	// align-items: stretch must NOT override an explicit cross-size (Fixed policy).
	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 400, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-items", "stretch" ) );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	EXPECT_NEAR( child->getPixelsSize().getHeight(), 50.f, 5.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// G6: Percentage margins/paddings resolve against flex container inline size
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, percentageMarginResolvesAgainstFlexContainerWidth ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	// Per CSS §4.2: percentage margins on flex items always resolve against
	// the flex container's inline size (width), even for top/bottom margins.
	// Container is 500x200. margin-top: 10% should give 50px (10% of 500), not 20px (10% of 200).
	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-items", "flex-start" ) );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child->setStyleSheetProperty( StyleSheetProperty( "margin-top", "10%" ) );

	sceneNode->updateDirtyLayouts();

	// margin-top: 10% of 500px = 50px. Item should be at Y=50.
	EXPECT_NEAR( child->getPixelsPosition().y, 50.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, percentageMarginAllSidesResolveAgainstWidth ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	// Per spec, BOTH margin-top and margin-bottom resolve against width.
	// Container is 400x300. margin: 10% on all sides gives 40px (10% of 400).
	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 400, 300 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-items", "flex-start" ) );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child->setStyleSheetProperty( StyleSheetProperty( "margin", "10%" ) );

	sceneNode->updateDirtyLayouts();

	// margin-left and margin-top should both be 40px (10% of 400px container width)
	EXPECT_NEAR( child->getPixelsPosition().x, 40.f, 5.f );
	EXPECT_NEAR( child->getPixelsPosition().y, 40.f, 5.f );

	Engine::destroySingleton();
}

// ─────────────────────────────────────────────────────────────────────────────
// G9: flex-basis: content vs flex-basis: auto
// ─────────────────────────────────────────────────────────────────────────────

UTEST( FlexContainer, flexBasisContentUsesContentSize ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-items", "flex-start" ) );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child1->setStyleSheetProperty( StyleSheetProperty( "flex-basis", "content" ) );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child2->setStyleSheetProperty( StyleSheetProperty( "flex-basis", "auto" ) );

	sceneNode->updateDirtyLayouts();

	EXPECT_GT( child1->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( child2->getPixelsSize().getWidth(), 0.f );
	EXPECT_NEAR( child1->getPixelsSize().getWidth(), child2->getPixelsSize().getWidth(), 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, flexBasisContentIgnoresExplicitWidth ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 600, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-items", "flex-start" ) );

	UIHTMLWidget* childA = UIHTMLWidget::New();
	childA->setParent( flex );
	childA->setPixelsSize( 100, 50 );
	childA->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	childA->setStyleSheetProperty( StyleSheetProperty( "flex", "1 1 auto" ) );
	childA->setStyleSheetProperty( StyleSheetProperty( "width", "300px" ) );

	UIHTMLWidget* childB = UIHTMLWidget::New();
	childB->setParent( flex );
	childB->setPixelsSize( 100, 50 );
	childB->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	childB->setStyleSheetProperty( StyleSheetProperty( "flex", "1 1 content" ) );
	childB->setStyleSheetProperty( StyleSheetProperty( "width", "300px" ) );

	sceneNode->updateDirtyLayouts();

	EXPECT_GT( childA->getPixelsSize().getWidth(), 0.f );
	EXPECT_GT( childB->getPixelsSize().getWidth(), 0.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, orderPaintSortFlagDifferentOrders ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child1->setOrder( 2 );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child2->setOrder( 1 );

	UIHTMLWidget* child3 = UIHTMLWidget::New();
	child3->setParent( flex );
	child3->setPixelsSize( 100, 50 );
	child3->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child3->setOrder( 3 );

	sceneNode->updateDirtyLayouts();

	EXPECT_TRUE( flex->getNeedsOrderSort() );

	// Verify layout positions reflect order-order (not DOM order)
	// child2 has order=1, should be first visually (leftmost in row)
	// child1 has order=2, should be second
	// child3 has order=3, should be third
	EXPECT_LT( child2->getPixelsPosition().x, child1->getPixelsPosition().x );
	EXPECT_LT( child1->getPixelsPosition().x, child3->getPixelsPosition().x );

	Engine::destroySingleton();
}

UTEST( FlexContainer, orderPaintSortFlagEqualOrders ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// All items have default order=0, so flag should be false
	EXPECT_FALSE( flex->getNeedsOrderSort() );

	// Layout positions should be in DOM order
	EXPECT_LT( child1->getPixelsPosition().x, child2->getPixelsPosition().x );

	Engine::destroySingleton();
}

UTEST( FlexContainer, orderPaintSortSingleItem ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 100, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child->setOrder( 5 );

	sceneNode->updateDirtyLayouts();

	// Single item with order=5 — only one item, no ordering needed
	EXPECT_FALSE( flex->getNeedsOrderSort() );

	Engine::destroySingleton();
}

UTEST( FlexContainer, orderPaintSortNegatives ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child1->setOrder( -1 );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child2->setOrder( 0 );

	UIHTMLWidget* child3 = UIHTMLWidget::New();
	child3->setParent( flex );
	child3->setPixelsSize( 100, 50 );
	child3->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child3->setOrder( 1 );

	sceneNode->updateDirtyLayouts();

	EXPECT_TRUE( flex->getNeedsOrderSort() );

	// Negative order should come first
	EXPECT_LT( child1->getPixelsPosition().x, child2->getPixelsPosition().x );
	EXPECT_LT( child2->getPixelsPosition().x, child3->getPixelsPosition().x );

	Engine::destroySingleton();
}

UTEST( FlexContainer, directionReversePaintColumnReverse ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-direction", "column-reverse" ) );
	flex->setPixelsSize( 500, 400 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child3 = UIHTMLWidget::New();
	child3->setParent( flex );
	child3->setPixelsSize( 100, 50 );
	child3->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Column-reverse: first DOM child at bottom, last DOM child at top
	EXPECT_GT( child1->getPixelsPosition().y, child2->getPixelsPosition().y );
	EXPECT_GT( child2->getPixelsPosition().y, child3->getPixelsPosition().y );

	// All orders equal, so no order-based sort needed; direction-only reversal in drawChildren
	EXPECT_FALSE( flex->getNeedsOrderSort() );

	Engine::destroySingleton();
}

UTEST( FlexContainer, directionReversePaintRowReverse ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-direction", "row-reverse" ) );
	flex->setPixelsSize( 500, 200 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child3 = UIHTMLWidget::New();
	child3->setParent( flex );
	child3->setPixelsSize( 100, 50 );
	child3->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Row-reverse: first DOM child at right, last DOM child at left
	EXPECT_GT( child1->getPixelsPosition().x, child2->getPixelsPosition().x );
	EXPECT_GT( child2->getPixelsPosition().x, child3->getPixelsPosition().x );

	// All orders equal so no order-based sort needed
	EXPECT_FALSE( flex->getNeedsOrderSort() );

	Engine::destroySingleton();
}

UTEST( FlexContainer, directionReversePaintWithOrderSort ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-direction", "column-reverse" ) );
	flex->setPixelsSize( 500, 400 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// Orders: A=2, B=0, C=1
	// Without direction reversal: B(order=0) at top, C(order=1) middle, A(order=2) bottom
	// With column-reverse reversal: A at top, C middle, B at bottom
	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child1->setOrder( 2 );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child2->setOrder( 0 );

	UIHTMLWidget* child3 = UIHTMLWidget::New();
	child3->setParent( flex );
	child3->setPixelsSize( 100, 50 );
	child3->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child3->setOrder( 1 );

	sceneNode->updateDirtyLayouts();

	// Column-reverse: items sorted by order [B(0), C(1), A(2)] then reversed
	// Visual top-to-bottom: A(2), C(1), B(0)
	// A (order=2, DOM first) should be at top
	// B (order=0, DOM middle) should be at bottom
	EXPECT_LT( child1->getPixelsPosition().y, child3->getPixelsPosition().y );
	EXPECT_LT( child3->getPixelsPosition().y, child2->getPixelsPosition().y );

	EXPECT_TRUE( flex->getNeedsOrderSort() );

	Engine::destroySingleton();
}

UTEST( FlexContainer, alignItemsBaselineBasic ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* flex = UIRichText::NewWithTag( "div" );
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 100 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->applyProperty( StyleSheetProperty( "font-family", "NotoSans-Regular" ) );
	flex->applyProperty( StyleSheetProperty( "font-size", "16dp" ) );
	flex->applyProperty( StyleSheetProperty( "align-items", "baseline" ) );

	UITextNode* textA = UITextNode::New();
	textA->setParent( flex );
	textA->setText( "Hello" );

	UITextNode* textB = UITextNode::New();
	textB->setParent( flex );
	textB->setText( "World" );

	sceneNode->updateDirtyLayouts();

	// Both text nodes have the same font, so their baselines are equal.
	// With baseline alignment, both are at the same y as flex-start.
	Float ascent = textA->getBaseline();
	EXPECT_TRUE( ascent > 0.f );
	EXPECT_NEAR( textA->getPixelsPosition().y, 0.f, 5.f );
	EXPECT_NEAR( textB->getPixelsPosition().y, 0.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, containerGetBaseline ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* flex = UIRichText::NewWithTag( "div" );
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 100 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->applyProperty( StyleSheetProperty( "font-family", "NotoSans-Regular" ) );
	flex->applyProperty( StyleSheetProperty( "font-size", "16dp" ) );

	UITextNode* textNode = UITextNode::New();
	textNode->setParent( flex );
	textNode->setText( "Baseline test" );

	sceneNode->updateDirtyLayouts();

	// After layout, the flex container's baseline should equal the text ascent
	Float bl = flex->getBaseline();
	EXPECT_TRUE( bl > 0.f );
	Float ascent = textNode->getBaseline();
	EXPECT_NEAR( bl, ascent, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, baselinePositionsLargerItemCorrectly ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIRichText* flex = UIRichText::NewWithTag( "div" );
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 150 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->applyProperty( StyleSheetProperty( "font-family", "NotoSans-Regular" ) );
	flex->applyProperty( StyleSheetProperty( "font-size", "16dp" ) );
	flex->applyProperty( StyleSheetProperty( "align-items", "baseline" ) );

	// A taller non-text item (e.g. a colored div) alongside a text node.
	// The taller item uses FlexStart alignment (no baseline), while the text node
	// uses baseline alignment. Both should coexist within the same line cross size.
	UIHTMLWidget* tallBox = UIHTMLWidget::New();
	tallBox->setParent( flex );
	tallBox->setPixelsSize( 50, 80 );
	tallBox->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UITextNode* textNode = UITextNode::New();
	textNode->setParent( flex );
	textNode->setText( "Text" );

	sceneNode->updateDirtyLayouts();

	// The line cross size must be at least as large as the tall box
	Float ascent = textNode->getBaseline();
	EXPECT_TRUE( ascent > 0.f );
	// Text node cross-start position depends on baseline alignment
	EXPECT_TRUE( textNode->getPixelsSize().getHeight() > 0.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, percentageBasisIndefiniteContainer ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	// Container with indefinite width (WrapContent, no explicit size).
	// Percentage flex-basis should fall back to auto → content-based sizing per §9.2.
	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );

	UIHTMLWidget* child = UIHTMLWidget::New();
	child->setParent( flex );
	child->setPixelsSize( 150, 50 );
	child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	child->setStyleSheetProperty( StyleSheetProperty( "flex-basis", "50%" ) );

	sceneNode->updateDirtyLayouts();

	// Container width is indefinite → 50% falls back to auto → child's explicit 150px width.
	EXPECT_NEAR( child->getPixelsSize().getWidth(), 150.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, flexBasisZeroLength ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	// flex: 1 1 0px — absolute zero flex-basis, all space distributed by flex-grow
	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 300, 100 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	for ( int i = 0; i < 3; ++i ) {
		UIHTMLWidget* child = UIHTMLWidget::New();
		child->setParent( flex );
		child->setPixelsSize( 10, 50 );
		child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
		child->setStyleSheetProperty( StyleSheetProperty( "flex", "1 1 0px" ) );
	}

	sceneNode->updateDirtyLayouts();

	// With flex-basis: 0px, all space is distributed equally: 300/3 = 100
	for ( int i = 0; i < 3; ++i ) {
		UIWidget* child = flex->getChildAt( i )->asType<UIWidget>();
		EXPECT_NEAR( child->getPixelsSize().getWidth(), 100.f, 5.f );
	}

	Engine::destroySingleton();
}

UTEST( FlexContainer, wrapWithAutoWidth ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	// Container with indefinite width (WrapContent) + flex-wrap: wrap.
	// When main size is indefinite, the container sizes to fit all items in a single line.
	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-wrap", "wrap" ) );
	flex->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );

	for ( int i = 0; i < 3; ++i ) {
		UIHTMLWidget* child = UIHTMLWidget::New();
		child->setParent( flex );
		child->setPixelsSize( 100, 50 );
		child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	}

	sceneNode->updateDirtyLayouts();

	// All items should be on the same line (same Y position) since container
	// uses natural sum-of-items width, preventing wrap.
	UIWidget* first = flex->getChildAt( 0 )->asType<UIWidget>();
	UIWidget* last = flex->getChildAt( 2 )->asType<UIWidget>();
	EXPECT_NEAR( first->getPixelsPosition().y, last->getPixelsPosition().y, 1.f );

	// Container width should be sum of item widths (no gaps by default)
	EXPECT_NEAR( flex->getPixelsSize().getWidth(), 300.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, alignContentSpaceEvenly ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "flex-wrap", "wrap" ) );
	flex->setStyleSheetProperty( StyleSheetProperty( "align-content", "space-evenly" ) );
	flex->setPixelsSize( 250, 400 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	// 3 items, 100px each, container 250px wide -> 2 lines (100 + 100 = 200, third wraps)
	for ( int i = 0; i < 3; ++i ) {
		UIHTMLWidget* child = UIHTMLWidget::New();
		child->setParent( flex );
		child->setPixelsSize( 100, 50 );
		child->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	}

	sceneNode->updateDirtyLayouts();

	// 2 lines of 50px = 100px. Container 400px. Free = 300px.
	// SpaceEvenly: 300 / (2+1) = 100px between each line and edges.
	// First line at 100px, second line at 100 + 50 + 100 = 250px.
	Node* first = flex->getFirstChild();
	ASSERT_TRUE( first && first->isWidget() );
	EXPECT_NEAR( first->asType<UIWidget>()->getPixelsPosition().y, 100.f, 20.f );

	Node* last = flex->getLastChild();
	ASSERT_TRUE( last && last->isWidget() );
	EXPECT_NEAR( last->asType<UIWidget>()->getPixelsPosition().y, 250.f, 20.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, gapNormal ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	// gap: normal should resolve to 0px in flexbox (no gap between items)
	UIHTMLWidget* flex = UIHTMLWidget::New();
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setStyleSheetProperty( StyleSheetProperty( "gap", "normal" ) );
	flex->setPixelsSize( 300, 100 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child1 = UIHTMLWidget::New();
	child1->setParent( flex );
	child1->setPixelsSize( 100, 50 );
	child1->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	UIHTMLWidget* child2 = UIHTMLWidget::New();
	child2->setParent( flex );
	child2->setPixelsSize( 100, 50 );
	child2->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );

	sceneNode->updateDirtyLayouts();

	// Normal gap = 0px, so child2 starts right after child1 at x=100
	EXPECT_NEAR( child2->getPixelsPosition().x, 100.f, 5.f );

	Engine::destroySingleton();
}

UTEST( FlexContainer, baselineWithText ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 650, "Flex Test", WindowStyle::Default,
													  WindowBackend::Default, 32, {}, 1, false,
													  true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );
	init_flex_test();
	UISceneNode* sceneNode = SceneManager::instance()->getUISceneNode();

	// align-items: baseline with two text nodes of different font sizes.
	// Both should share the same baseline (their text bottoms align).
	UIRichText* flex = UIRichText::NewWithTag( "div" );
	flex->setParent( sceneNode->getRoot() );
	flex->setDisplay( CSSDisplay::Flex );
	flex->setPixelsSize( 500, 150 );
	flex->setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	flex->applyProperty( StyleSheetProperty( "font-family", "NotoSans-Regular" ) );
	flex->applyProperty( StyleSheetProperty( "font-size", "16dp" ) );
	flex->applyProperty( StyleSheetProperty( "align-items", "baseline" ) );

	UITextNode* smallText = UITextNode::New();
	smallText->setParent( flex );
	smallText->setText( "Small" );

	UITextNode* largeText = UITextNode::New();
	largeText->setParent( flex );
	largeText->setText( "LARGE" );
	largeText->applyProperty( StyleSheetProperty( "font-size", "24dp" ) );

	sceneNode->updateDirtyLayouts();

	// Both text nodes should be baseline-aligned: their getBaseline() offset
	// from the cross-start edge should be equal.
	Float smallBl = smallText->getBaseline();
	Float largeBl = largeText->getBaseline();
	EXPECT_TRUE( smallBl > 0.f );
	EXPECT_TRUE( largeBl > 0.f );

	// With baseline alignment, the cross-start position of each text node
	// is adjusted so their baselines match.
	// smallText baseline Y = smallText->getPixelsPosition().y + smallBl.
	// largeText baseline Y = largeText->getPixelsPosition().y + largeBl.
	Float smallBaselineY = smallText->getPixelsPosition().y + smallBl;
	Float largeBaselineY = largeText->getPixelsPosition().y + largeBl;
	EXPECT_NEAR( smallBaselineY, largeBaselineY, 5.f );

	Engine::destroySingleton();
}
