#include "tabtransfer.hpp"
#include "utest.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/doc/textdocument.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uiapplication.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Doc;
using namespace EE::UI::Tools;

class EditorSplitterTestClient : public UICodeEditorSplitter::Client {
  public:
	int widgetFocusChanges{ 0 };

	void onTabCreated( UITab*, UIWidget* ) override {}

	void onCodeEditorCreated( UICodeEditor*, TextDocument& ) override {}

	void onCodeEditorFocusChange( UICodeEditor* ) override {}

	void onWidgetFocusChange( UIWidget* ) override { widgetFocusChanges++; }

	void onDocumentStateChanged( UICodeEditor*, TextDocument& ) override {}

	void onDocumentModified( UICodeEditor*, TextDocument& ) override {}

	void onDocumentSelectionChange( UICodeEditor*, TextDocument& ) override {}

	void onDocumentCursorPosChange( UICodeEditor*, TextDocument& ) override {}

	void onDocumentUndoRedo( UICodeEditor*, TextDocument& ) override {}

	void onColorSchemeChanged( const std::string& ) override {}

	void onDocumentLoaded( UICodeEditor*, const std::string& ) override {}
};

class TestableEditorSplitter : public UICodeEditorSplitter {
  public:
	static TestableEditorSplitter* New( UICodeEditorSplitter::Client* client,
										UISceneNode* sceneNode ) {
		return eeNew( TestableEditorSplitter, ( client, sceneNode ) );
	}

	TestableEditorSplitter( UICodeEditorSplitter::Client* client, UISceneNode* sceneNode ) :
		UICodeEditorSplitter( client, sceneNode, nullptr, {}, "" ) {}

	bool hasWidgetEventCbs( Node* node ) const { return mEventCbs.find( node ) != mEventCbs.end(); }

	bool hasEditorSelection( UICodeEditor* editor ) const {
		return mEditorSelections.find( editor ) != mEditorSelections.end();
	}

	bool hasEditorCloseCb( UICodeEditor* editor ) const {
		return mEditorCloseCbs.find( editor ) != mEditorCloseCbs.end();
	}
};

static UICodeEditor* editorInTab( UITabWidget* tabWidget, Uint32 index ) {
	return tabWidget->getTab( index )->getOwnedWidget()->asType<UICodeEditor>();
}

UTEST( UICodeEditorSplitter, EditorMovedBetweenSplitsKeepsFocusTracking ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	EditorSplitterTestClient client;
	auto* splitter = TestableEditorSplitter::New( &client, app.getUI() );
	auto* tabWidgetA = splitter->createEditorWithTabWidget( app.getUI(), false );
	UICodeEditor* editorA = editorInTab( tabWidgetA, 0 );
	auto* tabWidgetB = splitter->splitTabWidget( SplitDirection::Right, tabWidgetA );
	auto tabAndEditorB = splitter->createCodeEditorInTabWidget( tabWidgetB );
	UITab* tabB = tabAndEditorB.first;
	UICodeEditor* editorB = tabAndEditorB.second;

	transferTabTo( tabB, tabWidgetA );

	editorA->setFocus();
	EXPECT_EQ( splitter->getCurEditor(), editorA );

	editorB->setFocus();
	EXPECT_EQ( splitter->getCurEditor(), editorB );

	eeDelete( splitter );
}

UTEST( UICodeEditorSplitter, WidgetMovedBetweenSplitsKeepsFocusAndTitleTracking ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	EditorSplitterTestClient client;
	auto* splitter = TestableEditorSplitter::New( &client, app.getUI() );
	auto* tabWidgetA = splitter->createEditorWithTabWidget( app.getUI(), false );
	UICodeEditor* editorA = editorInTab( tabWidgetA, 0 );
	auto* tabWidgetB = splitter->splitTabWidget( SplitDirection::Right, tabWidgetA );
	auto tabAndWidget = splitter->createWidgetInTabWidget( tabWidgetB, UIWidget::New(), "Widget" );
	UITab* tabW = tabAndWidget.first;
	UIWidget* widgetW = tabAndWidget.second;

	transferTabTo( tabW, tabWidgetA );

	editorA->setFocus();
	EXPECT_TRUE( splitter->getCurWidget() != widgetW );

	widgetW->setFocus();
	EXPECT_EQ( splitter->getCurWidget(), widgetW );

	widgetW->sendTextEvent( Event::OnTitleChange, "Renamed" );
	EXPECT_TRUE( tabW->getText() == "Renamed" );

	eeDelete( splitter );
}

UTEST( UICodeEditorSplitter, WidgetMovedOutsideSplitterStopsNotifyingSplitter ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	EditorSplitterTestClient client;
	auto* splitter = TestableEditorSplitter::New( &client, app.getUI() );
	auto* tabWidget = splitter->createEditorWithTabWidget( app.getUI(), false );
	UICodeEditor* editorA = editorInTab( tabWidget, 0 );
	auto tabAndWidget = splitter->createWidgetInTabWidget( tabWidget, UIWidget::New(), "Widget" );
	UITab* tabW = tabAndWidget.first;
	UIWidget* widgetW = tabAndWidget.second;
	auto* external = UITabWidget::New();
	external->setParent( app.getUI() );
	external->setAllowDragAndDropTabs( true );

	transferTabTo( tabW, external );

	editorA->setFocus();
	EXPECT_TRUE( splitter->getCurWidget() != widgetW );

	int focusChanges = client.widgetFocusChanges;
	widgetW->setFocus();
	EXPECT_TRUE( splitter->getCurWidget() != widgetW );
	EXPECT_EQ( client.widgetFocusChanges, focusChanges );

	eeDelete( splitter );
}

UTEST( UICodeEditorSplitter, WidgetMovedIntoSplitterStartsNotifyingSplitter ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	EditorSplitterTestClient client;
	auto* splitter = TestableEditorSplitter::New( &client, app.getUI() );
	auto* tabWidgetA = splitter->createEditorWithTabWidget( app.getUI(), false );
	UICodeEditor* editorA = editorInTab( tabWidgetA, 0 );
	auto* tabWidgetB = splitter->splitTabWidget( SplitDirection::Right, tabWidgetA );
	auto* external = UITabWidget::New();
	external->setParent( app.getUI() );
	external->setAllowDragAndDropTabs( true );
	auto* widgetW = UIWidget::New();
	UITab* tabW = external->add( "External", widgetW );
	widgetW->setData( (UintPtr)tabW );

	transferTabTo( tabW, tabWidgetB );

	editorA->setFocus();
	EXPECT_TRUE( splitter->getCurWidget() != widgetW );

	widgetW->setFocus();
	EXPECT_EQ( splitter->getCurWidget(), widgetW );

	widgetW->sendTextEvent( Event::OnTitleChange, "Renamed" );
	EXPECT_TRUE( tabW->getText() == "Renamed" );

	eeDelete( splitter );
}

UTEST( UICodeEditorSplitter, EditorSelectionCacheSurvivesSplitMove ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	EditorSplitterTestClient client;
	auto* splitter = TestableEditorSplitter::New( &client, app.getUI() );
	auto* tabWidgetA = splitter->createEditorWithTabWidget( app.getUI(), false );
	UICodeEditor* editorA = editorInTab( tabWidgetA, 0 );
	auto* tabWidgetB = splitter->splitTabWidget( SplitDirection::Right, tabWidgetA );
	auto tabAndEditorB = splitter->createCodeEditorInTabWidget( tabWidgetB );
	UITab* tabB = tabAndEditorB.first;
	UICodeEditor* editorB = tabAndEditorB.second;
	TextDocument& docB = editorB->getDocument();
	const TextRange savedSelection( TextPosition( 0, 0 ), TextPosition( 0, 5 ) );
	docB.textInput( "hello world" );
	docB.setSelection( savedSelection );
	editorA->setFocus(); // B loses focus and its selection gets cached
	EXPECT_TRUE( splitter->hasEditorSelection( editorB ) );

	transferTabTo( tabB, tabWidgetA );
	EXPECT_TRUE( splitter->hasEditorSelection( editorB ) );

	// Exercise focus away/back with a mutated selection: refocusing B must restore the cached one.
	editorA->setFocus();
	docB.setSelection( TextRange( TextPosition( 0, 6 ), TextPosition( 0, 11 ) ) );
	editorB->setFocus();
	EXPECT_TRUE( docB.getSelection( true ) == savedSelection );

	eeDelete( splitter );
}

UTEST( UICodeEditorSplitter, EditorDestructionClearsSplitterBookkeeping ) {
	UIApplication app(
		WindowSettings( 800, 600, "eepp - unit tests" ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	EditorSplitterTestClient client;
	auto* splitter = TestableEditorSplitter::New( &client, app.getUI() );
	auto* tabWidgetA = splitter->createEditorWithTabWidget( app.getUI(), false );
	UICodeEditor* editorA = editorInTab( tabWidgetA, 0 );
	auto* tabWidgetB = splitter->splitTabWidget( SplitDirection::Right, tabWidgetA );
	auto tabAndEditorB = splitter->createCodeEditorInTabWidget( tabWidgetB );
	UITab* tabB = tabAndEditorB.first;
	UICodeEditor* editorB = tabAndEditorB.second;
	editorB->getDocument().textInput( "hello world" );
	editorB->getDocument().setSelection( TextRange( TextPosition( 0, 0 ), TextPosition( 0, 5 ) ) );
	editorA->setFocus(); // B loses focus and its selection gets cached
	EXPECT_TRUE( splitter->hasEditorSelection( editorB ) );
	EXPECT_TRUE( splitter->hasEditorCloseCb( editorB ) );
	EXPECT_TRUE( splitter->hasWidgetEventCbs( editorB ) );

	// Close and destroy the tab, which destroys the owned editor.
	tabWidgetB->removeTab( tabB, true, true );

	EXPECT_FALSE( splitter->hasEditorSelection( editorB ) );
	EXPECT_FALSE( splitter->hasEditorCloseCb( editorB ) );
	EXPECT_FALSE( splitter->hasWidgetEventCbs( editorB ) );

	eeDelete( splitter );
}
