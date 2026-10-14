#include "utest.h"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/accessibility/accessibilitymanager.hpp>
#include <eepp/ui/models/itemlistmodel.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uicalendar.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uicombobox.hpp>
#include <eepp/ui/uidatepicker.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uiprogressbar.hpp>
#include <eepp/ui/uipushbutton.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uiscrollview.hpp>
#include <eepp/ui/uislider.hpp>
#include <eepp/ui/uispinbox.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitab.hpp>
#include <eepp/ui/uitablecell.hpp>
#include <eepp/ui/uitableview.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>
#include <eepp/window/input.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;

namespace {

UIApplication focusApp() {
	UIApplication::Settings settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 );
	settings.accessibilityPolicy = AccessibilityPolicy::Disabled;
	return UIApplication( WindowSettings( 640, 480, "Tab focus regression tests",
										  WindowStyle::Default | WindowStyle::Hidden,
										  WindowBackend::Default, 32, {}, 1, false, true ),
						  settings );
}

// Exercise the actual dispatcher on the actual scene, including key delivery before Tab handling.
class FocusDispatcherAccess : public UIEventDispatcher {
  public:
	static void key( UISceneNode* scene, Keycode code, Uint32 mod = 0 ) {
		InputEvent event( InputEvent::KeyDown );
		event.key.repeat = 0;
		event.key.keysym.sym = code;
		event.key.keysym.scancode = SCANCODE_UNKNOWN;
		event.key.keysym.mod = mod;
		event.key.keysym.unicode = 0;
		( scene->getUIEventDispatcher()->*&FocusDispatcherAccess::inputCallback )( &event );
	}
};

Uint32 editorTabModifier() {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	return KEYMOD_CTRL;
#else
	return KeyMod::getDefaultModifier();
#endif
}

size_t tabbableCount( const Node* node ) {
	size_t count =
		node->isWidget() && static_cast<const UIWidget*>( node )->isTabFocusable() ? 1 : 0;
	for ( auto* child = node->getFirstChild(); child; child = child->getNextNode() )
		count += tabbableCount( child );
	return count;
}

// Three top-level rows without children, usable by flat views and trees alike.
class FlatRowsModel final : public Models::Model {
  public:
	size_t rowCount( const Models::ModelIndex& parent ) const override {
		return parent.isValid() ? 0 : 3;
	}

	size_t columnCount( const Models::ModelIndex& ) const override { return 1; }

	Variant data( const Models::ModelIndex& index, Models::ModelRole role ) const override {
		return role == Models::ModelRole::Display ? Variant( std::to_string( index.row() ) )
												  : Variant();
	}
};

} // namespace

UTEST( TabFocus, WidgetCreatorDefaultsAreIntentional ) {
	auto app = focusApp();
	const char* staticWidgets[] = { "widget",
									"textview",
									"tv",
									"linearlayout",
									"hbox",
									"vbox",
									"rlay",
									"relativelayout",
									"gridlayout",
									"flowlayout",
									"widgettable",
									"widgettablerow",
									"stackwidget",
									"tabwidget",
									"window",
									"windowvbox",
									"windowhbox",
									"windowrellay",
									"image",
									"sprite",
									"textureregion",
									"svg",
									"loader",
									"progressbar",
									"tooltip",
									"richtext",
									"textspan",
									"nodelink",
									"menuseparator",
									"menubar",
									"scrollbar",
									"hscrollbar",
									"vscrollbar",
									"scrollview",
									"touchdraggable",
									"splitter",
									"viewpager",
									"viewpagerhorizontal",
									"viewpagervertical",
									"chart",
									"imageviewer",
									"textureviewer",
									"markdownview",
									"scrollablemarkdownview",
									"diffview",
									"mergeview",
									"webview",
									"anchor",
									"a",
									"menucheckbox",
									"menuradiobutton",
									"span",
									"label",
									"em",
									"b",
									"strong",
									"p",
									"h1",
									"div",
									"section",
									"details",
									"form",
									"input",
									"img",
									"table",
									"tr",
									"td",
									"button",
									"spinbox",
									"combobox" };
	for ( const char* tag : staticWidgets ) {
		auto* widget = UIWidgetCreator::createFromName( tag );
		ASSERT_TRUE( widget != nullptr );
		EXPECT_FALSE( widget->isTabFocusable() );
		eeDelete( widget );
	}
	const char* interactiveWidgets[] = { "pushbutton",
										 "checkbox",
										 "radiobutton",
										 "selectbutton",
										 "textinput",
										 "textedit",
										 "codeeditor",
										 "console",
										 "slider",
										 "hslider",
										 "vslider",
										 "dropdownlist",
										 "dropdownmodellist",
										 "datetimeedit",
										 "datepicker",
										 "timepicker",
										 "datetimepicker",
										 "calendar",
										 "listbox",
										 "listview",
										 "tableview",
										 "treeview",
										 "tab",
										 "textarea",
										 "summary" };
	for ( const char* tag : interactiveWidgets ) {
		auto* widget = UIWidgetCreator::createFromName( tag );
		ASSERT_TRUE( widget != nullptr );
		EXPECT_TRUE( widget->isTabFocusable() );
		eeDelete( widget );
	}
}

UTEST( TabFocus, FormTraversesInteractiveDescendantsInBothDirections ) {
	auto app = focusApp();
	auto* scene = app.getUI();
	scene->loadLayoutFromString( R"xml(
		<vbox id="form">
			<textview text="Name" />
			<textinput id="name" />
			<widget><vbox><checkbox id="check" /><pushbutton id="save" /></vbox></widget>
			<pushbutton enabled="false" />
			<widget visible="false"><pushbutton /></widget>
			<slider id="slider" />
			<textview text="Status" />
		</vbox>)xml" );
	scene->update( Time::Zero );
	UIWidget* stops[] = { scene->find<UITextInput>( "name" ), scene->find<UICheckBox>( "check" ),
						  scene->find<UIPushButton>( "save" ), scene->find<UISlider>( "slider" ) };
	for ( auto* widget : stops )
		ASSERT_TRUE( widget != nullptr );
	stops[0]->setFocus();
	for ( size_t i = 1; i <= 4; ++i ) {
		FocusDispatcherAccess::key( scene, KEY_TAB );
		EXPECT_TRUE( scene->getEventDispatcher()->getFocusNode() == stops[i % 4] );
	}
	for ( size_t i = 0; i < 4; ++i ) {
		FocusDispatcherAccess::key( scene, KEY_TAB, KEYMOD_SHIFT );
		EXPECT_TRUE( scene->getEventDispatcher()->getFocusNode() == stops[3 - i] );
	}
	// Sequential eligibility is not general focus eligibility.
	auto* form = scene->find<UIWidget>( "form" );
	form->setFocus();
	EXPECT_TRUE( form->hasFocus() );
	FocusDispatcherAccess::key( scene, KEY_TAB );
	EXPECT_TRUE( stops[0]->hasFocus() );
}

UTEST( TabFocus, ReverseTraversalIncludesAnInteractiveParentBeforeItsChildren ) {
	auto app = focusApp();
	auto* before = UIPushButton::New();
	auto* parent = UIWidget::New();
	parent->setTabFocusable();
	auto* first = UIPushButton::New();
	first->setParent( parent );
	auto* last = UIPushButton::New();
	last->setParent( parent );
	auto* after = UIPushButton::New();
	EXPECT_TRUE( before->getNextTabWidget() == parent );
	EXPECT_TRUE( parent->getNextTabWidget() == first );
	EXPECT_TRUE( after->getPrevTabWidget() == last );
	EXPECT_TRUE( last->getPrevTabWidget() == first );
	EXPECT_TRUE( first->getPrevTabWidget() == parent );
	EXPECT_TRUE( parent->getPrevTabWidget() == before );
}

UTEST( TabFocus, SpinBoxKeepsPartialTextUntilCommitted ) {
	auto app = focusApp();
	auto* spin = UISpinBox::New();
	auto* other = UIPushButton::New();
	spin->setMinValue( 6 )->setMaxValue( 72 );
	spin->setValue( 12 );
	auto* input = spin->getTextInput();
	input->setFocus();
	// "1" on the way to "14" is out of range: neither applied nor rewritten.
	input->setText( "1" );
	EXPECT_EQ( spin->getValue(), 12.0 );
	EXPECT_TRUE( input->getText() == "1" );
	input->setText( "14" );
	EXPECT_EQ( spin->getValue(), 14.0 );
	input->setText( "" );
	EXPECT_EQ( spin->getValue(), 14.0 );
	// Committing clamps and normalizes the text.
	input->setText( "100" );
	other->setFocus();
	EXPECT_EQ( spin->getValue(), 72.0 );
	EXPECT_TRUE( input->getText() == "72" );
	input->setFocus();
	input->setText( "" );
	other->setFocus();
	EXPECT_EQ( spin->getValue(), 72.0 );
	EXPECT_TRUE( input->getText() == "72" );
}

UTEST( TabFocus, CompositeControlsHaveOneFunctionalEntry ) {
	auto app = focusApp();
	auto* before = UIPushButton::New();
	auto* spin = UISpinBox::New();
	auto* combo = UIComboBox::New();
	auto* after = UIPushButton::New();
	EXPECT_EQ( tabbableCount( spin ), 1u );
	// The popup list is hidden, and is a separate keyboard context when opened.
	EXPECT_FALSE( combo->isTabFocusable() );
	EXPECT_TRUE( combo->getDropDownList()->isTabFocusable() );
	EXPECT_TRUE( before->getNextTabWidget() == spin->getTextInput() );
	EXPECT_TRUE( spin->getTextInput()->getNextTabWidget() == combo->getDropDownList() );
	EXPECT_TRUE( combo->getDropDownList()->getNextTabWidget() == after );
	EXPECT_TRUE( after->getPrevTabWidget() == combo->getDropDownList() );
	EXPECT_TRUE( combo->getDropDownList()->getPrevTabWidget() == spin->getTextInput() );
	auto* manager = app.getUI()->getAccessibilityManager();
	for ( auto* control : { static_cast<UIWidget*>( spin ), static_cast<UIWidget*>( combo ) } ) {
		const auto ref = manager->getNodeRef( control );
		EXPECT_TRUE( static_cast<Uint64>( manager->getNodeState( ref ) ) &
					 static_cast<Uint64>( AccessibilityState::Focusable ) );
		EXPECT_TRUE( manager->performAction( ref, { AccessibilityAction::Focus, {} } ) );
		const auto expectedFocus =
			control == spin ? ref : manager->getNodeRef( combo->getDropDownList() );
		EXPECT_TRUE( manager->getKeyboardFocusedNode() == expectedFocus );
	}
	EXPECT_TRUE( combo->getDropDownList()->hasFocus() );
	spin->getTextInput()->setText( "12" );
	EXPECT_EQ( spin->getValue(), 12.0 );
	spin->getTextInput()->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_UP );
	EXPECT_EQ( spin->getValue(), 13.0 );
	FocusDispatcherAccess::key( app.getUI(), KEY_DOWN );
	EXPECT_EQ( spin->getValue(), 12.0 );
	combo->getListBox()->addListBoxItem( "First" );
	combo->getListBox()->addListBoxItem( "Second" );
	combo->getDropDownList()->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_F4 );
	EXPECT_TRUE( combo->getListBox()->isVisible() );
	EXPECT_TRUE( combo->getListBox()->isEnabled() );
	FocusDispatcherAccess::key( app.getUI(), KEY_DOWN );
	FocusDispatcherAccess::key( app.getUI(), KEY_RETURN );
	EXPECT_TRUE( combo->getDropDownList()->hasFocus() );
	EXPECT_FALSE( combo->getListBox()->isEnabled() );
	combo->getListBox()->setSelected( 0 );
	FocusDispatcherAccess::key( app.getUI(), KEY_DOWN, KEYMOD_ALT );
	EXPECT_TRUE( combo->getListBox()->isEnabled() );
	EXPECT_EQ( combo->getListBox()->getItemSelectedIndex(), 0u );
	FocusDispatcherAccess::key( app.getUI(), KEY_ESCAPE );
	EXPECT_TRUE( combo->getDropDownList()->hasFocus() );
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB );
	EXPECT_TRUE( after->hasFocus() );
	combo->getDropDownList()->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_F4 );
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB );
	EXPECT_TRUE( after->hasFocus() );
	EXPECT_FALSE( combo->getListBox()->isEnabled() );
	combo->getDropDownList()->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_F4 );
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB, KEYMOD_SHIFT );
	EXPECT_TRUE( spin->getTextInput()->hasFocus() );
	EXPECT_FALSE( combo->getListBox()->isEnabled() );
}

UTEST( TabFocus, ScrollbarsAndReadingRegionsRequireExplicitOptIn ) {
	auto app = focusApp();
	auto* scroll = UIScrollView::New();
	scroll->setSize( 200, 100 );
	scroll->setVerticalScrollMode( ScrollBarMode::AlwaysOn );
	scroll->setHorizontalScrollMode( ScrollBarMode::AlwaysOn );
	auto* content = UIWidget::New();
	content->setSize( 1000, 1000 );
	content->setParent( scroll->getContainer() );
	auto* button = UIPushButton::New();
	button->setParent( content );
	auto* after = UIPushButton::New();
	app.getUI()->update( Time::Zero );
	EXPECT_EQ( tabbableCount( scroll ), 1u );
	EXPECT_TRUE( scroll->getNextWidget() == button );
	EXPECT_TRUE( after->getPrevTabWidget() == button );
	scroll->setTabFocusable();
	scroll->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_END );
	EXPECT_GT( scroll->getVerticalScrollBar()->getValue(), 0.f );
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB );
	EXPECT_TRUE( button->hasFocus() );
	auto* bar = UIScrollBar::NewVertical();
	EXPECT_EQ( tabbableCount( bar ), 0u );
	// Standalone opt-in targets the existing keyboard-operable slider, not the host.
	bar->getSlider()->setTabFocusable();
	bar->getSlider()->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_PAGEDOWN );
	EXPECT_EQ( bar->getValue(), 1.f );
	EXPECT_EQ( tabbableCount( bar ), 1u );
}

UTEST( TabFocus, ExplicitFocusPoliciesSurviveStylesAndHrefChanges ) {
	auto app = focusApp();
	auto* scene = app.getUI();
	scene->loadLayoutFromString( R"xml(
		<style>.static-stop { focusable: true; } .skip { focusable: false; }</style>
		<vbox>
			<textview id="static" class="static-stop" />
			<pushbutton id="skip" class="skip" />
			<anchor id="native" href="https://example.test/native" />
			<anchor id="plain" />
			<richtext><a id="link" href="https://example.test/link">Link</a>
				<a id="off" href="https://example.test/off" focusable="false">Off</a>
				<a id="nohref">Plain</a></richtext>
		</vbox>)xml" );
	scene->update( Time::Zero );
	EXPECT_TRUE( scene->find<UITextView>( "static" )->isTabFocusable() );
	EXPECT_FALSE( scene->find<UIPushButton>( "skip" )->isTabFocusable() );
	EXPECT_TRUE( scene->find<UIAnchor>( "native" )->isTabFocusable() );
	EXPECT_FALSE( scene->find<UIAnchor>( "plain" )->isTabFocusable() );
	auto* link = scene->find<UIAnchorSpan>( "link" );
	auto* off = scene->find<UIAnchorSpan>( "off" );
	ASSERT_TRUE( link != nullptr );
	ASSERT_TRUE( off != nullptr );
	EXPECT_TRUE( link->isTabFocusable() );
	EXPECT_FALSE( off->isTabFocusable() );
	EXPECT_FALSE( scene->find<UIAnchorSpan>( "nohref" )->isTabFocusable() );
	for ( auto* anchor : { link, off } ) {
		const bool previous = anchor->isTabFocusable();
		anchor->setHref( "" );
		anchor->setHref( "https://example.test/changed" );
		EXPECT_EQ( anchor->isTabFocusable(), previous );
	}
	link->unsetTabFocusable();
	link->setHref( "https://example.test/again" );
	EXPECT_FALSE( link->isTabFocusable() );
	int requests = 0;
	const Node* source = nullptr;
	std::string uri;
	scene->setNavigationInterceptorCb( [&]( const NavigationRequest& request ) {
		++requests;
		source = request.source;
		uri = request.uri.toString();
		return true;
	} );
	link->setFocus();
	EXPECT_EQ( requests, 0 );
	FocusDispatcherAccess::key( scene, KEY_RETURN );
	EXPECT_EQ( requests, 1 );
	EXPECT_TRUE( source == link );
	EXPECT_TRUE( uri == "https://example.test/again" );
	FocusDispatcherAccess::key( scene, KEY_KP_ENTER );
	EXPECT_EQ( requests, 2 );
	for ( auto* composite : { static_cast<UIWidget*>( UISpinBox::New() ),
							  static_cast<UIWidget*>( UIComboBox::New() ) } ) {
		composite->applyProperty( StyleSheetProperty( "focusable", "false" ) );
		EXPECT_TRUE( composite->getNextWidget() == nullptr );
		composite->applyProperty( StyleSheetProperty( "focusable", "true" ) );
		EXPECT_FALSE( composite->isTabFocusable() );
		EXPECT_TRUE( composite->getNextWidget() != nullptr );
	}
}

UTEST( TabFocus, ListsKeepOneEntryAfterArrowNavigation ) {
	auto app = focusApp();
	auto* before = UIPushButton::New();
	auto* list = UIListBox::New();
	list->setSize( 200, 100 );
	list->addListBoxItem( "First" );
	list->addListBoxItem( "Second" );
	auto* after = UIPushButton::New();
	EXPECT_EQ( tabbableCount( list ), 1u );
	EXPECT_TRUE( before->getNextTabWidget() == list );
	list->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_DOWN );
	FocusDispatcherAccess::key( app.getUI(), KEY_DOWN );
	list->getItem( 1 )->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB );
	EXPECT_TRUE( after->hasFocus() );
	list->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_UP );
	list->getItem( 0 )->setFocus();
	list->getItem( 0 )->setTabStop();
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB, KEYMOD_SHIFT );
	EXPECT_TRUE( list->getItem( 0 )->hasFocus() );
	list->getItem( 0 )->unsetTabStop();
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB, KEYMOD_SHIFT );
	EXPECT_TRUE( before->hasFocus() );
	auto* manager = app.getUI()->getAccessibilityManager();
	const auto rowRef = manager->getNodeRef( list->getItem( 0 ) );
	EXPECT_TRUE( static_cast<Uint64>( manager->getNodeState( rowRef ) ) &
				 static_cast<Uint64>( AccessibilityState::Focusable ) );
	EXPECT_TRUE( manager->performAction( rowRef, { AccessibilityAction::Focus, {} } ) );
	EXPECT_TRUE( manager->getKeyboardFocusedNode() == rowRef );
	auto* empty = UIListBox::New();
	empty->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB );
	EXPECT_TRUE( before->hasFocus() );
}

UTEST( TabFocus, CalendarCellsAreNotStopsAndTodayRemainsReachable ) {
	auto app = focusApp();
	auto* picker = UIDatePicker::New();
	auto* after = UIPushButton::New();
	EXPECT_EQ( tabbableCount( picker ), 1u );
	picker->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_F4 );
	auto* calendar = picker->getCalendar();
	ASSERT_TRUE( calendar != nullptr );
	EXPECT_TRUE( calendar->hasFocus() );
	EXPECT_EQ( tabbableCount( calendar ), 5u );
	const CalendarDate date{ 2026, 10, 12 };
	calendar->setFocusedDate( date );
	FocusDispatcherAccess::key( app.getUI(), KEY_RIGHT );
	EXPECT_TRUE( calendar->getFocusedDate() == date.addDays( 1 ) );
	FocusDispatcherAccess::key( app.getUI(), KEY_RETURN );
	EXPECT_TRUE( picker->hasFocus() );
	EXPECT_TRUE( picker->getDate() == date.addDays( 1 ) );
	FocusDispatcherAccess::key( app.getUI(), KEY_DOWN, KEYMOD_ALT );
	EXPECT_TRUE( calendar->hasFocus() );
	auto today = calendar->findAllByTag( "calendar::today" );
	ASSERT_EQ( today.size(), 1u );
	today[0]->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_RETURN );
	EXPECT_TRUE( calendar->getFocusedDate() == CalendarDate::today() );
	FocusDispatcherAccess::key( app.getUI(), KEY_ESCAPE );
	EXPECT_TRUE( picker->hasFocus() );
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB );
	EXPECT_TRUE( after->hasFocus() );
}

UTEST( TabFocus, EditorsRetainTabInsertionAndModifierEscape ) {
	auto app = focusApp();
	auto* before = UIPushButton::New();
	auto* edit = UITextEdit::New();
	auto* after = UIPushButton::New();
	edit->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB );
	EXPECT_TRUE( edit->hasFocus() );
	EXPECT_TRUE( !edit->getText().empty() );
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB, editorTabModifier() );
	EXPECT_TRUE( after->hasFocus() );
	edit->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB, editorTabModifier() | KEYMOD_SHIFT );
	EXPECT_TRUE( before->hasFocus() );
	auto* code = UICodeEditor::New();
	EXPECT_TRUE( code->isTabFocusable() );
	EXPECT_TRUE( code->isTabStop() );
	code->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_TAB );
	EXPECT_TRUE( code->hasFocus() );
}

UTEST( TabFocus, InformationalWidgetsRemainSemanticallyAccessible ) {
	auto app = focusApp();
	auto* label = UITextView::New();
	label->setText( "Status" );
	auto* progress = UIProgressBar::New();
	auto* manager = app.getUI()->getAccessibilityManager();
	const auto labelRef = manager->getNodeRef( label );
	const auto progressRef = manager->getNodeRef( progress );
	EXPECT_EQ( manager->getNodeRole( labelRef ), AccessibilityRole::Label );
	EXPECT_EQ( manager->getNodeRole( progressRef ), AccessibilityRole::ProgressBar );
	EXPECT_TRUE( manager->getNodeInfo( labelRef ).name == "Status" );
	EXPECT_FALSE( static_cast<Uint64>( manager->getNodeState( labelRef ) ) &
				  static_cast<Uint64>( AccessibilityState::Focusable ) );
	EXPECT_FALSE( label->isTabFocusable() );
	EXPECT_FALSE( progress->isTabFocusable() );
}

UTEST( TabFocus, HTMLInputsExposeOnlyTheirInteractiveNativeChild ) {
	auto app = focusApp();
	for ( const char* type : { "text", "password", "number", "checkbox", "radio", "date", "time",
							   "datetime-local" } ) {
		auto* input = UIHTMLInput::New();
		input->setInputType( type );
		app.getUI()->update( Time::Zero );
		EXPECT_FALSE( input->isTabFocusable() );
		EXPECT_EQ( tabbableCount( input ), 1u );
		eeDelete( input );
	}
	auto* hidden = UIHTMLInput::New();
	hidden->setInputType( "hidden" );
	EXPECT_FALSE( hidden->isEnabled() );
	EXPECT_FALSE( hidden->isVisible() );
}

UTEST( TabFocus, HTMLInputFocusOverridesFollowTheNativeControl ) {
	auto app = focusApp();
	auto* scene = app.getUI();
	auto* input = UIHTMLInput::New();
	const StyleSheetProperty focusable( "focusable", "false" );
	ASSERT_TRUE( input->applyProperty( focusable ) );
	for ( const char* type : { "text", "password", "number", "checkbox", "radio", "date", "time",
							   "datetime-local", "button", "hidden", "text" } ) {
		input->setInputType( type );
		scene->update( Time::Zero ); // Remove the old anonymous control after a type change.
		EXPECT_FALSE( input->isTabFocusable() );
		EXPECT_EQ( tabbableCount( input ), 0u );
		EXPECT_TRUE( input->getPropertyString( focusable.getPropertyDefinition() ) == "false" );
	}
	input->setInputType( "hidden" );
	ASSERT_TRUE( input->getChildWidget() == nullptr );
	ASSERT_TRUE( input->applyProperty( StyleSheetProperty( "focusable", "true" ) ) );
	EXPECT_TRUE( input->getPropertyString( focusable.getPropertyDefinition() ) == "true" );
	for ( const char* type : { "text", "number", "date", "checkbox" } ) {
		input->setInputType( type );
		scene->update( Time::Zero );
		EXPECT_FALSE( input->isTabFocusable() );
		EXPECT_EQ( tabbableCount( input ), 1u );
		EXPECT_TRUE( input->getPropertyString( focusable.getPropertyDefinition() ) == "true" );
	}

	scene->loadLayoutFromString( R"xml(
		<style>.skip-html-input { focusable: false; }</style>
		<vbox>
			<pushbutton id="html-before" />
			<input id="html-off" type="text" focusable="false" />
			<input id="html-off-before-type" focusable="false" type="number" />
			<input id="html-on" type="number" focusable="true" />
			<input id="html-css-off" type="text" class="skip-html-input" />
			<pushbutton id="html-after" />
		</vbox>)xml" );
	scene->update( Time::Zero );
	for ( const char* id : { "html-off", "html-off-before-type", "html-css-off" } ) {
		auto* off = scene->find<UIHTMLInput>( id );
		ASSERT_TRUE( off != nullptr );
		EXPECT_EQ( tabbableCount( off ), 0u );
		off->setInputType( "password" );
		scene->update( Time::Zero );
		EXPECT_EQ( tabbableCount( off ), 0u );
	}
	auto* on = scene->find<UIHTMLInput>( "html-on" );
	ASSERT_TRUE( on != nullptr );
	EXPECT_FALSE( on->isTabFocusable() );
	EXPECT_EQ( tabbableCount( on ), 1u );
	auto* before = scene->find<UIPushButton>( "html-before" );
	auto* after = scene->find<UIPushButton>( "html-after" );
	ASSERT_TRUE( before != nullptr );
	ASSERT_TRUE( after != nullptr );
	auto* target = on->getChildWidget()->asType<UISpinBox>()->getTextInput();
	before->setFocus();
	FocusDispatcherAccess::key( scene, KEY_TAB );
	EXPECT_TRUE( target->hasFocus() );
	FocusDispatcherAccess::key( scene, KEY_TAB );
	EXPECT_TRUE( after->hasFocus() );
	FocusDispatcherAccess::key( scene, KEY_TAB, KEYMOD_SHIFT );
	EXPECT_TRUE( target->hasFocus() );
	FocusDispatcherAccess::key( scene, KEY_TAB, KEYMOD_SHIFT );
	EXPECT_TRUE( before->hasFocus() );
}

UTEST( TabFocus, ModelViewsDoNotTabThroughRecycledCells ) {
	auto app = focusApp();
	for ( auto* view : { static_cast<UITableView*>( UIListView::New() ), UITableView::New() } ) {
		view->setSize( 200, 150 );
		view->setModel( Models::ItemListOwnerModel<std::string>::create( { "One", "Two" } ) );
		app.getUI()->update( Time::Zero );
		view->drawChildren();
		EXPECT_TRUE( view->isTabFocusable() );
		auto* cell = view->getCellFromIndex( view->getModel()->index( 0 ) );
		ASSERT_TRUE( cell != nullptr );
		EXPECT_FALSE( cell->isTabFocusable() );
		EXPECT_FALSE( view->getVerticalScrollBar()->getSlider()->isTabFocusable() );
		EXPECT_FALSE( view->getHorizontalScrollBar()->getSlider()->isTabFocusable() );
	}
}

UTEST( TabFocus, ModelViewsOpenOnceOnEnterAfterKeyboardSelection ) {
	auto app = focusApp();
	// Focus-on-selection moves the focus to the selected row's cell: Enter there must open the row
	// once, also with single-click navigation (where a cell click opens it).
	for ( bool singleClick : { false, true } ) {
		for ( int kind = 0; kind < 3; ++kind ) {
			UIAbstractTableView* view =
				kind == 0	? static_cast<UIAbstractTableView*>( UIListView::New() )
				: kind == 1 ? static_cast<UIAbstractTableView*>( UITableView::New() )
							: UITreeView::New();
			auto draw = [kind, view] {
				if ( kind == 2 )
					static_cast<UITreeView*>( view )->drawChildren();
				else
					static_cast<UITableView*>( view )->drawChildren();
			};
			view->setSize( 200, 150 );
			view->setModel( std::make_shared<FlatRowsModel>() );
			view->setSingleClickNavigation( singleClick );
			std::vector<int> opened;
			view->on( Event::OnModelEvent, [&opened]( const Event* event ) {
				auto* modelEvent = static_cast<const Abstract::ModelEvent*>( event );
				if ( modelEvent->getModelEventType() == Abstract::ModelEventType::Open )
					opened.push_back( modelEvent->getModelIndex().row() );
			} );
			view->getSelection().set( view->getModel()->index( 0 ) );
			view->setFocus();
			app.getUI()->update( Time::Zero );
			draw();
			FocusDispatcherAccess::key( app.getUI(), KEY_DOWN );
			draw();
			EXPECT_TRUE( view->hasFocusWithin() && !view->hasFocus() );
			FocusDispatcherAccess::key( app.getUI(), KEY_RETURN );
			EXPECT_EQ( opened.size(), 1u );
			EXPECT_TRUE( !opened.empty() && opened[0] == 1 );
			view->close();
			app.getUI()->update( Time::Zero );
		}
	}
}

UTEST( TabFocus, MenuButtonsTabsAndRadioMembersRemainKeyboardReachable ) {
	auto app = focusApp();
	app.getUI()->getUIThemeManager()->setDefaultEffectsEnabled( false );
	auto* bar = UIMenuBar::New();
	auto* menu = UIPopUpMenu::New();
	auto* item = menu->add( "Open" );
	menu->addSeparator();
	menu->add( "Close" );
	bar->addMenuButton( "File", menu );
	EXPECT_FALSE( bar->isTabFocusable() );
	EXPECT_FALSE( item->isTabFocusable() );
	EXPECT_TRUE( bar->getButton( 0 )->isTabFocusable() );
	auto* manager = app.getUI()->getAccessibilityManager();
	for ( auto* control : { static_cast<UIWidget*>( menu ), static_cast<UIWidget*>( item ) } ) {
		const auto info = manager->getNodeInfo( manager->getNodeRef( control ) );
		EXPECT_TRUE( static_cast<Uint64>( info.states ) &
					 static_cast<Uint64>( AccessibilityState::Focusable ) );
		EXPECT_TRUE( info.actions & accessibilityActionMask( AccessibilityAction::Focus ) );
	}
	bar->getButton( 0 )->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_RETURN );
	EXPECT_TRUE( menu->isVisible() );
	EXPECT_TRUE( menu->isEnabled() );
	EXPECT_FALSE( bar->getButton( 0 )->getUIStyle()->getState() & UIState::StateFlagPressed );
	FocusDispatcherAccess::key( app.getUI(), KEY_DOWN );
	EXPECT_TRUE( menu->getItemSelected() != nullptr );
	FocusDispatcherAccess::key( app.getUI(), KEY_ESCAPE );
	EXPECT_FALSE( menu->isEnabled() );
	EXPECT_TRUE( bar->getButton( 0 )->hasFocus() );
	auto* tabs = UITabWidget::New();
	auto* firstTab = tabs->add( "First", UIPushButton::New() );
	auto* secondTab = tabs->add( "Second", UIPushButton::New() );
	EXPECT_FALSE( tabs->isTabFocusable() );
	EXPECT_TRUE( firstTab->isTabFocusable() );
	EXPECT_TRUE( secondTab->isTabFocusable() );
	secondTab->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_RETURN );
	EXPECT_TRUE( tabs->getTabSelected() == secondTab );
	auto* group = UIWidget::New();
	auto* firstRadio = UIRadioButton::New();
	firstRadio->setParent( group );
	auto* secondRadio = UIRadioButton::New();
	secondRadio->setParent( group );
	EXPECT_EQ( tabbableCount( group ), 2u );
	firstRadio->setActive( true );
	secondRadio->setFocus();
	FocusDispatcherAccess::key( app.getUI(), KEY_SPACE );
	EXPECT_TRUE( secondRadio->isActive() );
	EXPECT_FALSE( firstRadio->isActive() );
}

UTEST( TabFocus, EcodeSearchCallbacksKeepTheirTwoInputLoops ) {
	auto app = focusApp();
	for ( const char* secondId : { "search_replace", "global_search_where" } ) {
		auto* bar = UILinearLayout::NewHorizontal();
		auto* first = UITextInput::New();
		first->setParent( bar );
		auto* second = UITextInput::New();
		second->setId( secondId );
		second->setParent( bar );
		first->setSelectAllDocOnTabNavigate( false );
		second->setSelectAllDocOnTabNavigate( false );
		// DocSearchController and GlobalSearchController intentionally override departure from
		// the second field through this callback, irrespective of the requested direction.
		second->on( Event::OnTabNavigate, [first]( const Event* ) { first->setFocus(); } );
		first->setFocus();
		FocusDispatcherAccess::key( app.getUI(), KEY_TAB );
		EXPECT_TRUE( second->hasFocus() );
		FocusDispatcherAccess::key( app.getUI(), KEY_TAB );
		EXPECT_TRUE( first->hasFocus() );
		second->setFocus();
		FocusDispatcherAccess::key( app.getUI(), KEY_TAB, KEYMOD_SHIFT );
		EXPECT_TRUE( first->hasFocus() );
		eeDelete( bar );
	}
}
