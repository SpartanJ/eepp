#include "app.hpp"
#include "arkdrop.hpp"
#include "fileoperations.hpp"
#include "icons.hpp"
#include "location.hpp"
#include "mime.hpp"
#include "panemanager.hpp"
#include "placescontroller.hpp"
#include "settingsactions.hpp"
#include "terminalpanel.hpp"
#include <eepp/system/applications.hpp>

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
/* The inactive pane's view is a little dimmer than the active one, in both color schemes. */
:root {
	--pane-inactive-back: #1f2225;
}
@media (prefers-color-scheme: light) {
	:root {
		--pane-inactive-back: #f7f7f7;
	}
}
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
/* The frame draws the hover and focus border; the input itself has none. */
.location_frame:hover,
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
/* The application logo keeps its colors in light, dark and selected menu states. */
#open-in-eefm > Menu::Item::icon {
	tint: white;
}
/* Application images retain their native colors, including hovered and selected rows. */
PopUpMenu Menu::Item > Menu::Item::icon.application_icon_loaded {
	tint: white;
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
/* The same tints as the file rows. */
.place:hover {
	background-color: rgba( var(--primary), 0.12 );
}
.place.selected {
	background-color: rgba( var(--primary), 0.3 );
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
	margin: 3dp 0dp 1dp 22dp;
	border-width: 0dp;
	border-radius: 2dp;
	background-color: rgba( var(--font-hint), 0.2 );
}
/* Calm until the device is nearly full. */
.device_usage ProgressBar::filler {
	background-color: rgba( var(--primary), 0.6 );
	border-radius: 2dp;
}
.device_usage.full ProgressBar::filler {
	background-color: rgba( var(--theme-error), 0.8 );
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
	padding: 2dp 8dp;
	background-color: var(--back);
}
.pane_filter_bar {
	padding: 2dp 4dp 4dp 4dp;
	background-color: var(--back);
}
/* One field, as the location bar: the options sit inside it. */
.filter_frame {
	border-width: 1dp;
	border-color: var(--button-border);
	border-radius: 2dp;
	background-color: var(--list-back);
	margin-right: 2dp;
}
.filter_frame:hover,
.filter_frame:focus-within {
	border-color: var(--primary);
}
.filter_frame.invalid {
	border-color: var(--theme-error);
}
.pane_filter_bar PushButton, .pane_filter_bar SelectButton {
	background-color: transparent;
	border-color: transparent;
	padding: 2dp 4dp;
	icon-size: 14dp;
}
.pane_filter_bar SelectButton:selected {
	background-color: var(--item-hover);
}
.filter_input, .filter_input:focus {
	background-color: transparent;
	border-color: transparent;
	border-width: 0dp;
	padding: 3dp 2dp;
}
.filter_mode, .filter_mode:focus, .filter_mode:hover {
	background-color: transparent;
	border-color: transparent;
}
.new_tab {
	background-color: transparent;
	border-color: transparent;
	padding: 2dp 6dp;
	icon-size: 14dp;
}
/* Unselected tabs keep their close button, dimmed (hiding it would resize the tab). */
Tab:not(:selected) Tab::close {
	foreground-tint: rgba( var(--tab-close), 0.35 );
}
Tab:hover Tab::close {
	foreground-tint: var(--tab-close);
}
/* As specific as the rules above, or they would hide the theme's hover color. */
Tab Tab::close:hover,
Tab:not(:selected) Tab::close:hover {
	foreground-tint: var(--tab-close-hover);
}
/* The focused pane uses the theme's thumb color; the others stay muted. */
ScrollBar::vbutton, ScrollBar::hbutton {
	background-color: rgba( var(--font-hint), 0.45 );
}
.pane:focus-within ScrollBar::vbutton,
.pane:focus-within ScrollBar::hbutton {
	background-color: var(--scrollbar-button);
}
/* Keep Breeze's hover color above the more specific focused-pane rule. */
.pane:focus-within ScrollBar::vslider:hover ScrollBar::vbutton,
.pane:focus-within ScrollBar::vbutton:hover,
.pane:focus-within ScrollBar::hslider:hover ScrollBar::hbutton,
.pane:focus-within ScrollBar::hbutton:hover {
	background-color: var(--primary);
}
.transfer_text {
	margin-right: 6dp;
}
.transfer_bar {
	min-width: 120dp;
	margin-right: 4dp;
}
file_drag_label {
	background-color: var(--menu-back);
	border-width: 1dp;
	border-color: var(--menu-border);
	border-radius: 3dp;
	padding: 4dp 8dp 4dp 6dp;
	color: var(--font);
	/* The rows' tint, explicitly: a folder's tint would stay for the next drag otherwise. */
	tint: var(--font);
}
file_drag_label::icon {
	margin-right: 6dp;
}
/* Every cell and row gets explicit values for what the rules below change: a property no rule
   sets anymore keeps its last value (recycled cells change class, drags end, panes go
   inactive). */
tableview::cell {
	text-align: left;
	opacity: 1;
}
tableview::cell::text {
	text-overflow: ellipsis;
}
tableview::row {
	opacity: 1;
	border-width: 1dp;
	border-color: transparent;
	border-radius: 3dp;
}
/* Alternating rows, as eproc and Dolphin. */
tableview::row:nth-child(even) {
	background-color: var(--list-back-alt);
}
/* The inactive pane's view is a little dimmer; its text stays the same, both panes are read
   during operations. */
.pane:not(.active) TableView,
.pane:not(.active) TableView > ScrollBar,
.pane:not(.active) tableview::row {
	background-color: var(--pane-inactive-back);
}
.pane:not(.active) tableview::row:nth-child(even) {
	background-color: var(--list-back-alt);
}
/* Hover and selection are soft tints of the accent with an outline, as Dolphin. */
tableview::row:hover,
tableview::row:nth-child(even):hover,
.pane:not(.active) tableview::row:hover,
.pane:not(.active) tableview::row:nth-child(even):hover {
	background-color: rgba( var(--primary), 0.12 );
}
tableview::row:selected,
tableview::row:nth-child(even):selected {
	background-color: rgba( var(--primary), 0.3 );
	border-color: var(--primary);
}
/* In the inactive pane the selection is grey: the keyboard goes to the other one. */
.pane:not(.active) tableview::row:selected,
.pane:not(.active) tableview::row:nth-child(even):selected {
	background-color: rgba( var(--font-hint), 0.22 );
	border-color: rgba( var(--font-hint), 0.5 );
}
/* The text is readable on the tint (the theme's selected color is meant for a solid fill). */
tableview::row:selected tableview::cell {
	color: var(--font);
	tint: var(--font);
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
tableview::row:selected tableview::cell.vfs_size,
tableview::row:selected tableview::cell.vfs_modified,
tableview::row:selected tableview::cell.vfs_type,
tableview::row:selected tableview::cell.vfs_permissions,
tableview::row:selected tableview::cell.vfs_owner,
tableview::row:selected tableview::cell.vfs_group {
	color: var(--font-hint);
}
tableview::cell.vfs_size {
	text-align: right;
}
.glyph_icons tableview::cell.vfs_folder,
.glyph_icons file_drag_label.vfs_folder {
	tint: var(--primary);
}
tableview::cell.dragged {
	opacity: 0.5;
}
]]>
</style>
<vbox id="root" layout_width="match_parent" layout_height="match_parent">
	<hbox id="toolbar" layout_width="match_parent" layout_height="wrap_content">
		<PushButton id="back" class="tool" icon="go-previous" />
		<PushButton id="forward" class="tool" icon="go-next" />
		<PushButton id="up" class="tool" icon="go-parent" />
		<PushButton id="refresh" class="tool" icon="view-refresh" />
		<SelectButton id="places_toggle" class="tool" icon="places-panel" />
		<SelectButton id="split" class="tool" icon="view-split" />
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

} // namespace

// ---------------------------------------------------------------------------------------------
// Startup and shutdown
// ---------------------------------------------------------------------------------------------

App::App() = default;

App::~App() = default;

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
	// Producers stop before the executor they deliver to goes away: continuations hold it by
	// reference. Running transfers are cancelled (destroying the VFS waits for them).
	mFileOperations->cancelOperations();
	mPaneManager->clearTabs();
#ifdef EE_VFS_SFTP
	if ( mSSHPrompts )
		mSSHPrompts->shutdown();
#endif
	// The scene is still alive: quit() stops the application loop without destroying it.
	// Detach before waiting for producers, so late completions cannot schedule more UI work.
	mExecutor->shutdown();
#ifdef EE_VFS_SFTP
	if ( auto sftp = std::dynamic_pointer_cast<SFTPProvider>( mVFS->provider( "sftp" ) ) )
		sftp->disconnectAll();
#endif
	VirtualFileSystem::destroyInstance();
	mVFS = nullptr;
	// The UI thread pool belongs to the scene too: work finishing while it is joined finds the
	// executor detached (its functions are dropped), not destroyed.
	mSystemIcons.reset();
	mPool.reset();
	mApp.reset();
	// Widgets retain callbacks into these controllers, including OnWindowClose saving the
	// settings. Keep them alive until the scene and its producers have been destroyed.
	mPlacesController.reset();
	mSettingsActions.reset();
	mTerminalPanel.reset();
	mFileOperations.reset();
	mPaneManager.reset();
	mExecutor.reset();
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
					   Sys::getProcessPath() + "assets/icon/eefm.png" );
	UIApplication::Settings appSettings;
	if ( mConfig->window.pixelDensity > 0 )
		appSettings.pixelDensity = mConfig->window.pixelDensity;
	appSettings.fontHinting = mConfig->font.hinting;
	appSettings.fontAntialiasing = mConfig->font.antialiasing;
	// Shared with the scene: widgets such as the font picker load in the background with it.
	mPool = ThreadPool::createShared( 2 );
	appSettings.threadPool = mPool;
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
	mExecutor = std::make_unique<MainThreadExecutor>( mUI );
	mVFS = &VirtualFileSystem::instance();
	mPaneManager = std::make_unique<PaneManager>( *this );
	mPaneManager->loadState( mState );
	mFileOperations = std::make_unique<FileOperations>( *this );
	mSettingsActions = std::make_unique<SettingsActions>( *this );
	mUI->setColorSchemePreference( mConfig->theme.uiColorScheme );
	mUI->updateWindowTitleBarColor();
	if ( mConfig->font.uiSize > 0 )
		mSettingsActions->applyUIFont( appSettings.baseFont, mConfig->font.uiSize );
	mUI->setSmoothScrollEnabled( mConfig->files.smoothScroll );
	if ( !mConfig->font.fallbackPath.empty() ) {
		auto fallback = FontTrueType::New( "eefm-fallback-font", *mUI->getResourceScope() );
		if ( fallback->loadFromFile( mConfig->font.fallbackPath ) )
			mUI->getResourceScope()->getFontService().addFallbackFont( std::move( fallback ) );
	}
	if ( mState.contains( "window" ) && mState["window"].value( "maximized", false ) )
		mApp->getWindow()->maximize();
	Icons::load( mUI, Sys::getProcessPath() + "assets/", ws.Icon );

	// Theme detection and icon loading run on the pool: startup does not wait for them.
	mSystemIcons = UISystemIconTheme::New( *mPool, *mExecutor );
	mSystemIcons->setOnIconsLoaded( [this] {
		for ( auto* image : mUI->getRoot()->findAllByClass( "application_icon" ) )
			updateApplicationIcon( image->asType<UIImage>() );
		for ( int i = 0; i < PaneManager::Count; ++i ) {
			if ( auto* tab = mPaneManager->currentTab( mPaneManager->pane( i ) ) )
				tab->view->invalidateDraw();
		}
		if ( mPlacesController && mPlacesController->panel() )
			mPlacesController->panel()->refreshIcons();
	} );
#ifdef EE_VFS_SFTP
	mSSHPrompts = std::make_shared<SSHPrompts>( mUI, *mExecutor );
	if ( auto sftp = std::dynamic_pointer_cast<SFTPProvider>( mVFS->provider( "sftp" ) ) ) {
		sftp->setHostKeyVerifier( mSSHPrompts );
		sftp->setAuthenticationHandler( mSSHPrompts );
	}
#endif

	mUI->loadLayoutFromString( Layout );
	updateIconStyle();
	mToolbar = mUI->find<UIWidget>( "toolbar" );
	mMenuButton = mUI->find<UIWidget>( "app_menu" );
	mMainSplitter = mUI->find<UISplitter>( "main_splitter" );
	mTerminalPanel =
		std::make_unique<TerminalPanel>( *this, mUI->find<UISplitter>( "terminal_splitter" ) );
	if ( mState.contains( "terminal_partition" ) && mState["terminal_partition"].is_string() )
		mTerminalPanel->setPartition( mState["terminal_partition"].get<std::string>() );
	mPaneSplitter = mUI->find<UISplitter>( "pane_splitter" );
	mPlacesScroll = mUI->find<UIWidget>( "places_scroll" );
	mParking = mUI->find<UIWidget>( "parking" );

	mPlacesController = std::make_unique<PlacesController>( *this, mConfigDir + "places.json" );
	mPlacesController->createPanel( mPlacesScroll );

	// Pane tooltips resolve their shortcuts while the panes are created.
	registerShortcuts();

	// Panes and their location bars
	for ( int i = 0; i < 2; ++i ) {
		auto* box = mUI->find<UIWidget>( "location_" + String::toString( i ) );
		mUI->loadLayoutFromString( LocationLayout, box );
		mPaneManager->createPane( i, mPaneSplitter );
	}
	if ( mState.contains( "split_partition" ) && mState["split_partition"].is_string() ) {
		mPaneSplitter->setSplitPartition(
			StyleSheetLength( mState["split_partition"].get<std::string>() ) );
	}
	if ( mState.contains( "places_width" ) && mState["places_width"].is_number() ) {
		mMainSplitter->setSplitPartition(
			StyleSheetLength( mState["places_width"].get<Float>(), StyleSheetLength::Dp ) );
	}

	auto onClick = [this]( const char* id, std::function<void()> fn ) {
		mUI->find<UIWidget>( id )->onClick( [fn = std::move( fn )]( const MouseEvent* ) { fn(); } );
	};
	onClick( "back", [this] { mPaneManager->goBack(); } );
	onClick( "forward", [this] { mPaneManager->goForward(); } );
	onClick( "up", [this] { mPaneManager->goUp(); } );
	onClick( "refresh", [this] {
		if ( auto* tab = mPaneManager->currentTab() )
			tab->model->refresh();
	} );
	onClick( "split", [this] { setSplit( !mSplit ); } );
	onClick( "places_toggle", [this] { setPlacesVisible( !mPlacesVisible ); } );
	onClick( "app_menu", [this] { showAppMenu(); } );

	// Keep the location bars aligned with their panes.
	for ( auto& pane : mPaneManager->all() ) {
		pane->root->on( Event::OnSizeChange, [this]( const Event* ) { layoutLocationBars(); } );
		pane->root->on( Event::OnPositionChange, [this]( const Event* ) { layoutLocationBars(); } );
	}
	mToolbar->on( Event::OnSizeChange, [this]( const Event* ) { layoutLocationBars(); } );

	// Drags whose source creates the files where they are dropped. Most use XDS (Engrampa,
	// Xarchiver, browsers saving images); Ark names its own extraction service instead.
	auto dragAndDrop = DragAndDrop::forWindow( *mApp->getWindow() );
	dragAndDrop.setFileSaveDropHandler( [this]( const FileSaveDrop& drop ) {
		return mFileOperations->localDropFolder( drop.position );
	} );
	dragAndDrop.setDataDropHandler( ArkDrop::mimeTypes(), [this]( DataDrop drop ) {
		if ( ArkDrop::isArkDrop( drop ) )
			mFileOperations->dropArchiveEntries( std::move( drop ) );
	} );

	mApp->getWindow()->getInput()->pushCallback( [this]( InputEvent* event ) {
		// Discovery may still be running after a right click. Subsequent input dismisses
		// that request even when it leaves the selection and focused pane unchanged.
		if ( event->Type == InputEvent::MouseButtonDown || event->Type == InputEvent::MouseWheel ||
			 ( event->Type == InputEvent::KeyDown && event->key.keysym.sym == KEY_ESCAPE ) ||
			 ( event->Type == InputEvent::Window &&
			   event->window.type == InputEvent::WindowKeyboardFocusLost ) )
			mFileOperations->cancelContextMenu();
		if ( event->Type == InputEvent::FileDropped ) {
			// Files dropped from other applications: gathered, then copied in one transfer once
			// the events of this frame were delivered (a drop sends one event per file).
			std::string file( event->file.file );
			if ( FileSystem::fileExtension( file ) == "lnk" ) {
				std::string target = Sys::getShortcutTarget( file );
				if ( !target.empty() && FileSystem::fileExists( target ) )
					file = target;
			}
			std::optional<Vector2f> position;
			if ( event->file.hasPosition )
				position = Vector2f( event->file.x, event->file.y );
			mFileOperations->addDroppedFile( URI::fromFSPath( file ), position );
		} else if ( event->Type == InputEvent::EventsSent ) {
			if ( mFileOperations->hasDroppedFiles() )
				mFileOperations->dropExternalFiles();
		} else if ( event->Type == InputEvent::MouseButtonUp ) {
			// Side mouse buttons navigate the history.
			if ( event->button.button == EE_BUTTON_4 )
				mPaneManager->goBack();
			else if ( event->button.button == EE_BUTTON_5 )
				mPaneManager->goForward();
		}
	} );

	mApp->getWindow()->setCloseRequestCallback( [this]( EE::Window::Window* ) {
		quit();
		// Keep the scene and its dispatcher alive until producers and the executor stop.
		return false;
	} );

	// Keyboard focus determines the active pane; scrolling and background operations do not.
	// The keyboard belongs to the files: a click on a pane's bars or empty areas focuses the
	// container under the pointer, which would swallow the keys (typing, arrows, Enter). Hand
	// it to that pane's files on the next frame. Buttons and text fields keep it.
	mUI->getEventDispatcher()->addFocusEventCallback( [this]( const Uint32&, Node* focus, Node* ) {
		// Focus falls back to the scene (or the UI root) when the focused dialog or menu closes:
		// the active pane's files take it back.
		if ( focus && ( !focus->isWidget() || focus == mUI->getRoot() ) ) {
			mUI->runOnMainThread( [this, focus] {
				if ( mUI->getEventDispatcher()->getFocusNode() == focus ) {
					if ( auto* tab = mPaneManager->currentTab() )
						tab->view->setFocus();
				}
			} );
			return;
		}
		if ( !focus || !focus->isWidget() )
			return;
		Pane* pane = mPaneManager->paneForNode( focus );
		if ( !pane )
			return;
		auto* tab = mPaneManager->currentTab( *pane );
		if ( !tab )
			return;
		// Row clicks focus a cell (a push button), not the view itself. Refresh the pane's
		// decorations without replacing the focused control that must receive the click.
		mPaneManager->onFocusChange();
		if ( focus->isType( UI_TYPE_TEXTINPUT ) || tab->view == focus ||
			 tab->view->inParentTreeOf( focus ) )
			return;
		// Buttons and their parts (a tab's close button): the click that follows goes to
		// the focused node, it must stay there.
		for ( Node* node = focus; node; node = node->getParent() ) {
			if ( node->isType( UI_TYPE_PUSHBUTTON ) )
				return;
		}
		mUI->runOnMainThread( [this, i = pane->index, focus] {
			auto* current = mPaneManager->currentTab( mPaneManager->pane( i ) );
			// Only if nothing else took the focus meanwhile (focus is not dereferenced).
			if ( current && mUI->getEventDispatcher()->getFocusNode() == focus )
				current->view->setFocus();
		} );
	} );

	mPaneManager->restoreTabs( mState, locations, forceSplit );
	setPlacesVisible( mPlacesVisible );
	setFilterBarVisible( mFilterBarVisible );
	mPlacesController->refreshDeviceUsage();
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
	mFilterBarVisible = mState.value( "filter_bar", false );
	mShowHidden = mState.value( "show_hidden", false );
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
	mTerminalPanel->rememberSize();
	state["terminal_partition"] = mTerminalPanel->partition();
	state["split"] = mSplit;
	state["places_visible"] = mPlacesVisible;
	state["filter_bar"] = mFilterBarVisible;
	state["show_hidden"] = mShowHidden;
	if ( mPlacesVisible )
		state["places_width"] = PixelDensity::pxToDp( mPlacesScroll->getPixelsSize().getWidth() );
	else if ( mState.contains( "places_width" ) )
		state["places_width"] = mState["places_width"];
	if ( mSplit && mPaneSplitter->getPixelsSize().getWidth() > 0 ) {
		Float ratio = mPaneManager->pane( 0 ).root->getPixelsSize().getWidth() /
					  mPaneSplitter->getPixelsSize().getWidth() * 100.f;
		state["split_partition"] = String::format( "%.2f%%", ratio );
	} else if ( mState.contains( "split_partition" ) ) {
		state["split_partition"] = mState["split_partition"];
	}
	mPaneManager->saveState( state );
	FileSystem::makeDir( mConfigDir, true );
	FileSystem::fileWrite( mConfigDir + "state.json", std::string_view( state.dump( 2 ) ) );
	mPlacesController->saveConfig();
}

void App::quit() {
	saveState();
	mApp->requestQuit();
}

// ---------------------------------------------------------------------------------------------
// Layout toggles
// ---------------------------------------------------------------------------------------------

void App::setSplit( bool split ) {
	mSplit = split;
	Pane& right = mPaneManager->pane( 1 );
	if ( split && right.root->getParent() != mPaneSplitter ) {
		right.root->setParent( mPaneSplitter );
		right.root->setVisible( true );
	} else if ( !split && right.root->getParent() == mPaneSplitter ) {
		right.root->setParent( mParking );
		if ( mPaneManager->activeIndex() == 1 )
			mPaneManager->focusPane( 0 );
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
	for ( auto& pane : mPaneManager->all() ) {
		pane->filterBar->setVisible( visible );
		if ( !visible ) {
			// Closing the bar clears the filter, like Dolphin.
			for ( auto& tab : pane->tabList ) {
				tab->filter.clear();
				mPaneManager->applyFilter( tab.get() );
			}
			pane->filterInput->setText( "" );
		}
	}
}

void App::setShowHidden( bool show ) {
	mShowHidden = show;
	for ( auto& pane : mPaneManager->all() ) {
		for ( auto& tab : pane->tabList )
			tab->model->setShowHidden( show );
		mPaneManager->updateStatus( *pane );
	}
}

void App::layoutLocationBars() {
	if ( !mPaneManager || !mPaneManager->ready() )
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
		UIWidget* left = mPaneManager->pane( 0 ).location.box;
		UIWidget* right = mPaneManager->pane( 1 ).location.box;
		Float leftX = left->getScreenPos().x;
		Float menuX = mMenuButton->getScreenPos().x;
		Float leftWidth;
		if ( mSplit ) {
			Float rightPaneX = mPaneManager->pane( 1 ).root->getScreenPos().x;
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

std::string App::getKeybind( const std::string& command ) const {
	return mUI->getKeyBindings().getCommandKeybindString( command );
}

String App::shortcutTooltip( String text, const std::string& command ) const {
	const auto shortcut = getKeybind( command );
	if ( !shortcut.empty() )
		text += " (" + shortcut + ")";
	return text;
}

void App::showAppMenu() {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->add( mUI->i18n( "eefm_new_tab", "New Tab" ), menuIcon( "action-new-tab" ),
			   getKeybind( "new-tab" ) )
		->setId( "new-tab" );
	if ( mPaneManager->currentTab() &&
		 mPaneManager->currentTab()->model->uri().getScheme() == "file" ) {
		menu->add( mUI->i18n( "eefm_open_terminal", "Open Terminal" ),
				   menuIcon( "action-terminal" ), getKeybind( "terminal" ) )
			->setId( "open-terminal" );
	}
	menu->addSeparator();
	menu->addCheckBox( mUI->i18n( "eefm_show_hidden", "Show Hidden Files" ), mShowHidden,
					   getKeybind( "toggle-hidden" ) )
		->setId( "hidden" );
	menu->addCheckBox( mUI->i18n( "eefm_split_view", "Split View" ), mSplit,
					   getKeybind( "toggle-split" ) )
		->setId( "split" );
	// As Dolphin: the panels are in a submenu.
	UIPopUpMenu* panels = UIPopUpMenu::New();
	panels
		->addCheckBox( mUI->i18n( "eefm_panel_places", "Places" ), mPlacesVisible,
					   getKeybind( "toggle-places" ) )
		->setId( "places" );
	panels
		->addCheckBox( mUI->i18n( "eefm_panel_terminal", "Terminal" ), mTerminalPanel->isVisible(),
					   getKeybind( "toggle-terminal" ) )
		->setId( "terminal" );
	panels->on( Event::OnItemClicked, [this]( const Event* event ) {
		std::string id( event->getNode()->getId() );
		if ( id == "places" )
			setPlacesVisible( !mPlacesVisible );
		else if ( id == "terminal" )
			mTerminalPanel->toggle();
	} );
	menu->addSubMenu( mUI->i18n( "eefm_show_panels", "Show Panels" ), menuIcon( "places-panel" ),
					  panels );
	menu->addCheckBox( mUI->i18n( "eefm_show_filter_bar", "Show Filter Bar" ), mFilterBarVisible,
					   getKeybind( "toggle-filter" ) )
		->setId( "filter" );
	menu->addSeparator();
	menu->add( mUI->i18n( "eefm_configure", "Configure eefm..." ), menuIcon( "settings" ),
			   getKeybind( "settings" ) )
		->setId( "settings" );
	menu->add( mUI->i18n( "eefm_quit", "Quit" ), menuIcon( "quit" ), getKeybind( "quit" ) )
		->setId( "quit" );
	menu->on( Event::OnItemClicked, [this]( const Event* event ) {
		std::string id( event->getNode()->getId() );
		if ( id == "new-tab" ) {
			auto* tab = mPaneManager->currentTab();
			mPaneManager->openTab( mPaneManager->activePane(),
								   tab ? tab->model->uri()
									   : URI::fromFSPath( Sys::getUserDirectory() ) );
		} else if ( id == "hidden" ) {
			setShowHidden( !mShowHidden );
		} else if ( id == "split" ) {
			setSplit( !mSplit );
		} else if ( id == "places" ) {
			setPlacesVisible( !mPlacesVisible );
		} else if ( id == "filter" ) {
			setFilterBarVisible( !mFilterBarVisible );
		} else if ( id == "settings" ) {
			mSettingsActions->showSettings();
		} else if ( id == "open-terminal" ) {
			if ( auto* tab = mPaneManager->currentTab() )
				mFileOperations->openTerminalHere( tab );
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
			if ( auto* tab = mPaneManager->currentTab() )
				fn( tab );
		};
	};
	mUI->setKeyBindingCommand( "new-tab", [this] {
		auto* tab = mPaneManager->currentTab();
		mPaneManager->openTab( mPaneManager->activePane(),
							   tab ? tab->model->uri()
								   : URI::fromFSPath( Sys::getUserDirectory() ) );
	} );
	mUI->setKeyBindingCommand( "close-tab",
							   onTab( [this]( FileTab* tab ) { mPaneManager->closeTab( tab ); } ) );
	mUI->setKeyBindingCommand( "next-tab", [this] {
		Pane& p = mPaneManager->activePane();
		Uint32 count = p.tabs->getTabCount();
		if ( count > 1 )
			p.tabs->setTabSelected( ( p.tabs->getTabSelectedIndex() + 1 ) % count );
	} );
	mUI->setKeyBindingCommand( "previous-tab", [this] {
		Pane& p = mPaneManager->activePane();
		Uint32 count = p.tabs->getTabCount();
		if ( count > 1 )
			p.tabs->setTabSelected( ( p.tabs->getTabSelectedIndex() + count - 1 ) % count );
	} );
	mUI->setKeyBindingCommand( "focus-location", [this] {
		auto* input = mPaneManager->activePane().location.input;
		input->setFocus();
		input->getDocument().selectAll();
	} );
	mUI->setKeyBindingCommand( "toggle-hidden", [this] { setShowHidden( !mShowHidden ); } );
	mUI->setKeyBindingCommand( "toggle-split", [this] { setSplit( !mSplit ); } );
	mUI->setKeyBindingCommand( "toggle-places", [this] { setPlacesVisible( !mPlacesVisible ); } );
	mUI->setKeyBindingCommand( "toggle-filter", [this] {
		if ( !mFilterBarVisible )
			setFilterBarVisible( true );
		mPaneManager->activePane().filterInput->setFocus();
	} );
	mUI->setKeyBindingCommand( "switch-pane", fileCommand( [this] {
								   if ( !mSplit )
									   return;
								   mPaneManager->focusPane( 1 - mPaneManager->activeIndex() );
							   } ) );
	mUI->setKeyBindingCommand( "refresh", onTab( []( FileTab* tab ) { tab->model->refresh(); } ) );
	mUI->setKeyBindingCommand( "go-back", [this] { mPaneManager->goBack(); } );
	mUI->setKeyBindingCommand( "go-forward", [this] { mPaneManager->goForward(); } );
	mUI->setKeyBindingCommand( "go-up", [this] { mPaneManager->goUp(); } );
	mUI->setKeyBindingCommand( "go-home", [this] { mPaneManager->goHome(); } );
	mUI->setKeyBindingCommand( "go-up-file", fileCommand( [this] { mPaneManager->goUp(); } ) );
	mUI->setKeyBindingCommand(
		"new-folder", onTab( [this]( FileTab* tab ) { mFileOperations->newFolder( tab ); } ) );
	mUI->setKeyBindingCommand( "copy",
							   fileCommand( [this] { mFileOperations->copySelection( false ); } ) );
	mUI->setKeyBindingCommand( "cut",
							   fileCommand( [this] { mFileOperations->copySelection( true ); } ) );
	mUI->setKeyBindingCommand( "paste", fileCommand( [this] {
								   if ( auto* tab = mPaneManager->currentTab() )
									   mFileOperations->paste( tab );
							   } ) );
	mUI->setKeyBindingCommand( "select-all", fileCommand( [this] {
								   if ( auto* tab = mPaneManager->currentTab() ) {
									   std::vector<ModelIndex> all;
									   for ( size_t row = 0; row < tab->model->rowCount(); ++row )
										   all.push_back( tab->model->index( row ) );
									   tab->view->getSelection().set( all );
								   }
							   } ) );
	mUI->setKeyBindingCommand( "trash", fileCommand( [this] { mFileOperations->moveToTrash(); } ) );
	mUI->setKeyBindingCommand( "delete",
							   fileCommand( [this] { mFileOperations->deleteSelection(); } ) );
	mUI->setKeyBindingCommand( "rename",
							   fileCommand( [this] { mFileOperations->renameSelection(); } ) );
	mUI->setKeyBindingCommand( "properties", fileCommand( onTab( [this]( FileTab* tab ) {
								   mFileOperations->showProperties( tab );
							   } ) ) );
	mUI->setKeyBindingCommand(
		"terminal", onTab( [this]( FileTab* tab ) { mFileOperations->openTerminalHere( tab ); } ) );
	mUI->setKeyBindingCommand( "toggle-terminal", [this] { mTerminalPanel->toggle(); } );
	mUI->setKeyBindingCommand( "settings", [this] { mSettingsActions->showSettings(); } );
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
		{ "mod+return", "properties" },
		{ "alt+return", "properties" },
		{ "shift+f4", "terminal" },
		{ "f4", "toggle-terminal" },
		{ "mod+shift+,", "settings" },
		{ "mod+q", "quit" },
		{ "mod+shift+f11", "debug-widget-tree-view" },
	} );
	mUI->find<UIWidget>( "back" )->setTooltipText(
		shortcutTooltip( mUI->i18n( "eefm_back", "Back" ), "go-back" ) );
	mUI->find<UIWidget>( "forward" )
		->setTooltipText( shortcutTooltip( mUI->i18n( "eefm_forward", "Forward" ), "go-forward" ) );
	mUI->find<UIWidget>( "up" )->setTooltipText(
		shortcutTooltip( mUI->i18n( "eefm_go_up", "Go Up" ), "go-up" ) );
	mUI->find<UIWidget>( "refresh" )
		->setTooltipText( shortcutTooltip( mUI->i18n( "eefm_refresh", "Reload" ), "refresh" ) );
	mUI->find<UIWidget>( "places_toggle" )
		->setTooltipText( shortcutTooltip( mUI->i18n( "eefm_show_places", "Show Places Panel" ),
										   "toggle-places" ) );
	mUI->find<UIWidget>( "split" )->setTooltipText(
		shortcutTooltip( mUI->i18n( "eefm_split_view", "Split View" ), "toggle-split" ) );
}

void App::updateIconStyle() {
	// eefm's glyph icons are tinted by the stylesheet (folders in the accent color); a system
	// theme's icons keep their own colors.
	bool glyphs = !mConfig->files.systemIcons;
	if ( glyphs )
		mUI->getRoot()->addClass( "glyph_icons" );
	else
		mUI->getRoot()->removeClass( "glyph_icons" );
}

UIIcon* App::fileIcon( const VFSFileInfo& info ) {
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
		if ( mConfig->files.systemIcons ) {
			icon = mSystemIcons->getFileIcon( info.name, info.isDirectory(), mime.iconNames( info ),
											  icon );
		}
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

DrawablePtr App::icon( const std::string& name, int sizeDp ) const {
	UIIcon* found = mUI->findIcon( name );
	return found ? found->getSource( PixelDensity::dpToPxI( sizeDp ) ) : DrawablePtr();
}

DrawablePtr App::menuIcon( const std::string& name ) const {
	return mUI->findIconDrawable( name, menuIconSize() );
}

int App::menuIconSize() const {
	auto* manager = mUI->getUIThemeManager();
	auto* theme = manager->getDefaultTheme();
	return static_cast<Uint32>( theme ? theme->getDefaultFontSize()
									  : manager->getDefaultFontSize() );
}

void App::setApplicationIcon( UIPushButton* item, const Application& application ) {
	UIIcon* fallback = mUI->findIcon( "action-open-with" );
#if EE_PLATFORM == EE_PLATFORM_MACOS
	UIIcon* icon = mSystemIcons->getPathIcon( application.exec, {}, fallback );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	UIIcon* icon = application.icon.empty()
					   ? fallback
					   : mSystemIcons->getIconResource( application.icon, fallback );
#else
	UIIcon* icon = application.icon.empty()
					   ? fallback
					   : mSystemIcons->getIcon( { application.icon }, fallback );
#endif
	auto* image = item->getIcon();
	image->addClass( "application_icon" );
	// The theme owns this icon until shutdown. Binding to the live widget avoids keeping
	// menu pointers in callbacks: closed menus simply disappear from the refresh query.
	image->setData( reinterpret_cast<UintPtr>( icon ) );
	updateApplicationIcon( image );
}

void App::updateApplicationIcon( UIImage* image ) {
	auto* icon = reinterpret_cast<UIIcon*>( image->getData() );
	// Bundle icons have internal padding and finer detail than action glyphs.
	const int size = menuIconSize() + PixelDensity::dpToPxI( 2 );
	const DrawablePtr source = icon ? icon->getSource( size ) : DrawablePtr{};
	image->getParent()->asType<UIPushButton>()->setIcon( source ? source->clone() : DrawablePtr{} );
	if ( source && source->getDrawableType() != Drawable::GLYPH )
		image->addClass( "application_icon_loaded" );
	else
		image->removeClass( "application_icon_loaded" );
}

} // namespace eefm

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	eefm::App app;
	return app.run( argc, argv );
}
