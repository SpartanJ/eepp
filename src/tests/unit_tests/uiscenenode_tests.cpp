#include "utest.hpp"

#include <eepp/graphics/fontfamily.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uifiledialog.hpp>
#include <eepp/ui/uiroot.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/cursor.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Window;
using namespace EE::Scene;
using namespace EE::UI;

UTEST( UISceneNode, CssPointerCursorUsesHandCursor ) {
	EXPECT_EQ( Cursor::fromName( "pointer" ), Cursor::Hand );
	EXPECT_EQ( Cursor::fromName( "POINTER" ), Cursor::Hand );
	EXPECT_STREQ( Cursor::toName( Cursor::Hand ), "hand" );
	EXPECT_STREQ( Cursor::toName( Cursor::Arrow ), "arrow" );
}

UTEST( Node, DescendantWorldBoundsRefreshAfterDirtyAncestorMoves ) {
	Node* parent = Node::New();
	Node* child = Node::New();
	Node* descendant = Node::New();
	child->setParent( parent );
	descendant->setParent( child );
	child->setPosition( 10.f, 0.f );
	descendant->setPosition( 5.f, 0.f );
	descendant->setSize( 10.f, 10.f );

	EXPECT_EQ( 15.f, descendant->getWorldBounds().Left );
	child->setPosition( 20.f, 0.f );
	EXPECT_EQ( 25.f, descendant->getScreenRect().Left );
	EXPECT_EQ( 25.f, descendant->getWorldBounds().Left );

	eeDelete( parent );
}

static void init_test_scene_node( UISceneNode* sceneNode ) {
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
	FontTrueType* font = FontTrueType::New( "NotoSans-Regular" ).get();
	font->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	FontFamily::loadFromRegular( font );
	FontTrueType* monoFont = FontTrueType::New( "monospace" ).get();
	monoFont->loadFromFile( "../assets/fonts/NotoSans-Regular.ttf" );
	SceneManager::instance()->add( sceneNode );
	SceneManager::instance()->setCurrentUISceneNode( sceneNode );
	UIThemeManager* themeManager = sceneNode->getUIThemeManager();
	themeManager->setDefaultFont( font );
	themeManager->applyDefaultTheme( sceneNode->getRoot() );
}

static UISceneNode* init_test_scene_node() {
	UISceneNode* sceneNode = UISceneNode::New();
	init_test_scene_node( sceneNode );
	return sceneNode;
}

class InvalidationTestSceneNode : public UISceneNode {
  public:
	static InvalidationTestSceneNode* New() { return eeNew( InvalidationTestSceneNode, () ); }

	size_t pendingStyleCount() const { return mDirtyStyle.size(); }

	size_t pendingStyleStateCount() const { return mDirtyStyleState.size(); }

	size_t pendingStyleStateAnimationCount() const { return mDirtyStyleStateCSSAnimations.size(); }

	size_t processedStyleRootCount() const { return mDirtyStylesSnapshot.size(); }

	UIWidget* processedStyleRoot( size_t index ) const {
		return mDirtyStylesSnapshot[index].first;
	}

	bool processedStyleRootDisablesAnimations( size_t index ) const {
		return mDirtyStylesSnapshot[index].second;
	}

  protected:
	InvalidationTestSceneNode() : UISceneNode() {}
};

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

UTEST( UISceneNode, LayoutViewportRootTraversesDocumentExtentForHitTesting ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Embedded Scene Hit Test Bounds",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* hostScene = init_test_scene_node();
	UIWidget* host = UIWidget::New();
	host->setPixelsSize( 800, 600 );
	host->setParent( hostScene->getRoot() );

	UISceneNode* embeddedScene = UISceneNode::New();
	embeddedScene->setFollowParentSize( false );
	embeddedScene->setParent( host );
	embeddedScene->setPixelsSize( 800, 1000 );
	embeddedScene->setLayoutViewportPixelsSize( Sizef( 800, 300 ) );

	UIWidget* target = UIWidget::New();
	target->setId( "below-layout-viewport" );
	target->setPixelsSize( 100, 50 );
	target->setPosition( 10, 900 );
	target->setParent( embeddedScene->getRoot() );

	UIRoot* embeddedRoot = embeddedScene->getRoot()->asType<UIRoot>();
	ASSERT_TRUE( embeddedRoot != nullptr );
	ASSERT_TRUE( embeddedRoot->hasChildHitTestTraversalPixelsSize() );
	EXPECT_NEAR( embeddedScene->getRoot()->getPixelsSize().getHeight(), 300.f, 0.5f );
	EXPECT_NEAR( embeddedRoot->getChildHitTestTraversalPixelsSize().getHeight(), 1000.f, 0.5f );

	Node* hit = embeddedScene->overFind( Vector2f( 20, 920 ) );
	EXPECT_EQ( hit, target );

	embeddedScene->clearLayoutViewportPixelsSize();
	EXPECT_FALSE( embeddedRoot->hasChildHitTestTraversalPixelsSize() );

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
	UIWidget* nestedHostWidget = UIWidget::New();
	nestedHostWidget->setParent( nestedScene->getRoot() );
	UISceneNode* nestedChildScene = UISceneNode::New();
	nestedChildScene->setParent( nestedHostWidget );

	EXPECT_TRUE( nestedScene->getEventDispatcher() == hostSceneA->getEventDispatcher() );
	EXPECT_EQ( nestedScene->getColorSchemePreference(), ColorSchemePreference::Light );
	EXPECT_EQ( hostSceneA->getChildUISceneNodes().size(), 1u );
	EXPECT_TRUE( hostSceneA->getChildUISceneNodes()[0] == nestedScene );
	EXPECT_EQ( nestedScene->getChildUISceneNodes().size(), 1u );
	EXPECT_TRUE( nestedScene->getChildUISceneNodes()[0] == nestedChildScene );

	hostSceneA->setHighlightOverRecursive( true );
	hostSceneA->setHighlightFocusRecursive( true );
	hostSceneA->setDrawBoxesRecursive( true );
	hostSceneA->setDrawDebugDataRecursive( true );

	EXPECT_TRUE( hostSceneA->getHighlightOver() );
	EXPECT_TRUE( hostSceneA->getHighlightFocus() );
	EXPECT_TRUE( hostSceneA->getDrawBoxes() );
	EXPECT_TRUE( hostSceneA->getDrawDebugData() );
	EXPECT_TRUE( nestedScene->getHighlightOver() );
	EXPECT_TRUE( nestedScene->getHighlightFocus() );
	EXPECT_TRUE( nestedScene->getDrawBoxes() );
	EXPECT_TRUE( nestedScene->getDrawDebugData() );
	EXPECT_TRUE( nestedChildScene->getHighlightOver() );
	EXPECT_TRUE( nestedChildScene->getHighlightFocus() );
	EXPECT_TRUE( nestedChildScene->getDrawBoxes() );
	EXPECT_TRUE( nestedChildScene->getDrawDebugData() );

	nestedScene->setHighlightOverRecursive( false );
	nestedScene->setHighlightFocusRecursive( false );
	nestedScene->setDrawBoxesRecursive( false );
	nestedScene->setDrawDebugDataRecursive( false );

	EXPECT_TRUE( hostSceneA->getHighlightOver() );
	EXPECT_TRUE( hostSceneA->getHighlightFocus() );
	EXPECT_TRUE( hostSceneA->getDrawBoxes() );
	EXPECT_TRUE( hostSceneA->getDrawDebugData() );
	EXPECT_FALSE( nestedScene->getHighlightOver() );
	EXPECT_FALSE( nestedScene->getHighlightFocus() );
	EXPECT_FALSE( nestedScene->getDrawBoxes() );
	EXPECT_FALSE( nestedScene->getDrawDebugData() );
	EXPECT_FALSE( nestedChildScene->getHighlightOver() );
	EXPECT_FALSE( nestedChildScene->getHighlightFocus() );
	EXPECT_FALSE( nestedChildScene->getDrawBoxes() );
	EXPECT_FALSE( nestedChildScene->getDrawDebugData() );

	hostWidget->setParent( hostSceneB->getRoot() );

	EXPECT_TRUE( nestedScene->getEventDispatcher() == hostSceneB->getEventDispatcher() );
	EXPECT_EQ( nestedScene->getColorSchemePreference(), ColorSchemePreference::Dark );
	EXPECT_EQ( nestedScene->getUIThemeManager()->getDefaultFontSize(), 42.f );
	EXPECT_TRUE( nestedScene->getRoot()->getUISceneNode() == nestedScene );
	EXPECT_TRUE( hostSceneA->getChildUISceneNodes().empty() );
	EXPECT_EQ( hostSceneB->getChildUISceneNodes().size(), 1u );
	EXPECT_TRUE( hostSceneB->getChildUISceneNodes()[0] == nestedScene );
	EXPECT_EQ( nestedScene->getChildUISceneNodes().size(), 1u );
	EXPECT_TRUE( nestedScene->getChildUISceneNodes()[0] == nestedChildScene );

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

UTEST( UISceneNode, StyleStateUpdateAllowsWidgetCreation ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Style State Widget Creation Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_scene_node();
	UIFileDialog* dialog = UIFileDialog::New();
	dialog->setParent( sceneNode->getRoot() );

	// Applying the select-button state creates its UIImage icon. The new widget invalidates style
	// state while the current dirty-state pass is still being processed.
	sceneNode->flushDirtyStyleAndLayout();

	EXPECT_TRUE( dialog->getButtonOpen() != nullptr );

	Engine::destroySingleton();
}

UTEST( UISceneNode, StyleInvalidationCoalescesAtProcessingTime ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Deferred Style Invalidation Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	auto* sceneNode = InvalidationTestSceneNode::New();
	init_test_scene_node( sceneNode );
	sceneNode->flushDirtyStyleAndLayout();

	UIWidget* parent = UIWidget::New();
	parent->setParent( sceneNode->getRoot() );
	sceneNode->flushDirtyStyleAndLayout();

	constexpr size_t childCount = 2048;
	UIWidget* firstChild = nullptr;
	for ( size_t i = 0; i < childCount; ++i ) {
		UIWidget* child = UIWidget::New();
		child->setParent( parent );
		if ( !firstChild )
			firstChild = child;
	}

	EXPECT_EQ( childCount, sceneNode->pendingStyleCount() );
	EXPECT_EQ( childCount, sceneNode->pendingStyleStateCount() );

	sceneNode->invalidateStyle( parent );
	sceneNode->invalidateStyleState( parent, true );
	EXPECT_EQ( childCount + 1, sceneNode->pendingStyleCount() );
	EXPECT_EQ( childCount + 1, sceneNode->pendingStyleStateCount() );

	sceneNode->updateDirtyStyles();
	EXPECT_EQ( size_t{ 1 }, sceneNode->processedStyleRootCount() );
	EXPECT_EQ( parent, sceneNode->processedStyleRoot( 0 ) );
	EXPECT_EQ( size_t{ 0 }, sceneNode->pendingStyleCount() );

	sceneNode->updateDirtyStyleStates();
	EXPECT_EQ( size_t{ 1 }, sceneNode->processedStyleRootCount() );
	EXPECT_EQ( parent, sceneNode->processedStyleRoot( 0 ) );
	EXPECT_TRUE( sceneNode->processedStyleRootDisablesAnimations( 0 ) );
	EXPECT_EQ( size_t{ 0 }, sceneNode->pendingStyleStateCount() );

	// The existing ancestor fast path still avoids queueing descendants when the parent arrives
	// first, and the parent's animation policy governs the recursive state update.
	sceneNode->invalidateStyle( parent );
	sceneNode->invalidateStyle( firstChild );
	sceneNode->invalidateStyleState( parent, false );
	sceneNode->invalidateStyleState( firstChild, true );
	EXPECT_EQ( size_t{ 1 }, sceneNode->pendingStyleCount() );
	EXPECT_EQ( size_t{ 1 }, sceneNode->pendingStyleStateCount() );
	sceneNode->updateDirtyStyles();
	EXPECT_EQ( parent, sceneNode->processedStyleRoot( 0 ) );
	sceneNode->updateDirtyStyleStates();
	EXPECT_EQ( parent, sceneNode->processedStyleRoot( 0 ) );
	EXPECT_FALSE( sceneNode->processedStyleRootDisablesAnimations( 0 ) );

	// Reparenting an already-dirty widget under a dirty parent can leave both entries queued. The
	// current tree must decide which root gets processed.
	UIWidget* reparentTarget = UIWidget::New();
	reparentTarget->setParent( sceneNode->getRoot() );
	sceneNode->flushDirtyStyleAndLayout();
	sceneNode->invalidateStyle( firstChild );
	sceneNode->invalidateStyleState( firstChild );
	sceneNode->invalidateStyle( reparentTarget );
	sceneNode->invalidateStyleState( reparentTarget, true );
	firstChild->setParent( reparentTarget );
	sceneNode->updateDirtyStyles();
	EXPECT_EQ( size_t{ 1 }, sceneNode->processedStyleRootCount() );
	EXPECT_EQ( reparentTarget, sceneNode->processedStyleRoot( 0 ) );
	sceneNode->updateDirtyStyleStates();
	EXPECT_EQ( size_t{ 1 }, sceneNode->processedStyleRootCount() );
	EXPECT_EQ( reparentTarget, sceneNode->processedStyleRoot( 0 ) );
	EXPECT_TRUE( sceneNode->processedStyleRootDisablesAnimations( 0 ) );

	UIWidget* deletedWidget = UIWidget::New();
	deletedWidget->setParent( sceneNode->getRoot() );
	sceneNode->flushDirtyStyleAndLayout();
	sceneNode->invalidateStyleState( deletedWidget, true );
	EXPECT_EQ( size_t{ 1 }, sceneNode->pendingStyleStateAnimationCount() );
	eeDelete( deletedWidget );
	EXPECT_EQ( size_t{ 0 }, sceneNode->pendingStyleStateCount() );
	EXPECT_EQ( size_t{ 0 }, sceneNode->pendingStyleStateAnimationCount() );

	Engine::destroySingleton();
}

UTEST( UIWindow, ModalWindowStopsKeyBindingsFromReachingScene ) {
	Engine::instance()->createWindow( WindowSettings( 1024, 768, "Modal Key Binding Test",
													  WindowStyle::Default, WindowBackend::Default,
													  32, {}, 1, false, true ),
									  ContextSettings( false, 0, 0, GLv_default, true, false ) );

	UISceneNode* sceneNode = init_test_scene_node();
	int executedCommands = 0;
	sceneNode->setKeyBindingCommand( "global-command",
									 [&executedCommands] { executedCommands++; } );
	sceneNode->addKeyBinding( { KEY_F, KEYMOD_LMETA | KEYMOD_LSHIFT }, "global-command" );

	UIWindow* window = UIWindow::New();
	window->setParent( sceneNode->getRoot() );
	sceneNode->getEventDispatcher()->setFocusNode( window->getContainer() );

	sceneNode->getEventDispatcher()->sendKeyDown( KEY_F, SCANCODE_UNKNOWN, 0,
												  KEYMOD_LMETA | KEYMOD_LSHIFT );
	EXPECT_EQ( executedCommands, 1 );

	window->setWindowFlags( window->getWinFlags() | UI_WIN_MODAL );
	sceneNode->getEventDispatcher()->sendKeyDown( KEY_F, SCANCODE_UNKNOWN, 0,
												  KEYMOD_LMETA | KEYMOD_LSHIFT );
	EXPECT_EQ( executedCommands, 1 );

	Engine::destroySingleton();
}
