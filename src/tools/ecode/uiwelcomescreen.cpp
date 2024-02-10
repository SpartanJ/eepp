#include "uiwelcomescreen.hpp"
#include "ecode.hpp"
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/window.hpp>

namespace ecode {

static const auto LAYOUT = R"xml(
<style>
<![CDATA[
#welcome_ecode TextView {
	focusable: false;
	color: var(--font-hint);
}
#welcome_ecode TextView.anchor {
	color: var(--primary);
}
#welcome_ecode TextView.anchor:hover {
	color: var(--font);
	cursor: hand;
}
#home_logo {
	focusable: false;
	background-image: icon(ecode,256dp);
	background-position: center center;
	background-tint: var(--font-hint);
	background-size: 100% 100%;
}
#home_logo:hover {
	background-tint: var(--primary);
}
#home_title {
	font-size: 24dp;
	color: var(--font-hint);
	font-family: DejaVuSansMono;
	font-style: bold|shadow;
	text-shadow-color: rgba(0,0,0,0.4);
}
#welcome_ecode PushButton {
	min-width: 128dp;
	background-color: var(--list-back);
}
#welcome_ecode PushButton:hover {
	border-color: var(--primary);
}
#welcome_ecode .separator {
	focusable: false;
	background-image: rectangle(solid, var(--tab-line));
	background-size: 80% 1px;
	background-position: center;
}
#welcome_ecode LinearLayout.shortcut {
	padding-top: 2dp;
	padding-bottom: 2dp;
}
#welcome_ecode TextView.name {
	padding: 2dp;
}
#welcome_ecode TextView.shortcut {
	background-color: var(--list-back);
	padding: 2dp;
	border-width: 1dp;
	border-type: inside;
	border-color: var(--button-border);
	border-radius: 4dp;
}
#welcome_ecode .bold {
	font-size: 13dp;
}
#welcome_ecode .right > PushButton {
	margin-bottom: 6dp;
	layout-gravity: center;
}
#welcome_ecode .right > PushButton:last-of-type {
	margin-bottom: 0dp;
}
]]>
</style>
<RelativeLayout lw="mp" lh="mp" max-width="512dp" layout_gravity="center">
	<hbox lw="mp" lh="mp">
		<vbox class="left" lw="0" lh="wc" lw8="0.5" lg="center">
			<hbox lw="wc" lh="wc" lg="center">
				<image id="home_logo" lw="wc" min-width="128dp" lh="128dp" lg="center" />
				<tv id="home_title" text="ecode" lg="center" />
			</hbox>
			<tv class="bold" text="Shortcuts" lg="center" margin-top="16dp" />
			<vbox lw="mp" lh="wc" lg="center">
				<hbox class="shortcut" lg="center">
					<tv class="name" id="command_palette" text="@string(command_palette, Command Palette)" />
					<tv class="shortcut" id="command_palette_shortcut" text="Ctrl + P" />
				</hbox>
				<hbox class="shortcut" lg="center">
					<tv class="name" id="locate" text="@string(locate_files_and_symbols, Locate Files & Symbols)" />
					<tv class="shortcut" id="locate_shortcut" text="Ctrl + K" />
				</hbox>
				<hbox class="shortcut" lg="center">
					<tv class="name" text="@string(global_search, Global Search)" id="open-global-search" />
					<tv class="shortcut" text="Ctrl + Shift + F" id="global_search_shortcut" />
				</hbox>
				<hbox class="shortcut" lg="center">
					<tv class="name" text="@string(open_folder, Open Folder)" id="open_folder" />
					<tv class="shortcut" text="Ctrl + Shift + O" id="open_folder_shortcut" />
				</hbox>
				<hbox class="shortcut" lg="center">
					<tv class="name" id="open_file" text="@string(open_file, Open File)" />
					<tv class="shortcut" id="open_file_shortcut" text="Ctrl + O" />
				</hbox>
			</vbox>
		</vbox>
		<vbox class="right" lw="0" lh="wc" lw8="0.5" lg="center">
			<button id="open-folder" text="@string(open_a_folder, Open a Folder)" />
			<button id="open-file" text="@string(open_a_file, Open a File)" />
			<button id="create-new-terminal" text="@string(new_terminal, New Terminal)" />
			<button id="recent-folders" text="@string(recent_folders_ellipsis, Recent Folders...)" />
			<button id="recent-files" text="@string(recent_files_ellipsis, Recent Files...)" />
			<button id="plugin-manager-open" text="@string(plugin_manager, Plugin Manager)" />
			<button id="keybindings" text="@string(keybindings, Keybindings)" />
			<widget class="separator" lw="mp" lh="32dp" />
			<tv class="bold" text="For help, please visit:" lg="center" />
			<vbox lw="wc" lh="wc" lg="center">
				<hbox>
					<tv text="The ecode " />
					<a id="home_doc" text="documentation" href="https://github.com/SpartanJ/ecode" />
				</hbox>
				<hbox>
					<tv text="The ecode " />
					<a id="home_forum" text="forum" href="https://github.com/SpartanJ/ecode/discussions" />
				</hbox>
				<hbox>
					<tv text="The ecode " />
					<a id="home_issues" text="issues" href="https://github.com/SpartanJ/ecode/issues" />
				</hbox>
				<hbox>
					<a id="check-for-updates" text="Check for Updates" />
				</hbox>
			</vbox>
		</vbox>
	</hbox>
</RelativeLayout>
)xml";

UIWelcomeScreen* UIWelcomeScreen::createWelcomeScreen( App* app, UITabWidget* inTabWidget ) {
	UITabWidget* tabWidget = nullptr;

	if ( !inTabWidget ) {
		UIWidget* curWidget = app->getSplitter()->getCurWidget();
		if ( !curWidget )
			return nullptr;
		tabWidget = app->getSplitter()->tabWidgetFromWidget( curWidget );
	} else {
		tabWidget = inTabWidget;
	}

	if ( !tabWidget ) {
		if ( !app->getSplitter()->getTabWidgets().empty() ) {
			tabWidget = app->getSplitter()->getTabWidgets()[0];
		} else {
			return nullptr;
		}
	}

	UIWelcomeScreen* welcomeScreen = UIWelcomeScreen::New( app );
	auto ret = app->getSplitter()->createWidgetInTabWidget(
		tabWidget, welcomeScreen, app->i18n( "welcome", "Welcome" ), true );
	app->getSplitter()->removeUnusedTab( tabWidget, true, false );
	ret.first->setIcon( app->findIcon( "ecode", PixelDensity::dpToPxI( 16 ) ) );
	return welcomeScreen;
}

UIWelcomeScreen* UIWelcomeScreen::New( App* app ) {
	return eeNew( UIWelcomeScreen, ( app ) );
}

UIWelcomeScreen::UIWelcomeScreen( App* app ) :
	UIRelativeLayout(),
	WidgetCommandExecuter( getUISceneNode()->getWindow()->getInput() ),
	mApp( app ) {
	setId( "welcome_ecode" );
	setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	app->registerUnlockedCommands( *this );
	getUISceneNode()->loadLayoutFromString( LAYOUT, this, String::hash( "UIWelcomeScreen" ) );

	auto bindBtn = [this]( const std::string& id ) {
		UIWidget* node = find<UIWidget>( id );
		if ( !node )
			return;
		node->setTooltipText( getKeyBindings().getCommandKeybindString( id ) );
		node->onClick(
			[this]( const MouseEvent* event ) { mApp->runCommand( event->getNode()->getId() ); },
			EE_BUTTON_LEFT );
	};

	auto bindBtns = [bindBtn]( const std::initializer_list<std::string> ids ) {
		for ( const auto& id : ids )
			bindBtn( id );
	};

	bindBtns( { "open-folder", "open-file", "create-new-terminal", "check-for-updates",
				"plugin-manager-open", "keybindings" } );

	auto recentFolders = find( "recent-folders" );
	if ( !mApp->getRecentFolders().empty() ) {
		recentFolders->onClick(
			[this]( const MouseEvent* event ) {
				mApp->createAndShowRecentFolderPopUpMenu( event->getNode() );
			},
			EE_BUTTON_LEFT );
	} else {
		recentFolders->setEnabled( false );
	}

	auto recentFiles = find( "recent-files" );
	if ( !mApp->getRecentFiles().empty() ) {
		recentFiles->onClick(
			[this]( const MouseEvent* event ) {
				mApp->createAndShowRecentFilesPopUpMenu( event->getNode() );
			},
			EE_BUTTON_LEFT );
	} else {
		recentFiles->setEnabled( false );
	}

	find<UITextView>( "command_palette_shortcut" )
		->setText( mApp->getKeybind( "open-command-palette" ) );

	find<UITextView>( "locate_shortcut" )->setText( mApp->getKeybind( "open-locatebar" ) );

	find<UITextView>( "global_search_shortcut" )
		->setText( mApp->getKeybind( "open-global-search" ) );

	find<UITextView>( "open_folder_shortcut" )->setText( mApp->getKeybind( "open-folder" ) );

	find<UITextView>( "open_file_shortcut" )->setText( mApp->getKeybind( "open-file" ) );
}

} // namespace ecode
