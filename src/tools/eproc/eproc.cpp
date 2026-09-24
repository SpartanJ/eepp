#include "eproc.hpp"

#include <args/args.hxx>
#include <eepp/system/log.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimenuitem.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <string_view>
#if EE_PLATFORM == EE_PLATFORM_LINUX
#include <signal.h>
#include <unistd.h>
#endif

namespace eproc {

namespace {

constexpr int kProcessTableStateVersion = 4;
constexpr int kForceKillSignal = 9; // SIGKILL on Linux; TerminateProcess on Windows.
#if EE_PLATFORM == EE_PLATFORM_LINUX
constexpr int kEndProcessSignal = SIGTERM;
#else
constexpr int kEndProcessSignal = kForceKillSignal;
#endif

String endProcessLabel( UISceneNode* ui, size_t count ) {
	return count == 1 ? ui->i18n( "eproc_end_process", "End Process" )
					  : ui->i18n( "eproc_end_processes", "End Processes" );
}

String forceKillLabel( UISceneNode* ui, size_t count ) {
	return count == 1 ? ui->i18n( "eproc_forcibly_kill_process", "Forcibly Kill Process" )
					  : ui->i18n( "eproc_forcibly_kill_processes", "Forcibly Kill Processes" );
}
constexpr int kPreviousProcessTableStateVersion = 3;
constexpr int kLegacyProcessTableStateVersion = 2;
constexpr size_t kPreviousProcessColumnCount = 20;
constexpr size_t kPreviousCommandColumn = 11;

constexpr std::array<size_t, 10> kOptionalProcessColumns = { {
	ProcessModel::ColTotalMemory,
	ProcessModel::ColVirtualSize,
	ProcessModel::ColCpuTime,
	ProcessModel::ColNiceness,
	ProcessModel::ColRelativeStartTime,
	ProcessModel::ColTty,
	ProcessModel::ColIoRead,
	ProcessModel::ColIoWrite,
	ProcessModel::ColThreads,
	ProcessModel::ColMemoryPercent,
} };

#if EE_PLATFORM == EE_PLATFORM_WIN
constexpr std::array<size_t, 8> kUnavailableProcessColumns = { {
	ProcessModel::ColSharedMem,
	ProcessModel::ColGpuUsage,
	ProcessModel::ColGpuMemory,
	ProcessModel::ColDownload,
	ProcessModel::ColUpload,
	ProcessModel::ColVirtualSize,
	ProcessModel::ColNiceness,
	ProcessModel::ColTty,
} };
#else
constexpr std::array<size_t, 0> kUnavailableProcessColumns{};
#endif

bool isProcessColumnSupported( size_t column ) {
	return std::find( kUnavailableProcessColumns.begin(), kUnavailableProcessColumns.end(),
					  column ) == kUnavailableProcessColumns.end();
}

void hideUnsupportedProcessColumns( UIAbstractTableView& view ) {
	if ( !kUnavailableProcessColumns.empty() )
		view.setColumnsHidden( std::vector<size_t>( kUnavailableProcessColumns.begin(),
													kUnavailableProcessColumns.end() ),
							   true );
}

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
		 state["widths"]["widths"].size() != kPreviousProcessColumnCount )
		return false;

	nlohmann::json remappedWidths = nlohmann::json::array();
	for ( size_t column = 0; column < kPreviousProcessColumnCount; ++column )
		remappedWidths.push_back( 0 );
	for ( size_t previousColumn = 0; previousColumn < kPreviousProcessColumnCount;
		  ++previousColumn ) {
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
				 static_cast<size_t>( previousColumn ) < kPreviousProcessColumnCount )
				remappedHiddenColumns.push_back(
					remapPreviousProcessTableColumn( static_cast<size_t>( previousColumn ) ) );
		}
		state["hidden_columns"] = std::move( remappedHiddenColumns );
	}

	if ( state.contains( "sort" ) && state["sort"].is_object() &&
		 state["sort"].contains( "column" ) && state["sort"]["column"].is_number_integer() ) {
		const Int64 previousColumn = state["sort"]["column"].get<Int64>();
		if ( previousColumn >= 0 &&
			 static_cast<size_t>( previousColumn ) < kPreviousProcessColumnCount )
			state["sort"]["column"] =
				remapPreviousProcessTableColumn( static_cast<size_t>( previousColumn ) );
	}

	state["version"] = kPreviousProcessTableStateVersion;
	return true;
}

bool migrateProcessTableState( nlohmann::json& state ) {
	if ( state["version"] == kLegacyProcessTableStateVersion &&
		 !migratePreviousProcessTableState( state ) )
		return false;
	if ( state["version"] != kPreviousProcessTableStateVersion || !state.contains( "widths" ) ||
		 !state["widths"].is_object() || !state["widths"].contains( "widths" ) ||
		 !state["widths"]["widths"].is_array() ||
		 state["widths"]["widths"].size() != kPreviousProcessColumnCount )
		return false;
	state["widths"]["widths"].push_back( 0 );
	state["widths"]["widths"].push_back( 0 );
	if ( !state.contains( "hidden_columns" ) || !state["hidden_columns"].is_array() )
		state["hidden_columns"] = nlohmann::json::array();
	state["hidden_columns"].push_back( ProcessModel::ColThreads );
	state["hidden_columns"].push_back( ProcessModel::ColMemoryPercent );
	state["version"] = kProcessTableStateVersion;
	return true;
}

const char* usernameClass( const ProcessInfo& process ) {
	if ( process.status == ProcessStatus::Ended )
		return "eproc-process-username-ended";
	if ( process.tracerPid > 0 )
		return "eproc-process-username-traced";
#if EE_PLATFORM == EE_PLATFORM_LINUX
	if ( process.uid == static_cast<Int64>( getuid() ) )
#else
	if ( process.ownedByCurrentUser )
#endif
		return "eproc-process-username-own";
	if ( process.uid < 100 || !process.canLogin )
		return "eproc-process-username-system";
	return "eproc-process-username-other";
}

void setCellClassEnabled( UITableCell& cell, std::string_view className, bool enabled ) {
	const bool hasClass = cell.hasClass( className );
	if ( enabled == hasClass )
		return;
	if ( enabled )
		cell.addClass( std::string{ className } );
	else
		cell.removeClass( std::string{ className } );
}

constexpr std::array<std::string_view, 5> kUsernameClasses = {
	"eproc-process-username-ended",	 "eproc-process-username-traced", "eproc-process-username-own",
	"eproc-process-username-system", "eproc-process-username-other",
};

constexpr std::array<std::string_view, 11> kCpuFillClasses = {
	"eproc-cpu-fill-0",	 "eproc-cpu-fill-10", "eproc-cpu-fill-20",	"eproc-cpu-fill-30",
	"eproc-cpu-fill-40", "eproc-cpu-fill-50", "eproc-cpu-fill-60",	"eproc-cpu-fill-70",
	"eproc-cpu-fill-80", "eproc-cpu-fill-90", "eproc-cpu-fill-100",
};

} // namespace

App::App() {
	mConfig = std::make_unique<AppConfig>( Sys::getConfigPath( "eproc" ) );
	mConfig->load();
}

App::~App() {}

int App::run( int argc, char* argv[] ) {
	args::ArgumentParser parser( "eproc" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );
	args::ValueFlag<Float> pixelDensity( parser, "pixel-density",
										 "Set default application pixel density",
										 { 'd', "pixel-density" } );
	try {
		parser.ParseCLI( argc, argv );
	} catch ( const args::Help& ) {
		std::cout << parser;
		return EXIT_SUCCESS;
	} catch ( const args::ParseError& error ) {
		std::cerr << error.what() << '\n' << parser;
		return EXIT_FAILURE;
	} catch ( const args::ValidationError& error ) {
		std::cerr << error.what() << '\n' << parser;
		return EXIT_FAILURE;
	}
	if ( pixelDensity ) {
		if ( !std::isfinite( pixelDensity.Get() ) || pixelDensity.Get() <= 0 ) {
			std::cerr << "Pixel density must be a positive finite number\n";
			return EXIT_FAILURE;
		}
		mPixelDensity = pixelDensity.Get();
	}
	if ( !init() )
		return EXIT_FAILURE;
	const int result = mApp->run();
	if ( !mWindowStateSaved )
		saveWindowState();
	return result;
}

bool App::init() {
	// Start collection before creating the window so the first frame can use the staged snapshot.
	startCollection();
	const Sizei storedSize = mConfig->windowState.size;
	const Uint32 windowWidth =
		storedSize.getWidth() > 0 ? static_cast<Uint32>( storedSize.getWidth() ) : 1280;
	const Uint32 windowHeight =
		storedSize.getHeight() > 0 ? static_cast<Uint32>( storedSize.getHeight() ) : 720;
	WindowSettings ws( windowWidth, windowHeight, "", WindowStyle::Default, WindowBackend::Default,
					   32, Sys::getProcessPath() + "assets/icon/eproc.png" );
	ContextSettings ctx;
	ctx.Multisamples = 4;
	UIApplication::Settings settings;
	settings.pixelDensity = mPixelDensity;
	mApp = std::make_unique<UIApplication>( ws, settings, ctx );
	if ( mApp->getUI() && mApp->getWindow() ) {
		mApp->getWindow()->setTitle(
			mApp->getUI()->i18n( "eproc_window_title", "eproc - System Monitor" ) );
	}

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
	tableview::cell,
	treeview::cell {
		background-color: transparent;
		text-align: left;
	}
	tableview::cell.eproc-process-column-icon,
	treeview::cell.eproc-process-column-icon,
	tableview::cell.eproc-process-column-username,
	treeview::cell.eproc-process-column-username,
	tableview::cell.eproc-process-column-cpu,
	treeview::cell.eproc-process-column-cpu {
		text-align: center;
	}
	tableview::cell.eproc-cpu-fill-0,
	treeview::cell.eproc-cpu-fill-0 {
		background-image: none;
		background-size: 100% 0%;
	}
	tableview::cell.eproc-cpu-fill-10,
	treeview::cell.eproc-cpu-fill-10,
	tableview::cell.eproc-cpu-fill-20,
	treeview::cell.eproc-cpu-fill-20,
	tableview::cell.eproc-cpu-fill-30,
	treeview::cell.eproc-cpu-fill-30,
	tableview::cell.eproc-cpu-fill-40,
	treeview::cell.eproc-cpu-fill-40,
	tableview::cell.eproc-cpu-fill-50,
	treeview::cell.eproc-cpu-fill-50,
	tableview::cell.eproc-cpu-fill-60,
	treeview::cell.eproc-cpu-fill-60,
	tableview::cell.eproc-cpu-fill-70,
	treeview::cell.eproc-cpu-fill-70,
	tableview::cell.eproc-cpu-fill-80,
	treeview::cell.eproc-cpu-fill-80,
	tableview::cell.eproc-cpu-fill-90,
	treeview::cell.eproc-cpu-fill-90,
	tableview::cell.eproc-cpu-fill-100,
	treeview::cell.eproc-cpu-fill-100 {
		background-image: rectangle(solid, #264358);
		background-position: left bottom;
	}
	tableview::cell.eproc-cpu-fill-10,
	treeview::cell.eproc-cpu-fill-10 { background-size: 100% 10%; }
	tableview::cell.eproc-cpu-fill-20,
	treeview::cell.eproc-cpu-fill-20 { background-size: 100% 20%; }
	tableview::cell.eproc-cpu-fill-30,
	treeview::cell.eproc-cpu-fill-30 { background-size: 100% 30%; }
	tableview::cell.eproc-cpu-fill-40,
	treeview::cell.eproc-cpu-fill-40 { background-size: 100% 40%; }
	tableview::cell.eproc-cpu-fill-50,
	treeview::cell.eproc-cpu-fill-50 { background-size: 100% 50%; }
	tableview::cell.eproc-cpu-fill-60,
	treeview::cell.eproc-cpu-fill-60 { background-size: 100% 60%; }
	tableview::cell.eproc-cpu-fill-70,
	treeview::cell.eproc-cpu-fill-70 { background-size: 100% 70%; }
	tableview::cell.eproc-cpu-fill-80,
	treeview::cell.eproc-cpu-fill-80 { background-size: 100% 80%; }
	tableview::cell.eproc-cpu-fill-90,
	treeview::cell.eproc-cpu-fill-90 { background-size: 100% 90%; }
	tableview::cell.eproc-cpu-fill-100,
	treeview::cell.eproc-cpu-fill-100 { background-size: 100% 100%; }
	tableview::cell.eproc-process-username-own,
	treeview::cell.eproc-process-username-own {
		background-color: #00D0D432;
		border-top: 1dprd solid rgba(0, 255, 255, 0.026);
		border-bottom: 1dprd solid rgba(0, 255, 255, 0.085);
	}
	tableview::cell.eproc-process-username-system,
	treeview::cell.eproc-process-username-system {
		background-color: #DADCD732;
	}
	tableview::cell.eproc-process-username-other,
	treeview::cell.eproc-process-username-other {
		background-color: #029A3632;
	}
	tableview::cell.eproc-process-username-traced,
	treeview::cell.eproc-process-username-traced {
		background-color: #FFFF0088;
	}
	tableview::cell.eproc-process-username-ended,
	treeview::cell.eproc-process-username-ended {
		background-color: #D3D3D3;
	}
	tableview::cell.eproc-process-ended,
	treeview::cell.eproc-process-ended {
		color: #808080;
		tint: #808080;
	}
	tableview::cell.eproc-process-column-pid,
	treeview::cell.eproc-process-column-pid,
	tableview::cell.eproc-process-column-threads,
	treeview::cell.eproc-process-column-threads,
	tableview::cell.eproc-process-column-memory-percent,
	treeview::cell.eproc-process-column-memory-percent,
	tableview::cell.eproc-process-column-memory,
	treeview::cell.eproc-process-column-memory,
	tableview::cell.eproc-process-column-shared-memory,
	treeview::cell.eproc-process-column-shared-memory,
	tableview::cell.eproc-process-column-gpu-usage,
	treeview::cell.eproc-process-column-gpu-usage,
	tableview::cell.eproc-process-column-gpu-memory,
	treeview::cell.eproc-process-column-gpu-memory,
	tableview::cell.eproc-process-column-download,
	treeview::cell.eproc-process-column-download,
	tableview::cell.eproc-process-column-upload,
	treeview::cell.eproc-process-column-upload,
	tableview::cell.eproc-process-column-total-memory,
	treeview::cell.eproc-process-column-total-memory,
	tableview::cell.eproc-process-column-virtual-size,
	treeview::cell.eproc-process-column-virtual-size,
	tableview::cell.eproc-process-column-cpu-time,
	treeview::cell.eproc-process-column-cpu-time,
	tableview::cell.eproc-process-column-niceness,
	treeview::cell.eproc-process-column-niceness,
	tableview::cell.eproc-process-column-relative-start-time,
	treeview::cell.eproc-process-column-relative-start-time,
	tableview::cell.eproc-process-column-io-read,
	treeview::cell.eproc-process-column-io-read,
	tableview::cell.eproc-process-column-io-write,
	treeview::cell.eproc-process-column-io-write {
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
				<TreeView id="process_tree" lw="mp" lh="0" lw8="1" visible="false"
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

	if ( !mCollector ) {
		UIMessageBox* platformMessage = UIMessageBox::New(
			UIMessageBox::OK,
			ui->i18n(
				"eproc_platform_wip_message",
				"eproc is currently a work in progress. This operating system is not implemented "
				"yet." ) );
		platformMessage->setTitle( ui->i18n( "eproc_platform_wip_title", "Work in Progress" ) );
		platformMessage->center();
		platformMessage->showWhenReady();
	}

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
	if ( mProcessModel ) {
		mConfig->filterMode = mProcessModel->getFilter();
		mConfig->treeView = mConfig->filterMode == ProcessModel::AllProcessesInTreeForm;
	}
	mConfig->captureWindowState( mApp->getWindow() );
	if ( !mConfig->saveWindowState() )
		Log::error( "Could not save eproc window state to %s", mConfig->getConfigPath() );
	else
		mWindowStateSaved = true;
}

static nlohmann::json serializeProcessColumns( const UIAbstractTableView& view,
											   size_t columnCount ) {
	nlohmann::json state;
	state["widths"] = view.serializeColumnWidths();
	state["column_order"] = view.getColumnOrder();
	state["hidden_columns"] = nlohmann::json::array();
	for ( size_t column = 0; column < columnCount; ++column ) {
		if ( view.isColumnHidden( column ) )
			state["hidden_columns"].push_back( column );
	}
	return state;
}

static void restoreProcessColumns( UIAbstractTableView& view, const nlohmann::json& state,
								   size_t columnCount ) {
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
			if ( !hidden[column] && isProcessColumnSupported( column ) )
				visible.push_back( column );
		}
		// Always leave one column visible, even if a malformed state hides everything.
		if ( !visible.empty() )
			view.setColumnsVisible( visible );
	}
	hideUnsupportedProcessColumns( view );

	if ( state.contains( "widths" ) &&
		 isValidProcessTableWidthState( state["widths"], columnCount, view ) ) {
		// Pixel restoration sets each width separately, so disable automatic sizing first.
		const bool pixelWidths = state["widths"]["mode"] == "pixels";
		if ( pixelWidths )
			view.setAutoColumnsWidth( false );
		if ( !view.unserializeColumnWidths( state["widths"] ) && pixelWidths )
			view.setAutoColumnsWidth( true );
	}

	if ( state.contains( "column_order" ) && state["column_order"].is_array() &&
		 state["column_order"].size() == columnCount ) {
		std::vector<size_t> order;
		order.reserve( columnCount );
		for ( const auto& column : state["column_order"] ) {
			if ( !column.is_number_integer() || column.get<Int64>() < 0 )
				break;
			order.push_back( static_cast<size_t>( column.get<Int64>() ) );
		}
		if ( order.size() == columnCount )
			view.setColumnOrder( std::move( order ) );
	}
}

std::string App::serializeProcessTableState() const {
	if ( !mTableView || !mSortProxy )
		return {};

	nlohmann::json state = serializeProcessColumns( *mTableView, mSortProxy->columnCount() );
	state["version"] = kProcessTableStateVersion;
	state["sort"]["column"] = mSortProxy->keyColumn();
	state["sort"]["order"] = sortOrderName( mSortProxy->sortOrder() );
	if ( mTreeView )
		state["tree"] = serializeProcessColumns( *mTreeView, mTreeModel->columnCount() );
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
	if ( stateVersion == kLegacyProcessTableStateVersion ||
		 stateVersion == kPreviousProcessTableStateVersion ) {
		if ( !migrateProcessTableState( state ) )
			return;
	} else if ( stateVersion != kProcessTableStateVersion ) {
		return;
	}

	const size_t columnCount = mSortProxy->columnCount();
	if ( !state.contains( "widths" ) ||
		 !isValidProcessTableWidthState( state["widths"], columnCount, *mTableView ) )
		return;

	restoreProcessColumns( *mTableView, state, columnCount );

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
			 isProcessColumnSupported( static_cast<size_t>( column ) ) &&
			 parseSortOrder( orderName, order ) && order != SortOrder::None &&
			 mSortProxy->isColumnSortable( column ) )
			mTableView->sortByColumn( static_cast<size_t>( column ), order );
	}

	if ( mTreeView && state.contains( "tree" ) && state["tree"].is_object() )
		restoreProcessColumns( *mTreeView, state["tree"], columnCount );
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
	mTreeView = mRoot->find<UITreeView>( "process_tree" );
	mStatusText = mRoot->find<UITextView>( "process_count" );
	mCpuText = mRoot->find<UITextView>( "cpu_text" );
	mMemText = mRoot->find<UITextView>( "mem_text" );
	mSwapText = mRoot->find<UITextView>( "swap_text" );

	auto* ui = mApp->getUI();
	mRoot->find<UITab>( "tab_process_table" )
		->setText( ui->i18n( "eproc_process_table_tab", "Process Table" ) );
	mEndProcessBtn->setText( endProcessLabel( ui, 0 ) );
	mSearchInput->setHint( ui->i18n( "eproc_quick_search_hint", "Quick search" ) );
	mStatusText->setText( ui->i18n( "eproc_process_count", "0 processes" ) );
	mCpuText->setText( ui->i18n( "eproc_cpu_status", "CPU: 0%" ) );
	mMemText->setText( ui->i18n( "eproc_memory_status", "Memory: 0 / 0" ) );
	mSwapText->setText( ui->i18n( "eproc_swap_status", "Swap: 0 / 0" ) );

	// Tabs are declared in the XML layout via Tab elements with owns= attributes.

	// Filter entries mirror the original's ProcessFilter::State order.
	static const std::pair<const char*, const char*> filters[] = {
		{ "eproc_filter_all_processes", "All Processes" },
		{ "eproc_filter_all_processes_tree", "All Processes, Tree" },
		{ "eproc_filter_system_processes", "System Processes" },
		{ "eproc_filter_user_processes", "User Processes" },
		{ "eproc_filter_own_processes", "Own Processes" },
		{ "eproc_filter_programs_only", "Programs Only" },
	};
	auto* filterListBox = mFilterDropdown->getListBox();
	for ( size_t i = 0; i < std::size( filters ); ++i ) {
		if ( i == ProcessModel::ProgramsOnly &&
			 ( !mCollector || !mCollector->supportsProgramsOnly() ) )
			continue;
		filterListBox->addListBoxItem( ui->i18n( filters[i].first, filters[i].second ) );
	}
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

	mSearchInput->setFocus();
}

void App::setupProcessTable() {
	mProcessModel = ProcessModel::create( mApp->getUI() );
	mProcessModel->setDivideCpuUsage( mConfig->divideCpuUsage );
	mTreeModel = ProcessTreeModel::create( mProcessModel );
	mSortProxy = SortingProxyModel::New( mProcessModel );

	for ( UIAbstractTableView* view : { static_cast<UIAbstractTableView*>( mTableView ),
										static_cast<UIAbstractTableView*>( mTreeView ) } ) {
		if ( !view )
			continue;
		view->setSelectionKind( UIAbstractView::SelectionKind::Multiple );
		view->setColumnReorderingEnabled( true );
		if ( view == mTreeView )
			view->setModel( mTreeModel );
		else
			view->setModel( mSortProxy );
		view->setColumnsHidden(
			std::vector<size_t>( kOptionalProcessColumns.begin(), kOptionalProcessColumns.end() ),
			true );
		hideUnsupportedProcessColumns( *view );
		view->setOnUpdateCellCb( [this]( UITableCell* cell, Model* ) {
			if ( !cell )
				return;

			const ModelIndex index = cell->getCurIndex();
			const ProcessInfo* process = processForIndex( index );
			setCellClassEnabled( *cell, "eproc-process-ended",
								 process && process->status == ProcessStatus::Ended );

			const std::string_view desiredUsernameClass =
				process && index.column() == ProcessModel::ColUsername ? usernameClass( *process )
																	   : std::string_view{};
			for ( const auto className : kUsernameClasses )
				setCellClassEnabled( *cell, className, className == desiredUsernameClass );
			const int cpuUsage = process && index.column() == ProcessModel::ColCpu
									 ? mProcessModel->displayedCpuUsage( *process )
									 : 0;
			const size_t fillIndex =
				cpuUsage > 0 ? static_cast<size_t>( std::min( 10, ( cpuUsage + 9 ) / 10 ) ) : 0;
			// A removed class does not reset its CSS background image, so every reused cell
			// must receive an explicit fill class, including the 0% state.
			for ( size_t i = 0; i < kCpuFillClasses.size(); ++i ) {
				if ( i != fillIndex )
					setCellClassEnabled( *cell, kCpuFillClasses[i], false );
			}
			setCellClassEnabled( *cell, kCpuFillClasses[fillIndex], true );
		} );
		view->setOnHeaderContextMenuCb( [this]( UIPopUpMenu* menu, size_t column ) {
			for ( Uint32 i = 0; i < menu->getCount(); ) {
				const UIWidget* item = menu->getItem( i );
				if ( item->getId() == "show-column" &&
					 !isProcessColumnSupported( static_cast<size_t>( item->getData() ) ) )
					menu->remove( i );
				else
					++i;
			}
			if ( column != ProcessModel::ColCpu )
				return;
			menu->addSeparator();
			menu->addCheckBox( mApp->getUI()->i18n( "eproc_divide_cpu_usage",
													"Divide CPU usage by number of CPUs" ),
							   mConfig->divideCpuUsage )
				->setId( "divide-cpu-usage" );
			menu->on( Event::OnItemClicked, [this]( const Event* event ) {
				if ( event->getNode()->getId() != "divide-cpu-usage" )
					return;
				std::vector<Int64> selected = selectedPids();
				captureTreeExpansion();
				if ( mTreeView )
					mTreeView->clearViewMetadata();
				mConfig->divideCpuUsage = !mConfig->divideCpuUsage;
				mProcessModel->setDivideCpuUsage( mConfig->divideCpuUsage );
				restoreTreeExpansion();
				restoreSelection( selected );
			} );
		} );
		view->setRowHeight( 28 );
		// The flexible column is Name; the icon column is fixed so every row lines up.
		view->setMainColumn( ProcessModel::ColName );
		view->setSortIconSize( 12 );
		view->setIconSize( PixelDensity::dpToPxI( 16 ) );
		view->setColumnWidth( ProcessModel::ColIcon, PixelDensity::dpToPx( 26 ) );

		view->setOnSelectionChange( [this]() { onSelectionChange(); } );
		view->onModelEvent( [this]( const ModelEvent* event ) {
			if ( event->getModelEventType() == ModelEventType::OpenMenu )
				showProcessContextMenu( event->getModelIndex() );
		} );
	}

	// The original opens sorted by memory usage, descending, which surfaces the heavy processes
	// instead of the kernel threads that /proc happens to enumerate first. Sorting through the
	// view (not the model directly) also renders the sort indicator in the header.
	mTableView->sortByColumn( ProcessModel::ColMemory, SortOrder::Descending );
	if ( mConfig->filterMode == ProcessModel::ProgramsOnly &&
		 ( !mCollector || !mCollector->supportsProgramsOnly() ) )
		mConfig->filterMode = ProcessModel::AllProcesses;
	if ( mConfig->filterMode > ProcessModel::AllProcesses &&
		 mConfig->filterMode < ProcessModel::FilterModeCount ) {
		mFilterDropdown->getListBox()->setSelected( mConfig->filterMode );
		if ( mProcessModel->getFilter() != mConfig->filterMode )
			onFilterChanged();
	}
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
	if ( mProcessModel && mCollector && mCollector->supportsProgramsOnly() ) {
		mGuiWindows.refresh();
		mProcessModel->setGuiWindowPids( UnorderedSet<Int64>( mGuiWindows.windowPids().begin(),
															  mGuiWindows.windowPids().end() ) );
		for ( auto& process : processes ) {
			if ( process.iconPath.empty() && mGuiWindows.hasWindowForPid( process.pid ) )
				process.windowIcon = mGuiWindows.iconForPid( process.pid );
		}
	}

	// A full model reset clears the view's selection (SortingProxyModel drops it on every
	// invalidation), so selected processes are remembered by PID and re-selected afterwards. PIDs
	// are stable across snapshots while row indexes are not: the table re-sorts on every update.
	std::vector<Int64> selected = selectedPids();
	captureTreeExpansion();
	if ( mTreeView )
		mTreeView->clearViewMetadata();

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
	restoreTreeExpansion();
	restoreSelection( selected );
}

UIAbstractTableView* App::activeProcessView() const {
	return mTreeMode ? static_cast<UIAbstractTableView*>( mTreeView )
					 : static_cast<UIAbstractTableView*>( mTableView );
}

void App::captureTreeExpansion() {
	if ( !mTreeMode || !mTreeExpansionInitialized || !mTreeView || !mTreeModel || !mProcessModel ||
		 mTreeSearchActive )
		return;
	mExpandedTreePids.clear();
	for ( size_t row = 0; row < mProcessModel->visibleCount(); ++row ) {
		const ProcessInfo* process = mProcessModel->getProcessByRow( static_cast<int>( row ) );
		if ( process && mTreeView->isExpanded( mTreeModel->indexForPid( process->pid ) ) )
			mExpandedTreePids.push_back( process->pid );
	}
}

void App::restoreTreeExpansion() {
	if ( !mTreeMode || !mTreeView || !mTreeModel )
		return;
	std::vector<ModelIndex> indexes;
	if ( mTreeSearchActive ) {
		for ( Int64 pid : mProcessModel->textMatchedPids() ) {
			ModelIndex parent = mTreeModel->indexForPid( pid ).parent();
			while ( parent.isValid() ) {
				indexes.push_back( parent );
				parent = parent.parent();
			}
		}
		if ( !indexes.empty() )
			mTreeView->setExpanded( indexes, true );
		return;
	}
	if ( !mTreeExpansionInitialized ) {
		for ( size_t row = 0; row < mTreeModel->rowCount(); ++row )
			indexes.push_back(
				mTreeModel->index( static_cast<int>( row ), mTreeModel->treeColumn() ) );
		mTreeExpansionInitialized = !indexes.empty();
	} else {
		indexes.reserve( mExpandedTreePids.size() );
		for ( Int64 pid : mExpandedTreePids ) {
			ModelIndex index = mTreeModel->indexForPid( pid );
			if ( index.isValid() )
				indexes.push_back( index );
		}
	}
	if ( !indexes.empty() )
		mTreeView->setExpanded( indexes, true );
}

void App::restoreSelection( const std::vector<Int64>& pids ) {
	UIAbstractTableView* view = activeProcessView();
	if ( pids.empty() || !view || !mProcessModel )
		return;

	std::vector<ModelIndex> indexes;
	indexes.reserve( pids.size() );
	for ( Int64 pid : pids ) {
		ModelIndex index;
		if ( mTreeMode ) {
			index = mTreeModel->indexForPid( pid );
		} else {
			int row = mProcessModel->rowForPid( pid );
			if ( row >= 0 )
				index = mSortProxy->mapToProxy( mProcessModel->index( row, 0 ) );
		}
		if ( index.isValid() )
			indexes.push_back( index );
	}
	if ( !indexes.empty() ) {
		view->getSelection().set( indexes );
		if ( mTreeMode )
			mTreeView->openModelIndexParentTree( indexes.front() );
	}
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
	std::vector<Int64> pids = selectedPids();
	const std::string label = endProcessLabel( mApp->getUI(), pids.size() ).toUtf8();
	requestSignal( std::move( pids ), kEndProcessSignal, label, true );
}

void App::onSearchChanged() {
	if ( mSearchInput && mProcessModel ) {
		std::vector<Int64> selected = selectedPids();
		captureTreeExpansion();
		if ( mTreeView )
			mTreeView->clearViewMetadata();
		const std::string text = mSearchInput->getText().toUtf8();
		mTreeSearchActive = !text.empty();
		mProcessModel->setTextFilter( text );
		restoreTreeExpansion();
		restoreSelection( selected );
		updateStatusBar();
	}
}

void App::onFilterChanged() {
	if ( mFilterDropdown && mProcessModel ) {
		Uint32 selected = mFilterDropdown->getListBox()->getItemSelectedIndex();
		if ( selected >= ProcessModel::FilterModeCount )
			return;
		std::vector<Int64> selectedPidsBeforeSwitch = selectedPids();
		captureTreeExpansion();
		if ( mTreeView )
			mTreeView->clearViewMetadata();
		mProcessModel->setFilter( static_cast<ProcessModel::FilterMode>( selected ) );
		mTreeMode = selected == ProcessModel::AllProcessesInTreeForm;
		mTableView->setVisible( !mTreeMode );
		mTreeView->setVisible( mTreeMode );
		restoreTreeExpansion();
		restoreSelection( selectedPidsBeforeSwitch );
		updateStatusBar();
	}
}

void App::onSelectionChange() {
	// Update End Process button state
	UIAbstractTableView* view = activeProcessView();
	if ( mEndProcessBtn && view ) {
		bool hasProcess = false;
		size_t processCount = 0;
		for ( const auto& index : view->getSelection().indexes() ) {
			const ProcessInfo* proc = processForIndex( index );
			if ( proc && proc->status != ProcessStatus::Ended ) {
				hasProcess = true;
				++processCount;
			}
		}
		mEndProcessBtn->setEnabled( hasProcess );
		mEndProcessBtn->setText( endProcessLabel( mApp->getUI(), processCount ) );
	}
}

const ProcessInfo* App::processForIndex( const ModelIndex& index ) const {
	if ( !mProcessModel || !index.isValid() )
		return nullptr;
	if ( index.model() == mTreeModel.get() )
		return mTreeModel->processForIndex( index );
	if ( index.model() == mSortProxy.get() )
		return mProcessModel->getProcessByRow( mSortProxy->mapToSource( index ).row() );
	return nullptr;
}

std::vector<Int64> App::selectedPids() const {
	std::vector<Int64> pids;

	UIAbstractTableView* view = activeProcessView();
	if ( !mProcessModel || !view )
		return pids;

	const auto indexes = view->getSelection().indexes();
	pids.reserve( indexes.size() );
	for ( const auto& proxyIndex : indexes ) {
		const ProcessInfo* proc = processForIndex( proxyIndex );
		if ( proc && proc->status != ProcessStatus::Ended )
			pids.push_back( proc->pid );
	}

	return pids;
}

void App::selectProcess( Int64 pid ) {
	UIAbstractTableView* view = activeProcessView();
	if ( pid <= 0 || !mProcessModel || !view )
		return;

	int row = mProcessModel->rowForPid( pid );

	// The active filter may be hiding the target, so drop it instead of silently doing nothing
	// (the original clears its text filter for the same reason).
	if ( row < 0 && mSearchInput && !mSearchInput->getText().empty() ) {
		mSearchInput->setText( "" );
		if ( mTreeSearchActive )
			onSearchChanged();
		row = mProcessModel->rowForPid( pid );
	}

	if ( row < 0 )
		return;

	ModelIndex index = mTreeMode ? mTreeModel->indexForPid( pid )
								 : mSortProxy->mapToProxy( mProcessModel->index( row, 0 ) );
	if ( index.isValid() ) {
		if ( mTreeMode )
			mTreeView->openModelIndexParentTree( index );
		view->setSelection( index );
	}
}

void App::requestSignal( std::vector<Int64> pids, int signal, const std::string& actionLabel,
						 bool confirm ) {
	if ( pids.empty() )
		return;

	const size_t processCount = pids.size();
	const Int64 firstPid = pids.front();
	auto send = [pids = std::move( pids ), signal]() {
		for ( Int64 pid : pids )
			sendProcessSignal( pid, signal );
	};

	if ( !confirm ) {
		send();
		return;
	}

	const std::string target =
		processCount == 1
			? String::format(
				  mApp->getUI()->i18n( "eproc_process_target", "process %lld" ).toUtf8(),
				  static_cast<long long>( firstPid ) )
			: String::format(
				  mApp->getUI()->i18n( "eproc_processes_target", "%zu processes" ).toUtf8(),
				  processCount );
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
	UIAbstractTableView* view = activeProcessView();
	if ( !view )
		return;

	// Right-clicking outside the selection moves the selection to the clicked row, as the
	// original does, so the menu always acts on what the user pointed at.
	if ( const ProcessInfo* clicked = processForIndex( proxyIndex ) ) {
		bool alreadySelected = false;
		for ( const auto& selected : view->getSelection().indexes() ) {
			const ProcessInfo* process = processForIndex( selected );
			if ( process && process->pid == clicked->pid ) {
				alreadySelected = true;
				break;
			}
		}
		if ( !alreadySelected )
			view->setSelection( proxyIndex, false );
	}

	const std::vector<Int64> pids = selectedPids();
	if ( pids.empty() )
		return;

	const bool singleProcess = view->getSelection().size() == 1;
	const ProcessInfo* proc = singleProcess ? processForIndex( proxyIndex ) : nullptr;
	const Int64 parentPid = proc ? proc->parentPid : 0;
	const Int64 tracerPid = proc ? proc->tracerPid : 0;
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

	UIPopUpMenu* menu = UIPopUpMenu::New();
	menu->setId( "process_context_menu" );
#if EE_PLATFORM == EE_PLATFORM_LINUX
	UIPopUpMenu* signalMenu = UIPopUpMenu::New();
	signalMenu->setId( "process_signal_menu" );
	for ( const auto& item : signalItems )
		signalMenu->add( mApp->getUI()->i18n( item.key, item.label ) )->setId( item.id );
	menu->addSubMenu( mApp->getUI()->i18n( "eproc_send_signal", "Send Signal" ), nullptr,
					  signalMenu )
		->setId( "send-signal" );
#endif
	if ( singleProcess && proc ) {
		menu->add( mApp->getUI()->i18n( "eproc_jump_to_parent", "Jump to Parent Process" ) )
			->setId( "jump-parent" );
		if ( tracerPid > 0 ) {
			menu->add( mApp->getUI()->i18n( "eproc_jump_to_tracer",
											"Jump to Process Debugging This One" ) )
				->setId( "jump-tracer" );
		}
		menu->add( mApp->getUI()->i18n( "eproc_copy_command_line", "Copy Command Line" ) )
			->setId( "copy-command-line" );
	}

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_WIN
	menu->addSeparator();
	menu->add( endProcessLabel( mApp->getUI(), pids.size() ) )->setId( "end-process" );
	menu->add( forceKillLabel( mApp->getUI(), pids.size() ) )->setId( "kill-process" );
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
		} else if ( id == "end-process" ) {
			requestSignal( pids, kEndProcessSignal,
						   endProcessLabel( mApp->getUI(), pids.size() ).toUtf8(), true );
		} else if ( id == "kill-process" ) {
			requestSignal( pids, kForceKillSignal,
						   forceKillLabel( mApp->getUI(), pids.size() ).toUtf8(), true );
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

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	eproc::App app;
	return app.run( argc, argv );
}
