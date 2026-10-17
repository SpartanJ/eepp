#include "panemanager.hpp"
#include "app.hpp"
#include "fileoperations.hpp"
#include "location.hpp"
#include "mime.hpp"
#include "placescontroller.hpp"
#include "terminalpanel.hpp"
#include <ctime>

namespace eefm {

namespace {

std::string sortOrderName( SortOrder order ) {
	return order == SortOrder::Descending ? "descending" : "ascending";
}

const char* PaneLayout = R"xml(
<vbox class="pane" layout_width="0" layout_height="match_parent" min-width="200dp">
	<TabWidget class="pane_tabs" layout_width="match_parent" layout_height="0" layout_weight="1" />
	<hbox class="pane_status_bar" layout_width="match_parent" layout_height="wrap_content">
		<TextView class="pane_status" layout_width="wrap_content" layout_height="wrap_content" />
		<widget layout_width="0" layout_weight="1" layout_height="1dp" />
		<hbox class="pane_transfer" layout_width="wrap_content" layout_height="wrap_content" visible="false">
			<TextView class="transfer_text" layout_gravity="center_vertical" />
			<ProgressBar class="transfer_bar" layout_width="140dp" layout_gravity="center_vertical" display-percent="false" />
			<PushButton class="transfer_cancel tool" icon="action-close" tooltip="Cancel" />
		</hbox>
	</hbox>
	<hbox class="pane_filter_bar" layout_width="match_parent" layout_height="wrap_content">
		<hbox class="filter_frame" layout_width="0" layout_weight="1" layout_height="wrap_content">
			<SelectButton class="filter_lock" icon="filter-unlock" tooltip="Keep Filter When Changing Folders" layout_gravity="center_vertical" />
			<TextInput class="filter_input" layout_width="0" layout_weight="1" layout_height="wrap_content" layout_gravity="center_vertical" hint="Filter..." />
			<SelectButton class="filter_case" icon="filter-case" tooltip="Match Case" layout_gravity="center_vertical" />
			<DropDownList class="filter_mode" layout_width="72dp" layout_gravity="center_vertical" tooltip="Match Mode" />
		</hbox>
		<PushButton class="filter_close" icon="action-close" tooltip="Hide Filter Bar (Ctrl+I)" layout_gravity="center_vertical" />
	</hbox>
</vbox>
)xml";

/** Dolphin-like dates: "5 minute(s) ago", "Today at 12:02", "29/09/2026 at 12:02". */
std::string formatTime( Int64 seconds ) {
	std::time_t now = std::time( nullptr );
	std::time_t when = static_cast<std::time_t>( seconds );
	Int64 diff = static_cast<Int64>( now ) - seconds;
	if ( diff >= 0 && diff < 60 )
		return "Just now";
	if ( diff >= 0 && diff < 3600 )
		return String::format( "%d minute(s) ago", static_cast<int>( diff / 60 ) );
	std::tm local{}, today{};
#if EE_PLATFORM == EE_PLATFORM_WIN
	localtime_s( &local, &when );
	localtime_s( &today, &now );
#else
	localtime_r( &when, &local );
	localtime_r( &now, &today );
#endif
	char clock[16];
	std::strftime( clock, sizeof( clock ), "%H:%M", &local );
	if ( local.tm_year == today.tm_year && local.tm_yday == today.tm_yday )
		return std::string( "Today " ) + clock;
	std::time_t yesterday = now - 86400;
	std::tm yesterdayTm{};
#if EE_PLATFORM == EE_PLATFORM_WIN
	localtime_s( &yesterdayTm, &yesterday );
#else
	localtime_r( &yesterday, &yesterdayTm );
#endif
	if ( local.tm_year == yesterdayTm.tm_year && local.tm_yday == yesterdayTm.tm_yday )
		return std::string( "Yesterday " ) + clock;
	// Short, as Dolphin: the year in two digits.
	char date[32];
	std::strftime( date, sizeof( date ), "%d/%m/%y", &local );
	return std::string( date ) + " " + clock;
}

std::string itemCount( Uint64 count ) {
	return count == 1 ? std::string( "1 item" ) : String::toString( count ) + " items";
}

} // namespace

PaneManager::PaneManager( App& app ) : mApp( app ), mUI( app.ui() ) {}

PaneManager::~PaneManager() = default;

void PaneManager::clearTabs() {
	for ( auto& pane : mPanes ) {
		if ( pane )
			pane->tabList.clear();
	}
}

void PaneManager::loadState( const nlohmann::json& state ) {
	mFilterCaseSensitive = state.value( "filter_case_sensitive", false );
	std::string mode = state.value( "filter_mode", "glob" );
	mFilterMode = mode == "text"	? VFSDirectoryModel::NameFilterMode::PlainText
				  : mode == "regex" ? VFSDirectoryModel::NameFilterMode::RegEx
									: VFSDirectoryModel::NameFilterMode::Glob;
	if ( state.contains( "view" ) && state["view"].is_object() )
		mViewState = state["view"];
}

void PaneManager::saveState( nlohmann::json& state ) {
	state["filter_case_sensitive"] = mFilterCaseSensitive;
	state["filter_mode"] = mFilterMode == VFSDirectoryModel::NameFilterMode::PlainText ? "text"
						   : mFilterMode == VFSDirectoryModel::NameFilterMode::RegEx   ? "regex"
																					   : "glob";
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
}

void PaneManager::createPane( int index, UIWidget* parent ) {
	auto owned = std::make_unique<Pane>();
	Pane& p = *owned;
	p.index = index;
	p.root = mUI->loadLayoutFromString( PaneLayout, parent )->asType<UIWidget>();
	p.tabs = p.root->findByClass<UITabWidget>( "pane_tabs" );
	p.status = p.root->findByClass<UITextView>( "pane_status" );
	// The status bar counts as empty space for the context menu (as Dolphin): with a full,
	// scrolled list it is the only place to reach Create New, Paste...
	for ( UIWidget* part : { p.root->findByClass<UIWidget>( "pane_status_bar" ),
							 static_cast<UIWidget*>( p.status ) } ) {
		part->on( Event::MouseClick, [this, index]( const Event* event ) {
			if ( !( static_cast<const MouseEvent*>( event )->getFlags() & EE_BUTTON_RMASK ) )
				return;
			setActivePane( index );
			if ( auto* tab = currentTab( pane( index ) ) )
				mApp.fileOps().showContextMenu( tab, true );
		} );
	}
	p.transferBox = p.root->findByClass( "pane_transfer" );
	p.transferBar = p.root->findByClass<UIProgressBar>( "transfer_bar" );
	p.transferText = p.root->findByClass<UITextView>( "transfer_text" );
	p.filterBar = p.root->findByClass( "pane_filter_bar" );
	p.filterInput = p.root->findByClass<UITextInput>( "filter_input" );
	p.filterLock = p.root->findByClass<UIPushButton>( "filter_lock" );
	p.filterCase = p.root->findByClass<UIPushButton>( "filter_case" );
	p.filterMode = p.root->findByClass<UIDropDownList>( "filter_mode" );
	p.location.box = mUI->find<UIWidget>( "location_" + String::toString( index ) );
	p.location.input = p.location.box->findByClass<UITextInput>( "location_input" );

	p.tabs->setTabsClosable( true );
	p.tabs->setHideTabBarOnSingleTab( false );
	p.tabs->setAllowRearrangeTabs( true );
	// Tabs can be dragged to the other pane (dragged off the tab bar, then dropped there).
	p.tabs->setAllowDragAndDropTabs( true );
	p.tabs->setAcceptsDropOfWidgetFn( [this, index]( const UIWidget* widget ) {
		const auto* tab = widget->asConstType<UITab>();
		return mApp.isSplit() && tab->getTabWidget() == this->pane( 1 - index ).tabs;
	} );
	p.tabs->on( Event::OnTabSelected, [this, index]( const Event* ) {
		Pane& pane = this->pane( index );
		updateChrome( pane );
		if ( auto* tab = currentTab( pane ) ) {
			auto* filter = pane.filterInput;
			if ( filter->getText().toUtf8() != tab->filter )
				filter->setText( String::fromUtf8( tab->filter ) );
		}
	} );
	p.tabs->on( Event::OnTabClosed, [this, index]( const Event* event ) {
		Pane& pane = this->pane( index );
		const auto* tabEvent = static_cast<const TabEvent*>( event );
		auto it = std::find_if( pane.tabList.begin(), pane.tabList.end(),
								[&]( const auto& tab ) { return tab->tab == tabEvent->getTab(); } );
		// A tab that is not closing moves to the other pane: it is kept for OnTabAdded there.
		bool moving = it != pane.tabList.end() && !tabEvent->getTab()->isClosing();
		if ( moving )
			mMovingTab = std::move( *it );
		if ( it != pane.tabList.end() )
			pane.tabList.erase( it );
		positionNewTabButton( pane );
		if ( !pane.tabList.empty() )
			return;
		// Moving its last tab out leaves the pane on the same folder, in a new tab.
		if ( moving ) {
			openTab( pane, mMovingTab->model->uri(), true );
			return;
		}
		// The last tab of a pane closes the pane (or the window when it is the only one).
		if ( index == 1 ) {
			mApp.setSplit( false );
			openTab( pane, URI::fromFSPath( Sys::getUserDirectory() ), true );
		} else if ( mApp.isSplit() ) {
			Pane& right = this->pane( 1 );
			for ( auto& tab : right.tabList )
				openTab( pane, tab->model->uri(), false );
			mApp.setSplit( false );
			if ( pane.tabs->getTabCount() > 0 )
				pane.tabs->setTabSelected( 0u );
		} else {
			mApp.quit();
		}
	} );
	p.tabs->on( Event::OnTabAdded, [this, index]( const Event* event ) {
		Pane& pane = this->pane( index );
		const auto* tabEvent = static_cast<const TabEvent*>( event );
		if ( mMovingTab && mMovingTab->tab == tabEvent->getTab() ) {
			mMovingTab->pane = &pane;
			pane.tabList.push_back( std::move( mMovingTab ) );
			setActivePane( index );
		}
		positionNewTabButton( pane );
	} );
	p.tabs->getTabBar()->on( Event::OnSizeChange, [this, index]( const Event* ) {
		positionNewTabButton( this->pane( index ) );
	} );
	// Dolphin opens a tab with a double click on the empty tab bar.
	p.tabs->getTabBar()->on( Event::MouseDoubleClick, [this, index]( const Event* ) {
		Pane& pane = this->pane( index );
		auto* tab = currentTab( pane );
		openTab( pane, tab ? tab->model->uri() : URI::fromFSPath( Sys::getUserDirectory() ) );
	} );
	p.newTabButton = UIPushButton::New();
	p.newTabButton->setParent( p.tabs->getTabBar() );
	p.newTabButton->addClass( "new_tab" );
	p.newTabButton->setIcon( mApp.icon( "action-new-tab" ) );
	p.newTabButton->setTooltipText( "New Tab (Ctrl+T)" );
	p.newTabButton->onClick( [this, index]( const MouseEvent* ) {
		Pane& pane = this->pane( index );
		setActivePane( index );
		auto* tab = currentTab( pane );
		openTab( pane, tab ? tab->model->uri() : URI::fromFSPath( Sys::getUserDirectory() ) );
	} );

	// Location bar
	auto* frame = p.location.box->findByClass( "location_frame" );
	if ( auto* image = frame->findByClass<UIImage>( "location_icon" ) )
		image->setDrawable( mApp.icon( "mime-folder", 14 ) );
	p.location.input->on( Event::OnPressEnter, [this, index]( const Event* ) {
		Pane& pane = this->pane( index );
		if ( auto* tab = currentTab( pane ) ) {
			navigate( tab, Location::parse( pane.location.input->getText().toUtf8() ) );
			tab->view->setFocus();
		}
	} );
	p.location.input->on( Event::OnFocus,
						  [this, index]( const Event* ) { setActivePane( index ); } );
	frame->findByClass( "location_clear" )->onClick( [this, index]( const MouseEvent* ) {
		auto* input = this->pane( index ).location.input;
		input->setText( "" );
		input->setFocus();
	} );
	frame->findByClass( "location_history" )->onClick( [this, index]( const MouseEvent* ) {
		showLocationHistory( this->pane( index ) );
	} );
	auto* accept = UIPushButton::New();
	accept->setParent( p.location.box );
	accept->addClass( "tool" );
	accept->setIcon( mApp.icon( "action-accept" ) );
	accept->setTooltipText( "Go" );
	accept->setLayoutGravity( UI_VALIGN_CENTER );
	accept->onClick( [this, index]( const MouseEvent* ) {
		Pane& pane = this->pane( index );
		if ( auto* tab = currentTab( pane ) )
			navigate( tab, Location::parse( pane.location.input->getText().toUtf8() ) );
	} );

	// Transfers
	p.root->findByClass( "transfer_cancel" )->onClick( [this, index]( const MouseEvent* ) {
		mApp.fileOps().cancelTransfers( index );
	} );

	// Filter bar
	p.filterMode->getListBox()->addListBoxItems(
		{ mUI->i18n( "eefm_filter_text_short", "Text" ),
		  mUI->i18n( "eefm_filter_glob_short", "Glob" ),
		  mUI->i18n( "eefm_filter_regex_short", "Regex" ) } );
	p.filterMode->getListBox()->setSelected( static_cast<Uint32>( mFilterMode ) );
	p.filterMode->on( Event::OnItemSelected, [this, index]( const Event* ) {
		Uint32 selected = this->pane( index ).filterMode->getListBox()->getItemSelectedIndex();
		if ( selected > 2 )
			return;
		mFilterMode = static_cast<VFSDirectoryModel::NameFilterMode>( selected );
		for ( auto& pane : mPanes ) {
			if ( pane->filterMode->getListBox()->getItemSelectedIndex() != selected )
				pane->filterMode->getListBox()->setSelected( selected );
			for ( auto& tab : pane->tabList )
				applyFilter( tab.get() );
		}
	} );
	static_cast<UISelectButton*>( p.filterCase )->setSelected( mFilterCaseSensitive );
	p.filterCase->onClick( [this]( const MouseEvent* ) {
		mFilterCaseSensitive = !mFilterCaseSensitive;
		for ( auto& pane : mPanes ) {
			static_cast<UISelectButton*>( pane->filterCase )->setSelected( mFilterCaseSensitive );
			for ( auto& tab : pane->tabList )
				applyFilter( tab.get() );
		}
	} );
	p.filterLock->onClick( [this, index]( const MouseEvent* ) {
		Pane& pane = this->pane( index );
		pane.filterLocked = !pane.filterLocked;
		static_cast<UISelectButton*>( pane.filterLock )->setSelected( pane.filterLocked );
		pane.filterLock->setIcon(
			mApp.icon( pane.filterLocked ? "filter-lock" : "filter-unlock" ) );
	} );
	p.filterInput->on( Event::OnTextChanged, [this, index]( const Event* ) {
		Pane& pane = this->pane( index );
		if ( auto* tab = currentTab( pane ) ) {
			tab->filter = pane.filterInput->getText().toUtf8();
			applyFilter( tab );
		}
	} );
	p.filterInput->on( Event::OnFocus, [this, index]( const Event* ) { setActivePane( index ); } );
	// Escape returns to the files (the filter and the selection stay).
	p.filterInput->on( Event::KeyDown, [this, index]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() != KEY_ESCAPE )
			return;
		if ( auto* tab = currentTab( pane( index ) ) )
			tab->view->setFocus();
	} );
	p.root->findByClass( "filter_close" )->onClick( [this]( const MouseEvent* ) {
		mApp.setFilterBarVisible( false );
	} );

	mPanes[index] = std::move( owned );
}

FileTab* PaneManager::currentTab( Pane& pane ) {
	UITab* selected = pane.tabs->getTabSelected();
	for ( auto& tab : pane.tabList ) {
		if ( tab->tab == selected )
			return tab.get();
	}
	return nullptr;
}

FileTab* PaneManager::findTab( UITab* uitab ) {
	for ( auto& pane : mPanes ) {
		for ( auto& tab : pane->tabList ) {
			if ( tab->tab == uitab )
				return tab.get();
		}
	}
	return nullptr;
}

void PaneManager::setActivePane( int index ) {
	if ( index == 1 && !mApp.isSplit() )
		index = 0;
	mActivePane = index;
	for ( int i = 0; i < 2; ++i ) {
		Pane& p = pane( i );
		if ( i == index ) {
			p.root->addClass( "active" );
			p.location.box->addClass( "active" );
		} else {
			p.root->removeClass( "active" );
			p.location.box->removeClass( "active" );
		}
	}
	updateChrome( activePane() );
}

void PaneManager::positionNewTabButton( Pane& target ) {
	if ( !target.newTabButton )
		return;
	// Deferred: tabs are laid out after the event that triggered this.
	target.newTabButton->runOnMainThread( [this, index = target.index] {
		Pane& p = pane( index );
		Float x = 0;
		for ( Uint32 i = 0; i < p.tabs->getTabCount(); ++i ) {
			UITab* tab = p.tabs->getTab( i );
			x = std::max( x, tab->getPixelsPosition().x + tab->getPixelsSize().getWidth() );
		}
		Float barHeight = p.tabs->getTabBar()->getPixelsSize().getHeight();
		Float height = p.newTabButton->getPixelsSize().getHeight();
		p.newTabButton->setPixelsPosition( x + PixelDensity::dpToPx( 2 ),
										   std::max( 0.f, ( barHeight - height ) * 0.5f ) );
		p.newTabButton->toFront();
	} );
}

// ---------------------------------------------------------------------------------------------
// Tabs
// ---------------------------------------------------------------------------------------------

FileTab* PaneManager::openTab( Pane& pane, const URI& uri, bool select ) {
	// New tabs inherit the columns, widths and sorting of the tab they were opened from.
	if ( auto* current = currentTab( pane ) )
		rememberViewState( current );
	auto tab = std::make_unique<FileTab>();
	FileTab* raw = tab.get();
	raw->pane = &pane;
	VFSDirectoryModel::Config config;
	config.vfs = mApp.vfs();
	config.showHidden = mApp.showHidden();
	raw->model = VFSDirectoryModel::New( mApp.executor(), config );
	raw->model->setIconProvider( [this]( const VFSFileInfo& info ) -> Variant {
		UIIcon* icon = mApp.fileIcon( info );
		return icon ? Variant( icon ) : Variant();
	} );
	raw->model->setTypeProvider(
		[]( const VFSFileInfo& info ) { return MimeDatabase::instance().describe( info ); } );
	raw->model->setTimeFormatter( formatTime );
	raw->model->setItemCountFormatter( itemCount );
	raw->model->setOnStateChanged( [this, raw] { onTabStateChanged( raw ); } );
	raw->model->setOnRename( [this, raw]( const VFSFileInfo& entry, const std::string& name ) {
		mApp.fileOps().renameEntry( raw, entry, name );
	} );

	raw->view = FileTableView::New();
	auto* view = raw->view;
	view->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	view->setModel( raw->model );
	view->setSelectionType( UIAbstractView::SelectionType::Row );
	view->setSelectionKind( UIAbstractView::SelectionKind::Multiple );
	view->setAutoColumnsWidth( true );
	view->setMainColumn( VFSDirectoryModel::Name );
	view->setFitAllColumnsToWidget( true );
	view->setRowSearchByName( true );
	view->setSingleClickNavigation( mApp.config().files.singleClick );
	// Dragging from the empty area selects with a rectangle, as Dolphin.
	view->setRubberBandSelection( true );
	// Same size as the other icons: glyphs are not scaled to fit (see the stylesheet).
	view->setIconSize( PixelDensity::dpToPxI( 16 ) );
	view->setColumnReorderingEnabled( true );
	// Like Dolphin, the name keeps most of the width: the other columns are capped to a share
	// of the view (they still use less when their content is narrower).
	auto capColumns = [view] {
		Float width = view->getPixelsSize().getWidth();
		if ( width <= 0 )
			return;
		view->setColumnMaxWidth( VFSDirectoryModel::Size,
								 eemax( eefloor( width * 0.14f ), PixelDensity::dpToPx( 64 ) ) );
		view->setColumnMaxWidth( VFSDirectoryModel::Modified, eefloor( width * 0.27f ) );
		view->setColumnMaxWidth( VFSDirectoryModel::Type, eefloor( width * 0.18f ) );
	};
	view->on( Event::OnSizeChange, [capColumns]( const Event* ) { capColumns(); } );
	view->onModelEvent( [this, raw]( const ModelEvent* event ) {
		setActivePane( raw->pane->index );
		if ( event->getModelEventType() == ModelEventType::Open ) {
			mApp.fileOps().openRow( raw, event->getModelIndex().row() );
		} else if ( event->getModelEventType() == ModelEventType::OpenMenu ) {
			// Right-clicking an unselected row selects it first.
			if ( !raw->view->getSelection().contains( event->getModelIndex() ) )
				raw->view->getSelection().set( event->getModelIndex() );
			mApp.fileOps().showContextMenu( raw, false );
		}
	} );
	// Middle click opens a folder in a new tab, in the background (as Dolphin and browsers);
	// with the modifier (Ctrl, Cmd on macOS) the new tab is selected and focused.
	view->onRowMiddleClick = [this, raw]( size_t row ) {
		setActivePane( raw->pane->index );
		if ( row >= raw->model->rowCount() || !raw->model->entry( row ).isDirectory() )
			return;
		bool select =
			mApp.window()->getInput()->getSanitizedModState() & KeyMod::getDefaultModifier();
		FileTab* tab = openTab( *raw->pane, raw->model->entryURI( row ), select );
		if ( select && tab )
			tab->view->setFocus();
	};
	view->setOnSelectionChange( [this, raw] {
		if ( raw->pane && raw == currentTab( *raw->pane ) )
			updateStatus( *raw->pane );
	} );
	view->onBackgroundMenu = [this, raw] {
		setActivePane( raw->pane->index );
		mApp.fileOps().showContextMenu( raw, true );
	};
	view->dragPayload = [this, raw] { return raw->selectedURIs(); };
	view->onDragStarted = [this] {
		if ( auto* panel = mApp.places().panel() )
			panel->trackDrag();
	};
	view->isFolderRow = [raw]( size_t row ) {
		return row < raw->model->rowCount() && raw->model->entry( row ).isDirectory();
	};
	view->onFilesDropped = [this, raw]( std::optional<size_t> row ) {
		URI destination = raw->model->uri();
		if ( row && *row < raw->model->rowCount() && raw->model->entry( *row ).isDirectory() )
			destination = raw->model->entryURI( *row );
		mApp.fileOps().dropFiles( FileTableView::currentDrag().uris, destination,
								  raw->pane->index );
	};
	view->on( Event::OnFocus, [this, raw]( const Event* ) { setActivePane( raw->pane->index ); } );

	raw->tab = pane.tabs->add( Location::displayName( uri ), view, mApp.icon( "mime-folder" ) );
	// The new tab button would sit over a dragged tab: hidden while one moves (a press alone
	// starts a drag, so it waits for the movement), back in place after.
	raw->tab->on( Event::OnPositionChange, [this, raw]( const Event* ) {
		if ( raw->tab->isDragging() && raw->pane->newTabButton )
			raw->pane->newTabButton->setVisible( false );
	} );
	raw->tab->on( Event::OnDragStop, [this, raw]( const Event* ) {
		if ( !raw->pane->newTabButton )
			return;
		raw->pane->newTabButton->setVisible( true );
		positionNewTabButton( *raw->pane );
	} );
	// Files can be dropped on a tab: they go to that tab's folder.
	raw->tab->on( Event::OnNodeDropped, [this, raw]( const Event* event ) {
		const auto* drop = static_cast<const DropEvent*>( event );
		if ( drop->getDroppedNode() && drop->getDroppedNode()->isType( FileTableView::CellType ) ) {
			mApp.fileOps().dropFiles( FileTableView::currentDrag().uris, raw->model->uri(),
									  raw->pane->index );
		}
	} );
	pane.tabList.push_back( std::move( tab ) );
	applyViewState( raw );
	if ( select || pane.tabs->getTabCount() == 1 )
		pane.tabs->setTabSelected( raw->tab );
	navigate( raw, uri );
	return raw;
}

void PaneManager::closeTab( FileTab* tab ) {
	if ( tab && tab->pane )
		tab->pane->tabs->removeTab( tab->tab );
}

void PaneManager::updateTabTitle( FileTab* tab ) {
	tab->tab->setText( Location::displayName( tab->model->uri() ) );
	tab->tab->setTooltipText( Location::text( tab->model->uri() ) );
	positionNewTabButton( *tab->pane );
}

// ---------------------------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------------------------

void PaneManager::navigate( FileTab* tab, URI uri, bool recordHistory ) {
	if ( uri.getPath().empty() )
		uri.setRawPath( "/" );
	if ( recordHistory ) {
		if ( !tab->history.empty() && tab->history[tab->historyIndex] == uri ) {
			tab->model->refresh();
			return;
		}
		if ( !tab->history.empty() )
			tab->history.resize( tab->historyIndex + 1 );
		tab->history.push_back( uri );
		tab->historyIndex = tab->history.size() - 1;
	}
	// Folder item counts list every subfolder: only where that is cheap.
	tab->model->setCountDirectoryItems( mApp.config().files.countFolderItems &&
										uri.getScheme() == "file" );
	if ( !tab->pane->filterLocked && !tab->filter.empty() ) {
		tab->filter.clear();
		if ( tab == currentTab( *tab->pane ) )
			tab->pane->filterInput->setText( "" );
	}
	applyFilter( tab );
	tab->model->setURI( uri );
	updateTabTitle( tab );
	if ( tab == currentTab( *tab->pane ) )
		updateChrome( *tab->pane );
}

void PaneManager::goBack() {
	auto* tab = currentTab();
	if ( tab && tab->historyIndex > 0 ) {
		tab->selectLater( tab->model->uri().getLastPathSegment() );
		--tab->historyIndex;
		navigate( tab, tab->history[tab->historyIndex], false );
	}
}

void PaneManager::goForward() {
	auto* tab = currentTab();
	if ( tab && tab->historyIndex + 1 < tab->history.size() ) {
		++tab->historyIndex;
		navigate( tab, tab->history[tab->historyIndex], false );
	}
}

void PaneManager::goUp() {
	auto* tab = currentTab();
	if ( !tab )
		return;
	URI parent = tab->model->uri().getParent();
	if ( parent != tab->model->uri() ) {
		// Like Dolphin, the folder we leave stays selected in its parent.
		tab->selectLater( tab->model->uri().getLastPathSegment() );
		navigate( tab, parent );
	}
}

void PaneManager::goHome() {
	if ( auto* tab = currentTab() )
		navigate( tab, URI::fromFSPath( Sys::getUserDirectory() ) );
}

void PaneManager::showLocationHistory( Pane& pane ) {
	auto* tab = currentTab( pane );
	if ( !tab || tab->history.empty() )
		return;
	UIPopUpMenu* menu = UIPopUpMenu::New();
	// Most recent first, without duplicates.
	std::vector<size_t> shown;
	for ( size_t i = tab->history.size(); i-- > 0; ) {
		bool duplicate = false;
		for ( size_t other : shown )
			duplicate = duplicate || tab->history[other] == tab->history[i];
		if ( duplicate )
			continue;
		shown.push_back( i );
		auto* item = menu->add( Location::text( tab->history[i] ), mApp.icon( "mime-folder" ) );
		item->setData( i );
		if ( i == tab->historyIndex )
			item->addClass( "current" );
		if ( shown.size() >= 20 )
			break;
	}
	menu->on( Event::OnItemClicked, [this, tab]( const Event* event ) {
		size_t index = static_cast<size_t>( event->getNode()->getData() );
		if ( index < tab->history.size() ) {
			tab->historyIndex = index;
			navigate( tab, tab->history[index], false );
		}
	} );
	UIWidget* box = pane.location.box;
	Vector2f pos = box->getScreenPos();
	pos.y += box->getPixelsSize().getHeight();
	menu->showAtScreenPosition( pos );
}

// ---------------------------------------------------------------------------------------------
// Chrome, status and filter
// ---------------------------------------------------------------------------------------------

void PaneManager::updateChrome( Pane& pane ) {
	auto* tab = currentTab( pane );
	if ( !tab )
		return;
	const URI& uri = tab->model->uri();
	if ( !pane.location.input->hasFocus() )
		pane.location.input->setText( Location::text( uri ) );
	updateStatus( pane );
	if ( pane.index != mActivePane )
		return;
	mUI->find<UIWidget>( "back" )->setEnabled( tab->historyIndex > 0 );
	mUI->find<UIWidget>( "forward" )->setEnabled( tab->historyIndex + 1 < tab->history.size() );
	mUI->find<UIWidget>( "up" )->setEnabled( uri.getParent() != uri );
	mApp.window()->setTitle( Location::text( uri ) + " — eefm" );
	if ( auto* panel = mApp.places().panel() )
		panel->setCurrent( uri );
	mApp.terminal().syncDirectory();
}

void PaneManager::onTabStateChanged( FileTab* tab ) {
	if ( !tab->selectWhenListed.empty() ) {
		if ( auto row = tab->model->rowOf( tab->selectWhenListed ) ) {
			tab->view->setSelection( tab->model->index( *row ) );
			tab->selectWhenListed.clear();
		} else if ( tab->model->loadState() != VFSDirectoryModel::LoadState::Loading &&
					tab->selectWhenListedSince.getElapsedTime() > Seconds( 5 ) ) {
			tab->selectWhenListed.clear();
		}
	}
	if ( tab == currentTab( *tab->pane ) )
		updateStatus( *tab->pane );
}

void PaneManager::applyFilter( FileTab* tab ) {
	bool changed = tab->filter != tab->model->nameFilter() ||
				   mFilterMode != tab->model->nameFilterMode() ||
				   mFilterCaseSensitive != tab->model->nameFilterCaseSensitive();
	// The view keeps the selection by row, which shifts when the filter changes: remember the
	// selected entries by name and select those that are still listed.
	std::vector<std::string> selected;
	if ( changed ) {
		for ( size_t row : tab->selectedRows() )
			selected.push_back( tab->model->entry( row ).name );
		tab->view->getSelection().clear();
	}
	tab->model->setNameFilter( tab->filter, mFilterMode, mFilterCaseSensitive );
	std::vector<ModelIndex> visible;
	for ( const auto& name : selected ) {
		if ( auto row = tab->model->rowOf( name ) )
			visible.push_back( tab->model->index( static_cast<int>( *row ) ) );
	}
	if ( !visible.empty() )
		tab->view->getSelection().set( visible );
	if ( tab == currentTab( *tab->pane ) ) {
		// The frame around the input draws the border.
		auto* frame = tab->pane->filterInput->getParent()->asType<UIWidget>();
		if ( tab->model->isNameFilterValid() )
			frame->removeClass( "invalid" );
		else
			frame->addClass( "invalid" );
		updateStatus( *tab->pane );
	}
}

void PaneManager::updateStatus( Pane& pane ) {
	auto* tab = currentTab( pane );
	if ( !tab )
		return;
	auto& model = *tab->model;
	String status;
	if ( model.loadState() == VFSDirectoryModel::LoadState::Error ) {
		const VFSError& error = *model.error();
		std::string text = error.message.empty() ? error.toString() : error.message;
		// A server that stopped answering may come back.
		if ( error.is( VFSErrorCode::TimedOut ) || error.is( VFSErrorCode::ConnectionLost ) )
			text += " (F5 to retry)";
		status = String::fromUtf8( text );
	} else {
		Uint64 folders = 0, files = 0, bytes = 0;
		Uint64 selectedFolders = 0, selectedFiles = 0, selectedBytes = 0;
		for ( size_t row = 0; row < model.rowCount(); ++row ) {
			const VFSFileInfo& info = model.entry( row );
			bool selected = tab->view->getSelection().containsRow( static_cast<int>( row ) );
			if ( info.isDirectory() ) {
				++folders;
				selectedFolders += selected;
			} else {
				++files;
				bytes += info.size().value_or( 0 );
				if ( selected ) {
					++selectedFiles;
					selectedBytes += info.size().value_or( 0 );
				}
			}
		}
		auto describe = []( Uint64 dirs, Uint64 regular ) {
			std::string text;
			if ( dirs )
				text = std::to_string( dirs ) + ( dirs == 1 ? " folder" : " folders" );
			if ( regular ) {
				text += ( text.empty() ? "" : ", " ) + std::to_string( regular ) +
						( regular == 1 ? " file" : " files" );
			}
			return text.empty() ? std::string( "Empty folder" ) : text;
		};
		std::string text;
		if ( selectedFolders + selectedFiles > 0 ) {
			text = describe( selectedFolders, selectedFiles ) + " selected";
			if ( selectedFiles ) {
				text +=
					" (" + FileSystem::sizeToString( static_cast<Int64>( selectedBytes ) ) + ")";
			}
		} else {
			text = describe( folders, files );
			if ( files )
				text += " (" + FileSystem::sizeToString( static_cast<Int64>( bytes ) ) + ")";
		}
		if ( model.loadState() == VFSDirectoryModel::LoadState::Loading )
			text = "Loading... " + text;
		size_t filtered = model.filteredOutCount();
		if ( filtered )
			text += ", " + std::to_string( filtered ) + " hidden by the filter";
		auto watch = model.watchStatus();
		if ( watch && *watch == WatchStatus::Polling )
			text += " - polling for changes";
		else if ( watch && *watch == WatchStatus::Reconnecting )
			text += " - reconnecting watcher";
		status = String::fromUtf8( text );
	}
	pane.status->setText( status );
}

void PaneManager::rememberViewState( FileTab* tab ) {
	if ( !tab )
		return;
	nlohmann::json view = nlohmann::json::object();
	// Automatic widths follow the view size; only widths set by the user are remembered.
	if ( !tab->view->getAutoColumnsWidth() )
		view["columns"] = tab->view->serializeColumnWidths();
	nlohmann::json hidden = nlohmann::json::array();
	for ( size_t col = 0; col < VFSDirectoryModel::ColumnCount; ++col ) {
		if ( tab->view->isColumnHidden( col ) )
			hidden.push_back( col );
	}
	view["hidden"] = hidden;
	view["order"] = tab->view->getColumnOrder();
	view["sort_column"] = tab->model->keyColumn();
	view["sort_order"] = sortOrderName( tab->model->sortOrder() );
	mViewState = std::move( view );
}

void PaneManager::applyViewState( FileTab* tab ) {
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
		for ( const auto& value : mViewState["order"] ) {
			if ( value.is_number() )
				order.push_back( value.get<size_t>() );
		}
		view->setColumnOrder( std::move( order ) );
	}
	if ( mViewState.contains( "columns" ) ) {
		view->setAutoColumnsWidth( false );
		view->unserializeColumnWidths( mViewState["columns"] );
	}
	size_t column = mViewState.value( "sort_column", 0 );
	if ( column < VFSDirectoryModel::ColumnCount ) {
		view->sortByColumn( column, mViewState.value( "sort_order", "ascending" ) == "descending"
										? SortOrder::Descending
										: SortOrder::Ascending );
	}
}

void PaneManager::restoreTabs( const nlohmann::json& state,
							   const std::vector<std::string>& locations, bool forceSplit ) {
	URI home = URI::fromFSPath( Sys::getUserDirectory() );
	bool split = mApp.isSplit();
	if ( !locations.empty() ) {
		if ( forceSplit && locations.size() >= 2 ) {
			openTab( pane( 0 ), Location::parse( locations[0] ) );
			for ( size_t i = 1; i < locations.size(); ++i )
				openTab( pane( 1 ), Location::parse( locations[i] ) );
			split = true;
		} else {
			for ( const auto& location : locations )
				openTab( pane( 0 ), Location::parse( location ) );
			openTab( pane( 1 ), Location::parse( locations.back() ) );
			split = forceSplit || split;
		}
	} else if ( state.contains( "panes" ) && state["panes"].is_array() ) {
		for ( int i = 0; i < 2 && i < static_cast<int>( state["panes"].size() ); ++i ) {
			const auto& saved = state["panes"][i];
			if ( !saved.contains( "tabs" ) || !saved["tabs"].is_array() )
				continue;
			for ( const auto& uri : saved["tabs"] ) {
				if ( uri.is_string() )
					openTab( pane( i ), URI( uri.get<std::string>() ), false );
			}
			Uint32 current = saved.value( "current", 0u );
			if ( current < pane( i ).tabs->getTabCount() )
				pane( i ).tabs->setTabSelected( current );
		}
	}
	for ( int i = 0; i < 2; ++i ) {
		if ( pane( i ).tabList.empty() )
			openTab( pane( i ), home );
	}
	mApp.setSplit( split );
	// Start on the left pane, with the keyboard there too: restoring tabs focuses the views it
	// selects (the right pane's last), and the highlight alone does not move the focus.
	setActivePane( 0 );
	if ( auto* tab = currentTab( pane( 0 ) ) )
		tab->view->setFocus();
}

} // namespace eefm
