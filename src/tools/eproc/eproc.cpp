#include "eproc.hpp"

#include <eepp/system/log.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_MACOS || \
	EE_PLATFORM == EE_PLATFORM_BSD
#include <unistd.h>
#endif
#if EE_PLATFORM == EE_PLATFORM_LINUX
#include <signal.h>
#endif

namespace eproc {

namespace {

constexpr int kProcessTableStateVersion = 3;
constexpr int kPreviousProcessTableStateVersion = 2;
constexpr size_t kPreviousCommandColumn = 11;

constexpr std::array<size_t, 8> kOptionalProcessColumns = { {
	ProcessModel::ColTotalMemory,
	ProcessModel::ColVirtualSize,
	ProcessModel::ColCpuTime,
	ProcessModel::ColNiceness,
	ProcessModel::ColRelativeStartTime,
	ProcessModel::ColTty,
	ProcessModel::ColIoRead,
	ProcessModel::ColIoWrite,
} };

const char* sortOrderName( SortOrder order ) {
	switch ( order ) {
		case SortOrder::Ascending:
			return "ascending";
		case SortOrder::Descending:
			return "descending";
		default:
			return "none";
	}
}

bool parseSortOrder( const std::string& value, SortOrder& order ) {
	if ( value == "ascending" ) {
		order = SortOrder::Ascending;
		return true;
	}
	if ( value == "descending" ) {
		order = SortOrder::Descending;
		return true;
	}
	return false;
}

bool isValidProcessTableWidthState( const nlohmann::json& widths, size_t columnCount,
									const UIAbstractTableView& tableView ) {
	if ( !widths.is_object() || !widths.contains( "mode" ) || !widths["mode"].is_string() ||
		 !widths.contains( "widths" ) || !widths["widths"].is_array() ||
		 widths["widths"].size() != columnCount )
		return false;

	const std::string mode = widths["mode"].get<std::string>();
	if ( mode != "pixels" && mode != "percentage" )
		return false;

	bool hasVisibleWidth = false;
	for ( size_t column = 0; column < columnCount; ++column ) {
		const auto& width = widths["widths"][column];
		const double value = width.is_number() ? width.get<double>() : 0;
		if ( !width.is_number() || !std::isfinite( value ) || value < 0 )
			return false;
		if ( mode == "pixels" && !tableView.isColumnHidden( column ) && value <= 1.0 )
			return false;
		if ( !tableView.isColumnHidden( column ) && value > 0 )
			hasVisibleWidth = true;
	}

	return hasVisibleWidth;
}

size_t remapPreviousProcessTableColumn( size_t column ) {
	if ( column == kPreviousCommandColumn )
		return ProcessModel::ColCommand;
	if ( column > kPreviousCommandColumn )
		return column - 1;
	return column;
}

bool migratePreviousProcessTableState( nlohmann::json& state ) {
	if ( !state.contains( "widths" ) || !state["widths"].is_object() ||
		 !state["widths"].contains( "widths" ) || !state["widths"]["widths"].is_array() ||
		 state["widths"]["widths"].size() != ProcessModel::ColCount )
		return false;

	nlohmann::json remappedWidths = nlohmann::json::array();
	for ( size_t column = 0; column < ProcessModel::ColCount; ++column )
		remappedWidths.push_back( 0 );
	for ( size_t previousColumn = 0; previousColumn < ProcessModel::ColCount; ++previousColumn ) {
		remappedWidths[remapPreviousProcessTableColumn( previousColumn )] =
			state["widths"]["widths"][previousColumn];
	}
	state["widths"]["widths"] = std::move( remappedWidths );

	if ( state.contains( "hidden_columns" ) && state["hidden_columns"].is_array() ) {
		nlohmann::json remappedHiddenColumns = nlohmann::json::array();
		for ( const auto& column : state["hidden_columns"] ) {
			if ( !column.is_number_integer() )
				continue;
			const Int64 previousColumn = column.get<Int64>();
			if ( previousColumn >= 0 &&
				 static_cast<size_t>( previousColumn ) < ProcessModel::ColCount )
				remappedHiddenColumns.push_back(
					remapPreviousProcessTableColumn( static_cast<size_t>( previousColumn ) ) );
		}
		state["hidden_columns"] = std::move( remappedHiddenColumns );
	}

	if ( state.contains( "sort" ) && state["sort"].is_object() &&
		 state["sort"].contains( "column" ) && state["sort"]["column"].is_number_integer() ) {
		const Int64 previousColumn = state["sort"]["column"].get<Int64>();
		if ( previousColumn >= 0 && static_cast<size_t>( previousColumn ) < ProcessModel::ColCount )
			state["sort"]["column"] =
				remapPreviousProcessTableColumn( static_cast<size_t>( previousColumn ) );
	}

	state["version"] = kProcessTableStateVersion;
	return true;
}

bool isCurrentUser( const ProcessInfo& process ) {
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_MACOS || \
	EE_PLATFORM == EE_PLATFORM_BSD
	return process.uid == static_cast<long>( getuid() );
#else
	return false;
#endif
}

const char* usernameClass( const ProcessInfo& process ) {
	if ( process.status == ProcessStatus::Ended )
		return "eproc-process-username-ended";
	if ( process.tracerPid > 0 )
		return "eproc-process-username-traced";
	if ( isCurrentUser( process ) )
		return "eproc-process-username-own";
	if ( process.uid < 100 || !process.canLogin )
		return "eproc-process-username-system";
	return "eproc-process-username-other";
}

} // namespace

App::App() {
	mConfig = std::make_unique<AppConfig>( Sys::getConfigPath( "eproc" ) );
	mConfig->load();

	// Scanning /proc takes long enough to race the first frame, which would leave the table briefly
	// empty. Kick the worker off before the window exists so the first snapshot is already staged
	// by the time the scene is rendered.
	startCollection();

	const Sizei storedSize = mConfig->windowState.size;
	const Uint32 windowWidth =
		storedSize.getWidth() > 0 ? static_cast<Uint32>( storedSize.getWidth() ) : 1280;
	const Uint32 windowHeight =
		storedSize.getHeight() > 0 ? static_cast<Uint32>( storedSize.getHeight() ) : 720;
	WindowSettings ws( windowWidth, windowHeight, "", WindowStyle::Default, WindowBackend::Default,
					   32, Sys::getProcessPath() + "assets/icon/ee.png" );
	mApp = std::make_unique<UIApplication>( ws );
	if ( mApp->getUI() && mApp->getWindow() )
		mApp->getWindow()->setTitle(
			mApp->getUI()->i18n( "eproc_window_title", "System Monitor" ) );
}

App::~App() {}

int App::run() {
	if ( !init() )
		return EXIT_FAILURE;
	const int result = mApp->run();
	if ( !mWindowStateSaved )
		saveWindowState();
	return result;
}

bool App::init() {
	auto* ui = mApp->getUI();
	if ( !ui )
		return false;

	restoreWindowState();
	if ( mApp->getWindow() ) {
		mApp->getWindow()->setCloseRequestCallback(
			[this]( EE::Window::Window* window ) { return closeWindow( window ); } );
		mApp->getWindow()->setQuitCallback( [this]( EE::Window::Window* window ) {
			if ( window->isOpen() && closeWindow( window ) )
				window->close();
		} );
	}

	mRoot = ui->loadLayoutFromString( R"xml(
	<style>
	tableview::cell {
		background-color: transparent;
		text-align: left;
	}
	tableview::cell.eproc-process-column-icon,
	tableview::cell.eproc-process-column-username,
	tableview::cell.eproc-process-column-cpu {
		text-align: center;
	}
	tableview::cell.eproc-process-username-own {
		background-color: #00D0D432;
		border-top: 1dprd solid rgba(0, 255, 255, 0.026);
		border-bottom: 1dprd solid rgba(0, 255, 255, 0.085);
	}
	tableview::cell.eproc-process-username-system {
		background-color: #DADCD732;
	}
	tableview::cell.eproc-process-username-other {
		background-color: #029A3632;
	}
	tableview::cell.eproc-process-username-traced {
		background-color: #FFFF00;
	}
	tableview::cell.eproc-process-username-ended {
		background-color: #D3D3D3;
	}
	tableview::cell.eproc-process-ended {
		color: #808080;
		tint: #808080;
	}
	tableview::cell.eproc-process-column-pid,
	tableview::cell.eproc-process-column-memory,
	tableview::cell.eproc-process-column-shared-memory,
	tableview::cell.eproc-process-column-gpu-usage,
	tableview::cell.eproc-process-column-gpu-memory,
	tableview::cell.eproc-process-column-download,
	tableview::cell.eproc-process-column-upload,
	tableview::cell.eproc-process-column-total-memory,
	tableview::cell.eproc-process-column-virtual-size,
	tableview::cell.eproc-process-column-cpu-time,
	tableview::cell.eproc-process-column-niceness,
	tableview::cell.eproc-process-column-relative-start-time,
	tableview::cell.eproc-process-column-io-read,
	tableview::cell.eproc-process-column-io-write {
		text-align: right;
	}
	tableview::row:nth-child(even),
	treeview::row:nth-child(even) {
		background-color: var(--list-back-alt);
	}
	tableview::row:nth-child(even):hover,
	treeview::row:nth-child(even):hover {
		background-color: var(--item-hover);
	}

	tableview::row:nth-child(even):selected,
	treeview::row:nth-child(even):selected {
		background-color: var(--primary);
	}
	</style>
	<vbox id="main_layout" lw="mp" lh="mp">
		<TabWidget id="tab_widget" lw="mp" lh="mp">
			<!-- Process Table tab content -->
			<vbox id="process_table_area" lw="mp" lh="mp">
				<hbox id="toolbar" lw="mp" lh="wc" padding="4dp">
					<PushButton id="end_process_btn" lw="wc" lh="wc" margin-right="4dp" />
					<TextInput id="search_input" lw="0" lw8="1" lh="wc" margin-right="4dp" />
					<DropDownList id="filter_dropdown" lw="120dp" lh="wc" margin-right="4dp" />
				</hbox>
				<TableView id="process_table" lw="mp" lh="0" lw8="1"
					 column-width-mode-menu="true"
					 table-flags="headers|row-search|focus-on-selection|auto-columns" />
				<hbox id="status_bar" lw="mp" lh="wc" padding="4dp">
					<TextView id="process_count" lw="0" lw8="0.25" lh="wc" />
					<TextView id="cpu_text" lw="0" lw8="0.25" lh="wc" />
					<TextView id="mem_text" lw="0" lw8="0.25" lh="wc" halign="right" />
					<TextView id="swap_text" lw="0" lw8="0.25" lh="wc" />
				</hbox>
			</vbox>
			<!-- System Load placeholder tab content -->
			<!--
			<TextView id="system_load_area" lw="mp" lh="mp"
				text="System Load - Coming soon" />
			-->
			<!-- Tab definitions -->
			<Tab id="tab_process_table" owns="process_table_area" />
			<!-- <Tab id="tab_system_load" text="System Load" owns="system_load_area" /> -->
		</TabWidget>
	</vbox>
	)xml" );

	if ( !mRoot ) {
		return false;
	}

	setupUI();
	setupProcessTable();

#if EE_PLATFORM != EE_PLATFORM_LINUX && EE_PLATFORM != EE_PLATFORM_BSD
	UIMessageBox* platformMessage = UIMessageBox::New(
		UIMessageBox::OK,
		ui->i18n( "eproc_platform_wip_message",
				  "eproc is currently a work in progress. This operating system is not implemented "
				  "yet." ) );
	platformMessage->setTitle( ui->i18n( "eproc_platform_wip_title", "Work in Progress" ) );
	platformMessage->center();
	platformMessage->showWhenReady();
#endif

	// The first snapshot was requested before the window existed, so it is normally ready by now.
	// Publishing it here (before the loop, so nothing is being drawn yet) means the very first
	// frame already shows data instead of flashing an empty table.
	waitForFirstSnapshot( 250 );
	publishStagedSnapshot();

	setupRefreshTimer();

	return true;
}

void App::restoreWindowState() {
	if ( !mConfig || !mApp || !mApp->getWindow() )
		return;

	EE::Window::Window* window = mApp->getWindow();
	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	const auto& state = mConfig->windowState;
	if ( state.position != Vector2i( -1, -1 ) && displayManager && state.displayIndex >= 0 &&
		 state.displayIndex < displayManager->getDisplayCount() ) {
		window->setPosition( state.position.x + ( state.maximized ? -1 : 0 ), state.position.y );
	}

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	if ( state.maximized ) {
#if EE_PLATFORM == EE_PLATFORM_LINUX
		mApp->getUI()->runOnMainThread( [window] { window->maximize(); } );
#elif EE_PLATFORM != EE_PLATFORM_MACOS
		window->maximize();
#endif
	}
#endif
}

void App::saveWindowState() {
	if ( !mConfig || !mApp )
		return;

	if ( mTableView && mSortProxy )
		mConfig->processTableState = serializeProcessTableState();
	mConfig->captureWindowState( mApp->getWindow() );
	if ( !mConfig->saveWindowState() )
		Log::error( "Could not save eproc window state to %s", mConfig->getConfigPath() );
	else
		mWindowStateSaved = true;
}

std::string App::serializeProcessTableState() const {
	if ( !mTableView || !mSortProxy )
		return {};

	nlohmann::json state;
	state["version"] = kProcessTableStateVersion;
	state["widths"] = mTableView->serializeColumnWidths();
	state["hidden_columns"] = nlohmann::json::array();
	for ( size_t column = 0; column < mSortProxy->columnCount(); ++column ) {
		if ( mTableView->isColumnHidden( column ) )
			state["hidden_columns"].push_back( column );
	}
	state["sort"]["column"] = mSortProxy->keyColumn();
	state["sort"]["order"] = sortOrderName( mSortProxy->sortOrder() );
	return state.dump();
}

void App::restoreProcessTableState() {
	if ( !mConfig || mConfig->processTableState.empty() || !mTableView || !mSortProxy )
		return;

	nlohmann::json state =
		nlohmann::json::parse( mConfig->processTableState, nullptr, false, true );
	if ( state.is_discarded() || !state.is_object() || !state.contains( "version" ) ||
		 !state["version"].is_number_integer() )
		return;

	const int stateVersion = state["version"].get<int>();
	if ( stateVersion == kPreviousProcessTableStateVersion ) {
		if ( !migratePreviousProcessTableState( state ) )
			return;
	} else if ( stateVersion != kProcessTableStateVersion ) {
		return;
	}

	const size_t columnCount = mSortProxy->columnCount();
	if ( !state.contains( "widths" ) ||
		 !isValidProcessTableWidthState( state["widths"], columnCount, *mTableView ) )
		return;

	if ( state.contains( "hidden_columns" ) && state["hidden_columns"].is_array() ) {
		std::vector<bool> hidden( columnCount, false );
		for ( const auto& column : state["hidden_columns"] ) {
			if ( !column.is_number_integer() )
				continue;
			const Int64 index = column.get<Int64>();
			if ( index >= 0 && static_cast<size_t>( index ) < columnCount )
				hidden[static_cast<size_t>( index )] = true;
		}

		std::vector<size_t> visible;
		visible.reserve( columnCount );
		for ( size_t column = 0; column < columnCount; ++column ) {
			if ( !hidden[column] )
				visible.push_back( column );
		}
		// Always leave one column visible, even if a malformed or old state hides everything.
		if ( !visible.empty() )
			mTableView->setColumnsVisible( visible );
	}

	// Pixel restoration calls setColumnWidth() once per column. Automatic sizing must already be
	// disabled, otherwise each call immediately recalculates all columns and overwrites the saved
	// width with content-based sizing.
	const bool pixelWidths = state["widths"]["mode"] == "pixels";
	if ( pixelWidths )
		mTableView->setAutoColumnsWidth( false );
	if ( !mTableView->unserializeColumnWidths( state["widths"] ) && pixelWidths )
		mTableView->setAutoColumnsWidth( true );

	if ( state.contains( "sort" ) && state["sort"].is_object() ) {
		const auto& sort = state["sort"];
		const int column = sort.contains( "column" ) && sort["column"].is_number_integer()
							   ? sort["column"].get<int>()
							   : -1;
		const std::string orderName = sort.contains( "order" ) && sort["order"].is_string()
										  ? sort["order"].get<std::string>()
										  : "none";
		SortOrder order = SortOrder::None;
		if ( column >= 0 && static_cast<size_t>( column ) < columnCount &&
			 parseSortOrder( orderName, order ) && order != SortOrder::None &&
			 mSortProxy->isColumnSortable( column ) )
			mTableView->sortByColumn( static_cast<size_t>( column ), order );
	}
}

bool App::closeWindow( EE::Window::Window* ) {
	saveWindowState();
	return true;
}

void App::setupUI() {
	mTabWidget = mRoot->find<UITabWidget>( "tab_widget" );
	mEndProcessBtn = mRoot->find<UIPushButton>( "end_process_btn" );
	mSearchInput = mRoot->find<UITextInput>( "search_input" );
	mFilterDropdown = mRoot->find<UIDropDownList>( "filter_dropdown" );
	mTableView = mRoot->find<UITableView>( "process_table" );
	mStatusText = mRoot->find<UITextView>( "process_count" );
	mCpuText = mRoot->find<UITextView>( "cpu_text" );
	mMemText = mRoot->find<UITextView>( "mem_text" );
	mSwapText = mRoot->find<UITextView>( "swap_text" );

	auto* ui = mApp->getUI();
	mRoot->find<UITab>( "tab_process_table" )
		->setText( ui->i18n( "eproc_process_table_tab", "Process Table" ) );
	mEndProcessBtn->setText( ui->i18n( "eproc_end_process_button", "End Process..." ) );
	mSearchInput->setHint( ui->i18n( "eproc_quick_search_hint", "Quick search" ) );
	mStatusText->setText( ui->i18n( "eproc_process_count", "0 processes" ) );
	mCpuText->setText( ui->i18n( "eproc_cpu_status", "CPU: 0%" ) );
	mMemText->setText( ui->i18n( "eproc_memory_status", "Memory: 0 / 0" ) );
	mSwapText->setText( ui->i18n( "eproc_swap_status", "Swap: 0 / 0" ) );

	// Tabs are declared in the XML layout via Tab elements with owns= attributes.

	// Filter entries mirror the original's ProcessFilter::State order (flat variants only; the
	// tree variants need a hierarchical model).
	static const std::pair<const char*, const char*> filters[] = {
		{ "eproc_filter_all_processes", "All Processes" },
		{ "eproc_filter_system_processes", "System Processes" },
		{ "eproc_filter_user_processes", "User Processes" },
		{ "eproc_filter_own_processes", "Own Processes" },
		{ "eproc_filter_programs_only", "Programs Only" },
	};
	auto* filterListBox = mFilterDropdown->getListBox();
	for ( const auto& filter : filters )
		filterListBox->addListBoxItem( ui->i18n( filter.first, filter.second ) );
	filterListBox->setSelected( 0 );

	// Connect events
	if ( mEndProcessBtn )
		mEndProcessBtn->onClick( [this]( const MouseEvent* ) { onEndProcess(); } );

	if ( mSearchInput )
		mSearchInput->on( Event::OnTextChanged, [this]( const Event* ) { onSearchChanged(); } );

	if ( mFilterDropdown )
		mFilterDropdown->on( Event::OnItemSelected, [this]( const Event* ) { onFilterChanged(); } );

	mApp->getUI()->on( Event::KeyUp, [this]( const Event* event ) {
		if ( event->asKeyEvent()->getKeyCode() == KEY_F11 ) {
			UIWidgetInspector::create( mApp->getUI() );
		}
	} );
}

void App::setupProcessTable() {
	mProcessModel = ProcessModel::create( mApp->getUI() );
	mSortProxy = SortingProxyModel::New( mProcessModel );

	if ( mTableView ) {
		mTableView->setModel( mSortProxy );
		mTableView->setColumnsHidden(
			std::vector<size_t>( kOptionalProcessColumns.begin(), kOptionalProcessColumns.end() ),
			true );
		mTableView->setOnUpdateCellCb( [this]( UITableCell* cell, Model* model ) {
			if ( !cell || !model )
				return;

			const ModelIndex index = cell->getCurIndex();
			const ProcessInfo* process = processForProxyIndex( index );
			const Variant columnClass = model->data( index, ModelRole::Class );
			std::vector<std::string> classes;
			if ( columnClass.isValid() )
				classes.emplace_back( columnClass.toString() );
			if ( process ) {
				if ( process->status == ProcessStatus::Ended )
					classes.emplace_back( "eproc-process-ended" );
				if ( index.column() == ProcessModel::ColUsername )
					classes.emplace_back( usernameClass( *process ) );
			}
			cell->setClasses( classes );
		} );
		mTableView->setRowHeight( 28 );
		// The flexible column is Name; the icon column is fixed so every row lines up.
		mTableView->setMainColumn( ProcessModel::ColName );
		mTableView->setSortIconSize( 12 );
		mTableView->setIconSize( PixelDensity::dpToPxI( 16 ) );
		mTableView->setColumnWidth( ProcessModel::ColIcon, PixelDensity::dpToPx( 26 ) );

		mTableView->setOnSelectionChange( [this]() { onSelectionChange(); } );
		mTableView->onModelEvent( [this]( const ModelEvent* event ) {
			if ( event->getModelEventType() == ModelEventType::OpenMenu )
				showProcessContextMenu( event->getModelIndex() );
		} );
	}

	// The original opens sorted by memory usage, descending, which surfaces the heavy processes
	// instead of the kernel threads that /proc happens to enumerate first. Sorting through the
	// view (not the model directly) also renders the sort indicator in the header.
	mTableView->sortByColumn( ProcessModel::ColMemory, SortOrder::Descending );
}

void App::startCollection() {
	mCollector = ProcessCollector::create();
	if ( !mCollector ) {
		Log::error( "eproc: no process collector for this platform, the table will stay empty" );
		return;
	}

	// A single worker keeps the collector's previous-sample state exclusive to one thread, and
	// keeps /proc walking off the UI thread entirely.
	//
	// terminateOnClose must stay false: the pool destructor then joins the worker, which
	// guarantees no in-flight collect() can outlive mCollector (destroyed after mThreadPool,
	// since members are destroyed in reverse declaration order).
	mThreadPool = ThreadPool::createShared( 1, false );

	// Timed from this first dispatch so the next sample lands a full period later, giving the CPU
	// deltas a meaningful window instead of the few milliseconds of a back-to-back pair.
	mDispatchClock.getElapsedTimeAndReset();
	collectAsync();
}

void App::setupRefreshTimer() {
	if ( !mCollector )
		return;

	// The worker finishes a snapshot long before the next sample is due, so the tick only polls
	// for staged results and dispatches a new sample once the refresh period has elapsed.
	// Publishing on the same 2s cadence would leave the table empty until the second tick.
	mRoot->setInterval( [this] { onRefreshTick(); }, Milliseconds( mTickIntervalMs ) );
}

bool App::waitForFirstSnapshot( Uint32 timeoutMs ) {
	Clock clock;
	while ( clock.getElapsedTime().asMilliseconds() < timeoutMs ) {
		{
			Lock lock( mStagingMutex );
			if ( mStagedReady )
				return true;
		}
		Sys::sleep( Milliseconds( 2 ) );
	}
	return false;
}

void App::onRefreshTick() {
	publishStagedSnapshot();

	if ( mDispatchClock.getElapsedTime().asMilliseconds() < mUpdateIntervalMs )
		return;

	mDispatchClock.getElapsedTimeAndReset();
	collectAsync();
}

void App::collectAsync() {
	if ( !mCollector || !mThreadPool )
		return;

	// Never overlap collections: CPU usage is computed from the delta against the instance's
	// previous sample, so concurrent runs would corrupt it.
	if ( mCollectInFlight.exchange( true ) )
		return;

	mThreadPool->run( [this] {
		std::vector<ProcessInfo> processes;
		SystemInfo sysInfo;

		if ( mCollector->collect( processes, sysInfo ) ) {
			Lock lock( mStagingMutex );
			mStagedProcesses = std::move( processes );
			mStagedSystemInfo = sysInfo;
			mStagedReady = true;
		}

		mCollectInFlight.store( false );
	} );
}

void App::publishStagedSnapshot() {
	std::vector<ProcessInfo> processes;
	SystemInfo sysInfo;

	{
		Lock lock( mStagingMutex );
		if ( !mStagedReady )
			return;
		processes = std::move( mStagedProcesses );
		sysInfo = mStagedSystemInfo;
		mStagedReady = false;
	}

	// Window ownership is needed by the Programs Only filter, so it is refreshed on the UI thread
	// once per published snapshot rather than per tick.
	if ( mProcessModel ) {
		mGuiWindows.refresh();
		mProcessModel->setGuiWindowPids( UnorderedSet<long>( mGuiWindows.windowPids() ) );
	}

	// A full model reset clears the view's selection (SortingProxyModel drops it on every
	// invalidation), so the chosen process is remembered by PID and re-selected afterwards. PIDs
	// are stable across snapshots while row indexes are not: the table re-sorts on every update.
	long selectedPid = getSelectedPid();

	if ( mProcessModel )
		mProcessModel->applySnapshot( std::move( processes ), sysInfo );

	if ( !mProcessTableStateRestored && !mProcessTableStateRestoreScheduled && mApp &&
		 mApp->getUI() ) {
		mProcessTableStateRestoreScheduled = true;
		mApp->getUI()->runOnMainThread( [this] {
			if ( !mProcessTableStateRestored ) {
				restoreProcessTableState();
				mProcessTableStateRestored = true;
			}
			mProcessTableStateRestoreScheduled = false;
		} );
	}

	updateStatusBar();
	restoreSelection( selectedPid );
}

long App::getSelectedPid() const {
	if ( !mTableView || !mProcessModel )
		return -1;

	ModelIndex proxyIndex = mTableView->getSelection().first();
	if ( !proxyIndex.isValid() )
		return -1;

	ModelIndex sourceIndex = mSortProxy ? mSortProxy->mapToSource( proxyIndex ) : proxyIndex;
	const ProcessInfo* proc = mProcessModel->getProcessByRow( sourceIndex.row() );
	return proc ? proc->pid : -1;
}

void App::restoreSelection( long pid ) {
	if ( pid < 0 || !mTableView || !mProcessModel || !mSortProxy )
		return;

	int row = mProcessModel->rowForPid( pid );
	if ( row < 0 )
		return; // the process exited, or the filter no longer matches it

	ModelIndex proxyIndex = mSortProxy->mapToProxy( mProcessModel->index( row, 0 ) );
	if ( proxyIndex.isValid() )
		mTableView->setSelection( proxyIndex, false );
}

void App::updateStatusBar() {
	if ( !mProcessModel )
		return;

	const auto& sys = mProcessModel->getSystemInfo();

	if ( mStatusText )
		mStatusText->setText( String::format(
			mApp->getUI()->i18n( "eproc_process_count_format", "%zu processes" ).toUtf8(),
			mProcessModel->visibleCount() ) );

	if ( mCpuText )
		mCpuText->setText(
			String::format( mApp->getUI()->i18n( "eproc_cpu_status_format", "CPU: %d%%" ).toUtf8(),
							static_cast<Int32>( sys.cpuUsage ) ) );

	if ( mMemText )
		mMemText->setText( String::format(
			mApp->getUI()->i18n( "eproc_memory_status_format", "Memory: %s / %s" ).toUtf8(),
			formatKiBIEC( sys.getUsedMemoryKB() ).c_str(),
			formatKiBIEC( sys.getTotalMemoryKB() ).c_str() ) );

	if ( mSwapText )
		mSwapText->setText( String::format(
			mApp->getUI()->i18n( "eproc_swap_status_format", "Swap: %s / %s" ).toUtf8(),
			formatKiBIEC( sys.getUsedSwapKB() ).c_str(), formatKiBIEC( sys.totalSwap ).c_str() ) );
}

void App::onEndProcess() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	requestSignal( selectedPids(), SIGTERM,
				   mApp->getUI()->i18n( "eproc_end_process", "End Process" ).toUtf8(), true );
#endif
}

void App::onSearchChanged() {
	if ( mSearchInput && mProcessModel ) {
		mProcessModel->setTextFilter( mSearchInput->getText().toUtf8() );
		updateStatusBar();
	}
}

void App::onFilterChanged() {
	if ( mFilterDropdown && mProcessModel ) {
		// The dropdown is built in the same order as the enum, so the index maps directly.
		Uint32 selected = mFilterDropdown->getListBox()->getItemSelectedIndex();
		if ( selected < ProcessModel::FilterModeCount )
			mProcessModel->setFilter( static_cast<ProcessModel::FilterMode>( selected ) );
		updateStatusBar();
	}
}

void App::onSelectionChange() {
	// Update End Process button state
	if ( mEndProcessBtn && mTableView ) {
		mEndProcessBtn->setEnabled( !selectedPids().empty() );
	}
}

const ProcessInfo* App::processForProxyIndex( const ModelIndex& proxyIndex ) const {
	if ( !mProcessModel || !mSortProxy || !proxyIndex.isValid() )
		return nullptr;

	return mProcessModel->getProcessByRow( mSortProxy->mapToSource( proxyIndex ).row() );
}

std::vector<long> App::selectedPids() const {
	std::vector<long> pids;

	if ( !mProcessModel || !mTableView )
		return pids;

	for ( const auto& proxyIndex : mTableView->getSelection().indexes() ) {
		const ProcessInfo* proc = processForProxyIndex( proxyIndex );
		if ( proc && proc->status != ProcessStatus::Ended )
			pids.push_back( proc->pid );
	}

	return pids;
}

void App::selectProcess( long pid ) {
	if ( pid <= 0 || !mProcessModel || !mSortProxy || !mTableView )
		return;

	int row = mProcessModel->rowForPid( pid );

	// The active filter may be hiding the target, so drop it instead of silently doing nothing
	// (the original clears its text filter for the same reason).
	if ( row < 0 && mSearchInput && !mSearchInput->getText().empty() ) {
		mProcessModel->setTextFilter( "" );
		mSearchInput->setText( "" );
		row = mProcessModel->rowForPid( pid );
	}

	if ( row < 0 )
		return;

	ModelIndex proxyIndex = mSortProxy->mapToProxy( mProcessModel->index( row, 0 ) );
	if ( proxyIndex.isValid() )
		mTableView->setSelection( proxyIndex );
}

void App::requestSignal( std::vector<long> pids, int signal, const std::string& actionLabel,
						 bool confirm ) {
	if ( pids.empty() )
		return;

	auto send = [pids, signal]() {
		for ( long pid : pids )
			sendProcessSignal( pid, signal );
	};

	if ( !confirm ) {
		send();
		return;
	}

	const std::string target =
		pids.size() == 1
			? String::format( mApp->getUI()->i18n( "eproc_process_target", "process %ld" ).toUtf8(),
							  pids.front() )
			: String::format(
				  mApp->getUI()->i18n( "eproc_processes_target", "%zu processes" ).toUtf8(),
				  pids.size() );
	const std::string message =
		String::format( mApp->getUI()->i18n( "eproc_confirm_action", "%s %s?" ).toUtf8(),
						actionLabel.c_str(), target.c_str() );

	UIMessageBox* box = UIMessageBox::New( UIMessageBox::OK_CANCEL, message );
	box->setTitle( actionLabel );
	box->on( Event::OnConfirm, [send]( const Event* ) { send(); } );
	box->center();
	box->showWhenReady();
}

void App::showProcessContextMenu( const ModelIndex& proxyIndex ) {
	if ( !mTableView )
		return;

	// Right-clicking outside the selection moves the selection to the clicked row, as the
	// original does, so the menu always acts on what the user pointed at.
	if ( proxyIndex.isValid() && !mTableView->getSelection().contains( proxyIndex ) )
		mTableView->setSelection( proxyIndex, false );

	const std::vector<long> pids = selectedPids();
	if ( pids.empty() )
		return;

	const ProcessInfo* proc = processForProxyIndex( proxyIndex );
	const long parentPid = proc ? proc->parentPid : 0;
	const long tracerPid = proc ? proc->tracerPid : 0;
	std::string copyCommandLine = proc ? proc->commandLine : std::string();

	struct SignalItem {
		const char* id;
		const char* key;
		const char* label;
		int signal;
	};

// The same signal set the original offers, in the same order.
#if EE_PLATFORM == EE_PLATFORM_LINUX
	static const std::array<SignalItem, 8> signalItems = { {
		{ "signal-stop", "eproc_signal_suspend", "Suspend (STOP)", SIGSTOP },
		{ "signal-cont", "eproc_signal_continue", "Continue (CONT)", SIGCONT },
		{ "signal-hup", "eproc_signal_hangup", "Hangup (HUP)", SIGHUP },
		{ "signal-int", "eproc_signal_interrupt", "Interrupt (INT)", SIGINT },
		{ "signal-term", "eproc_signal_terminate", "Terminate (TERM)", SIGTERM },
		{ "signal-kill", "eproc_signal_kill", "Kill (KILL)", SIGKILL },
		{ "signal-usr1", "eproc_signal_user1", "User 1 (USR1)", SIGUSR1 },
		{ "signal-usr2", "eproc_signal_user2", "User 2 (USR2)", SIGUSR2 },
	} };
#else
	static const std::array<SignalItem, 0> signalItems{};
#endif

	UIPopUpMenu* signalMenu = UIPopUpMenu::New();
	signalMenu->setId( "process_signal_menu" );
	for ( const auto& item : signalItems )
		signalMenu->add( mApp->getUI()->i18n( item.key, item.label ) )->setId( item.id );

	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->setId( "process_context_menu" );
	menu->addSubMenu( mApp->getUI()->i18n( "eproc_send_signal", "Send Signal" ), nullptr,
					  signalMenu )
		->setId( "send-signal" );
	menu->add( mApp->getUI()->i18n( "eproc_jump_to_parent", "Jump to Parent Process" ) )
		->setId( "jump-parent" );

	if ( tracerPid > 0 )
		menu->add( mApp->getUI()->i18n( "eproc_jump_to_tracer",
										"Jump to Process Debugging This One" ) )
			->setId( "jump-tracer" );

	menu->add( mApp->getUI()->i18n( "eproc_copy_command_line", "Copy Command Line" ) )
		->setId( "copy-command-line" );

#if EE_PLATFORM == EE_PLATFORM_LINUX
	menu->addSeparator();
	menu->add( mApp->getUI()->i18n( "eproc_end_process", "End Process" ) )->setId( "end-process" );
	menu->add( mApp->getUI()->i18n( "eproc_forcibly_kill_process", "Forcibly Kill Process" ) )
		->setId( "kill-process" );
#endif

	menu->on( Event::OnItemClicked, [this, pids, parentPid, tracerPid,
									 copyCommandLine =
										 std::move( copyCommandLine )]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		if ( !item )
			return;

		const std::string id( item->getId() );

		if ( id == "jump-parent" ) {
			selectProcess( parentPid );
		} else if ( id == "jump-tracer" ) {
			selectProcess( tracerPid );
		} else if ( id == "copy-command-line" ) {
			if ( !copyCommandLine.empty() && mApp->getWindow()->getClipboard() )
				mApp->getWindow()->getClipboard()->setText( copyCommandLine );
#if EE_PLATFORM == EE_PLATFORM_LINUX
		} else if ( id == "end-process" ) {
			requestSignal( pids, SIGTERM,
						   mApp->getUI()->i18n( "eproc_end_process", "End Process" ).toUtf8(),
						   true );
		} else if ( id == "kill-process" ) {
			requestSignal( pids, SIGKILL,
						   mApp->getUI()
							   ->i18n( "eproc_forcibly_kill_process", "Forcibly Kill Process" )
							   .toUtf8(),
						   true );
#endif
		} else {
			// Signals picked explicitly from the submenu are sent straight away: the original
			// only asks for confirmation on End Process and Forcibly Kill.
			for ( const auto& sig : signalItems ) {
				if ( id == sig.id ) {
					requestSignal(
						pids, sig.signal,
						mApp->getUI()->i18n( "eproc_send_signal", "Send Signal" ).toUtf8(), false );
					break;
				}
			}
		}
	} );

	Vector2f pos( mApp->getWindow()->getInput()->getMousePos().asFloat() );
	menu->nodeToWorldTranslation( pos );
	UIMenu::findBestMenuPos( pos, menu );
	menu->setPixelsPosition( pos );
	menu->show();
}

} // namespace eproc

EE_MAIN_FUNC int main( int, char*[] ) {
	eproc::App app;
	return app.run();
}
