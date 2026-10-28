#include "utest.hpp"
#include <array>
#include <eepp/scene/eventconnection.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/tools/uikeybindingseditor.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiselectbutton.hpp>
#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/window/input.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Tools;
using namespace EE::Window;

namespace {

class TestKeybindingsEditor : public UIKeybindingsEditor {
  public:
	TestKeybindingsEditor( UIWidget* parent, KeybindingsConfigData model, Save save ) :
		UIKeybindingsEditor( parent, std::move( model ), std::move( save ) ) {}

	using UIKeybindingsEditor::propose;

	using UIKeybindingsEditor::edit;

	using UIKeybindingsEditor::onKeyDown;

	UIMessageBox* dialog() {
		UIMessageBox* result = nullptr;
		for ( auto* child = getUISceneNode()->getRoot()->getFirstChild(); child;
			  child = child->getNextNode() )
			if ( auto* box = dynamic_cast<UIMessageBox*>( child ) )
				result = box;
		return result;
	}
};

KeybindingsConfigData sample() {
	return { { { "editor", "copy", {}, { "ctrl+c", "ctrl+insert" }, { "ctrl+c" } },
			   { "editor", "paste", {}, { "ctrl+v" }, { "ctrl+v" } },
			   { "search", "find", {}, { "ctrl+c" }, {} } },
			 "ctrl",
			 "alt" };
}

} // namespace

UTEST( UIKeybindingsEditor, SymbolicModifiersResolveWithoutChangingGlobalState ) {
	UIApplication app( WindowSettings{ 640, 480, "keybindings editor tests" } );
	auto model = sample();
	auto* input = app.getUI()->getWindow()->getInput();
	auto original = KeyMod::getDefaultModifier();
	model.modifier = "meta";
	EXPECT_EQ( static_cast<Uint32>( KEYMOD_META ), model.resolve( input, "mod+c" ).mod );
	EXPECT_EQ( static_cast<Uint32>( KEYMOD_CTRL ), model.resolve( input, "ctrl+c" ).mod );
	EXPECT_EQ( static_cast<Uint32>( KEYMOD_LALT ), model.resolve( input, "mod2+c" ).mod );
	EXPECT_EQ( original, KeyMod::getDefaultModifier() );
	const auto modeName = input->getKeyName( KEY_MODE );
	if ( input->getKeyFromName( modeName ) == KEY_MODE )
		EXPECT_EQ( KEY_MODE, model.resolve( input, modeName ).key );
}

UTEST( UIKeybindingsEditor, ReassignRequiresConfirmationAndPreservesOtherBindings ) {
	UIApplication app( WindowSettings{ 640, 480, "keybindings editor tests" } );
	int saves = 0;
	KeybindingsConfigData saved;
	auto* editor =
		new TestKeybindingsEditor( app.getUI()->getRoot(), sample(), [&]( const auto& model ) {
			++saves;
			saved = model;
			return String();
		} );
	auto proposal = sample();
	proposal.commands[1].bindings = { "ctrl+c" };
	editor->propose( proposal );
	EXPECT_EQ( 0, saves );
	ASSERT_TRUE( editor->dialog() != nullptr );
	EXPECT_EQ( 2u, editor->getConfiguration().commands[0].bindings.size() );
	editor->dialog()->sendCommonEvent( Event::OnConfirm );
	EXPECT_EQ( 1, saves );
	ASSERT_EQ( 1u, saved.commands[0].bindings.size() );
	EXPECT_TRUE( saved.commands[0].bindings[0] == "ctrl+insert" );
	EXPECT_FALSE( saved.commands[0].unbound );
	EXPECT_TRUE( saved.commands[2].bindings[0] == "ctrl+c" );
}

UTEST( UIKeybindingsEditor, CancelAndSaveFailureLeaveModelUnchanged ) {
	UIApplication app( WindowSettings{ 640, 480, "keybindings editor tests" } );
	int saves = 0;
	auto* editor = new TestKeybindingsEditor( app.getUI()->getRoot(), sample(), [&]( const auto& ) {
		++saves;
		return String( "write failed" );
	} );
	auto proposal = sample();
	proposal.commands[1].bindings = { "ctrl+c" };
	editor->propose( proposal );
	ASSERT_TRUE( editor->dialog() != nullptr );
	editor->dialog()->sendCommonEvent( Event::OnCancel );
	EXPECT_EQ( 0, saves );
	EXPECT_TRUE( editor->getConfiguration().commands[1].bindings[0] == "ctrl+v" );
	// A separate editor exercises a nonconflicting edit rejected by persistence.
	auto* failing =
		new TestKeybindingsEditor( app.getUI()->getRoot(), sample(), [&]( const auto& ) {
			++saves;
			return String( "write failed" );
		} );
	proposal.commands[1].bindings.clear();
	failing->propose( proposal );
	EXPECT_EQ( 1, saves );
	EXPECT_TRUE( failing->getConfiguration().commands[1].bindings[0] == "ctrl+v" );
}

UTEST( UIKeybindingsEditor, ModifierChangeConfirmsResolvedCollisions ) {
	UIApplication app( WindowSettings{ 640, 480, "keybindings editor tests" } );
	auto model = sample();
	model.commands[0].bindings = { "mod+c" };
	model.commands[1].bindings = { "meta+c" };
	int saves = 0;
	auto* editor = new TestKeybindingsEditor( app.getUI()->getRoot(), model, [&]( const auto& ) {
		++saves;
		return String();
	} );
	model.modifier = "meta";
	editor->propose( model );
	EXPECT_EQ( 0, saves );
	ASSERT_TRUE( editor->dialog() != nullptr );
	editor->dialog()->sendCommonEvent( Event::OnConfirm );
	EXPECT_EQ( 1, saves );
	EXPECT_TRUE( editor->getConfiguration().commands[1].bindings.empty() );
}

UTEST( UIKeybindingsEditor, CaptureCancelsAndPreservesAdditionalBindings ) {
	UIApplication app( WindowSettings{ 640, 480, "keybindings editor tests" } );
	auto model = sample();
	model.commands[0].bindings.push_back( "ctrl+shift+c" );
	int saves = 0;
	auto* editor = new TestKeybindingsEditor( app.getUI()->getRoot(), model, [&]( const auto& ) {
		++saves;
		return String();
	} );
	editor->edit( 0, 1 );
	EXPECT_EQ( 1u, editor->onKeyDown(
					   KeyEvent( editor, Event::KeyDown, KEY_ESCAPE, SCANCODE_ESCAPE, 0, 0 ) ) );
	EXPECT_EQ( 0, saves );
	editor->edit( 0, 1 );
	EXPECT_EQ( 1u, editor->onKeyDown(
					   KeyEvent( editor, Event::KeyDown, KEY_K, SCANCODE_K, 0, KEYMOD_CTRL ) ) );
	EXPECT_EQ( 1, saves );
	ASSERT_EQ( 3u, editor->getConfiguration().commands[0].bindings.size() );
	EXPECT_TRUE( editor->getConfiguration().commands[0].bindings[2] == "ctrl+shift+c" );
}

UTEST( UIKeybindingsEditor, SelectingCommandCreatesBindingControls ) {
	UIApplication app( WindowSettings{ 640, 480, "keybindings editor tests" } );
	auto* editor = UIKeybindingsEditor::New( app.getUI()->getRoot(), sample(), {} );
	auto* table = static_cast<UITreeView*>( editor->querySelector( "treeview" ) );
	ASSERT_TRUE( table != nullptr );
	auto context = table->getModel()->index( 0, 0 );
	EXPECT_EQ( 2u, table->getModel()->rowCount() );
	EXPECT_EQ( 2u, table->getModel()->rowCount( context ) );
	table->getSelection().set( table->getModel()->index( 0, 0, context ) );
	EXPECT_TRUE( editor->querySelector( "pushbutton" ) != nullptr );
}

UTEST( UIKeybindingsEditor, FilteringKeepsCommandsUnderTheirContexts ) {
	UIApplication app( WindowSettings{ 800, 700, "keybindings tree filter test" } );
	auto* editor = UIKeybindingsEditor::New( app.getUI()->getRoot(), sample(), {} );
	auto* tree = static_cast<UITreeView*>( editor->querySelector( "treeview" ) );
	auto* search = editor->find<UITextInput>( "kb_search" );
	search->setText( "copy" );
	EXPECT_EQ( 1u, tree->getModel()->rowCount() );
	auto context = tree->getModel()->index( 0, 0 );
	EXPECT_EQ( 1u, tree->getModel()->rowCount( context ) );
	auto command = tree->getModel()->index( 0, 0, context );
	EXPECT_TRUE( tree->getModel()->parentIndex( command ) == context );
	tree->getSelection().set( command );
	EXPECT_TRUE( editor->querySelector( "pushbutton" ) != nullptr );
	search->setText( "not-a-command" );
	EXPECT_EQ( 0u, tree->getModel()->rowCount() );
	search->setText( "search" );
	EXPECT_EQ( 1u, tree->getModel()->rowCount() );
	context = tree->getModel()->index( 0, 0 );
	EXPECT_EQ( 1u, tree->getModel()->rowCount( context ) );
}

UTEST( UIKeybindingsEditor, ComponentSpacingSurvivesThemeReplacement ) {
	UIApplication app( WindowSettings{ 1000, 800, "keybindings theme reload test" },
					   UIApplication::Settings( EE::System::Sys::getProcessPath() + ".." +
													EE::System::FileSystem::getOSSlash(),
												1 ) );
	auto* scene = app.getUI();
	auto* editor = UIKeybindingsEditor::New( scene->getRoot(), sample(), {} );
	scene->update( Milliseconds( 16 ) );
	auto* search = editor->find<UITextInput>( "kb_search" );
	const auto margin = search->getLayoutMargin().Bottom;
	ASSERT_GT( margin, 0.f );
	CSS::StyleSheetParser parser;
	ASSERT_TRUE( parser.loadFromFile( "assets/ui/breeze.css" ) );
	auto components = scene->getStyleSheet().getAllWithMarkers();
	scene->setStyleSheet( parser.getStyleSheet(), false );
	scene->getStyleSheet().combineStyleSheet( components );
	scene->getStyleSheet().updateMediaLists( scene->getMediaFeatures() );
	scene->reloadStyle( true, true, true );
	scene->update( Milliseconds( 16 ) );
	EXPECT_EQ( margin, search->getLayoutMargin().Bottom );
	// Controls materialized after theme replacement must also retain the component rules.
	auto* tree = static_cast<UITreeView*>( editor->querySelector( "treeview" ) );
	auto context = tree->getModel()->index( 0, 0 );
	tree->getSelection().set( tree->getModel()->index( 0, 0, context ) );
	scene->update( Milliseconds( 16 ) );
	auto* row = editor->querySelector( ".kb_binding_row" );
	ASSERT_TRUE( row != nullptr );
	EXPECT_GT( row->getLayoutMargin().Bottom, 0.f );
}

UTEST( UIKeybindingsEditor, RecordingButtonShowsAndClearsCaptureState ) {
	UIApplication app( WindowSettings{ 900, 700, "record button state" } );
	auto* editor = new TestKeybindingsEditor(
		app.getUI()->getRoot(), sample(), []( const auto& ) { return String( "write failed" ); } );
	auto* tree = static_cast<UITreeView*>( editor->querySelector( "treeview" ) );
	auto context = tree->getModel()->index( 0, 0 );
	tree->getSelection().set( tree->getModel()->index( 0, 0, context ) );
	auto* main = editor->find<UISelectButton>( "kb_record_0" );
	auto* alternative = editor->find<UISelectButton>( "kb_record_1" );
	ASSERT_TRUE( main != nullptr );
	ASSERT_TRUE( alternative != nullptr );
	const String original = main->getText();
	EXPECT_FALSE( main->isSelected() );
	editor->edit( 0, 0 );
	EXPECT_TRUE( main->isSelected() );
	EXPECT_TRUE( main->getText() != original );
	editor->onKeyDown( KeyEvent( editor, Event::KeyDown, KEY_ESCAPE, SCANCODE_ESCAPE, 0, 0 ) );
	EXPECT_FALSE( main->isSelected() );
	EXPECT_TRUE( main->getText() == original );
	editor->edit( 0, 0 );
	editor->edit( 0, 1 );
	EXPECT_FALSE( main->isSelected() );
	EXPECT_TRUE( alternative->isSelected() );
	editor->onKeyDown( KeyEvent( editor, Event::KeyDown, KEY_K, SCANCODE_K, 0, KEYMOD_CTRL ) );
	EXPECT_FALSE( alternative->isSelected() );
	EXPECT_TRUE( alternative->getText() == "ctrl+insert" );
	editor->edit( 0, 0 );
	editor->find<UITextInput>( "kb_search" )->setFocus();
	EXPECT_FALSE( main->isSelected() );
	EXPECT_TRUE( main->getText() == original );
}

UTEST( UIKeybindingsEditor, ManualEditIsConsumerControlledAndHiddenByDefault ) {
	UIApplication app( WindowSettings{ 900, 700, "manual edit callback" } );
	auto* editor = UIKeybindingsEditor::New( app.getUI()->getRoot(), sample(), {} );
	auto* manual = editor->find<UIPushButton>( "kb_manual_edit" );
	ASSERT_TRUE( manual != nullptr );
	EXPECT_FALSE( manual->isVisible() );
	EXPECT_FALSE( manual->isEnabled() );
	int first = 0;
	int second = 0;
	editor->setManualEditCallback( [&] { ++first; } );
	EXPECT_TRUE( manual->isVisible() );
	EXPECT_TRUE( manual->isEnabled() );
	manual->sendMouseEvent( Event::MouseClick, Vector2i(), EE_BUTTON_LMASK );
	EXPECT_EQ( 1, first );
	editor->setManualEditCallback( [&] { ++second; } );
	manual->sendMouseEvent( Event::MouseClick, Vector2i(), EE_BUTTON_LMASK );
	EXPECT_EQ( 1, first );
	EXPECT_EQ( 1, second );
	editor->setManualEditCallback( {} );
	EXPECT_FALSE( manual->isVisible() );
	EXPECT_FALSE( manual->isEnabled() );
}

UTEST( UIKeybindingsEditor, RestoreAllRequiresConfirmationAndRestoresEveryContext ) {
	UIApplication app( WindowSettings{ 900, 700, "restore shortcuts" } );
	auto model = sample();
	model.commands[0].bindings.clear();
	model.commands[1].bindings = { "ctrl+k", "ctrl+shift+k" };
	model.commands[2].bindings = { "f3" };
	model.commands[2].unbound = true;
	int saves = 0;
	auto* editor = new TestKeybindingsEditor( app.getUI()->getRoot(), model, [&]( const auto& ) {
		++saves;
		return String();
	} );
	auto* restore = editor->find<UIPushButton>( "kb_restore_all" );
	ASSERT_TRUE( restore != nullptr );
	EXPECT_TRUE( restore->isVisible() );
	EXPECT_TRUE( restore->isEnabled() );
	restore->sendMouseEvent( Event::MouseClick, Vector2i(), EE_BUTTON_LMASK );
	EXPECT_EQ( 0, saves );
	ASSERT_TRUE( editor->dialog() != nullptr );
	editor->dialog()->sendCommonEvent( Event::OnConfirm );
	EXPECT_EQ( 1, saves );
	for ( const auto& command : editor->getConfiguration().commands ) {
		EXPECT_TRUE( command.bindings == command.defaults );
		EXPECT_FALSE( command.unbound );
	}
	EXPECT_TRUE( editor->getConfiguration().modifier == model.modifier );
	EXPECT_TRUE( editor->getConfiguration().secondaryModifier == model.secondaryModifier );
}

UTEST( UIKeybindingsEditor, CancelledRestoreAndFailedSavePreserveCustomBindings ) {
	UIApplication app( WindowSettings{ 900, 700, "restore shortcuts failure" } );
	auto model = sample();
	model.commands[0].bindings = { "ctrl+k" };
	int saves = 0;
	auto* editor = new TestKeybindingsEditor( app.getUI()->getRoot(), model, [&]( const auto& ) {
		++saves;
		return String( "write failed" );
	} );
	editor->restoreDefaults();
	ASSERT_TRUE( editor->dialog() != nullptr );
	editor->dialog()->sendCommonEvent( Event::OnCancel );
	EXPECT_EQ( 0, saves );
	EXPECT_TRUE( editor->getConfiguration().commands[0].bindings == model.commands[0].bindings );
	editor->dialog()->sendCommonEvent( Event::OnConfirm );
	EXPECT_EQ( 0, saves );
	editor->restoreDefaults();
	editor->dialog()->sendCommonEvent( Event::OnConfirm );
	EXPECT_EQ( 1, saves );
	EXPECT_TRUE( editor->getConfiguration().commands[0].bindings == model.commands[0].bindings );
}

UTEST( UIKeybindingsEditor, ClearAndResetDistinguishExplicitUnbindingFromEmptyDefaults ) {
	UIApplication app( WindowSettings{ 900, 700, "explicit unbound editor state" } );
	int saves = 0;
	auto* editor = UIKeybindingsEditor::New( app.getUI()->getRoot(), sample(), [&]( const auto& ) {
		++saves;
		return String();
	} );
	auto* tree = static_cast<UITreeView*>( editor->querySelector( "treeview" ) );
	auto select = [&] {
		tree->getSelection().set(
			tree->getModel()->index( 0, 0, tree->getModel()->index( 1, 0 ) ) );
	};
	select();
	editor->find<UIPushButton>( "kb_clear_0" )
		->sendMouseEvent( Event::MouseClick, Vector2i(), EE_BUTTON_LMASK );
	EXPECT_EQ( 1, saves );
	EXPECT_TRUE( editor->getConfiguration().commands[2].bindings.empty() );
	EXPECT_TRUE( editor->getConfiguration().commands[2].unbound );
	select();
	editor->find<UIPushButton>( "kb_reset_command" )
		->sendMouseEvent( Event::MouseClick, Vector2i(), EE_BUTTON_LMASK );
	EXPECT_EQ( 2, saves );
	EXPECT_TRUE( editor->getConfiguration().commands[2].bindings.empty() );
	EXPECT_FALSE( editor->getConfiguration().commands[2].unbound );
}

UTEST( UIKeybindingsEditor, RecordingUsesItsOwnModifiersWithoutChangingGlobalState ) {
	UIApplication app( WindowSettings{ 640, 480, "independent recording modifiers" } );
	auto model = sample();
	const auto global = KeyMod::getDefaultModifier();
	model.modifier = global == KEYMOD_META ? "ctrl" : "meta";
	model.secondaryModifier = "alt";
	const auto physical = KeyMod::getKeyMod( model.modifier );
	int saves = 0;
	auto* editor = new TestKeybindingsEditor( app.getUI()->getRoot(), model, [&]( const auto& ) {
		++saves;
		return String();
	} );
	editor->edit( 0, 0 );
	editor->onKeyDown( KeyEvent( editor, Event::KeyDown, KEY_K, SCANCODE_K, 0, physical ) );
	ASSERT_EQ( 1, saves );
	EXPECT_TRUE( editor->getConfiguration().commands[0].bindings[0] == "mod+k" );
	EXPECT_EQ( KeyBindings::sanitizeShortcut( { KEY_K, physical } ).mod,
			   editor->getConfiguration().resolve( app.getWindow()->getInput(), "mod+k" ).mod );
	EXPECT_EQ( global, KeyMod::getDefaultModifier() );
	EXPECT_EQ( 1u, editor->onKeyDown(
					   KeyEvent( editor, Event::KeyDown, KEY_K, SCANCODE_K, 0, physical, true ) ) );
	EXPECT_EQ( 1, saves );
}

UTEST( UIKeybindingsEditor, RestoreReportsCollisionsCausedByTheChosenModifiers ) {
	UIApplication app( WindowSettings{ 640, 480, "restore shortcut conflicts" } );
	auto model = sample();
	model.modifier = "meta";
	model.commands[0].defaults = { "mod+c" };
	model.commands[1].defaults = { "meta+c" };
	int saves = 0;
	auto* editor = new TestKeybindingsEditor( app.getUI()->getRoot(), model, [&]( const auto& ) {
		++saves;
		return String();
	} );
	editor->restoreDefaults();
	ASSERT_TRUE( editor->dialog() != nullptr );
	const auto report = editor->dialog()->getTextBox()->getText().toUtf8();
	EXPECT_TRUE( report.find( "copy" ) != std::string::npos );
	EXPECT_TRUE( report.find( "paste" ) != std::string::npos );
	EXPECT_EQ( 0, saves );
	editor->dialog()->sendCommonEvent( Event::OnConfirm );
	EXPECT_EQ( 1, saves );
	EXPECT_TRUE( editor->getConfiguration().commands[1].bindings.empty() );
	EXPECT_TRUE( editor->getConfiguration().commands[1].unbound );
}

UTEST( UIKeybindingsEditor, ConfirmationCanOnlyApplyAProposalOnce ) {
	UIApplication app( WindowSettings{ 640, 480, "one shot shortcut confirmation" } );
	int saves = 0;
	auto* editor = new TestKeybindingsEditor( app.getUI()->getRoot(), sample(), [&]( const auto& ) {
		++saves;
		return String();
	} );
	auto proposal = sample();
	proposal.commands[1].bindings = { "ctrl+c" };
	editor->propose( std::move( proposal ) );
	ASSERT_TRUE( editor->dialog() != nullptr );
	editor->dialog()->sendCommonEvent( Event::OnConfirm );
	editor->dialog()->sendCommonEvent( Event::OnConfirm );
	EXPECT_EQ( 1, saves );
	EXPECT_EQ( 3u, editor->getConfiguration().commands.size() );
}

UTEST( UIKeybindingsEditor, ClosingEditorDoesNotDeleteAQueuedDialogTwice ) {
	UIApplication app( WindowSettings{ 640, 480, "queued shortcut dialog teardown" } );
	for ( bool queued : { false, true } ) {
		auto* editor = new TestKeybindingsEditor( app.getUI()->getRoot(), sample(), {} );
		auto proposal = sample();
		proposal.commands[1].bindings = { "ctrl+c" };
		editor->propose( std::move( proposal ) );
		auto* dialog = editor->dialog();
		ASSERT_TRUE( dialog != nullptr );
		if ( queued )
			dialog->close();
		int destroyed = 0;
		auto observer = dialog->connect( Event::OnClose, [&]( const Event* ) { ++destroyed; } );
		delete editor;
		app.getUI()->update( Milliseconds( 16 ) );
		EXPECT_EQ( 1, destroyed );
	}
}

UTEST( UIKeybindingsEditor, DismissedDialogCannotCallDestroyedEditorOrBlockFurtherEdits ) {
	UIApplication app( WindowSettings{ 640, 480, "dismissed shortcut dialog callbacks" } );
	for ( auto event : { Event::OnConfirm, Event::OnCancel, Event::OnDiscard } ) {
		int saves = 0;
		auto* editor =
			new TestKeybindingsEditor( app.getUI()->getRoot(), sample(), [&]( const auto& ) {
				++saves;
				return String();
			} );
		auto proposal = sample();
		proposal.commands[1].bindings = { "ctrl+c" };
		editor->propose( std::move( proposal ) );
		auto* dialog = editor->dialog();
		ASSERT_TRUE( dialog != nullptr );
		int observed = 0;
		EventConnectionList observers;
		for ( auto type :
			  { Event::OnConfirm, Event::OnCancel, Event::OnDiscard, Event::OnClose } ) {
			observers += dialog->connect( type, [&]( const Event* ) { ++observed; } );
		}
		dialog->sendCommonEvent( event );
		EXPECT_EQ( 1, observed );
		const auto confirmed = saves;
		auto next = editor->getConfiguration();
		next.commands[1].bindings = { "ctrl+k" };
		editor->propose( std::move( next ) );
		EXPECT_EQ( confirmed + 1, saves );
		delete editor;
		dialog->sendCommonEvent( Event::OnConfirm );
		dialog->sendCommonEvent( Event::OnCancel );
		dialog->sendCommonEvent( Event::OnDiscard );
		EXPECT_EQ( confirmed + 1, saves );
		EXPECT_EQ( 4, observed );
		dialog->close();
		app.getUI()->update( Milliseconds( 16 ) );
		EXPECT_EQ( 5, observed );
	}
}

UTEST( UIKeybindingsEditor, EditingPreservesTheSelectedCommandAndBindingControls ) {
	UIApplication app( WindowSettings{ 640, 480, "shortcut selection after edits" } );
	auto* editor = new TestKeybindingsEditor( app.getUI()->getRoot(), sample(), {} );
	auto* tree = static_cast<UITreeView*>( editor->querySelector( "treeview" ) );
	const auto context = tree->getModel()->index( 0, 0 );
	tree->getSelection().set( tree->getModel()->index( 1, 0, context ) );
	for ( const char* binding : { "ctrl+k", "escape", "" } ) {
		auto proposal = editor->getConfiguration();
		proposal.commands[1].bindings =
			*binding ? std::vector<std::string>{ binding } : std::vector<std::string>{};
		editor->propose( std::move( proposal ) );
		EXPECT_TRUE( tree->getSelection().first().isValid() );
		EXPECT_EQ( 1, tree->getSelection().first().row() );
		EXPECT_TRUE( editor->find<UISelectButton>( "kb_record_0" ) != nullptr );
	}
}

UTEST( UIKeybindingsEditor, DialogDestructionBeforeEditorAllowsAnotherConfirmation ) {
	UIApplication app( WindowSettings{ 640, 480, "shortcut dialog emitter lifetime" } );
	int saves = 0;
	auto* editor = new TestKeybindingsEditor( app.getUI()->getRoot(), sample(), [&]( const auto& ) {
		++saves;
		return String();
	} );
	editor->restoreDefaults();
	auto* dialog = editor->dialog();
	ASSERT_TRUE( dialog != nullptr );
	// Destroy the emitter first; queued teardown is covered separately above.
	delete dialog;
	EXPECT_EQ( 0, saves );
	editor->restoreDefaults();
	dialog = editor->dialog();
	ASSERT_TRUE( dialog != nullptr );
	dialog->sendCommonEvent( Event::OnConfirm );
	EXPECT_EQ( 1, saves );
	delete editor;
	dialog->close();
	app.getUI()->update( Milliseconds( 16 ) );
}

UTEST( UIKeybindingsEditor, FileEditorWritesTheFileAndRefusesStaleChanges ) {
	UIApplication app( WindowSettings{ 900, 700, "file keybindings editor" } );
	const auto path = Sys::getTempPath() + "eepp_keybindings_file_editor.cfg";
	FileSystem::fileWrite(
		path, std::string_view( "[modifier]\nmod=ctrl\nmod2=alt\n\n[viewer]\nctrl+o=open\n"
								"ctrl+w=close\n" ) );
	static constexpr std::array<KeybindingsConfig::Context, 1> contexts{ { { "viewer" } } };
	int applied = 0;
	auto* widget = UIKeybindingsEditor::NewForFile(
		app.getUI()->getRoot(), { .path = path,
								  .contexts = contexts,
								  .catalog =
									  []( KeybindingsConfig::Catalog& catalog ) {
										  catalog.add( "viewer",
													   std::unordered_map<std::string, std::string>{
														   { "ctrl+o", "open" } } );
									  },
								  .apply =
									  [&] {
										  ++applied;
										  return String();
									  } } );
	auto* editor = dynamic_cast<UIKeybindingsEditor*>( widget );
	ASSERT_TRUE( editor != nullptr );
	ASSERT_EQ( 2u, editor->getConfiguration().commands.size() );
	auto* tree = static_cast<UITreeView*>( editor->querySelector( "treeview" ) );
	auto clear = [&]( int row ) {
		tree->getSelection().set(
			tree->getModel()->index( row, 0, tree->getModel()->index( 0, 0 ) ) );
		editor->find<UIPushButton>( "kb_clear_0" )
			->sendMouseEvent( Event::MouseClick, Vector2i(), EE_BUTTON_LMASK );
	};

	clear( 0 ); // close
	EXPECT_EQ( 1, applied );
	IniFile saved( path );
	EXPECT_FALSE( saved.keyValueExists( "viewer", "ctrl+w" ) );
	EXPECT_TRUE( saved.getValue( "viewer.state", "unbound" ) == R"(["close"])" );
	EXPECT_TRUE( saved.getValue( "viewer", "ctrl+o" ) == "open" );

	// An external edit after the editor read the file must not be overwritten.
	const std::string external = "[viewer]\nctrl+o=open\nctrl+e=edit\n";
	FileSystem::fileWrite( path, std::string_view( external ) );
	clear( 1 ); // open
	EXPECT_EQ( 1, applied );
	std::string contents;
	FileSystem::fileGet( path, contents );
	EXPECT_TRUE( contents == external );
	EXPECT_FALSE( editor->find<UITextView>( "kb_status" )->getText().empty() );
	FileSystem::fileRemove( path );
}

UTEST( UIKeybindingsEditor, FileEditorOffersManualEditingForInvalidFiles ) {
	UIApplication app( WindowSettings{ 900, 700, "invalid keybindings file" } );
	const auto path = Sys::getTempPath() + "eepp_keybindings_invalid_file.cfg";
	FileSystem::fileWrite( path, std::string_view( "[modifier]\nmod=not-a-modifier\n" ) );
	static constexpr std::array<KeybindingsConfig::Context, 1> contexts{ { { "viewer" } } };
	int manualEdits = 0;
	auto* widget = UIKeybindingsEditor::NewForFile(
		app.getUI()->getRoot(),
		{ .path = path, .contexts = contexts, .manualEdit = [&] { ++manualEdits; } } );
	EXPECT_TRUE( dynamic_cast<UIKeybindingsEditor*>( widget ) == nullptr );
	auto* button = widget->querySelector( "pushbutton" );
	ASSERT_TRUE( button != nullptr );
	button->sendMouseEvent( Event::MouseClick, Vector2i(), EE_BUTTON_LMASK );
	EXPECT_EQ( 1, manualEdits );
	FileSystem::fileRemove( path );
}
