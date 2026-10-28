#include "eterm.hpp"

#include <array>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/ui/tools/keybindingsconfig.hpp>
#include <eepp/ui/tools/uikeybindingseditor.hpp>
#include <eepp/ui/uicodeeditor.hpp>

namespace eterm {

static constexpr std::array<KeybindingsConfig::Context, 1> keybindingContexts{ { { "eterm" } } };

KeyBindings::ShortcutMap App::getDefaultKeybindings() const {
	const Uint32 modifier = KeyMod::getDefaultModifier();
	const Uint32 secondary = KeyMod::getDefaultSecondaryModifier();
#if EE_PLATFORM == EE_PLATFORM_MACOS
	const Uint32 clipboardModifier = modifier;
	const Uint32 actionModifier = modifier;
	const Uint32 tabModifier = modifier;
	const Uint32 splitModifier = modifier | KEYMOD_SHIFT;
	const Uint32 navigationModifier = modifier;
#else
	const Uint32 clipboardModifier = KEYMOD_CTRL | KEYMOD_SHIFT;
	const Uint32 actionModifier = modifier | KEYMOD_SHIFT;
	const Uint32 tabModifier = secondary | KEYMOD_SHIFT;
	const Uint32 splitModifier = secondary | KEYMOD_SHIFT;
	const Uint32 navigationModifier = modifier | secondary;
#endif
	return {
		{ { KEY_C, clipboardModifier }, "terminal-copy" },
		{ { KEY_V, clipboardModifier }, "terminal-paste" },
		{ { KEY_F, actionModifier }, "terminal-find" },
		{ { KEY_G, actionModifier }, "terminal-find-next" },
		{ { KEY_G, modifier | secondary | KEYMOD_SHIFT }, "terminal-find-previous" },
		{ { KEY_T, actionModifier }, "create-new-terminal" },
		{ { KEY_E, modifier | secondary | KEYMOD_SHIFT },
		  UITerminal::getExclusiveModeToggleCommandName() },
		{ { KEY_S, modifier | secondary }, "terminal-rename" },
		{ { KEY_COMMA, modifier }, "open-settings" },
		{ { KEY_W, actionModifier }, "close-tab" },
		{ { KEY_F11, modifier | KEYMOD_SHIFT }, "debug-widget-tree-view" },
		{ { KEY_PAGEDOWN, KEYMOD_CTRL }, "next-tab" },
		{ { KEY_PAGEUP, KEYMOD_CTRL }, "previous-tab" },
		{ { KEY_TAB, KEYMOD_CTRL }, "next-tab" },
		{ { KEY_TAB, KEYMOD_CTRL | KEYMOD_SHIFT }, "previous-tab" },
		{ { KEY_1, tabModifier }, "switch-to-tab-1" },
		{ { KEY_2, tabModifier }, "switch-to-tab-2" },
		{ { KEY_3, tabModifier }, "switch-to-tab-3" },
		{ { KEY_4, tabModifier }, "switch-to-tab-4" },
		{ { KEY_5, tabModifier }, "switch-to-tab-5" },
		{ { KEY_6, tabModifier }, "switch-to-tab-6" },
		{ { KEY_7, tabModifier }, "switch-to-tab-7" },
		{ { KEY_8, tabModifier }, "switch-to-tab-8" },
		{ { KEY_9, tabModifier }, "switch-to-tab-9" },
		{ { KEY_0, tabModifier }, "switch-to-last-tab" },
		{ { KEY_LEFT, modifier | KEYMOD_SHIFT }, "move-tab-left" },
		{ { KEY_RIGHT, modifier | KEYMOD_SHIFT }, "move-tab-right" },
		{ { KEY_L, splitModifier }, "split-right" },
		{ { KEY_K, splitModifier }, "split-bottom" },
		{ { KEY_J, splitModifier }, "split-left" },
		{ { KEY_I, splitModifier }, "split-top" },
		{ { KEY_J, navigationModifier }, "switch-to-previous-split" },
		{ { KEY_L, navigationModifier }, "switch-to-next-split" },
	};
}

bool App::loadKeybindings( bool defaultsOnly, std::string* outputError ) {
	std::string error;
	if ( !KeybindingsConfig::loadContext(
			 defaultsOnly ? "" : keybindingsPath, "eterm",
			 [this] { return getDefaultKeybindings(); }, appWindow->getInput(), keybindings,
			 error ) ) {
		Log::error( "%s", error.c_str() );
		if ( outputError )
			*outputError = error;
		return false;
	}
	if ( outputError )
		outputError->clear();
	return true;
}

void App::applyKeybindings( UITerminal* terminal ) {
	terminal->getKeyBindings().reset();
	keybindings.apply( terminal->getKeyBindings() );
}

void App::applyKeybindings( UICodeEditor* editor ) {
	editor->getKeyBindings().setKeybinds( *UICodeEditor::getDefaultKeybindings() );
	editor->addKeyBinding( { KEY_S, KeyMod::getDefaultModifier() }, "save-doc", true );
	// Terminal-only commands must not shadow text editing (notably Meta+C and Meta+V).
	keybindings.apply( editor->getKeyBindings(), [editor]( const std::string& command ) {
		return editor->getDocument().hasCommand( command );
	} );
}

bool App::reloadKeybindings( std::string* error ) {
	if ( !FileSystem::fileExists( keybindingsPath ) ) {
		if ( error )
			*error = "Keybindings file does not exist: " + keybindingsPath;
		return false;
	}
	if ( !loadKeybindings( false, error ) )
		return false;
	forEachTerminal( [this]( UITerminal* terminal ) { applyKeybindings( terminal ); } );
	if ( keybindingsFileEditor && tabSplitter->ownedWidgetExists( keybindingsFileEditor ) )
		applyKeybindings( keybindingsFileEditor );
#if EE_PLATFORM == EE_PLATFORM_MACOS
	syncGlobalMenuKeybindings();
#endif
	return true;
}

UIWidget* App::createKeybindingsEditor( UIWidget* parent ) {
	return UIKeybindingsEditor::NewForFile(
		parent, { .path = keybindingsPath,
				  .contexts = keybindingContexts,
				  // eterm closes with its last terminal, so one is always available to list the
				  // assignable commands that have no default shortcut.
				  .catalog =
					  [this]( KeybindingsConfig::Catalog& catalog ) {
						  std::vector<std::string> commands;
						  forEachTerminal( [&]( UITerminal* terminal ) {
							  if ( commands.empty() )
								  commands = terminal->getCommandList();
						  } );
						  catalog.add( "eterm", getDefaultKeybindings(), commands,
									   i18n( "terminal", "Terminal" ) );
					  },
				  .apply =
					  [this] {
						  std::string error;
						  return reloadKeybindings( &error ) ? String() : String( error );
					  },
				  .manualEdit =
					  [this] {
						  settingsActions->closeSettings();
						  openKeybindingsFile();
					  } } );
}

void App::openKeybindingsFile() {
	if ( keybindingsFileEditor && tabSplitter->ownedWidgetExists( keybindingsFileEditor ) ) {
		if ( auto* tabs = tabSplitter->tabWidgetFromWidget( keybindingsFileEditor ) )
			tabs->setTabSelected( tabs->getTabFromOwnedWidget( keybindingsFileEditor ) );
		keybindingsFileEditor->setFocus();
		return;
	}

	auto* editor = UICodeEditor::New();
	if ( editor->loadFromFile( keybindingsPath ) == TextDocument::LoadStatus::Failed ) {
		editor->close();
		return;
	}
	editor->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	editor->setFontSize( terminalFontSize );
	editor->setSyntaxDefinition( editor->getDocument().guessSyntax() );
	editor->getDocument().setCommand( "save-doc", [editor] { editor->getDocument().save(); } );
	registerTabCommands( editor->getDocument(), editor );
	applyKeybindings( editor );
	editor->on( Event::OnClose, [this, editor]( const Event* ) {
		if ( keybindingsFileEditor == editor )
			keybindingsFileEditor = nullptr;
	} );

	auto* current = tabSplitter->getCurWidget();
	auto* target =
		current ? tabSplitter->tabWidgetFromWidget( current ) : tabSplitter->getFirstTabWidget();
	keybindingsFileEditor = editor;
	auto* tab = tabSplitter->createWidgetInTabWidget( target, editor, "keybindings.cfg" ).first;
	const auto updateTabState = [editor, tab]( const Event* ) {
		const bool dirty = editor->getDocument().isDirty();
		tab->removeClass( dirty ? "tab_clear" : "tab_modified" );
		tab->addClass( dirty ? "tab_modified" : "tab_clear" );
	};
	editor->on( Event::OnTextChanged, updateTabState );
	editor->on( Event::OnDocumentUndoRedo, updateTabState );
	editor->on( Event::OnDocumentSave, updateTabState );
	updateTabState( nullptr );
	editor->setFocus();
}

void App::handleFileAction( efsw::WatchID, const std::string&, const std::string& filename,
							efsw::Action action, const std::string& oldFilename ) {
	if ( ( filename == "keybindings.cfg" || oldFilename == "keybindings.cfg" ) &&
		 ( action == efsw::Actions::Add || action == efsw::Actions::Modified ||
		   action == efsw::Actions::Moved ) ) {
		keybindingsChanged.store( true, std::memory_order_release );
	}
}

} // namespace eterm
