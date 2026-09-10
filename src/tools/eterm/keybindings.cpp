#include "eterm.hpp"

#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/ui/uicodeeditor.hpp>

namespace eterm {

KeyBindings::ShortcutMap App::getDefaultKeybindings() const {
	return {
		{ { KEY_C, KEYMOD_CTRL | KEYMOD_SHIFT }, "terminal-copy" },
		{ { KEY_V, KEYMOD_CTRL | KEYMOD_SHIFT }, "terminal-paste" },
		{ { KEY_F, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "terminal-find" },
		{ { KEY_G, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "terminal-find-next" },
		{ { KEY_G, KeyMod::getDefaultModifier() | KEYMOD_SHIFT | KEYMOD_ALT },
		  "terminal-find-previous" },
		{ { KEY_T, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "create-new-terminal" },
		{ { KEY_E,
			KeyMod::getDefaultModifier() | KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT },
		  UITerminal::getExclusiveModeToggleCommandName() },
		{ { KEY_S, KeyMod::getDefaultModifier() | KeyMod::getDefaultSecondaryModifier() },
		  "terminal-rename" },
		{ { KEY_COMMA, KeyMod::getDefaultModifier() }, "open-settings" },
		{ { KEY_W, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "close-tab" },
		{ { KEY_F11, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "debug-widget-tree-view" },
		{ { KEY_PAGEDOWN, KEYMOD_CTRL }, "next-tab" },
		{ { KEY_PAGEUP, KEYMOD_CTRL }, "previous-tab" },
		{ { KEY_TAB, KEYMOD_CTRL }, "next-tab" },
		{ { KEY_TAB, KEYMOD_CTRL | KEYMOD_SHIFT }, "previous-tab" },
		{ { KEY_1, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "switch-to-tab-1" },
		{ { KEY_2, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "switch-to-tab-2" },
		{ { KEY_3, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "switch-to-tab-3" },
		{ { KEY_4, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "switch-to-tab-4" },
		{ { KEY_5, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "switch-to-tab-5" },
		{ { KEY_6, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "switch-to-tab-6" },
		{ { KEY_7, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "switch-to-tab-7" },
		{ { KEY_8, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "switch-to-tab-8" },
		{ { KEY_9, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "switch-to-tab-9" },
		{ { KEY_0, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "switch-to-last-tab" },
		{ { KEY_LEFT, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "move-tab-left" },
		{ { KEY_RIGHT, KeyMod::getDefaultModifier() | KEYMOD_SHIFT }, "move-tab-right" },
		{ { KEY_L, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "split-right" },
		{ { KEY_K, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "split-bottom" },
		{ { KEY_J, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "split-left" },
		{ { KEY_I, KeyMod::getDefaultSecondaryModifier() | KEYMOD_SHIFT }, "split-top" },
		{ { KEY_J, KeyMod::getDefaultModifier() | KeyMod::getDefaultSecondaryModifier() },
		  "switch-to-previous-split" },
		{ { KEY_L, KeyMod::getDefaultModifier() | KeyMod::getDefaultSecondaryModifier() },
		  "switch-to-next-split" },
	};
}

void App::loadKeybindings() {
	IniFile ini( keybindingsPath );
	bool changed = false;
	std::string defaultModifier = ini.getValue( "modifier", "mod" );
	if ( defaultModifier.empty() ) {
		defaultModifier = KeyMod::getDefaultModifierString();
		ini.setValue( "modifier", "mod", defaultModifier );
		changed = true;
	}
	if ( const Uint32 modifier = KeyMod::getKeyMod( defaultModifier ); modifier != KEYMOD_NONE )
		KeyMod::setDefaultModifier( modifier );

	std::string defaultSecondaryModifier = ini.getValue( "modifier", "mod2" );
	if ( defaultSecondaryModifier.empty() ) {
		defaultSecondaryModifier = KeyMod::getDefaultSecondaryModifierString();
		ini.setValue( "modifier", "mod2", defaultSecondaryModifier );
		changed = true;
	}
	if ( const Uint32 modifier = KeyMod::getKeyMod( defaultSecondaryModifier );
		 modifier != KEYMOD_NONE && !( modifier & KeyMod::getDefaultModifier() ) ) {
		KeyMod::setDefaultSecondaryModifier( modifier );
	} else {
		defaultSecondaryModifier = KeyMod::getDefaultSecondaryModifierString();
		ini.setValue( "modifier", "mod2", defaultSecondaryModifier );
		changed = true;
	}

	KeyBindings formatter( appWindow->getInput() );
	auto defaults = getDefaultKeybindings();
	const bool hasKeybindings = ini.findKey( "eterm" ) != IniFile::noID;
	if ( hasKeybindings ) {
		keybindings = ini.getKeyUnorderedMap( "eterm" );
	} else {
		keybindings.clear();
		for ( const auto& shortcut : KeyBindings::getOrderedShortcuts( defaults ) ) {
			const auto& command = defaults.find( shortcut )->second;
			const std::string shortcutString = formatter.getShortcutString( shortcut );
			keybindings[shortcutString] = command;
			ini.setValue( "eterm", shortcutString, command );
		}
		changed = true;
	}

	if ( hasKeybindings ) {
		std::unordered_map<std::string, std::string> commands;
		for ( const auto& [shortcut, command] : keybindings )
			commands[command] = shortcut;
		for ( const auto& shortcut : KeyBindings::getOrderedShortcuts( defaults ) ) {
			const auto& command = defaults.find( shortcut )->second;
			const std::string shortcutString = formatter.getShortcutString( shortcut );
			if ( commands.find( command ) == commands.end() &&
				 keybindings.find( shortcutString ) == keybindings.end() ) {
				keybindings[shortcutString] = command;
				commands[command] = shortcutString;
				ini.setValue( "eterm", shortcutString, command );
				changed = true;
			}
		}
	}
	if ( changed )
		ini.writeFile();
}

void App::applyKeybindings( UITerminal* terminal ) {
	terminal->getKeyBindings().reset();
	terminal->getKeyBindings().addKeybindsStringUnordered( keybindings );
}

void App::applyKeybindings( UICodeEditor* editor ) {
	editor->getKeyBindings().setKeybinds( *UICodeEditor::getDefaultKeybindings() );
	editor->addKeyBinding( { KEY_S, KeyMod::getDefaultModifier() }, "save-doc", true );
	editor->getKeyBindings().addKeybindsStringUnordered( keybindings );
}

void App::reloadKeybindings() {
	if ( !FileSystem::fileExists( keybindingsPath ) )
		return;
	keybindings.clear();
	loadKeybindings();
	forEachTerminal( [this]( UITerminal* terminal ) { applyKeybindings( terminal ); } );
	if ( keybindingsEditor && tabSplitter->ownedWidgetExists( keybindingsEditor ) )
		applyKeybindings( keybindingsEditor );
}

void App::openKeybindings() {
	if ( keybindingsEditor && tabSplitter->ownedWidgetExists( keybindingsEditor ) ) {
		if ( auto* tabs = tabSplitter->tabWidgetFromWidget( keybindingsEditor ) )
			tabs->setTabSelected( tabs->getTabFromOwnedWidget( keybindingsEditor ) );
		keybindingsEditor->setFocus();
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
		if ( keybindingsEditor == editor )
			keybindingsEditor = nullptr;
	} );

	auto* current = tabSplitter->getCurWidget();
	auto* target =
		current ? tabSplitter->tabWidgetFromWidget( current ) : tabSplitter->getFirstTabWidget();
	keybindingsEditor = editor;
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
