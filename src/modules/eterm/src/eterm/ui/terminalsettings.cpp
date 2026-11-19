#include <eterm/ui/terminalsettings.hpp>

#include <eepp/scene/mainthreadlifetime.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/tools/uisettingspanel.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uifiledialog.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eterm/ui/programstatusui.hpp>
#include <eterm/ui/terminalpreferences.hpp>
#include <eterm/ui/uiterminal.hpp>
#include <limits>

namespace eterm::UI {

using namespace EE;
using namespace EE::Scene;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Tools;
using namespace eterm::Terminal;

TerminalSettings::TerminalSettings( UISettingsPanel* panel, VisitTerminals visit, Save save ) :
	mPanel( panel ),
	mScene( panel ? panel->getUISceneNode() : nullptr ),
	mActions(
		std::make_shared<const Actions>( Actions{ std::move( visit ), std::move( save ) } ) ) {}

void TerminalSettings::addBehavior( TerminalPreferences& preferences ) {
	if ( !mScene )
		return;

	auto* scene = mScene;
	const auto add = [&]( const char* id, const String& name, const String& description,
						  bool& value, void ( TerminalDisplay::*apply )( bool ) ) {
		mPanel->addBool( { id, "terminal.behavior", name, description }, &value,
						 [actions = mActions, apply]( bool enabled ) {
							 actions->visit( [apply, enabled]( UITerminal* terminal ) {
								 ( terminal->getTerm().get()->*apply )( enabled );
							 } );
							 actions->save();
						 } );
	};
	add( "enableOsc8Hyperlinks",
		 scene->i18n( "enable_osc8_hyperlinks", "Enable Hyperlinks (OSC 8)" ),
		 scene->i18n( "enable_osc8_hyperlinks_desc",
					  "Allow terminal programs to attach clickable links to text." ),
		 preferences.enableOsc8Hyperlinks, &TerminalDisplay::setOsc8HyperlinksEnabled );
	add( "detectLinks", scene->i18n( "detect_links", "Detect Links and Paths" ),
		 scene->i18n( "detect_links_desc",
					  "Underline URLs and file paths in the text while the default modifier is "
					  "held, and open them on click." ),
		 preferences.detectLinks, &TerminalDisplay::setLinkDetectionEnabled );
	add( "graphemeClustering", scene->i18n( "grapheme_clustering", "Grapheme Clustering" ),
		 scene->i18n( "grapheme_clustering_desc",
					  "Join emoji sequences, flags and combining characters into single "
					  "characters (DEC mode 2027)." ),
		 preferences.graphemeClustering, &TerminalDisplay::setGraphemeClusteringEnabled );
	add( "allowOsc52ClipboardWrite",
		 scene->i18n( "allow_osc52_clipboard_write",
					  "Allow Programs to Write to Clipboard (OSC 52)" ),
		 scene->i18n( "allow_osc52_clipboard_write_desc",
					  "Allow terminal programs, including programs over SSH, to change the local "
					  "clipboard." ),
		 preferences.allowOsc52ClipboardWrite, &TerminalDisplay::setAllowOsc52ClipboardWrite );
	mPanel->addInteger(
		{ "terminalScrollback", "terminal.behavior",
		  scene->i18n( "terminal_scrollback", "Terminal Scrollback" ),
		  scene->i18n( "configure_terminal_scrollback_desc",
					   "Set how many history lines new terminals retain." ) },
		0, std::numeric_limits<int>::max(),
		[&preferences] {
			return static_cast<int>(
				std::min<Uint64>( preferences.scrollback, std::numeric_limits<int>::max() ) );
		},
		[&preferences, actions = mActions]( int lines ) {
			preferences.scrollback = lines;
			actions->save();
		} );
}

void TerminalSettings::addExclusiveMode( bool& enabled ) {
	if ( !mScene )
		return;

	mPanel->addBool(
		{ "terminalExclusiveMode", "terminal.behavior",
		  mScene->i18n( "enable_exclusive_mode_by_default", "Enable Exclusive Mode by Default" ),
		  mScene->i18n( "terminal_exclusive_mode_desc",
						"Disable global keybindings in open and newly created terminals." ) },
		&enabled, [actions = mActions]( bool value ) {
			actions->visit(
				[value]( UITerminal* terminal ) { terminal->setExclusiveMode( value ); } );
			actions->save();
		} );
}

void TerminalSettings::addShell( std::string& shell, std::string& arguments, Save applyShell,
								 Save applyArguments ) {
	if ( !mScene )
		return;

	static constexpr const char* knownShells[] = { "bash", "sh",  "zsh",		"fish",
												   "nu",   "csh", "tcsh",		"ksh",
												   "dash", "cmd", "powershell", "pwsh" };
	const auto defaultShell = mScene->i18n( "system_default", "System Default" );
	std::vector<String> shells{ defaultShell };
	for ( const auto* name : knownShells ) {
		auto path = Sys::which( name );
		if ( !path.empty() )
			shells.emplace_back( path );
	}
	if ( !shell.empty() &&
		 std::find( shells.begin(), shells.end(), String( shell ) ) == shells.end() )
		shells.emplace_back( shell );
	mPanel->addEditableChoice(
		{ "terminalShell", "terminal.behavior", mScene->i18n( "terminal_shell", "Terminal Shell" ),
		  mScene->i18n( "terminal_shell_desc",
						"Set the shell executable used by new terminals." ) },
		std::move( shells ),
		[&shell, defaultShell] { return shell.empty() ? defaultShell : String( shell ); },
		[&shell, defaultShell, actions = mActions,
		 apply = std::move( applyShell )]( const String& choice ) {
			auto path = choice == defaultShell ? std::string{} : choice.toUtf8();
			if ( path == shell )
				return true;
			if ( !path.empty() && Sys::which( path ).empty() && !FileSystem::fileExists( path ) )
				return false;
			shell = std::move( path );
			if ( apply )
				apply();
			actions->save();
			return true;
		} );
	mPanel->addText(
		{ "terminalShellArguments", "terminal.behavior",
		  mScene->i18n( "terminal_shell_arguments", "Terminal Shell Arguments" ),
		  mScene->i18n( "terminal_shell_arguments_desc",
						"Set shell arguments used by new terminals." ) },
		[&arguments] { return arguments; },
		[&arguments, actions = mActions,
		 apply = std::move( applyArguments )]( const std::string& value ) {
			if ( arguments != value ) {
				arguments = value;
				if ( apply )
					apply();
				actions->save();
			}
			return true;
		},
		true );
}

void TerminalSettings::addWorkingDirectory( WorkingDirectoryOptions options,
											std::function<bool( size_t, std::string )> change ) {
	if ( !mScene )
		return;

	options.contextChoices.emplace_back( mScene->i18n( "user_home_directory", "User Home" ) );
	options.contextChoices.emplace_back( mScene->i18n( "other_directory", "Other" ) );
	mPanel->addCustomWidget(
		{ "terminalWorkingDirectory", "terminal.behavior",
		  mScene->i18n( "default_working_directory", "Default Working Directory" ),
		  mScene->i18n(
			  "terminal_default_directory_desc",
			  "Choose where new terminals start when they do not inherit a working directory." ) },
		[options = std::move( options ), change = std::move( change )]( UIWidget* parent ) {
			static constexpr auto layoutXml = R"xml(
				<vbox lw="mp" lh="wc">
					<DropDownList id="working_dir_combo" lw="mp" lh="wc" popup-to-root="true" />
					<hbox id="other_dir_container" lw="mp" lh="wc" margin-top="4dp">
						<TextInput id="other_dir_input" lw="0" lw8="1" lh="wc"
							hint="@string(other_working_directory, Other Working Directory)" />
						<PushButton id="other_dir_browse" lw="wc" lh="mp" margin-left="4dp"
							text="@string(browse, Browse…)" />
					</hbox>
				</vbox>
			)xml";
			auto* scene = parent->getUISceneNode();
			auto* layout = scene->loadLayoutFromString( layoutXml, parent );
			auto* mode = layout->find<UIDropDownList>( "working_dir_combo" );
			auto* custom = layout->find<UIWidget>( "other_dir_container" );
			auto* path = custom->find<UITextInput>( "other_dir_input" );
			const size_t other = options.contextChoices.size() - 1;
			mode->getListBox()->addListBoxItems( options.contextChoices );
			mode->getListBox()->setSelected( options.selected );
			custom->setVisible( options.selected == other );
			path->setText( String::fromUtf8( options.path ) );

			const auto commit = [mode, path, change] {
				if ( change( mode->getListBox()->getItemSelectedIndex(),
							 path->getText().toUtf8() ) )
					path->removeClass( "error" );
				else
					path->addClass( "error" );
			};
			const auto debounceTag = reinterpret_cast<Action::UniqueID>( path );
			const auto flush = [path, commit, debounceTag]( const Event* ) {
				path->removeActionsByTag( debounceTag );
				commit();
			};
			mode->on( Event::OnValueChange, [mode, custom, other, flush]( const Event* event ) {
				custom->setVisible( mode->getListBox()->getItemSelectedIndex() == other );
				flush( event );
			} );
			if ( options.commitPathOnFocusLoss ) {
				path->on( Event::OnTextChanged, [path, commit, debounceTag]( const Event* ) {
					path->debounce( commit, Milliseconds( 500 ), debounceTag );
				} );
				path->on( Event::OnPressEnter, flush );
				path->on( Event::OnFocusLoss, flush );
			} else {
				path->on( Event::OnTextChanged, [commit]( const Event* ) { commit(); } );
			}

			// The Browse button owns the connection so a native picker can outlive the settings.
			auto* owner = parent->getParentOfType( UI_TYPE_WINDOW );
			custom->find<UIPushButton>( "other_dir_browse" )
				->onClick( [scene, owner, path, browseFrom = options.browseFrom,
							flags = options.dialogFlags,
							singleClick = options.singleClickNavigation,
							result = std::make_shared<EventConnection>()]( const Event* ) {
					auto folder = FileSystem::expandTilde( path->getText().toUtf8() );
					if ( !FileSystem::isDirectory( folder ) )
						folder = browseFrom ? browseFrom() : Sys::getUserDirectory();
					auto* dialog = UIFileDialog::New( UIFileDialog::DefaultFlags |
														  UIFileDialog::AllowFolderSelect |
														  UIFileDialog::ShowOnlyFolders | flags,
													  "*", folder );
					dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON |
											UI_WIN_MODAL );
					dialog->setTitle(
						scene->i18n( "select_working_directory", "Select Working Directory" ) );
					dialog->setCloseShortcut( KEY_ESCAPE );
					dialog->setSingleClickNavigation( singleClick );
					*result =
						dialog->connect( Event::OpenFile, [owner, path, dialog]( const Event* ) {
							if ( !owner || !owner->isClosing() )
								path->setText( String::fromUtf8( dialog->getFullPath() ) );
						} );
					dialog->center();
					dialog->show();
				} );
			return layout;
		} );
}

void TerminalSettings::addAppearance( TerminalPreferences& preferences ) {
	if ( !mScene )
		return;

	auto* scene = mScene;
	static constexpr TerminalCursorMode cursorModes[] = {
		TerminalCursorMode::BlinkingBlock,	TerminalCursorMode::SteadyBlock,
		TerminalCursorMode::BlinkUnderline, TerminalCursorMode::SteadyUnderline,
		TerminalCursorMode::BlinkBar,		TerminalCursorMode::SteadyBar };
	mPanel->addChoice(
		{ "terminalCursorStyle", "terminal.appearance",
		  scene->i18n( "cursor_style", "Cursor Style" ),
		  scene->i18n( "cursor_style_desc", "Choose the terminal cursor shape and animation." ) },
		{ scene->i18n( "blinking_block", "Blinking Block" ),
		  scene->i18n( "steady_block", "Steady Block" ),
		  scene->i18n( "blink_underline", "Blink Underline" ),
		  scene->i18n( "steady_underline", "Steady Underline" ),
		  scene->i18n( "blink_bar", "Blink Bar" ), scene->i18n( "steady_bar", "Steady Bar" ) },
		[&preferences] {
			const auto found = std::find( std::begin( cursorModes ), std::end( cursorModes ),
										  preferences.cursorStyle );
			return found == std::end( cursorModes ) ? size_t{ 0 } : size_t( found - cursorModes );
		},
		[&preferences, actions = mActions]( size_t selected ) {
			preferences.cursorStyle =
				cursorModes[std::min( selected, std::size( cursorModes ) - 1 )];
			actions->visit( [mode = preferences.cursorStyle]( UITerminal* terminal ) {
				terminal->getTerm()->setCursorMode( mode );
			} );
			actions->save();
		} );
	mPanel->addChoice(
		{ "terminalScrollbarType", "terminal.appearance",
		  scene->i18n( "scrollbar_type", "Scrollbar Type" ),
		  scene->i18n( "scrollbar_type_desc",
					   "Place the terminal scrollbar over or outside its content." ) },
		{ scene->i18n( "overlay", "Overlay" ), scene->i18n( "outside", "Outside" ) },
		[&preferences] { return preferences.scrollBarType == ScrollViewType::Overlay ? 0 : 1; },
		[&preferences, actions = mActions]( size_t selected ) {
			preferences.scrollBarType =
				selected == 0 ? ScrollViewType::Overlay : ScrollViewType::Outside;
			actions->visit( [type = preferences.scrollBarType]( UITerminal* terminal ) {
				terminal->setScrollViewType( type );
			} );
			actions->save();
		},
		{ scene->i18n( "scroll_overlay_tooltip", "Scrollbar appears over content." ),
		  scene->i18n( "scroll_outside_tooltip",
					   "Scrollbar has its own space, never covers content." ) } );
	static constexpr ScrollBarMode scrollbarModes[] = {
		ScrollBarMode::Auto, ScrollBarMode::AlwaysOn, ScrollBarMode::AlwaysOff };
	mPanel->addChoice(
		{ "terminalScrollbarMode", "terminal.appearance",
		  scene->i18n( "scrollbar_mode", "Scrollbar Mode" ),
		  scene->i18n( "scrollbar_mode_desc", "Control when the terminal scrollbar is visible." ) },
		{ scene->i18n( "auto", "Auto" ), scene->i18n( "always_visible", "Always Visible" ),
		  scene->i18n( "always_hidden", "Always Hidden" ) },
		[&preferences] {
			const auto found = std::find( std::begin( scrollbarModes ), std::end( scrollbarModes ),
										  preferences.scrollBarMode );
			return found == std::end( scrollbarModes ) ? size_t{ 0 }
													   : size_t( found - scrollbarModes );
		},
		[&preferences, actions = mActions]( size_t selected ) {
			preferences.scrollBarMode =
				scrollbarModes[std::min( selected, std::size( scrollbarModes ) - 1 )];
			actions->visit( [mode = preferences.scrollBarMode]( UITerminal* terminal ) {
				terminal->setVerticalScrollMode( mode );
			} );
			actions->save();
		},
		{ scene->i18n( "scrollbar_mode_auto_tooltip",
					   "With an overlay scrollbar, show it when the mouse moves over the terminal. "
					   "With an outside scrollbar, show it when there is a scrollable area." ),
		  scene->i18n( "scrollbar_mode_always_visible_tooltip", "Keep the scrollbar visible." ),
		  scene->i18n( "scrollbar_mode_always_hidden_tooltip", "Keep the scrollbar hidden." ) } );
}

void TerminalSettings::addColorScheme( const std::map<std::string, TerminalColorScheme>& schemes,
									   std::function<const std::string&()> get,
									   std::function<void( const std::string& )> set,
									   bool includeDefault ) {
	if ( !mScene )
		return;

	if ( schemes.empty() && !includeDefault )
		return;
	auto ids = std::make_shared<std::vector<std::string>>();
	std::vector<String> labels;
	ids->reserve( schemes.size() + includeDefault );
	labels.reserve( schemes.size() + includeDefault );
	if ( includeDefault ) {
		ids->emplace_back();
		labels.emplace_back( mScene->i18n( "default", "Default" ) );
	}
	for ( const auto& [name, scheme] : schemes ) {
		ids->emplace_back( name );
		labels.emplace_back( name );
	}
	mPanel->addChoice(
		{ "terminalColorScheme", "terminal.appearance",
		  mScene->i18n( "terminal_color_scheme", "Terminal Color Scheme" ),
		  mScene->i18n( "terminal_color_scheme_desc", "Choose the colors used by terminals." ) },
		labels,
		[ids, get = std::move( get )] {
			const auto found = std::find( ids->begin(), ids->end(), get() );
			return found == ids->end() ? size_t{ 0 } : size_t( found - ids->begin() );
		},
		[ids, set = std::move( set ), actions = mActions]( size_t selected ) {
			set( ( *ids )[std::min( selected, ids->size() - 1 )] );
			actions->save();
		} );
}

void TerminalSettings::addFrameBuffer( bool& enabled ) {
	if ( !mScene )
		return;

	mPanel->addBool( { "terminalFrameBuffer", "terminal.behavior",
					   mScene->i18n( "framebuffer", "Use Framebuffer" ),
					   mScene->i18n( "framebuffer_desc",
									 "Use framebuffer rendering for newly opened terminals." ) },
					 &enabled, [actions = mActions]( bool ) { actions->save(); } );
}

void TerminalSettings::addProgramStatus( ProgramStatusPreferences& preferences,
										 ProgramStatusUI& status ) {
	if ( !mScene )
		return;

	auto& alerts = status.alerts();
	auto* panel = mPanel;
	auto* scene = mScene;
	panel->addBool(
		{ "showProgramStatus", "terminal.behavior",
		  scene->i18n( "show_program_status", "Show Program Status" ),
		  scene->i18n( "show_program_status_desc",
					   "Show in each tab whether its program is working, waiting for you, "
					   "finished or failed, when the program reports it." ) },
		&preferences.show, [actions = mActions, &status]( bool ) {
			actions->visit( [&status]( UITerminal* terminal ) { status.refresh( terminal ); } );
			actions->save();
		} );
	panel->addCategory( "terminal.alerts", scene->i18n( "terminal", "Terminal" ),
						scene->i18n( "program_status_alerts", "Program Status Alerts" ) );
	auto lifetime = std::make_shared<MainThreadLifetime<UISettingsPanel>>( panel, scene );
	panel->addBool(
		{ "programStatusNotifications", "terminal.alerts",
		  scene->i18n( "programStatusNotifications", "OS Notifications" ),
		  scene->i18n( "programStatusNotifications_desc",
					   "Send OS notifications when a program needs attention, finishes or "
					   "fails." ) },
		&preferences.notifications, [actions = mActions]( bool ) { actions->save(); } );
	panel->addAction(
		{ "testProgramStatusNotification", "terminal.alerts",
		  scene->i18n( "test_notification", "Test Notification" ),
		  scene->i18n(
			  "test_notification_desc",
			  "Send one OS notification, even when automatic notifications are disabled." ) },
		scene->i18n( "test", "Test" ),
		[panel, scene, &alerts, applicationName = status.applicationName(), lifetime] {
			const auto feedback = []( UISettingsPanel* panel, const String& text ) {
				if ( auto* row =
						 panel->find<UIWidget>( "setting_testProgramStatusNotification" ) ) {
					if ( auto* description = row->find<UITextView>( "setting_description" ) )
						description->setText( text );
				}
			};
			feedback( panel, scene->i18n( "sending_notification", "Sending test notification…" ) );
			const bool accepted = alerts.deliver(
				scene
					->i18n( "test_notification_title",
							String( applicationName ) + " · Test notification" )
					.toUtf8(),
				scene
					->i18n( "test_notification_message",
							String( "This is a test notification from " ) + applicationName + "." )
					.toUtf8(),
				true, false, {}, 0, true,
				[handle = lifetime->weakHandle(), feedback]( bool success, std::string error ) {
					handle.run(
						[feedback, success, error = std::move( error )]( UISettingsPanel* panel ) {
							const auto text = success ? panel->getUISceneNode()->i18n(
															"notification_submitted",
															"Notification submitted. If no banner "
															"appears, check OS notification "
															"permissions and Do Not Disturb." )
													  : String::fromUtf8( error );
							feedback( panel, text );
						} );
				} );
			if ( !accepted ) {
				feedback( panel,
						  scene->i18n( "notification_busy",
									   "Another alert is being delivered. Try again shortly." ) );
			}
		} );
	panel->addBool( { "programStatusSounds", "terminal.alerts",
					  scene->i18n( "programStatusSounds", "Play Sound" ),
					  scene->i18n( "programStatusSounds_desc",
								   "Play the selected sound on program status changes." ) },
					&preferences.sounds, [actions = mActions]( bool ) { actions->save(); } );
	panel->addBool( { "programStatusAlertsWhenActive", "terminal.alerts",
					  scene->i18n( "programStatusAlertsWhenActive", "Alert for Active Terminal" ),
					  scene->i18n( "programStatusAlertsWhenActive_desc",
								   "Also alert while you are using that terminal." ) },
					&preferences.whenActive, [actions = mActions]( bool ) { actions->save(); } );
	panel->addBool(
		{ "programStatusAlertsAllStates", "terminal.alerts",
		  scene->i18n( "programStatusAlertsAllStates", "Include Working and Idle" ),
		  scene->i18n( "programStatusAlertsAllStates_desc",
					   "Also alert when programs start working or become idle. Percentage "
					   "and text updates stay silent." ) },
		&preferences.allStates, [actions = mActions]( bool ) { actions->save(); } );
	panel->addCustomWidget(
		{ "programStatusSound", "terminal.alerts", scene->i18n( "alert_sound", "Sound" ),
		  scene->i18n( "alert_sound_desc",
					   "Choose a built-in sound or an MP3, WAV (PCM), Ogg/Vorbis or FLAC file." ) },
		[panel, scene, &preferences, actions = mActions,
		 weak = std::weak_ptr{ lifetime }]( UIWidget* parent ) {
			auto* layout = scene->loadLayoutFromString( R"xml(
				<vbox lw="mp" lh="wc">
					<hbox lw="mp" lh="wc">
						<DropDownList id="program_status_sound_choice" lw="0" lw8="1" lh="wc" />
						<PushButton id="program_status_sound_browse" lw="wc" lh="wc" margin-left="4dp" />
					</hbox>
					<TextView id="program_status_sound_path" lw="mp" lh="wc" word-wrap="true" />
				</vbox>
			)xml",
														parent );
			auto* choice = layout->find<UIDropDownList>( "program_status_sound_choice" );
			choice->getListBox()->addListBoxItems( { scene->i18n( "chime", "Chime" ),
													 scene->i18n( "bell", "Bell" ),
													 scene->i18n( "pulse", "Pulse" ) } );
			const auto sync = [panel, &preferences, weak] {
				if ( weak.expired() )
					return;
				auto* drop = panel->find<UIDropDownList>( "program_status_sound_choice" );
				auto* path = panel->find<UITextView>( "program_status_sound_path" );
				if ( !drop || !path )
					return;
				const auto& sound = preferences.sound;
				const Uint32 selected = sound == "chime"   ? 0
										: sound == "bell"  ? 1
										: sound == "pulse" ? 2
														   : 3;
				// This widget's data is a local guard against programmatic selection callbacks.
				drop->setData( 1 );
				if ( selected == 3 ) {
					const auto filename = String::fromUtf8( FileSystem::fileNameFromPath( sound ) );
					if ( drop->getListBox()->getItemsCount() == 3 )
						drop->getListBox()->addListBoxItem( filename );
					else
						drop->getListBox()->setItemText( 3, filename );
				} else if ( drop->getListBox()->getItemsCount() > 3 ) {
					drop->getListBox()->removeListBoxItem( 3 );
				}
				drop->getListBox()->setSelected( selected );
				drop->setData( 0 );
				path->setText( selected == 3 ? String::fromUtf8( sound ) : String{} );
				path->setTooltipText( path->getText() );
				path->setVisible( selected == 3 );
			};
			sync();
			const auto browse = [scene, &preferences, actions, weak, sync] {
				if ( scene->find<UIFileDialog>( "program_status_sound_dialog" ) )
					return;
				const auto& sound = preferences.sound;
				const std::string directory = FileSystem::fileExists( sound )
												  ? FileSystem::fileRemoveFileName( sound )
												  : Sys::getUserDirectory();
				auto* dialog = UIFileDialog::New( UIFileDialog::DefaultFlags,
												  "*.mp3;*.wav;*.ogg;*.flac", directory );
				dialog->setId( "program_status_sound_dialog" );
				dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON |
										UI_WIN_MODAL );
				dialog->setTitle( scene->i18n( "select_alert_sound", "Select Alert Sound" ) );
				dialog->setCloseShortcut( KEY_ESCAPE );
				dialog->on(
					Event::OpenFile, [&preferences, actions, weak, sync]( const Event* event ) {
						if ( weak.expired() )
							return;
						preferences.sound = event->getNode()->asType<UIFileDialog>()->getFullPath();
						actions->save();
						sync();
					} );
				dialog->on( Event::OnWindowClose, [sync]( const Event* ) { sync(); } );
				dialog->center();
				dialog->show();
			};
			auto* browseButton = layout->find<UIPushButton>( "program_status_sound_browse" );
			browseButton->setText( scene->i18n( "browse", "Browse…" ) );
			browseButton->onClick( [browse]( const Event* ) { browse(); } );
			choice->on(
				Event::OnValueChange, [&preferences, actions, choice, sync]( const Event* ) {
					if ( choice->getData() )
						return;
					const auto selected = choice->getListBox()->getItemSelectedIndex();
					if ( selected < 3 ) {
						static constexpr const char* sounds[] = { "chime", "bell", "pulse" };
						preferences.sound = sounds[selected];
						actions->save();
						sync();
					}
				} );
			return layout;
		} );
	panel->addInteger(
		{ "programStatusSoundVolume", "terminal.alerts",
		  scene->i18n( "alert_volume", "Sound Volume" ),
		  scene->i18n( "alert_volume_desc", "Volume from 0 to 100." ) },
		0, 100, [&preferences] { return preferences.volume; },
		[&preferences, actions = mActions]( int value ) {
			preferences.volume = value;
			actions->save();
		} );
	panel->addAction(
		{ "previewProgramStatusSound", "terminal.alerts",
		  scene->i18n( "preview_sound", "Preview Sound" ),
		  scene->i18n( "preview_sound_desc",
					   "Play the selected sound once, even when sound alerts are disabled." ) },
		scene->i18n( "play", "Play" ), [&alerts, &preferences] {
			alerts.deliver( {}, {}, false, true, preferences.sound, preferences.volume, true );
		} );
}

} // namespace eterm::UI
