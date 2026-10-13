#include "app.hpp"
#include "mime.hpp"
#include <ctime>

namespace eefm {

namespace {

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
		<SelectButton class="filter_lock" icon="filter-unlock" tooltip="Keep Filter When Changing Folders" layout_gravity="center_vertical" />
		<TextInput class="filter_input" layout_width="0" layout_weight="1" layout_height="wrap_content" hint="Filter..." />
		<SelectButton class="filter_case" icon="filter-case" tooltip="Match Case" layout_gravity="center_vertical" />
		<DropDownList class="filter_mode" layout_width="140dp" layout_gravity="center_vertical" />
		<PushButton class="filter_close" icon="action-close" tooltip="Hide Filter Bar" layout_gravity="center_vertical" />
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
		return std::string( "Today at " ) + clock;
	std::time_t yesterday = now - 86400;
	std::tm yesterdayTm{};
#if EE_PLATFORM == EE_PLATFORM_WIN
	localtime_s( &yesterdayTm, &yesterday );
#else
	localtime_r( &yesterday, &yesterdayTm );
#endif
	if ( local.tm_year == yesterdayTm.tm_year && local.tm_yday == yesterdayTm.tm_yday )
		return std::string( "Yesterday at " ) + clock;
	char date[32];
	std::strftime( date, sizeof( date ), "%d/%m/%Y", &local );
	return std::string( date ) + " at " + clock;
}

std::string itemCount( Uint64 count ) {
	return count == 1 ? std::string( "1 item" ) : String::toString( count ) + " items";
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Panes
// ---------------------------------------------------------------------------------------------

void App::createPane( int index, UIWidget* parent ) {
	auto owned = std::make_unique<Pane>();
	Pane& p = *owned;
	p.index = index;
	p.root = mUI->loadLayoutFromString( PaneLayout, parent )->asType<UIWidget>();
	p.tabs = p.root->findByClass<UITabWidget>( "pane_tabs" );
	p.status = p.root->findByClass<UITextView>( "pane_status" );
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
		if ( it != pane.tabList.end() )
			pane.tabList.erase( it );
		positionNewTabButton( pane );
		if ( !pane.tabList.empty() )
			return;
		// The last tab of a pane closes the pane (or the window when it is the only one).
		if ( index == 1 ) {
			setSplit( false );
			openTab( pane, URI::fromFSPath( Sys::getUserDirectory() ), true );
		} else if ( mSplit ) {
			Pane& right = this->pane( 1 );
			for ( auto& tab : right.tabList )
				openTab( pane, tab->model->uri(), false );
			setSplit( false );
			if ( pane.tabs->getTabCount() > 0 )
				pane.tabs->setTabSelected( 0u );
		} else {
			quit();
		}
	} );
	p.tabs->on( Event::OnTabAdded,
				[this, index]( const Event* ) { positionNewTabButton( this->pane( index ) ); } );
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
	p.newTabButton->setIcon( icon( "action-new-tab" ) );
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
		image->setDrawable( icon( "mime-folder", 14 ) );
	p.location.input->on( Event::OnPressEnter, [this, index]( const Event* ) {
		Pane& pane = this->pane( index );
		if ( auto* tab = currentTab( pane ) ) {
			navigate( tab, parseLocation( pane.location.input->getText().toUtf8() ) );
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
	accept->setIcon( icon( "action-accept" ) );
	accept->setTooltipText( "Go" );
	accept->setLayoutGravity( UI_VALIGN_CENTER );
	accept->onClick( [this, index]( const MouseEvent* ) {
		Pane& pane = this->pane( index );
		if ( auto* tab = currentTab( pane ) )
			navigate( tab, parseLocation( pane.location.input->getText().toUtf8() ) );
	} );

	// Transfers
	p.root->findByClass( "transfer_cancel" )->onClick( [this, index]( const MouseEvent* ) {
		for ( auto& transfer : mTransfers )
			if ( transfer.second.pane == index )
				transfer.second.transfer.cancel();
	} );

	// Filter bar
	p.filterMode->getListBox()->addListBoxItems(
		{ mUI->i18n( "eefm_filter_text", "Plain Text" ),
		  mUI->i18n( "eefm_filter_glob", "Glob Pattern" ),
		  mUI->i18n( "eefm_filter_regex", "Regular Expression" ) } );
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
		pane.filterLock->setIcon( icon( pane.filterLocked ? "filter-lock" : "filter-unlock" ) );
	} );
	p.filterInput->on( Event::OnTextChanged, [this, index]( const Event* ) {
		Pane& pane = this->pane( index );
		if ( auto* tab = currentTab( pane ) ) {
			tab->filter = pane.filterInput->getText().toUtf8();
			applyFilter( tab );
		}
	} );
	p.filterInput->on( Event::OnFocus, [this, index]( const Event* ) { setActivePane( index ); } );
	p.root->findByClass( "filter_close" )->onClick( [this]( const MouseEvent* ) {
		setFilterBarVisible( false );
	} );

	mPanes[index] = std::move( owned );
}

FileTab* App::currentTab( Pane& pane ) {
	UITab* selected = pane.tabs->getTabSelected();
	for ( auto& tab : pane.tabList )
		if ( tab->tab == selected )
			return tab.get();
	return nullptr;
}

FileTab* App::findTab( UITab* uitab ) {
	for ( auto& pane : mPanes )
		for ( auto& tab : pane->tabList )
			if ( tab->tab == uitab )
				return tab.get();
	return nullptr;
}

void App::setActivePane( int index ) {
	if ( index == 1 && !mSplit )
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

void App::positionNewTabButton( Pane& target ) {
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

FileTab* App::openTab( Pane& pane, const URI& uri, bool select ) {
	// New tabs inherit the columns, widths and sorting of the tab they were opened from.
	if ( auto* current = currentTab( pane ) )
		rememberViewState( current );
	auto tab = std::make_unique<FileTab>();
	FileTab* raw = tab.get();
	raw->pane = &pane;
	VFSDirectoryModel::Config config;
	config.vfs = mVFS;
	config.showHidden = mShowHidden;
	raw->model = VFSDirectoryModel::New( *mExecutor, config );
	UISceneNode* ui = mUI;
	raw->model->setIconProvider( [ui]( const FileInfo& info ) -> Variant {
		UIIcon* icon = ui->findIcon( MimeDatabase::instance().iconName( info ) );
		if ( !icon )
			icon = ui->findIcon( info.isDirectory() ? "folder" : "file" );
		return icon ? Variant( icon ) : Variant();
	} );
	raw->model->setTypeProvider(
		[]( const FileInfo& info ) { return MimeDatabase::instance().describe( info ); } );
	raw->model->setTimeFormatter( formatTime );
	raw->model->setItemCountFormatter( itemCount );
	raw->model->setOnStateChanged( [this, raw] { onTabStateChanged( raw ); } );

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
								 eemax( eefloor( width * 0.15f ), PixelDensity::dpToPx( 72 ) ) );
		view->setColumnMaxWidth( VFSDirectoryModel::Modified, eefloor( width * 0.24f ) );
		view->setColumnMaxWidth( VFSDirectoryModel::Type, eefloor( width * 0.24f ) );
	};
	view->on( Event::OnSizeChange, [capColumns]( const Event* ) { capColumns(); } );
	view->onModelEvent( [this, raw]( const ModelEvent* event ) {
		setActivePane( raw->pane->index );
		if ( event->getModelEventType() == ModelEventType::Open ) {
			openRow( raw, event->getModelIndex().row() );
		} else if ( event->getModelEventType() == ModelEventType::OpenMenu ) {
			// Right-clicking an unselected row selects it first.
			if ( !raw->view->getSelection().contains( event->getModelIndex() ) )
				raw->view->getSelection().set( event->getModelIndex() );
			showContextMenu( raw, false );
		}
	} );
	view->setOnSelectionChange( [this, raw] {
		if ( raw->pane && raw == currentTab( *raw->pane ) )
			updateStatus( *raw->pane );
	} );
	view->onBackgroundMenu = [this, raw] {
		setActivePane( raw->pane->index );
		showContextMenu( raw, true );
	};
	view->dragPayload = [this, raw] { return selectedURIs( raw ); };
	view->onDragStarted = [this] { mPlacesPanel->trackDrag(); };
	view->isFolderRow = [raw]( size_t row ) {
		return row < raw->model->rowCount() && raw->model->entry( row ).isDirectory();
	};
	view->onFilesDropped = [this, raw]( std::optional<size_t> row ) {
		URI destination = raw->model->uri();
		if ( row && *row < raw->model->rowCount() && raw->model->entry( *row ).isDirectory() )
			destination = raw->model->entryURI( *row );
		dropFiles( currentFileDrag().uris, destination, raw->pane->index );
	};
	view->on( Event::OnFocus, [this, raw]( const Event* ) { setActivePane( raw->pane->index ); } );

	raw->tab = pane.tabs->add( displayName( uri ), view, icon( "mime-folder" ) );
	// Files can be dropped on a tab: they go to that tab's folder.
	raw->tab->on( Event::OnNodeDropped, [this, raw]( const Event* event ) {
		const auto* drop = static_cast<const DropEvent*>( event );
		if ( drop->getDroppedNode() && drop->getDroppedNode()->isType( FileTableView::CellType ) )
			dropFiles( currentFileDrag().uris, raw->model->uri(), raw->pane->index );
	} );
	pane.tabList.push_back( std::move( tab ) );
	applyViewState( raw );
	if ( select || pane.tabs->getTabCount() == 1 )
		pane.tabs->setTabSelected( raw->tab );
	navigate( raw, uri );
	return raw;
}

void App::closeTab( FileTab* tab ) {
	if ( tab && tab->pane )
		tab->pane->tabs->removeTab( tab->tab );
}

void App::updateTabTitle( FileTab* tab ) {
	tab->tab->setText( displayName( tab->model->uri() ) );
	tab->tab->setTooltipText( locationText( tab->model->uri() ) );
	positionNewTabButton( *tab->pane );
}

// ---------------------------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------------------------

URI App::parseLocation( const std::string& text ) {
	std::string trimmed = String::trim( text );
	if ( trimmed.find( "://" ) != std::string::npos ) {
		URI uri( trimmed );
		if ( uri.getPath().empty() )
			uri.setRawPath( "/" );
		return uri;
	}
	return URI::fromFSPath( FileSystem::expandTilde( trimmed ) );
}

String App::displayName( const URI& uri ) {
	std::string name = uri.getLastPathSegment();
	if ( name.empty() || uri.getPath() == "/" )
		name = uri.getScheme() == "file" ? std::string( "/" ) : uri.getHost();
	return String::fromUtf8( name );
}

String App::locationText( const URI& uri ) const {
	if ( uri.getScheme() == "file" ) {
		std::string path = uri.getFSPath();
		FileSystem::dirAddSlashAtEnd( path );
		return String::fromUtf8( path );
	}
	return String::fromUtf8( uri.toString() );
}

std::string App::clipboardText( const URI& uri ) const {
	if ( uri.getScheme() != "file" )
		return uri.toString();
	std::string path = uri.getFSPath();
	if ( path.size() > 1 && path.back() == '/' )
		path.pop_back();
	return path;
}

void App::navigate( FileTab* tab, URI uri, bool recordHistory ) {
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
	tab->model->setCountDirectoryItems( uri.getScheme() == "file" );
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

void App::goBack() {
	auto* tab = currentTab();
	if ( tab && tab->historyIndex > 0 ) {
		tab->selectWhenListed = tab->model->uri().getLastPathSegment();
		--tab->historyIndex;
		navigate( tab, tab->history[tab->historyIndex], false );
	}
}

void App::goForward() {
	auto* tab = currentTab();
	if ( tab && tab->historyIndex + 1 < tab->history.size() ) {
		++tab->historyIndex;
		navigate( tab, tab->history[tab->historyIndex], false );
	}
}

void App::goUp() {
	auto* tab = currentTab();
	if ( !tab )
		return;
	URI parent = tab->model->uri().getParent();
	if ( parent != tab->model->uri() ) {
		// Like Dolphin, the folder we leave stays selected in its parent.
		tab->selectWhenListed = tab->model->uri().getLastPathSegment();
		navigate( tab, parent );
	}
}

void App::goHome() {
	if ( auto* tab = currentTab() )
		navigate( tab, URI::fromFSPath( Sys::getUserDirectory() ) );
}

void App::showLocationHistory( Pane& pane ) {
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
		auto* item = menu->add( locationText( tab->history[i] ), icon( "mime-folder" ) );
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

void App::updateChrome( Pane& pane ) {
	auto* tab = currentTab( pane );
	if ( !tab )
		return;
	const URI& uri = tab->model->uri();
	if ( !pane.location.input->hasFocus() )
		pane.location.input->setText( locationText( uri ) );
	updateStatus( pane );
	if ( pane.index != mActivePane )
		return;
	mUI->find<UIWidget>( "back" )->setEnabled( tab->historyIndex > 0 );
	mUI->find<UIWidget>( "forward" )->setEnabled( tab->historyIndex + 1 < tab->history.size() );
	mUI->find<UIWidget>( "up" )->setEnabled( uri.getParent() != uri );
	mApp->getWindow()->setTitle( locationText( uri ) + " — eefm" );
	if ( mPlacesPanel )
		mPlacesPanel->setCurrent( uri );
}

void App::onTabStateChanged( FileTab* tab ) {
	if ( !tab->selectWhenListed.empty() ) {
		if ( auto row = tab->model->rowOf( tab->selectWhenListed ) ) {
			tab->view->setSelection( tab->model->index( *row ) );
			tab->selectWhenListed.clear();
		} else if ( tab->model->loadState() != VFSDirectoryModel::LoadState::Loading ) {
			tab->selectWhenListed.clear();
		}
	}
	if ( tab == currentTab( *tab->pane ) )
		updateStatus( *tab->pane );
}

void App::applyFilter( FileTab* tab ) {
	bool changed = tab->filter != tab->model->nameFilter() ||
				   mFilterMode != tab->model->nameFilterMode() ||
				   mFilterCaseSensitive != tab->model->nameFilterCaseSensitive();
	// The selection is kept by row: it would point at other entries once rows change.
	if ( changed )
		tab->view->getSelection().clear();
	tab->model->setNameFilter( tab->filter, mFilterMode, mFilterCaseSensitive );
	if ( tab == currentTab( *tab->pane ) ) {
		if ( tab->model->isNameFilterValid() )
			tab->pane->filterInput->removeClass( "invalid" );
		else
			tab->pane->filterInput->addClass( "invalid" );
		updateStatus( *tab->pane );
	}
}

void App::updateStatus( Pane& pane ) {
	auto* tab = currentTab( pane );
	if ( !tab )
		return;
	auto& model = *tab->model;
	String status;
	if ( model.loadState() == VFSDirectoryModel::LoadState::Error ) {
		status = String::fromUtf8( model.error()->toString() );
	} else {
		Uint64 folders = 0, files = 0, bytes = 0;
		Uint64 selectedFolders = 0, selectedFiles = 0, selectedBytes = 0;
		for ( size_t row = 0; row < model.rowCount(); ++row ) {
			const FileInfo& info = model.entry( row );
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
			if ( regular )
				text += ( text.empty() ? "" : ", " ) + std::to_string( regular ) +
						( regular == 1 ? " file" : " files" );
			return text.empty() ? std::string( "Empty folder" ) : text;
		};
		std::string text;
		if ( selectedFolders + selectedFiles > 0 ) {
			text = describe( selectedFolders, selectedFiles ) + " selected";
			if ( selectedFiles )
				text +=
					" (" + FileSystem::sizeToString( static_cast<Int64>( selectedBytes ) ) + ")";
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

} // namespace eefm
