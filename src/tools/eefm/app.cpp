#include "app.hpp"
#include "icons.hpp"
#include "mime.hpp"

#include <args/args.hxx>
#include <iostream>
#ifdef EE_VFS_SFTP
#include <eepp/vfs/sftp/sftpprovider.hpp>
#endif

namespace eefm {

namespace {

const char* Layout = R"xml(
<style>
<![CDATA[
#toolbar {
	padding: 3dp 4dp;
	background-color: var(--back);
}
PushButton.tool, SelectButton.tool {
	padding: 4dp;
	margin-right: 1dp;
	background-color: transparent;
	border-color: transparent;
	icon-size: 16dp;
}
PushButton.tool:hover, SelectButton.tool:hover {
	background-color: var(--item-hover);
}
SelectButton.tool:selected {
	background-color: var(--list-back);
	border-color: transparent;
}
.location {
	margin-left: 4dp;
	margin-right: 2dp;
}
.location_frame {
	border-width: 1dp;
	border-color: var(--button-border);
	border-radius: 2dp;
	background-color: var(--list-back);
	padding-left: 4dp;
}
/* The frame draws the focus border; the input itself has none. */
.location_frame:focus-within {
	border-color: var(--primary);
}
.location_input, .location_input:focus {
	background-color: transparent;
	border-color: transparent;
	border-width: 0dp;
	padding: 3dp 2dp;
}
/* The active pane is marked on its current tab, like Dolphin. */
.pane.active Tab:selected {
	border-top-color: var(--primary);
	border-top-width: 2dp;
}
.location_frame PushButton {
	background-color: transparent;
	border-color: transparent;
	padding: 2dp 4dp;
	icon-size: 14dp;
}
.location_icon {
	margin-right: 2dp;
	tint: var(--icon);
	min-width: 16dp;
	min-height: 16dp;
}
/* Glyph icons are drawn at their tight bounds, so their sizes differ: every icon gets a fixed
   16dp box (menus included, with or without an icon) and the glyph is centered in it at its
   natural size (fitting would scale each glyph differently). */
PopUpMenu {
	min-icon-size: 16dp 16dp;
}
/* The check box is 12dp (theme): centered in the same 16dp column as the icons. */
PopUpMenu Menu::CheckBox::icon {
	margin-left: 2dp;
	margin-right: 2dp;
	margin-inline-end: 6dp;
}
tableview::cell::icon,
Menu::Item::icon,
Menu::SubMenu::icon,
Menu::CheckBox::icon,
Menu::RadioButton::icon,
tab::icon,
.place_icon,
.location_icon {
	gravity: center;
	/* Images take text-align as their horizontal alignment, and cells set it (inherited). */
	text-align: center;
	scale-type: none;
}
#places_scroll {
	background-color: var(--back);
}
.places_header {
	color: var(--font-hint);
	padding: 8dp 6dp 2dp 6dp;
}
.place {
	padding: 3dp 6dp 3dp 6dp;
	opacity: 1;
	border-width: 0dp;
	border-color: transparent;
	/* Explicit: the hover/selected/drop backgrounds must be reverted when they stop matching. */
	background-color: transparent;
}
#place_dialog .place_hint {
	color: var(--font-hint);
	margin-top: 2dp;
}
#place_dialog .place_icon_choice {
	padding: 4dp;
	margin-right: 1dp;
	background-color: transparent;
	border-color: transparent;
	icon-size: 16dp;
}
#place_dialog .place_icon_choice:hover {
	background-color: var(--item-hover);
}
#place_dialog .place_icon_choice:selected {
	background-color: var(--primary);
}
.hidden_place, .place.dragged {
	opacity: 0.5;
}
/* Where a dragged place (or folder) would be inserted. */
.place.drop_before {
	border-top-width: 2dp;
	border-top-color: var(--primary);
}
.place.drop_after {
	border-bottom-width: 2dp;
	border-bottom-color: var(--primary);
}
.place:hover {
	background-color: #ffffff10;
}
.place.selected {
	background-color: var(--item-hover);
}
.place_icon {
	margin-right: 6dp;
	tint: var(--icon);
	min-width: 16dp;
	min-height: 16dp;
}
.device_usage {
	min-height: 3dp;
	max-height: 3dp;
	margin: 2dp 0dp 0dp 22dp;
	border-width: 0dp;
	background-color: var(--list-back-alt);
}
.device_usage ProgressBar::filler {
	background-color: var(--primary);
	border-radius: 0dp;
}
.device_usage.full ProgressBar::filler {
	background-color: var(--theme-error);
}
.place.droppable-hovering, placerow:droppable-hovering {
	background-color: var(--primary);
}
.pane_status_bar {
	padding: 3dp 4dp;
	/* Same as the table above it, so the status pill sits inside the view (Dolphin). */
	background-color: var(--list-back);
}
.pane_status {
	border-width: 1dp;
	border-color: var(--tab-line);
	border-radius: 3dp;
	padding: 2dp 6dp;
}
.pane_filter_bar {
	padding: 2dp 4dp 4dp 4dp;
}
.pane_filter_bar PushButton, .pane_filter_bar SelectButton {
	background-color: transparent;
	border-color: transparent;
	padding: 3dp 5dp;
	icon-size: 14dp;
}
.pane_filter_bar SelectButton:selected {
	background-color: var(--item-hover);
}
.filter_input {
	margin: 0dp 4dp;
}
.filter_input.invalid {
	border-color: var(--theme-error);
}
.filter_mode {
	margin-right: 4dp;
}
.new_tab {
	background-color: transparent;
	border-color: transparent;
	padding: 2dp 6dp;
	icon-size: 14dp;
}
.pane:not(.active) tableview::row:selected {
	opacity: 0.6;
}
.transfer_text {
	margin-right: 6dp;
}
.transfer_bar {
	min-width: 120dp;
	margin-right: 4dp;
}
.file_drag_label {
	background-color: var(--menu-back);
	border-width: 1dp;
	border-color: var(--menu-border);
	border-radius: 3dp;
	padding: 3dp 8dp;
}
/* Every cell and row gets explicit values for what the rules below change: a property no rule
   sets anymore keeps its last value (recycled cells change class, drags end, panes go
   inactive). */
tableview::cell {
	text-align: left;
	opacity: 1;
}
tableview::row {
	opacity: 1;
}
/* Alternating rows, as eproc and Dolphin. */
tableview::row:nth-child(even) {
	background-color: var(--list-back-alt);
}
tableview::row:nth-child(even):hover {
	background-color: var(--item-hover);
}
tableview::row:nth-child(even):selected {
	background-color: var(--primary);
}
/* Secondary columns are dimmed, like the places headers (selected rows keep the theme's
   selected text color: that rule is more specific). */
tableview::cell.vfs_size,
tableview::cell.vfs_modified,
tableview::cell.vfs_type,
tableview::cell.vfs_permissions,
tableview::cell.vfs_owner,
tableview::cell.vfs_group {
	color: var(--font-hint);
}
tableview::cell.vfs_size {
	text-align: right;
}
tableview::cell.dragged {
	opacity: 0.5;
}
]]>
</style>
<vbox id="root" layout_width="match_parent" layout_height="match_parent">
	<hbox id="toolbar" layout_width="match_parent" layout_height="wrap_content">
		<PushButton id="back" class="tool" icon="go-previous" tooltip="Back (Alt+Left)" />
		<PushButton id="forward" class="tool" icon="go-next" tooltip="Forward (Alt+Right)" />
		<PushButton id="up" class="tool" icon="go-parent" tooltip="Go Up (Alt+Up)" />
		<PushButton id="refresh" class="tool" icon="view-refresh" tooltip="Reload (F5)" />
		<SelectButton id="places_toggle" class="tool" icon="places-panel" tooltip="Show Places Panel (F9)" />
		<SelectButton id="split" class="tool" icon="view-split" tooltip="Split View (F3)" />
		<hbox id="location_0" class="location" layout_width="200dp" layout_height="wrap_content" />
		<hbox id="location_1" class="location" layout_width="200dp" layout_height="wrap_content" />
		<widget layout_width="0" layout_weight="1" layout_height="1dp" />
		<PushButton id="app_menu" class="tool" icon="app-menu" tooltip="Menu" />
	</hbox>
	<Splitter id="terminal_splitter" layout_width="match_parent" layout_height="0" layout_weight="1" orientation="vertical" splitter-partition="70%">
	<Splitter id="main_splitter" layout_width="match_parent" layout_height="match_parent" orientation="horizontal" splitter-partition="185dp">
		<ScrollView id="places_scroll" layout_width="0" layout_height="match_parent" min-width="120dp" />
		<Splitter id="pane_splitter" layout_width="0" layout_height="match_parent" orientation="horizontal" splitter-partition="50%" />
	</Splitter>
	</Splitter>
	<widget id="parking" layout_width="0" layout_height="0" visible="false" enabled="false" />
</vbox>
)xml";

const char* LocationLayout = R"xml(
<hbox class="location_frame" layout_width="0" layout_weight="1" layout_height="wrap_content">
	<Image class="location_icon" layout_gravity="center_vertical" />
	<TextInput class="location_input" layout_width="0" layout_weight="1" layout_height="wrap_content" hint="Location" />
	<PushButton class="location_clear" icon="location-clear" tooltip="Clear" layout_gravity="center_vertical" />
	<PushButton class="location_history" icon="location-history" tooltip="Recent Locations" layout_gravity="center_vertical" />
</hbox>
)xml";

std::string sortOrderName( SortOrder order ) {
	return order == SortOrder::Descending ? "descending" : "ascending";
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Startup and shutdown
// ---------------------------------------------------------------------------------------------

int App::run( int argc, char* argv[] ) {
	args::ArgumentParser parser( "eefm - file manager" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );
	args::Flag split( parser, "split", "Open the locations side by side", { 's', "split" } );
	args::PositionalList<std::string> locations(
		parser, "locations", "Paths or URIs to open (one tab each, or one per side with --split)" );
	try {
		parser.ParseCLI( argc, argv );
	} catch ( const args::Help& ) {
		std::cout << parser;
		return EXIT_SUCCESS;
	} catch ( const args::Error& error ) {
		std::cerr << error.what() << '\n' << parser;
		return EXIT_FAILURE;
	}
	if ( !init( locations.Get(), split.Get() ) )
		return EXIT_FAILURE;
	int result = mApp->run();
	mTransfers.clear();
	for ( auto& pane : mPanes )
		if ( pane )
			pane->tabList.clear();
	mExecutor.reset();
	mPool.reset();
	VirtualFileSystem::destroyInstance();
	return result;
}

bool App::init( const std::vector<std::string>& locations, bool forceSplit ) {
	mConfigDir = Sys::getConfigPath( "eefm" );
	FileSystem::dirAddSlashAtEnd( mConfigDir );
	loadState();

	int width = 1400, height = 820;
	if ( mState.contains( "window" ) && mState["window"].is_object() ) {
		width = std::max( 640, mState["window"].value( "width", width ) );
		height = std::max( 400, mState["window"].value( "height", height ) );
	}
	mConfig = std::make_unique<AppConfig>( mConfigDir );
	mConfig->load();
	WindowSettings ws( width, height, "eefm", WindowStyle::Default, WindowBackend::Default, 32,
					   Sys::getProcessPath() + "assets/icon/ee.png" );
	UIApplication::Settings appSettings;
	if ( mConfig->window.pixelDensity > 0 )
		appSettings.pixelDensity = mConfig->window.pixelDensity;
	appSettings.fontHinting = mConfig->font.hinting;
	appSettings.fontAntialiasing = mConfig->font.antialiasing;
	FontTrueTypePtr uiFont;
	if ( !mConfig->font.uiPath.empty() && FileSystem::fileExists( mConfig->font.uiPath ) ) {
		uiFont = FontTrueType::New( "eefm-ui-font" );
		if ( uiFont->loadFromFile( mConfig->font.uiPath ) )
			appSettings.baseFont = uiFont.get();
	}
	mApp = std::make_unique<UIApplication>(
		ws, appSettings,
		ContextSettings( mConfig->window.vsync, static_cast<Int32>( mConfig->window.maxFPS ),
						 mConfig->window.multisamples, mConfig->window.rendererVersion ) );
	mUI = mApp->getUI();
	if ( !mUI )
		return false;
	mUI->setColorSchemePreference( mConfig->theme.uiColorScheme );
	mUI->updateWindowTitleBarColor();
	if ( mConfig->font.uiSize > 0 )
		applyUIFont( appSettings.baseFont, mConfig->font.uiSize );
	mUI->setSmoothScrollEnabled( mConfig->files.smoothScroll );
	if ( !mConfig->font.fallbackPath.empty() ) {
		auto fallback = FontTrueType::New( "eefm-fallback-font", *mUI->getResourceScope() );
		if ( fallback->loadFromFile( mConfig->font.fallbackPath ) )
			mUI->getResourceScope()->getFontService().addFallbackFont( std::move( fallback ) );
	}
	if ( mState.contains( "window" ) && mState["window"].value( "maximized", false ) )
		mApp->getWindow()->maximize();
	loadIcons( mUI, Sys::getProcessPath() + "assets/" );

	mExecutor = std::make_unique<MainThreadExecutor>( mUI );
	mPool = ThreadPool::createShared( 2 );
	// Theme detection and icon loading run on the pool: startup does not wait for them.
	mSystemIcons = UISystemIconTheme::New( *mPool, *mExecutor );
	mSystemIcons->setOnIconsLoaded( [this] {
		for ( int i = 0; i < 2; ++i )
			if ( auto* tab = currentTab( pane( i ) ) )
				tab->view->invalidateDraw();
		if ( mPlacesPanel )
			mPlacesPanel->refreshIcons();
	} );
	mVFS = &VirtualFileSystem::instance();
#ifdef EE_VFS_SFTP
	mSSHPrompts = std::make_shared<SSHPrompts>( mUI, *mExecutor );
	if ( auto sftp = std::dynamic_pointer_cast<SFTPProvider>( mVFS->provider( "sftp" ) ) ) {
		sftp->setHostKeyVerifier( mSSHPrompts );
		sftp->setAuthenticationHandler( mSSHPrompts );
	}
#endif

	mUI->loadLayoutFromString( Layout );
	mToolbar = mUI->find<UIWidget>( "toolbar" );
	mMenuButton = mUI->find<UIWidget>( "app_menu" );
	mMainSplitter = mUI->find<UISplitter>( "main_splitter" );
	mTerminalSplitter = mUI->find<UISplitter>( "terminal_splitter" );
	if ( mState.contains( "terminal_partition" ) && mState["terminal_partition"].is_string() )
		mTerminalPartition = mState["terminal_partition"].get<std::string>();
	mPaneSplitter = mUI->find<UISplitter>( "pane_splitter" );
	mPlacesScroll = mUI->find<UIWidget>( "places_scroll" );
	mParking = mUI->find<UIWidget>( "parking" );

	// Places
	mPlaces = std::make_unique<Places>( mConfigDir + "places.json" );
	mPlaces->load();
	mPlacesPanel = PlacesPanel::New();
	mPlacesPanel->iconProvider = [this]( const Place& place ) { return placeIcon( place ); };
	mPlacesPanel->setParent( mPlacesScroll );
	mPlacesPanel->onActivate = [this]( const PlacesPanel::Entry& entry, bool newTab ) {
		if ( newTab )
			openTab( activePane(), entry.place.uri );
		else if ( auto* tab = currentTab() )
			navigate( tab, entry.place.uri );
	};
	mPlacesPanel->onContextMenu = [this]( const PlacesPanel::Entry& entry ) {
		showPlaceMenu( entry );
	};
	mPlacesPanel->onPlaceMoved = [this]( size_t index, std::optional<size_t> before ) {
		mPlaces->moveBefore( index, before );
		savePlaces();
	};
	mPlacesPanel->onFoldersAdded = [this]( std::optional<size_t> before ) {
		for ( const auto& uri : currentFileDrag().uris ) {
			Place place;
			place.uri = uri;
			place.name = displayName( uri ).toUtf8();
			place.icon = Places::defaultIcon( uri );
			// Each one after the previous: before moves along with the inserted places.
			if ( mPlaces->add( std::move( place ), before ) && before )
				++*before;
		}
		savePlaces();
	};
	mPlacesPanel->onSectionMenu = [this]( PlaceSection section ) {
		showPlacesSectionMenu( section );
	};
	mPlacesPanel->onFilesDropped = [this]( const PlacesPanel::Entry& entry ) {
		dropFiles( currentFileDrag().uris, entry.place.uri, mActivePane );
	};
	refreshPlaces();

	// Panes and their location bars
	for ( int i = 0; i < 2; ++i ) {
		auto* box = mUI->find<UIWidget>( "location_" + String::toString( i ) );
		mUI->loadLayoutFromString( LocationLayout, box );
		createPane( i, mPaneSplitter );
	}
	if ( mState.contains( "split_partition" ) && mState["split_partition"].is_string() )
		mPaneSplitter->setSplitPartition(
			StyleSheetLength( mState["split_partition"].get<std::string>() ) );
	if ( mState.contains( "places_width" ) && mState["places_width"].is_number() )
		mMainSplitter->setSplitPartition(
			StyleSheetLength( mState["places_width"].get<Float>(), StyleSheetLength::Dp ) );

	auto onClick = [this]( const char* id, std::function<void()> fn ) {
		mUI->find<UIWidget>( id )->onClick( [fn = std::move( fn )]( const MouseEvent* ) { fn(); } );
	};
	onClick( "back", [this] { goBack(); } );
	onClick( "forward", [this] { goForward(); } );
	onClick( "up", [this] { goUp(); } );
	onClick( "refresh", [this] {
		if ( auto* tab = currentTab() )
			tab->model->refresh();
	} );
	onClick( "split", [this] { setSplit( !mSplit ); } );
	onClick( "places_toggle", [this] { setPlacesVisible( !mPlacesVisible ); } );
	onClick( "app_menu", [this] { showAppMenu(); } );

	// Keep the location bars aligned with their panes.
	for ( auto& pane : mPanes ) {
		pane->root->on( Event::OnSizeChange, [this]( const Event* ) { layoutLocationBars(); } );
		pane->root->on( Event::OnPositionChange, [this]( const Event* ) { layoutLocationBars(); } );
	}
	mToolbar->on( Event::OnSizeChange, [this]( const Event* ) { layoutLocationBars(); } );

	mApp->getWindow()->getInput()->pushCallback( [this]( InputEvent* event ) {
		if ( event->Type == InputEvent::FileDropped ) {
			// Files dropped from other applications: gathered, then copied in one transfer once
			// the events of this frame were delivered (a drop sends one event per file).
			std::string file( event->file.file );
			if ( FileSystem::fileExtension( file ) == "lnk" ) {
				std::string target = Sys::getShortcutTarget( file );
				if ( !target.empty() && FileSystem::fileExists( target ) )
					file = target;
			}
			mDroppedFiles.push_back( URI::fromFSPath( file ) );
			if ( event->file.hasPosition )
				mDropPosition = Vector2f( event->file.x, event->file.y );
		} else if ( event->Type == InputEvent::EventsSent ) {
			if ( !mDroppedFiles.empty() )
				dropExternalFiles();
		} else if ( event->Type == InputEvent::MouseButtonDown ) {
			// Any press inside a pane (or its location bar) makes it the active one. Widgets
			// handle their own clicks, so this is tracked at the window level.
			// The widget under the pointer must belong to the pane: menus and dialogs drawn over
			// it are not the pane.
			Vector2f mouse( event->button.x, event->button.y );
			Node* over = mUI->overFind( mouse );
			for ( int i = 0; i < 2; ++i ) {
				Pane& p = pane( i );
				if ( over && ( ( p.root->getParent() == mPaneSplitter &&
								 ( p.root == over || p.root->inParentTreeOf( over ) ) ) ||
							   ( p.location.box->isVisible() &&
								 ( p.location.box == over ||
								   p.location.box->inParentTreeOf( over ) ) ) ) ) {
					if ( i != mActivePane )
						setActivePane( i );
					break;
				}
			}
		} else if ( event->Type == InputEvent::MouseButtonUp ) {
			// Side mouse buttons navigate the history.
			if ( event->button.button == EE_BUTTON_4 )
				goBack();
			else if ( event->button.button == EE_BUTTON_5 )
				goForward();
		}
	} );

	mApp->getWindow()->setCloseRequestCallback( [this]( EE::Window::Window* ) {
		saveState();
		return true;
	} );

	// The keyboard belongs to the files: a click on a pane's bars or empty areas focuses the
	// container under the pointer, which would swallow the keys (typing, arrows, Enter). Hand
	// it to that pane's files on the next frame. Buttons and text fields keep it.
	mUI->getEventDispatcher()->addFocusEventCallback( [this]( const Uint32&, Node* focus, Node* ) {
		// Focus falls back to the scene (or the UI root) when the focused dialog or menu closes:
		// the active pane's files take it back.
		if ( focus && ( !focus->isWidget() || focus == mUI->getRoot() ) ) {
			mUI->runOnMainThread( [this, focus] {
				if ( mUI->getEventDispatcher()->getFocusNode() == focus )
					if ( auto* tab = currentTab() )
						tab->view->setFocus();
			} );
			return;
		}
		if ( !focus || !focus->isWidget() || focus->isType( UI_TYPE_PUSHBUTTON ) ||
			 focus->isType( UI_TYPE_TEXTINPUT ) )
			return;
		for ( int i = 0; i < 2; ++i ) {
			auto* tab = currentTab( pane( i ) );
			if ( !tab || !pane( i ).root->inParentTreeOf( focus ) || tab->view == focus ||
				 tab->view->inParentTreeOf( focus ) )
				continue;
			mUI->runOnMainThread( [this, i, focus] {
				auto* current = currentTab( pane( i ) );
				// Only if nothing else took the focus meanwhile (focus is not dereferenced).
				if ( current && mUI->getEventDispatcher()->getFocusNode() == focus )
					current->view->setFocus();
			} );
			return;
		}
	} );

	registerShortcuts();
	restoreTabs( locations, forceSplit );
	setPlacesVisible( mPlacesVisible );
	setFilterBarVisible( mFilterBarVisible );
	refreshDeviceUsage();
	return true;
}

// ---------------------------------------------------------------------------------------------
// Persistent state
// ---------------------------------------------------------------------------------------------

void App::loadState() {
	std::string data;
	if ( FileSystem::fileGet( mConfigDir + "state.json", data ) )
		mState = nlohmann::json::parse( data, nullptr, false );
	if ( !mState.is_object() )
		mState = nlohmann::json::object();
	mSplit = mState.value( "split", true );
	mPlacesVisible = mState.value( "places_visible", true );
	mFilterBarVisible = mState.value( "filter_bar", true );
	mShowHidden = mState.value( "show_hidden", false );
	mFilterCaseSensitive = mState.value( "filter_case_sensitive", false );
	std::string mode = mState.value( "filter_mode", "glob" );
	mFilterMode = mode == "text"	? VFSDirectoryModel::NameFilterMode::PlainText
				  : mode == "regex" ? VFSDirectoryModel::NameFilterMode::RegEx
									: VFSDirectoryModel::NameFilterMode::Glob;
	if ( mState.contains( "view" ) && mState["view"].is_object() )
		mViewState = mState["view"];
}

void App::saveState() {
	nlohmann::json state = nlohmann::json::object();
	auto* window = mApp->getWindow();
	bool maximized = window->isMaximized();
	state["window"] = { { "maximized", maximized } };
	if ( !maximized ) {
		state["window"]["width"] = window->getSize().getWidth();
		state["window"]["height"] = window->getSize().getHeight();
	} else if ( mState.contains( "window" ) ) {
		state["window"]["width"] = mState["window"].value( "width", 1400 );
		state["window"]["height"] = mState["window"].value( "height", 820 );
	}
	rememberTerminalSize();
	state["terminal_partition"] = mTerminalPartition;
	state["split"] = mSplit;
	state["places_visible"] = mPlacesVisible;
	state["filter_bar"] = mFilterBarVisible;
	state["show_hidden"] = mShowHidden;
	state["filter_case_sensitive"] = mFilterCaseSensitive;
	state["filter_mode"] = mFilterMode == VFSDirectoryModel::NameFilterMode::PlainText ? "text"
						   : mFilterMode == VFSDirectoryModel::NameFilterMode::RegEx   ? "regex"
																					   : "glob";
	if ( mPlacesVisible )
		state["places_width"] = PixelDensity::pxToDp( mPlacesScroll->getPixelsSize().getWidth() );
	else if ( mState.contains( "places_width" ) )
		state["places_width"] = mState["places_width"];
	if ( mSplit && mPaneSplitter->getPixelsSize().getWidth() > 0 ) {
		Float ratio = pane( 0 ).root->getPixelsSize().getWidth() /
					  mPaneSplitter->getPixelsSize().getWidth() * 100.f;
		state["split_partition"] = String::format( "%.2f%%", ratio );
	} else if ( mState.contains( "split_partition" ) ) {
		state["split_partition"] = mState["split_partition"];
	}
	nlohmann::json panes = nlohmann::json::array();
	for ( auto& pane : mPanes ) {
		nlohmann::json tabs = nlohmann::json::array();
		size_t current = 0;
		for ( Uint32 i = 0; i < pane->tabs->getTabCount(); ++i ) {
			FileTab* tab = findTab( pane->tabs->getTab( i ) );
			if ( !tab )
				continue;
			if ( tab->tab == pane->tabs->getTabSelected() )
				current = tabs.size();
			tabs.push_back( tab->model->uri().toString() );
		}
		panes.push_back( { { "tabs", tabs }, { "current", current } } );
	}
	state["panes"] = panes;
	if ( auto* tab = currentTab() )
		rememberViewState( tab );
	state["view"] = mViewState;
	FileSystem::makeDir( mConfigDir, true );
	FileSystem::fileWrite( mConfigDir + "state.json", std::string_view( state.dump( 2 ) ) );
	mPlaces->save();
}

void App::quit() {
	// Closing the window programmatically skips the close request callback.
	saveState();
	mApp->getWindow()->close();
}

void App::rememberViewState( FileTab* tab ) {
	if ( !tab )
		return;
	nlohmann::json view = nlohmann::json::object();
	// Automatic widths follow the view size; only widths set by the user are remembered.
	if ( !tab->view->getAutoColumnsWidth() )
		view["columns"] = tab->view->serializeColumnWidths();
	nlohmann::json hidden = nlohmann::json::array();
	for ( size_t col = 0; col < VFSDirectoryModel::ColumnCount; ++col )
		if ( tab->view->isColumnHidden( col ) )
			hidden.push_back( col );
	view["hidden"] = hidden;
	view["order"] = tab->view->getColumnOrder();
	view["sort_column"] = tab->model->keyColumn();
	view["sort_order"] = sortOrderName( tab->model->sortOrder() );
	mViewState = std::move( view );
}

void App::applyViewState( FileTab* tab ) {
	auto* view = tab->view;
	if ( !mViewState.is_object() || mViewState.empty() ) {
		view->setColumnsHidden(
			{ VFSDirectoryModel::Permissions, VFSDirectoryModel::Owner, VFSDirectoryModel::Group },
			true );
		return;
	}
	if ( mViewState.contains( "hidden" ) && mViewState["hidden"].is_array() ) {
		for ( size_t col = 0; col < VFSDirectoryModel::ColumnCount; ++col ) {
			bool hidden = false;
			for ( const auto& value : mViewState["hidden"] )
				hidden = hidden || ( value.is_number() && value.get<size_t>() == col );
			view->setColumnHidden( col, hidden );
		}
	}
	if ( mViewState.contains( "order" ) && mViewState["order"].is_array() ) {
		std::vector<size_t> order;
		for ( const auto& value : mViewState["order"] )
			if ( value.is_number() )
				order.push_back( value.get<size_t>() );
		view->setColumnOrder( std::move( order ) );
	}
	if ( mViewState.contains( "columns" ) ) {
		view->setAutoColumnsWidth( false );
		view->unserializeColumnWidths( mViewState["columns"] );
	}
	size_t column = mViewState.value( "sort_column", 0 );
	if ( column < VFSDirectoryModel::ColumnCount )
		view->sortByColumn( column, mViewState.value( "sort_order", "ascending" ) == "descending"
										? SortOrder::Descending
										: SortOrder::Ascending );
}

void App::restoreTabs( const std::vector<std::string>& locations, bool forceSplit ) {
	URI home = URI::fromFSPath( Sys::getUserDirectory() );
	if ( !locations.empty() ) {
		if ( forceSplit && locations.size() >= 2 ) {
			openTab( pane( 0 ), parseLocation( locations[0] ) );
			for ( size_t i = 1; i < locations.size(); ++i )
				openTab( pane( 1 ), parseLocation( locations[i] ) );
			mSplit = true;
		} else {
			for ( const auto& location : locations )
				openTab( pane( 0 ), parseLocation( location ) );
			openTab( pane( 1 ), parseLocation( locations.back() ) );
			mSplit = forceSplit || mSplit;
		}
	} else if ( mState.contains( "panes" ) && mState["panes"].is_array() ) {
		for ( int i = 0; i < 2 && i < static_cast<int>( mState["panes"].size() ); ++i ) {
			const auto& saved = mState["panes"][i];
			if ( !saved.contains( "tabs" ) || !saved["tabs"].is_array() )
				continue;
			for ( const auto& uri : saved["tabs"] )
				if ( uri.is_string() )
					openTab( pane( i ), URI( uri.get<std::string>() ), false );
			Uint32 current = saved.value( "current", 0u );
			if ( current < pane( i ).tabs->getTabCount() )
				pane( i ).tabs->setTabSelected( current );
		}
	}
	for ( int i = 0; i < 2; ++i )
		if ( pane( i ).tabList.empty() )
			openTab( pane( i ), home );
	setSplit( mSplit );
	// Start on the left pane, with the keyboard there too: restoring tabs focuses the views it
	// selects (the right pane's last), and the highlight alone does not move the focus.
	setActivePane( 0 );
	if ( auto* tab = currentTab( pane( 0 ) ) )
		tab->view->setFocus();
}

// ---------------------------------------------------------------------------------------------
// Layout toggles
// ---------------------------------------------------------------------------------------------

void App::setSplit( bool split ) {
	mSplit = split;
	Pane& right = pane( 1 );
	if ( split && right.root->getParent() != mPaneSplitter ) {
		right.root->setParent( mPaneSplitter );
		right.root->setVisible( true );
	} else if ( !split && right.root->getParent() == mPaneSplitter ) {
		right.root->setParent( mParking );
		if ( mActivePane == 1 )
			setActivePane( 0 );
	}
	right.location.box->setVisible( split );
	if ( auto* button = mUI->find<UISelectButton>( "split" ) )
		button->setSelected( split );
	layoutLocationBars();
}

void App::setPlacesVisible( bool visible ) {
	mPlacesVisible = visible;
	if ( visible && mPlacesScroll->getParent() != mMainSplitter ) {
		// The splitter keeps its children in insertion order: re-add the panes after places.
		mPaneSplitter->setParent( mParking );
		mPlacesScroll->setParent( mMainSplitter );
		mPaneSplitter->setParent( mMainSplitter );
	} else if ( !visible && mPlacesScroll->getParent() == mMainSplitter ) {
		mPlacesScroll->setParent( mParking );
	}
	if ( auto* button = mUI->find<UISelectButton>( "places_toggle" ) )
		button->setSelected( visible );
	layoutLocationBars();
}

void App::setFilterBarVisible( bool visible ) {
	mFilterBarVisible = visible;
	for ( auto& pane : mPanes ) {
		pane->filterBar->setVisible( visible );
		if ( !visible ) {
			// Closing the bar clears the filter, like Dolphin.
			for ( auto& tab : pane->tabList ) {
				tab->filter.clear();
				applyFilter( tab.get() );
			}
			pane->filterInput->setText( "" );
		}
	}
}

void App::setShowHidden( bool show ) {
	mShowHidden = show;
	for ( auto& pane : mPanes ) {
		for ( auto& tab : pane->tabList )
			tab->model->setShowHidden( show );
		updateStatus( *pane );
	}
}

void App::layoutLocationBars() {
	if ( !mPanes[0] || !mPanes[1] )
		return;
	// Deferred: positions are only final after the layout pass that triggered this.
	static bool pending = false;
	if ( pending )
		return;
	pending = true;
	mToolbar->runOnMainThread( [this] {
		pending = false;
		Float gap = PixelDensity::dpToPx( 6 );
		Float minWidth = PixelDensity::dpToPx( 120 );
		UIWidget* left = pane( 0 ).location.box;
		UIWidget* right = pane( 1 ).location.box;
		Float leftX = left->getScreenPos().x;
		Float menuX = mMenuButton->getScreenPos().x;
		Float leftWidth;
		if ( mSplit ) {
			Float rightPaneX = pane( 1 ).root->getScreenPos().x;
			leftWidth = std::max( minWidth, rightPaneX - leftX - gap );
			right->setPixelsSize( std::max( minWidth, menuX - gap - rightPaneX ),
								  right->getPixelsSize().getHeight() );
		} else {
			leftWidth = std::max( minWidth, menuX - gap - leftX );
		}
		left->setLayoutWidthPolicy( SizePolicy::Fixed );
		right->setLayoutWidthPolicy( SizePolicy::Fixed );
		left->setPixelsSize( leftWidth, left->getPixelsSize().getHeight() );
		// The toolbar must lay its children out again with the new widths.
		left->notifyLayoutAttrChangeParent();
	} );
}

// ---------------------------------------------------------------------------------------------
// Menus and shortcuts
// ---------------------------------------------------------------------------------------------

void App::showAppMenu() {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->add( mUI->i18n( "eefm_new_tab", "New Tab" ), icon( "action-new-tab" ), "Ctrl+T" )
		->setId( "new-tab" );
	if ( currentTab() && currentTab()->model->uri().getScheme() == "file" )
		menu->add( mUI->i18n( "eefm_open_terminal", "Open Terminal" ), icon( "action-terminal" ),
				   "Shift+F4" )
			->setId( "open-terminal" );
	menu->addSeparator();
	menu->addCheckBox( mUI->i18n( "eefm_show_hidden", "Show Hidden Files" ), mShowHidden, "Ctrl+H" )
		->setId( "hidden" );
	menu->addCheckBox( mUI->i18n( "eefm_split_view", "Split View" ), mSplit, "F3" )
		->setId( "split" );
	// As Dolphin: the panels are in a submenu.
	UIPopUpMenu* panels = UIPopUpMenu::New();
	panels->addCheckBox( mUI->i18n( "eefm_panel_places", "Places" ), mPlacesVisible, "F9" )
		->setId( "places" );
	panels->addCheckBox( mUI->i18n( "eefm_panel_terminal", "Terminal" ), isTerminalVisible(), "F4" )
		->setId( "terminal" );
	panels->on( Event::OnItemClicked, [this]( const Event* event ) {
		std::string id( event->getNode()->getId() );
		if ( id == "places" )
			setPlacesVisible( !mPlacesVisible );
		else if ( id == "terminal" )
			toggleTerminal();
	} );
	menu->addSubMenu( mUI->i18n( "eefm_show_panels", "Show Panels" ), icon( "places-panel" ),
					  panels );
	menu->addCheckBox( mUI->i18n( "eefm_show_filter_bar", "Show Filter Bar" ), mFilterBarVisible,
					   "Ctrl+I" )
		->setId( "filter" );
	menu->addSeparator();
	menu->add( mUI->i18n( "eefm_configure", "Configure eefm..." ), icon( "settings" ),
			   "Ctrl+Shift+," )
		->setId( "settings" );
	menu->add( mUI->i18n( "eefm_quit", "Quit" ), icon( "quit" ), "Ctrl+Q" )->setId( "quit" );
	menu->on( Event::OnItemClicked, [this]( const Event* event ) {
		std::string id( event->getNode()->getId() );
		if ( id == "new-tab" ) {
			auto* tab = currentTab();
			openTab( activePane(),
					 tab ? tab->model->uri() : URI::fromFSPath( Sys::getUserDirectory() ) );
		} else if ( id == "hidden" ) {
			setShowHidden( !mShowHidden );
		} else if ( id == "split" ) {
			setSplit( !mSplit );
		} else if ( id == "places" ) {
			setPlacesVisible( !mPlacesVisible );
		} else if ( id == "filter" ) {
			setFilterBarVisible( !mFilterBarVisible );
		} else if ( id == "settings" ) {
			showSettings();
		} else if ( id == "open-terminal" ) {
			if ( auto* tab = currentTab() )
				openTerminalHere( tab );
		} else if ( id == "quit" ) {
			quit();
		}
	} );
	Vector2f pos = mMenuButton->getScreenPos();
	pos.y += mMenuButton->getPixelsSize().getHeight();
	menu->showAtScreenPosition( pos );
	// Keep it inside the window: right-align with the button.
	menu->runOnMainThread( [menu, this] {
		Vector2f p( mMenuButton->getScreenPos().x + mMenuButton->getPixelsSize().getWidth() -
						menu->getPixelsSize().getWidth(),
					mMenuButton->getScreenPos().y + mMenuButton->getPixelsSize().getHeight() );
		menu->setPixelsPosition( menu->getParent()->convertToNodeSpace( p ) );
	} );
}

void App::registerShortcuts() {
	// Commands that act on files must not steal keys from text inputs.
	auto fileCommand = [this]( std::function<void()> fn ) {
		return [this, fn = std::move( fn )] {
			Node* focus = mUI->getEventDispatcher()->getFocusNode();
			if ( !focus || !focus->isType( UI_TYPE_TEXTINPUT ) )
				fn();
		};
	};
	auto onTab = [this]( std::function<void( FileTab* )> fn ) {
		return [this, fn = std::move( fn )] {
			if ( auto* tab = currentTab() )
				fn( tab );
		};
	};
	mUI->setKeyBindingCommand( "new-tab", [this] {
		auto* tab = currentTab();
		openTab( activePane(),
				 tab ? tab->model->uri() : URI::fromFSPath( Sys::getUserDirectory() ) );
	} );
	mUI->setKeyBindingCommand( "close-tab", onTab( [this]( FileTab* tab ) { closeTab( tab ); } ) );
	mUI->setKeyBindingCommand( "next-tab", [this] {
		Pane& p = activePane();
		Uint32 count = p.tabs->getTabCount();
		if ( count > 1 )
			p.tabs->setTabSelected( ( p.tabs->getTabSelectedIndex() + 1 ) % count );
	} );
	mUI->setKeyBindingCommand( "previous-tab", [this] {
		Pane& p = activePane();
		Uint32 count = p.tabs->getTabCount();
		if ( count > 1 )
			p.tabs->setTabSelected( ( p.tabs->getTabSelectedIndex() + count - 1 ) % count );
	} );
	mUI->setKeyBindingCommand( "focus-location", [this] {
		auto* input = activePane().location.input;
		input->setFocus();
		input->getDocument().selectAll();
	} );
	mUI->setKeyBindingCommand( "toggle-hidden", [this] { setShowHidden( !mShowHidden ); } );
	mUI->setKeyBindingCommand( "toggle-split", [this] { setSplit( !mSplit ); } );
	mUI->setKeyBindingCommand( "toggle-places", [this] { setPlacesVisible( !mPlacesVisible ); } );
	mUI->setKeyBindingCommand( "toggle-filter", [this] {
		if ( !mFilterBarVisible )
			setFilterBarVisible( true );
		activePane().filterInput->setFocus();
	} );
	mUI->setKeyBindingCommand( "switch-pane", fileCommand( [this] {
								   if ( !mSplit )
									   return;
								   setActivePane( 1 - mActivePane );
								   if ( auto* tab = currentTab() )
									   tab->view->setFocus();
							   } ) );
	mUI->setKeyBindingCommand( "refresh", onTab( []( FileTab* tab ) { tab->model->refresh(); } ) );
	mUI->setKeyBindingCommand( "go-back", [this] { goBack(); } );
	mUI->setKeyBindingCommand( "go-forward", [this] { goForward(); } );
	mUI->setKeyBindingCommand( "go-up", [this] { goUp(); } );
	mUI->setKeyBindingCommand( "go-home", [this] { goHome(); } );
	mUI->setKeyBindingCommand( "go-up-file", fileCommand( [this] { goUp(); } ) );
	mUI->setKeyBindingCommand( "new-folder",
							   onTab( [this]( FileTab* tab ) { newFolder( tab ); } ) );
	mUI->setKeyBindingCommand( "copy", fileCommand( [this] { copySelection( false ); } ) );
	mUI->setKeyBindingCommand( "cut", fileCommand( [this] { copySelection( true ); } ) );
	mUI->setKeyBindingCommand( "paste", fileCommand( [this] {
								   if ( auto* tab = currentTab() )
									   paste( tab );
							   } ) );
	mUI->setKeyBindingCommand( "select-all", fileCommand( [this] {
								   if ( auto* tab = currentTab() ) {
									   std::vector<ModelIndex> all;
									   for ( size_t row = 0; row < tab->model->rowCount(); ++row )
										   all.push_back( tab->model->index( row ) );
									   tab->view->getSelection().set( all );
								   }
							   } ) );
	mUI->setKeyBindingCommand( "trash", fileCommand( [this] { moveToTrash(); } ) );
	mUI->setKeyBindingCommand( "delete", fileCommand( [this] { deleteSelection(); } ) );
	mUI->setKeyBindingCommand( "rename", fileCommand( [this] { renameSelection(); } ) );
	mUI->setKeyBindingCommand(
		"properties", fileCommand( onTab( [this]( FileTab* tab ) { showProperties( tab ); } ) ) );
	mUI->setKeyBindingCommand( "terminal",
							   onTab( [this]( FileTab* tab ) { openTerminalHere( tab ); } ) );
	mUI->setKeyBindingCommand( "toggle-terminal", [this] { toggleTerminal(); } );
	mUI->setKeyBindingCommand( "settings", [this] { showSettings(); } );
	mUI->setKeyBindingCommand( "quit", [this] { quit(); } );
	mUI->setKeyBindingCommand( "debug-widget-tree-view",
							   [this] { Tools::UIWidgetInspector::create( mUI ); } );
	mUI->getKeyBindings().addKeybindsString( {
		{ "mod+t", "new-tab" },
		{ "mod+w", "close-tab" },
		{ "mod+tab", "next-tab" },
		{ "mod+shift+tab", "previous-tab" },
		{ "mod+l", "focus-location" },
		{ "f6", "focus-location" },
		{ "mod+h", "toggle-hidden" },
		{ "alt+.", "toggle-hidden" },
		{ "f3", "toggle-split" },
		{ "f9", "toggle-places" },
		{ "mod+i", "toggle-filter" },
		{ "tab", "switch-pane" },
		{ "f5", "refresh" },
		{ "alt+left", "go-back" },
		{ "alt+right", "go-forward" },
		{ "alt+up", "go-up" },
		{ "alt+home", "go-home" },
		{ "backspace", "go-up-file" },
		{ "mod+shift+n", "new-folder" },
		{ "f10", "new-folder" },
		{ "mod+c", "copy" },
		{ "mod+x", "cut" },
		{ "mod+v", "paste" },
		{ "mod+a", "select-all" },
		{ "delete", "trash" },
		{ "shift+delete", "delete" },
		{ "f2", "rename" },
		{ "alt+return", "properties" },
		{ "shift+f4", "terminal" },
		{ "f4", "toggle-terminal" },
		{ "mod+shift+,", "settings" },
		{ "mod+q", "quit" },
		{ "mod+shift+f11", "debug-widget-tree-view" },
	} );
}

// ---------------------------------------------------------------------------------------------
// Places panel
// ---------------------------------------------------------------------------------------------

void App::refreshPlaces() {
	mDevices = Places::devices();
	std::vector<PlacesPanel::Entry> entries;
	bool showHidden = mPlaces->showHidden();
	auto listed = [&]( const Place& place ) {
		return showHidden || ( !place.hidden && !mPlaces->isSectionHidden( place.section ) );
	};
	const auto& places = mPlaces->places();
	// Each section: the saved places, then the discovered mounts (network shares are Remote).
	for ( auto section : { PlaceSection::Places, PlaceSection::Remote, PlaceSection::Devices } ) {
		for ( size_t i = 0; i < places.size(); ++i )
			if ( places[i].section == section && listed( places[i] ) )
				entries.push_back(
					{ places[i], static_cast<int>( i ), mPlaces->isSectionHidden( section ) } );
		for ( const auto& device : mDevices ) {
			bool saved = std::any_of( places.begin(), places.end(), [&]( const Place& place ) {
				return place.uri == device.uri;
			} );
			if ( device.section == section && !saved && listed( device ) )
				entries.push_back( { device, -1, mPlaces->isSectionHidden( section ) } );
		}
	}
	mPlacesPanel->setEntries( std::move( entries ) );
	if ( auto* tab = currentTab() )
		mPlacesPanel->setCurrent( tab->model->uri() );
}

void App::savePlaces() {
	mPlaces->save();
	refreshPlaces();
	refreshDeviceUsage();
}

void App::refreshDeviceUsage() {
	for ( const auto& device : mDevices ) {
		// statvfs on a network share blocks while its server is unreachable.
		if ( device.section != PlaceSection::Devices )
			continue;
		URI uri = device.uri;
		mVFS->spaceInfo( uri ).thenOn( *mExecutor, [this, uri]( VFSResult<SpaceInfo>&& space ) {
			if ( !space || space.value().capacity == 0 )
				return;
			const auto& info = space.value();
			Float used =
				1.f - static_cast<Float>( info.available ) / static_cast<Float>( info.capacity );
			mPlacesPanel->setUsage( uri, used );
		} );
	}
}

void App::showPlaceMenu( const PlacesPanel::Entry& entry ) {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->add( mUI->i18n( "eefm_open_new_tab", "Open in New Tab" ), icon( "action-new-tab" ) )
		->setId( "open-tab" );
	if ( mSplit )
		menu->add( mUI->i18n( "eefm_open_other_pane", "Open in Other Pane" ) )
			->setId( "open-other" );
	if ( entry.place.trash ) {
		menu->addSeparator();
		menu->add( mUI->i18n( "eefm_empty_trash", "Empty Trash" ), icon( "place-trash" ) )
			->setId( "empty-trash" );
	}
	if ( entry.index >= 0 ) {
		size_t index = static_cast<size_t>( entry.index );
		menu->addSeparator();
		menu->add( mUI->i18n( "eefm_edit_place", "Edit..." ), icon( "edit" ) )->setId( "edit" );
		if ( entry.place.hidden )
			menu->add( mUI->i18n( "eefm_show_place", "Show" ) )->setId( "show" );
		else
			menu->add( mUI->i18n( "eefm_hide_place", "Hide" ) )->setId( "hide" );
		if ( entry.place.id.empty() )
			menu->add( mUI->i18n( "eefm_remove_place", "Remove" ), icon( "delete-bin" ) )
				->setId( "remove" );
		if ( mPlaces->canMove( index, -1 ) )
			menu->add( mUI->i18n( "eefm_move_place_up", "Move Up" ), icon( "arrow-up-s" ) )
				->setId( "move-up" );
		if ( mPlaces->canMove( index, 1 ) )
			menu->add( mUI->i18n( "eefm_move_place_down", "Move Down" ), icon( "arrow-down-s" ) )
				->setId( "move-down" );
	}
	if ( entry.place.section == PlaceSection::Devices ) {
		menu->addSeparator();
		menu->add( mUI->i18n( "eefm_refresh_devices", "Refresh Devices" ), icon( "view-refresh" ) )
			->setId( "refresh-devices" );
	}
	addPlacesPanelItems( menu, entry.place.section );
	menu->on( Event::OnItemClicked, [this, entry]( const Event* event ) {
		std::string id( event->getNode()->getId() );
		size_t index = static_cast<size_t>( std::max( entry.index, 0 ) );
		if ( onPlacesPanelItem( id, entry.place.section ) ) {
		} else if ( id == "open-tab" ) {
			openTab( activePane(), entry.place.uri );
		} else if ( id == "open-other" ) {
			if ( auto* tab = currentTab( pane( 1 - mActivePane ) ) )
				navigate( tab, entry.place.uri );
		} else if ( id == "empty-trash" ) {
			emptyTrash();
		} else if ( id == "edit" ) {
			showPlaceDialog( index, entry.place );
		} else if ( id == "hide" || id == "show" ) {
			mPlaces->setHidden( index, id == "hide" );
			savePlaces();
		} else if ( id == "remove" ) {
			mPlaces->remove( index );
			savePlaces();
		} else if ( id == "move-up" || id == "move-down" ) {
			mPlaces->move( index, id == "move-up" ? -1 : 1 );
			savePlaces();
		} else if ( id == "refresh-devices" ) {
			refreshPlaces();
			refreshDeviceUsage();
		}
	} );
	menu->showOverMouseCursor();
}

void App::showPlacesSectionMenu( PlaceSection section ) {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	addPlacesPanelItems( menu, section );
	menu->on( Event::OnItemClicked, [this, section]( const Event* event ) {
		onPlacesPanelItem( std::string( event->getNode()->getId() ), section );
	} );
	menu->showOverMouseCursor();
}

void App::addPlacesPanelItems( UIPopUpMenu* menu, PlaceSection section ) {
	if ( menu->getCount() > 0 )
		menu->addSeparator();
	menu->add( mUI->i18n( "eefm_add_place_entry", "Add Entry..." ), icon( "action-add-place" ) )
		->setId( "add-entry" );
	if ( mPlaces->isSectionHidden( section ) )
		menu->add( mUI->i18n( "eefm_show_section", "Show Section" ) )->setId( "show-section" );
	else
		menu->add( mUI->i18n( "eefm_hide_section", "Hide Section" ) )->setId( "hide-section" );
	if ( mPlaces->hasHidden() || mPlaces->showHidden() )
		menu->addCheckBox( mUI->i18n( "eefm_show_hidden_places", "Show Hidden Places" ),
						   mPlaces->showHidden() )
			->setId( "show-hidden-places" );
}

bool App::onPlacesPanelItem( const std::string& id, PlaceSection section ) {
	if ( id == "add-entry" ) {
		Place place;
		auto* tab = currentTab();
		// Start from the current folder, or an empty sftp location for the Remote section.
		if ( section == PlaceSection::Remote ) {
			place.uri = URI( "sftp://" );
		} else if ( tab ) {
			place.uri = tab->model->uri();
			place.name = displayName( place.uri ).toUtf8();
		}
		place.icon = section == PlaceSection::Remote ? "place-remote" : "";
		showPlaceDialog( std::nullopt, place );
	} else if ( id == "show-section" || id == "hide-section" ) {
		mPlaces->setSectionHidden( section, id == "hide-section" );
		savePlaces();
	} else if ( id == "show-hidden-places" ) {
		mPlaces->setShowHidden( !mPlaces->showHidden() );
		savePlaces();
	} else {
		return false;
	}
	return true;
}

void App::showPlaceDialog( std::optional<size_t> index, Place initial ) {
	static const auto layout( R"xml(
	<window id="place_dialog" layout_width="460dp" layout_height="250dp" window-flags="default|shadow|modal">
	<vbox lw="mp" lh="wrap_content" padding="8dp">
		<TextView text='@string(eefm_place_label, "Label:")' />
		<TextInput id="place_name" lw="mp" lh="wrap_content" margin-bottom="8dp" />
		<TextView text='@string(eefm_place_location, "Location:")' />
		<ComboBox id="place_location" lw="mp" lh="wrap_content" popup-to-root="true" />
		<TextView class="place_hint" lw="mp" lh="wrap_content" word-wrap="true" margin-bottom="8dp"
			text='@string(eefm_place_location_hint, "A folder path or a URI, e.g. sftp://user@host/path. Hosts from ~/.ssh/config are listed.")' />
		<TextView text='@string(eefm_place_icon, "Icon:")' />
		<hbox id="place_icons" lw="mp" lh="wrap_content" margin-bottom="8dp" />
		<hbox layout_gravity="right">
			<PushButton id="ok" text="@string(msg_box_ok, Ok)" />
			<PushButton id="cancel" text="@string(msg_box_cancel, Cancel)" margin-left="8dp" />
		</hbox>
	</vbox>
	</window>
	)xml" );
	UIWindow* window = mUI->loadLayoutFromString( layout )->asType<UIWindow>();
	window->setTitle( index ? mUI->i18n( "eefm_edit_place_title", "Edit Place" )
							: mUI->i18n( "eefm_add_place_title", "Add Place" ) );
	auto* nameInput = window->find<UITextInput>( "place_name" );
	auto* location = window->find<UIComboBox>( "place_location" );
	auto* icons = window->find<UIWidget>( "place_icons" );

	auto toText = []( const URI& uri ) {
		return uri.getScheme() == "file" ? uri.getFSPath() : uri.toString();
	};
	std::vector<String> suggestions;
	for ( const auto& host : Places::sshHosts() )
		suggestions.emplace_back( String::fromUtf8( host.toString() ) );
	location->getListBox()->addListBoxItems( suggestions );
	nameInput->setText( String::fromUtf8( initial.name ) );
	std::string locationText;
	if ( initial.uri.getScheme() == "file" || !initial.uri.getHost().empty() )
		locationText = toText( initial.uri );
	else if ( !initial.uri.getScheme().empty() )
		locationText = initial.uri.getScheme() + "://";
	location->setText( String::fromUtf8( locationText ) );

	// One toggle per icon; an empty choice follows the location (folder or remote).
	auto chosen = std::make_shared<std::string>( initial.icon );
	auto buttons = std::make_shared<std::vector<UISelectButton*>>();
	for ( const auto& name : Places::iconNames() ) {
		UISelectButton* button = UISelectButton::New();
		button->setParent( icons );
		button->addClass( "place_icon_choice" );
		if ( UIIcon* themeIcon = mUI->findIcon( name ) )
			button->setIcon( themeIcon->getSource( PixelDensity::dpToPxI( 16 ) ) );
		button->setTooltipText( name );
		if ( name == *chosen )
			button->select();
		buttons->push_back( button );
		button->onClick( [button, buttons, chosen, name]( const MouseEvent* ) {
			for ( auto* other : *buttons )
				if ( other != button )
					other->unselect();
			button->select();
			*chosen = name;
		} );
	}

	auto accept = [this, window, nameInput, location, chosen, index, toText] {
		std::string text = String::trim( location->getText().toUtf8() );
		URI uri;
		if ( text.find( "://" ) != std::string::npos ) {
			uri = URI( text );
		} else if ( !text.empty() ) {
			std::string path = text;
			if ( String::startsWith( path, "~" ) )
				path = Sys::getUserDirectory() + path.substr( 1 );
			FileSystem::dirAddSlashAtEnd( path );
			uri = URI::fromFSPath( path );
		}
		if ( uri.getScheme().empty() || ( uri.getScheme() != "file" && uri.getHost().empty() ) ) {
			showError( mUI, mUI->i18n( "eefm_invalid_place_location",
									   "Enter a folder path or a URI such as "
									   "sftp://user@host/path." ) );
			return;
		}
		if ( uri.getPath().empty() )
			uri.setRawPath( "/" );
		std::string name = String::trim( nameInput->getText().toUtf8() );
		if ( name.empty() )
			name = displayName( uri ).toUtf8();
		std::string iconName = chosen->empty() ? Places::defaultIcon( uri ) : *chosen;
		if ( index ) {
			mPlaces->edit( *index, name, uri, iconName );
		} else {
			Place place;
			place.name = name;
			place.uri = uri;
			place.icon = iconName;
			if ( !mPlaces->add( std::move( place ) ) ) {
				showError(
					mUI, String::format(
							 mUI->i18n( "eefm_place_exists", "%s is already in Places." ).toUtf8(),
							 toText( uri ).c_str() ) );
				return;
			}
		}
		savePlaces();
		window->closeWindow();
	};
	window->find<UIPushButton>( "ok" )->onClick( [accept]( const MouseEvent* ) { accept(); },
												 EE_BUTTON_LEFT );
	window->find<UIPushButton>( "cancel" )
		->onClick( [window]( const MouseEvent* ) { window->closeWindow(); }, EE_BUTTON_LEFT );
	nameInput->on( Event::OnPressEnter, [accept]( const Event* ) { accept(); } );
	location->getDropDownList()->on( Event::OnPressEnter, [accept]( const Event* ) { accept(); } );
	window->on( Event::KeyDown, [window]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_ESCAPE )
			window->closeWindow();
	} );
	window->center();
	window->on( Event::OnWindowReady, [nameInput, location, index]( const Event* ) {
		// Adding a remote place starts at the location; otherwise at the label.
		if ( !index && String::startsWith( location->getText().toUtf8(), "sftp://" ) &&
			 location->getText().size() <= 7 ) {
			location->getDropDownList()->setFocus();
			location->getDropDownList()->getDocument().moveToEndOfDoc();
		} else {
			nameInput->setFocus();
			nameInput->getDocument().selectAll();
		}
	} );
}

UIIcon* App::fileIcon( const FileInfo& info ) {
	// Runs on every cell update: the icon depends only on the type, so it is resolved once per
	// type (typeOf returns a reference: no allocation on the way).
	auto& mime = MimeDatabase::instance();
	const std::string& type = mime.typeOf( info );
	auto found = mFileIcons.find( type );
	if ( found == mFileIcons.end() ) {
		UIIcon* icon = mUI->findIcon( mime.iconName( info ) );
		if ( !icon )
			icon = mUI->findIcon( info.isDirectory() ? "folder" : "file" );
#if EE_PLATFORM != EE_PLATFORM_WIN && EE_PLATFORM != EE_PLATFORM_MACOS
		if ( mConfig->files.systemIcons )
			icon = mSystemIcons->getFileIcon( info.name, info.isDirectory(), mime.iconNames( info ),
											  icon );
#endif
		found = mFileIcons.emplace( type, icon ).first;
	}
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS
	// The shell's icons go by extension, not by type: only the fallback is cached here (the
	// theme caches the shell's per extension).
	if ( mConfig->files.systemIcons ) {
		return mSystemIcons->getFileIcon( info.name, info.isDirectory(), mime.iconNames( info ),
										  found->second );
	}
#endif
	return found->second;
}

UIIcon* App::placeIcon( const Place& place ) {
	// eefm's place icons and the standard (freedesktop) names that show the same thing.
	static const std::unordered_map<std::string, std::vector<std::string>> names{
		{ "place-home", { "user-home" } },
		{ "place-desktop", { "user-desktop" } },
		{ "place-documents", { "folder-documents" } },
		{ "place-downloads", { "folder-download" } },
		{ "place-music", { "folder-music" } },
		{ "place-pictures", { "folder-pictures" } },
		{ "place-videos", { "folder-videos" } },
		{ "place-trash", { "user-trash" } },
		{ "place-folder", { "folder" } },
		{ "place-remote", { "folder-remote", "network-server" } },
		{ "place-network", { "network-workgroup", "folder-remote" } },
		{ "device-drive", { "drive-harddisk" } },
		{ "device-root", { "drive-harddisk-root", "drive-harddisk" } },
		{ "device-usb", { "drive-removable-media-usb", "drive-removable-media" } },
		{ "device-phone", { "phone", "smartphone" } },
		{ "action-add-place", { "starred", "bookmarks" } },
	};
	UIIcon* fallback = mUI->findIcon( place.icon );
	if ( !mConfig->files.systemIcons )
		return fallback;
	auto found = names.find( place.icon );
	if ( found == names.end() )
		return fallback;
	// Local places ask for their path: Windows and macOS show special folder and drive icons.
	if ( place.uri.getScheme() == "file" && !place.trash )
		return mSystemIcons->getPathIcon( place.uri.getFSPath(), found->second, fallback );
	return mSystemIcons->getIcon( found->second, fallback );
}

void App::dropExternalFiles() {
	std::vector<URI> files = std::move( mDroppedFiles );
	mDroppedFiles.clear();
	// Where it was dropped (the backend may not report it: then the pointer).
	Vector2f position = mDropPosition ? *mDropPosition : mUI->getEventDispatcher()->getMousePosf();
	mDropPosition.reset();
	Node* over = mUI->overFind( position );
	int target = mActivePane;
	for ( int i = 0; i < 2; ++i ) {
		Pane& p = pane( i );
		if ( over && p.root->getParent() == mPaneSplitter &&
			 ( p.root == over || p.root->inParentTreeOf( over ) ) )
			target = i;
	}
	auto* tab = currentTab( pane( target ) );
	if ( !tab )
		return;
	// Onto a folder: into it (as drops between panes); anywhere else: the pane's folder.
	URI destination = tab->model->uri();
	if ( auto row = tab->view->rowOf( over );
		 row && *row < tab->model->rowCount() && tab->model->entry( *row ).isDirectory() )
		destination = tab->model->entryURI( *row );
	setActivePane( target );
	transferFiles( files, destination, false, target );
}

DrawablePtr App::icon( const std::string& name, int sizeDp ) const {
	UIIcon* found = mUI->findIcon( name );
	return found ? found->getSource( PixelDensity::dpToPxI( sizeDp ) ) : DrawablePtr();
}

void App::addToPlaces( const URI& uri ) {
	Place place;
	place.uri = uri;
	place.name = displayName( uri ).toUtf8();
	place.icon = Places::defaultIcon( uri );
	if ( !mPlaces->add( std::move( place ) ) ) {
		// Already a place: adding it again shows it if it was hidden.
		const auto& places = mPlaces->places();
		for ( size_t i = 0; i < places.size(); ++i )
			if ( places[i].uri == uri )
				mPlaces->setHidden( i, false );
	}
	savePlaces();
}

} // namespace eefm

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	eefm::App app;
	return app.run( argc, argv );
}
