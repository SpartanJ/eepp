#include "utest.h"
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/charts/uichart.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/tools/uiimageviewer.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiconsole.hpp>
#include <eepp/ui/uidatetimeedit.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uidropdownmodellist.hpp>
#include <eepp/ui/uiimage.hpp>
#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uiwidgettable.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/inputevent.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Models;

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

namespace {

struct WheelPropagationTest {
	UIApplication app{
		WindowSettings{ 320, 240, "eepp - wheel propagation test" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) };
	UIScrollView* scrollView{ UIScrollView::New() };
	UIWidget* content{ UIWidget::New() };

	WheelPropagationTest() {
		scrollView->setPixelsSize( 280, 160 );
		scrollView->setParent( app.getUI()->getRoot() );
		content->setPixelsSize( 260, 600 );
		content->setParent( scrollView );
		app.getUI()->getRoot()->setFocus();
	}

	void add( UIWidget* widget, const Sizef& size = { 200.f, 30.f } ) {
		widget->setPixelsSize( size );
		widget->setParent( content );
		app.getUI()->flushDirtyStyleAndLayout();
	}

	void hover( Node* node, const Vector2f& position = { 10.f, 10.f } ) {
		app.getWindow()->getInput()->setMousePos( node->convertToWorldSpace( position ).asInt() );
	}

	void wheel( const Vector2f& offset, bool discrete = true ) {
		auto* input = app.getWindow()->getInput();
		input->cleanStates();
		InputEvent event{};
		event.WinID = app.getWindow()->getWindowID();
		if ( discrete ) {
			// Native backends emit wheel button events as well as the precise wheel event.
			event.Type = InputEvent::MouseButtonDown;
			const Uint8 button =
				offset.y != 0.f ? ( offset.y > 0.f ? EE_BUTTON_WHEELUP : EE_BUTTON_WHEELDOWN )
								: ( offset.x > 0.f ? EE_BUTTON_WHEELRIGHT : EE_BUTTON_WHEELLEFT );
			event.button = { 0, button, 1, 0, 0 };
			input->processEventForWindow( &event );
			event.Type = InputEvent::MouseButtonUp;
			event.button.state = 0;
			input->processEventForWindow( &event );
		}
		event.Type = InputEvent::MouseWheel;
		event.wheel = { 0, offset.x, offset.y, InputEvent::WheelEvent::Normal };
		input->processEventForWindow( &event );
		app.getUI()->update( Milliseconds( 16 ) );
	}
};

} // namespace

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

UTEST( UIScrolling, DropDownWheelChangesSelectionWithoutScrollingAncestor ) {
	WheelPropagationTest test;
	auto* dropdown = UIDropDownList::New();
	test.add( dropdown );
	auto* list = dropdown->getListBox();
	list->addListBoxItems( { "First", "Second", "Third" } );
	list->setSelected( 0 );
	test.hover( dropdown );
	ASSERT_TRUE( test.scrollView->getVerticalScrollBar()->isEnabled() );

	// Selection changes once per step and retains wheel ownership at either end of the list.
	const Uint32 expectedSelection[] = { 1, 2, 2, 1, 0, 0 };
	for ( size_t i = 0; i < 6; ++i ) {
		test.wheel( { 0.f, i < 3 ? -1.f : 1.f } );
		EXPECT_EQ( expectedSelection[i], list->getItemSelectedIndex() );
		EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );
	}

	// A fractional tail without a discrete step must not scroll the containing view either.
	test.wheel( { 0.f, -0.25f }, false );
	EXPECT_EQ( 0u, list->getItemSelectedIndex() );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );

	test.hover( test.content, { 220.f, 80.f } );
	test.wheel( { 0.f, -1.f } );
	EXPECT_EQ( 0u, list->getItemSelectedIndex() );
	EXPECT_GT( test.scrollView->getVerticalScrollBar()->getValue(), 0.f );
}

UTEST( UIScrolling, ModelDropDownWheelDoesNotScrollAncestor ) {
	WheelPropagationTest test;
	auto* dropdown = UIDropDownModelList::New();
	dropdown->setModel( Models::ItemListOwnerModel<std::string>::create(
		std::vector<std::string>{ "First", "Second", "Third" } ) );
	test.add( dropdown );
	auto* list = dropdown->getListView();
	list->getSelection().set( list->getModel()->index( 0, 0 ) );
	test.hover( dropdown );
	test.wheel( { 0.f, -1.f } );
	ASSERT_FALSE( list->getSelection().isEmpty() );
	EXPECT_EQ( 1, list->getSelection().first().row() );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );
}

UTEST( UIScrolling, SliderAndScrollbarWheelChangesValueOnceWithoutScrollingAncestor ) {
	WheelPropagationTest test;
	auto* slider = UISlider::NewHorizontal();
	test.add( slider );
	slider->setValue( 0.5f );
	slider->setClickStep( 0.1f );
	test.hover( slider->getSliderButton(), { 1.f, 1.f } );
	test.wheel( { 0.f, -1.f } );
	EXPECT_NEAR( 0.6f, slider->getValue(), 0.0001f );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );
	test.hover( slider, { 10.f, 10.f } );
	test.wheel( { 1.f, 0.f } );
	EXPECT_NEAR( 0.7f, slider->getValue(), 0.0001f );
	slider->setValue( 1.f );
	test.wheel( { 0.f, -1.f } );
	EXPECT_NEAR( 1.f, slider->getValue(), 0.0001f );
	test.wheel( { 0.f, -0.25f }, false );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );
	eeDelete( slider );

	auto* bar = UIScrollBar::NewVertical();
	test.add( bar, { 30.f, 120.f } );
	bar->setValue( 0.5f );
	bar->setClickStep( 0.1f );
	test.hover( bar->getSlider(), { 5.f, 5.f } );
	test.wheel( { 0.f, -1.f } );
	EXPECT_NEAR( 0.6f, bar->getValue(), 0.0001f );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );
}

UTEST( UIScrolling, SpinBoxWheelDoesNotScrollAncestorAtValueLimits ) {
	WheelPropagationTest test;
	auto* spin = UISpinBox::New();
	spin->setMinValue( 0 );
	spin->setMaxValue( 2 );
	spin->setValue( 1 );
	spin->setClickStep( 1 );
	test.add( spin );
	spin->getTextInput()->setFocus();
	test.hover( spin->getTextInput() );
	test.wheel( { 0.f, -1.f } );
	EXPECT_EQ( 0.0, spin->getValue() );
	test.wheel( { 0.f, -1.f } );
	EXPECT_EQ( 0.0, spin->getValue() );
	test.wheel( { 0.f, 1.f } );
	EXPECT_EQ( 1.0, spin->getValue() );
	test.wheel( { 0.f, -0.25f }, false );
	EXPECT_EQ( 1.0, spin->getValue() );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );
}

UTEST( UIScrolling, TabWheelConsumesOnlyTabBarGestures ) {
	WheelPropagationTest test;
	auto* tabs = UITabWidget::New();
	tabs->add( "A", UIWidget::New() );
	tabs->add( "B", UIWidget::New() );
	tabs->setTabSelected( tabs->getTab( 0 ) );
	test.add( tabs, { 240.f, 120.f } );
	test.hover( tabs->getTab( 0 ) );
	test.wheel( { 0.f, -1.f } );
	EXPECT_EQ( 1u, tabs->getTabSelectedIndex() );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );

	tabs->setAllowSwitchTabsInEmptySpaces( true );
	test.hover( tabs->getTabBar(), { 230.f, 5.f } );
	test.wheel( { 0.f, 1.f } );
	EXPECT_EQ( 0u, tabs->getTabSelectedIndex() );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );

	// A page that does not handle the wheel must still let the outer scroll view move.
	test.hover( tabs->getTab( 0 )->getOwnedWidget(), { 30.f, 30.f } );
	test.wheel( { 0.f, -1.f } );
	EXPECT_GT( test.scrollView->getVerticalScrollBar()->getValue(), 0.f );
}

UTEST( UIScrolling, LegacyListAndTableWheelDoNotScrollAncestor ) {
	WheelPropagationTest test;
	auto* list = UIListBox::New();
	test.add( list, { 200.f, 120.f } );
	for ( int i = 0; i < 20; ++i )
		list->addListBoxItem( "Row" );
	test.app.getUI()->flushDirtyStyleAndLayout();
	ASSERT_TRUE( list->getVerticalScrollBar()->isVisible() );
	test.hover( list->getItem( 0 ) );
	test.wheel( { 0.f, -1.f } );
	EXPECT_GT( list->getVerticalScrollBar()->getValue(), 0.f );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );
	eeDelete( list );

	auto* table = UIWidgetTable::New();
	table->setColumnsCount( 1 );
	table->setRowHeight( 24 );
	test.add( table, { 200.f, 120.f } );
	for ( int i = 0; i < 20; ++i ) {
		auto* row = UIWidgetTableRow::New();
		row->setParent( table->getContainer() );
		row->setColumn( 0, UIWidget::New() );
		table->add( row );
	}
	test.app.getUI()->flushDirtyStyleAndLayout();
	ASSERT_TRUE( table->getVerticalScrollBar()->isVisible() );
	test.hover( table->getRow( 0 )->getColumn( 0 ) );
	test.wheel( { 0.f, -1.f } );
	EXPECT_GT( table->getVerticalScrollBar()->getValue(), 0.f );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );
}

UTEST( UIScrolling, ImageZoomWheelDoesNotScrollAncestor ) {
	WheelPropagationTest test;
	auto* viewer = Tools::UIImageViewer::New();
	test.add( viewer, { 200.f, 120.f } );
	test.hover( viewer );
	auto* input = test.app.getWindow()->getInput();
	InputEvent modifiers{};
	modifiers.Type = InputEvent::KeyDown;
	modifiers.WinID = test.app.getWindow()->getWindowID();
	// InputEvent's constructor does not initialize its union payload. In particular, an invalid
	// scancode can make Input skip the modifier update or write outside the key-state array.
	modifiers.key = { 0, 1, 0, { SCANCODE_UNKNOWN, KEY_UNKNOWN, KeyMod::getDefaultModifier(), 0 } };
	input->processEventForWindow( &modifiers );
	const Float scale = viewer->getImage()->getScale().x;
	test.wheel( { 0.f, 1.f } );
	EXPECT_GT( viewer->getImage()->getScale().x, scale );
	test.wheel( { 0.f, -0.25f }, false );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );
	modifiers.Type = InputEvent::KeyUp;
	modifiers.key.state = 0;
	modifiers.key.keysym.mod = 0;
	input->processEventForWindow( &modifiers );
}

UTEST( UIScrolling, DateTimeWheelEditingConsumesOnlyWhenItChangesTheValue ) {
	WheelPropagationTest test;
	auto* edit = UIDateTimeEdit::New( DateTimeEditMode::Date );
	test.add( edit );
	edit->setDate( CalendarDate{ 2026, 10, 26 } );
	edit->setWheelEditingEnabled( true );
	edit->setFocus();
	edit->setActiveSection( DateTimeSection::Day );
	test.hover( edit );
	test.wheel( { 0.f, -1.f } );
	ASSERT_TRUE( edit->getDate().has_value() );
	EXPECT_EQ( 25, edit->getDate()->day );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );

	edit->setWheelEditingEnabled( false );
	test.wheel( { 0.f, -1.f } );
	EXPECT_EQ( 25, edit->getDate()->day );
	EXPECT_GT( test.scrollView->getVerticalScrollBar()->getValue(), 0.f );
}

UTEST( UIScrolling, ChartWheelZoomConsumesOnlyInsideThePlot ) {
	WheelPropagationTest test;
	auto* chart = Charts::UIChart::New();
	test.add( chart, { 240.f, 140.f } );
	chart->setAxisRange( chart->xAxis(), { 0, 10 } );
	chart->setAxisRange( chart->yAxis(), { 0, 10 } );
	test.app.getUI()->draw();
	test.hover( chart, { 120.f, 60.f } );
	test.wheel( { 0.f, -1.f } );
	const auto range = chart->axisRange( chart->xAxis() );
	EXPECT_GT( range.max - range.min, 10.0 );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );

	test.hover( chart, { 2.f, 2.f } );
	test.wheel( { 0.f, -1.f } );
	EXPECT_NEAR( range.max, chart->axisRange( chart->xAxis() ).max, 0.0001 );
	EXPECT_GT( test.scrollView->getVerticalScrollBar()->getValue(), 0.f );
}

UTEST( UIScrolling, ScrollableEditorAndConsoleConsumeBeforeAncestor ) {
	WheelPropagationTest test;
	auto* editor = UICodeEditor::New();
	test.add( editor, { 200.f, 100.f } );
	editor->getDocument().textInput( String( 100, '\n' ) );
	test.app.getUI()->flushDirtyStyleAndLayout();
	test.hover( editor );
	test.wheel( { 0.f, -1.f } );
	EXPECT_GT( editor->getScroll().y, 0.f );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );
	eeDelete( editor );

	auto* console = eeNew( ScrollingTestConsole, () );
	test.add( console, { 200.f, 100.f } );
	console->setScrollRange( 20 );
	test.hover( console );
	test.wheel( { 0.f, 1.f } );
	EXPECT_EQ( 6, console->getScrollOffset() );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );
}

UTEST( UIScrolling, NestedScrollViewsConsumeBeforeAncestorAndBubbleAtBoundary ) {
	WheelPropagationTest test;
	auto* inner = UIScrollView::New();
	test.add( inner, { 200.f, 120.f } );
	auto* content = UIWidget::New();
	content->setPixelsSize( 160, 500 );
	content->setParent( inner );
	test.app.getUI()->flushDirtyStyleAndLayout();
	test.hover( content );
	test.wheel( { 0.f, -1.f } );
	EXPECT_GT( inner->getVerticalScrollBar()->getValue(), 0.f );
	EXPECT_NEAR( 0.f, test.scrollView->getVerticalScrollBar()->getValue(), 0.0001f );

	inner->getVerticalScrollBar()->setValue( 1.f );
	test.hover( inner, { 10.f, 10.f } );
	test.wheel( { 0.f, -1.f } );
	EXPECT_NEAR( 1.f, inner->getVerticalScrollBar()->getValue(), 0.0001f );
	EXPECT_GT( test.scrollView->getVerticalScrollBar()->getValue(), 0.f );
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

UTEST( UIScrolling, XmlOffModesHideScrollBarsEvenWhenContentOverflows ) {
	// The model borrows its rows, which must outlive the scene and its list view.
	const std::vector<std::string> rows( 8, "Overflow row" );
	UIApplication app(
		WindowSettings{ 320, 240, "eepp - XML scroll modes" },
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	ASSERT_NE( scene, nullptr );
	auto* scroll = scene
					   ->loadLayoutFromString( R"xml(
		<ScrollView lw="120dp" lh="100dp" hscroll-mode="off" vscroll-mode="off">
			<widget lw="400dp" lh="300dp" />
		</ScrollView>
	)xml" )
					   ->asType<UIScrollView>();
	scene->flushDirtyStyleAndLayout();
	EXPECT_TRUE( scroll->getHorizontalScrollMode() == ScrollBarMode::AlwaysOff );
	EXPECT_TRUE( scroll->getVerticalScrollMode() == ScrollBarMode::AlwaysOff );
	EXPECT_FALSE( scroll->getHorizontalScrollBar()->isVisible() );
	EXPECT_FALSE( scroll->getVerticalScrollBar()->isVisible() );

	auto* list = scene
					 ->loadLayoutFromString( R"xml(
		<ListView lw="120dp" lh="100dp" row-height="30dp" hscroll-mode="off" vscroll-mode="off" />
	)xml" )
					 ->asType<UIListView>();
	list->setModel( ItemListModel<std::string>::create( rows ) );
	list->setAutoExpandOnSingleColumn( false );
	list->setColumnWidth( 0, PixelDensity::dpToPx( 400 ) );
	scene->flushDirtyStyleAndLayout();
	EXPECT_GT( list->getContentSize().getWidth(), list->getPixelsSize().getWidth() );
	EXPECT_GT( list->getContentSize().getHeight(), list->getPixelsSize().getHeight() );
	EXPECT_TRUE( list->getHorizontalScrollMode() == ScrollBarMode::AlwaysOff );
	EXPECT_TRUE( list->getVerticalScrollMode() == ScrollBarMode::AlwaysOff );
	EXPECT_FALSE( list->getHorizontalScrollBar()->isVisible() );
	EXPECT_FALSE( list->getVerticalScrollBar()->isVisible() );
}
