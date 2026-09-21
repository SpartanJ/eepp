#include "utest.h"
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiconsole.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/inputevent.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;

class ScrollingTestEditor : public UICodeEditor {
  public:
	ScrollingTestEditor() : UICodeEditor() {}

	using UICodeEditor::onMouseWheel;
};

class WheelInterceptPlugin : public UICodeEditorPlugin {
  public:
	std::string getId() override { return "WheelInterceptPlugin"; }
	std::string getTitle() override { return getId(); }
	std::string getDescription() override { return getId(); }
	bool isReady() const override { return true; }
	void onRegister( UICodeEditor* ) override {}
	void onUnregister( UICodeEditor* ) override {}
	bool onMouseWheel( UICodeEditor*, const Vector2i& position, const Vector2f& offset,
					   bool flipped ) override {
		++calls;
		lastPosition = position;
		lastOffset = offset;
		lastFlipped = flipped;
		return true;
	}

	int calls{ 0 };
	Vector2i lastPosition;
	Vector2f lastOffset;
	bool lastFlipped{ false };
};

class WheelOverrideWidget : public UIWidget {
  public:
	Uint32 onMouseWheel( const Vector2f&, bool ) override {
		++calls;
		return 1;
	}

	int calls{ 0 };
};

class ScrollingTestConsole : public UIConsole {
  public:
	ScrollingTestConsole() : UIConsole( nullptr, false, false, 128 ) {}

	using UIConsole::onKeyDown;
	using UIConsole::onMouseWheel;

	void setScrollRange( Int32 maximum ) {
		mCon.min = maximum;
		mCon.modif = 0;
	}

	Int32 getScrollOffset() const { return mCon.modif; }

	void pushText( String text ) { privPushText( std::move( text ) ); }

	String::HashType getLastLogHash() const { return mCmdLog.back().hash; }
};

UTEST( UIScrolling, SceneSmoothScrollingDefaultsCanBeInheritedSnapshottedAndAppliedNow ) {
	UIApplication app(
		WindowSettings{ 320, 240, "eepp - scrolling test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	auto* editor = UICodeEditor::New();
	editor->setPixelsSize( 160, 80 );
	editor->setParent( scene->getRoot() );
	editor->getDocument().textInput( String( 100, '\n' ) );
	scene->flushDirtyStyleAndLayout();

	EXPECT_FALSE( editor->isSmoothScrollEnabled() );
	scene->setSmoothScrollEnabled( true );
	EXPECT_TRUE( scene->isSmoothScrollEnabled() );
	EXPECT_FALSE( editor->isSmoothScrollEnabled() );

	auto* newEditor = UICodeEditor::New();
	newEditor->setParent( scene->getRoot() );
	EXPECT_TRUE( newEditor->isSmoothScrollEnabled() );
	newEditor->setSmoothScrollEnabled( false );
	EXPECT_FALSE( newEditor->isSmoothScrollEnabled() );

	auto* embeddedHost = UIWidget::New();
	embeddedHost->setParent( scene->getRoot() );
	auto* embeddedScene = UISceneNode::New();
	embeddedScene->setParent( embeddedHost );
	EXPECT_TRUE( embeddedScene->isSmoothScrollEnabled() );
	UICodeEditor* embeddedEditor;
	{
		auto context = embeddedScene->makeCurrent();
		embeddedEditor = UICodeEditor::New();
		embeddedEditor->setParent( embeddedScene->getRoot() );
	}
	EXPECT_TRUE( embeddedEditor->isSmoothScrollEnabled() );

	embeddedScene->setSmoothScrollEnabled( false, true );
	EXPECT_FALSE( scene->isSmoothScrollEnabled() );
	EXPECT_FALSE( embeddedScene->isSmoothScrollEnabled() );
	EXPECT_FALSE( editor->isSmoothScrollEnabled() );
	EXPECT_FALSE( newEditor->isSmoothScrollEnabled() );
	EXPECT_FALSE( embeddedEditor->isSmoothScrollEnabled() );

	editor->setSmoothScrollEnabled( true );
	EXPECT_TRUE( editor->isSmoothScrollEnabled() );
	ASSERT_GT( editor->getMaxScroll().y, 10.75f );
	editor->setScroll( { 0.f, 10.75f } );
	EXPECT_NEAR( 10.f, editor->getScroll().y, 0.0001f );

	eeDelete( editor );
}

UTEST( UIScrolling, CodeEditorKeepsLegacyWheelStepButSoftensFractionalMomentumTail ) {
	UIApplication app(
		WindowSettings{ 320, 240, "eepp - wheel precision test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* editor = eeNew( ScrollingTestEditor, () );
	editor->setPixelsSize( 160, 80 );
	editor->setParent( app.getUI()->getRoot() );
	editor->getDocument().textInput( String( 100, '\n' ) );
	editor->setMouseWheelScroll( 50.f );
	app.getUI()->flushDirtyStyleAndLayout();

	editor->setScrollY( 100.f );
	EXPECT_EQ( 1u, editor->onMouseWheel( { 0.f, -0.1f }, false ) );
	EXPECT_NEAR( 105.f, editor->getScroll().y, 0.0001f );

	EXPECT_EQ( 1u, editor->onMouseWheel( { 0.f, -4.f }, false ) );
	EXPECT_NEAR( 155.f, editor->getScroll().y, 0.0001f );

	eeDelete( editor );
}

UTEST( UIScrolling, ConsoleHandlesWheelAndKeyboardScrolling ) {
	UIApplication app(
		WindowSettings{ 320, 240, "eepp - console scrolling test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* console = eeNew( ScrollingTestConsole, () );
	console->setParent( app.getUI()->getRoot() );
	console->setScrollRange( 20 );
	const String logLine( "console cache hash regression" );
	console->pushText( logLine );
	EXPECT_EQ( String::hash( logLine ), console->getLastLogHash() );

	EXPECT_EQ( 1u, console->onMouseWheel( { 0.f, 1.f }, false ) );
	EXPECT_EQ( 6, console->getScrollOffset() );
	EXPECT_EQ( 1u, console->onMouseWheel( { 0.f, -1.f }, false ) );
	EXPECT_EQ( 0, console->getScrollOffset() );

	KeyEvent pageUp( console, Event::KeyDown, KEY_PAGEUP, SCANCODE_PAGEUP, 0, KEYMOD_SHIFT );
	EXPECT_EQ( 1u, console->onKeyDown( pageUp ) );
	EXPECT_GT( console->getScrollOffset(), 0 );

	KeyEvent pageDown( console, Event::KeyDown, KEY_PAGEDOWN, SCANCODE_PAGEDOWN, 0, KEYMOD_SHIFT );
	EXPECT_EQ( 1u, console->onKeyDown( pageDown ) );
	EXPECT_EQ( 0, console->getScrollOffset() );

	eeDelete( console );
}

UTEST( UIScrolling, WheelEventRebuildsHoverStateAndBubblesWhenEditorCannotScroll ) {
	UIApplication app(
		WindowSettings{ 320, 240, "eepp - wheel bubbling test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	auto* scrollView = UIScrollView::New();
	scrollView->setPixelsSize( 280, 160 );
	scrollView->setParent( scene->getRoot() );
	auto* content = UIWidget::New();
	content->setPixelsSize( 260, 600 );
	content->setParent( scrollView );
	auto* editor = UICodeEditor::New();
	editor->setPixelsSize( 240, 80 );
	editor->setParent( content );
	scene->flushDirtyStyleAndLayout();

	ASSERT_TRUE( scrollView->getVerticalScrollBar()->isEnabled() );
	EXPECT_NEAR( scrollView->getVerticalScrollBar()->getValue(), 0.f, 0.0001f );
	app.getWindow()->getInput()->setMousePos(
		editor->convertToWorldSpace( { 20.f, 20.f } ).asInt() );

	InputEvent event{};
	event.Type = InputEvent::MouseWheel;
	event.WinID = app.getWindow()->getWindowID();
	event.wheel.x = 0.f;
	event.wheel.y = -1.f;
	event.wheel.direction = InputEvent::WheelEvent::Normal;
	ASSERT_TRUE( app.getWindow()->getInput()->pushEvent( event ) );
	Engine::instance()->updateInput();
	SceneManager::instance()->update();

	EXPECT_GT( scrollView->getVerticalScrollBar()->getValue(), 0.f );
}

UTEST( UIScrolling, CodeEditorPluginCanConsumePreciseWheelBeforeEditorScrolls ) {
	UIApplication app(
		WindowSettings{ 320, 240, "eepp - wheel plugin test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* editor = eeNew( ScrollingTestEditor, () );
	editor->setPixelsSize( 160, 80 );
	editor->setParent( app.getUI()->getRoot() );
	editor->getDocument().textInput( String( 100, '\n' ) );
	app.getUI()->flushDirtyStyleAndLayout();
	editor->setScrollY( 100.f );
	app.getWindow()->getInput()->setMousePos( { 25, 35 } );
	SceneManager::instance()->update();
	const Vector2i expectedPosition = editor->getEventDispatcher()->getMousePos();

	WheelInterceptPlugin plugin;
	editor->registerPlugin( &plugin );
	EXPECT_EQ( 1u, editor->onMouseWheel( { 0.f, -0.25f }, true ) );
	EXPECT_EQ( plugin.calls, 1 );
	EXPECT_EQ( plugin.lastPosition.x, expectedPosition.x );
	EXPECT_EQ( plugin.lastPosition.y, expectedPosition.y );
	EXPECT_NEAR( plugin.lastOffset.x, 0.f, 0.0001f );
	EXPECT_NEAR( plugin.lastOffset.y, -0.25f, 0.0001f );
	EXPECT_TRUE( plugin.lastFlipped );
	EXPECT_NEAR( editor->getScroll().y, 100.f, 0.0001f );
	editor->unregisterPlugin( &plugin );

	eeDelete( editor );
}

UTEST( UIScrolling, MouseWheelListenerConsumesBeforeScrollableAncestor ) {
	UIApplication app(
		WindowSettings{ 320, 240, "eepp - wheel listener test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	auto* scrollView = UIScrollView::New();
	scrollView->setPixelsSize( 280, 160 );
	scrollView->setParent( scene->getRoot() );
	auto* content = UIWidget::New();
	content->setPixelsSize( 260, 600 );
	content->setParent( scrollView );
	auto* child = UIWidget::New();
	child->setPixelsSize( 240, 80 );
	child->setParent( content );
	scene->flushDirtyStyleAndLayout();

	int wheelEvents = 0;
	Vector2f receivedOffset;
	child->on( Event::MouseWheel, [&]( const Event* event ) {
		++wheelEvents;
		receivedOffset = event->asMouseWheelEvent()->getOffset();
	} );
	app.getWindow()->getInput()->setMousePos(
		child->convertToWorldSpace( { 20.f, 20.f } ).asInt() );

	InputEvent event{};
	event.Type = InputEvent::MouseWheel;
	event.WinID = app.getWindow()->getWindowID();
	event.wheel.x = 0.f;
	event.wheel.y = -1.25f;
	event.wheel.direction = InputEvent::WheelEvent::Normal;
	ASSERT_TRUE( app.getWindow()->getInput()->pushEvent( event ) );
	Engine::instance()->updateInput();
	SceneManager::instance()->update();

	EXPECT_EQ( wheelEvents, 1 );
	EXPECT_NEAR( receivedOffset.y, -1.25f, 0.0001f );
	EXPECT_NEAR( scrollView->getVerticalScrollBar()->getValue(), 0.f, 0.0001f );
}

UTEST( UIScrolling, MouseWheelOverrideCanSuppressListenerDispatch ) {
	UIApplication app(
		WindowSettings{ 320, 240, "eepp - wheel override test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* widget = eeNew( WheelOverrideWidget, () );
	widget->setPixelsSize( 160, 80 );
	widget->setParent( app.getUI()->getRoot() );
	app.getUI()->flushDirtyStyleAndLayout();

	int wheelEvents = 0;
	widget->on( Event::MouseWheel, [&]( const Event* ) { ++wheelEvents; } );
	app.getWindow()->getInput()->setMousePos(
		widget->convertToWorldSpace( { 20.f, 20.f } ).asInt() );

	InputEvent event{};
	event.Type = InputEvent::MouseWheel;
	event.WinID = app.getWindow()->getWindowID();
	event.wheel.y = -1.f;
	event.wheel.direction = InputEvent::WheelEvent::Normal;
	ASSERT_TRUE( app.getWindow()->getInput()->pushEvent( event ) );
	Engine::instance()->updateInput();
	SceneManager::instance()->update();

	EXPECT_EQ( widget->calls, 1 );
	EXPECT_EQ( wheelEvents, 0 );
}
