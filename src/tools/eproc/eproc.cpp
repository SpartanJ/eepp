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
#include <limits>
#include <string_view>
#include <unordered_map>
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_MACOS || \
	EE_PLATFORM == EE_PLATFORM_BSD
#include <signal.h>
#include <unistd.h>
#endif

namespace eproc {

namespace {

constexpr size_t kPerformanceHistorySamples = 600;
constexpr double kPerformanceVisibleSeconds = 60.0;

constexpr int kProcessTableStateVersion = 6;
constexpr int kForceKillSignal = 9; // SIGKILL on Linux; TerminateProcess on Windows.
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_MACOS || \
	EE_PLATFORM == EE_PLATFORM_BSD
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
constexpr int kPriorProcessTableStateVersion = 4;
constexpr int kFamilyProcessTableStateVersion = 5;
constexpr size_t kPreviousProcessColumnCount = 20;
constexpr size_t kPreviousCommandColumn = 11;

constexpr std::array<size_t, 12> kOptionalProcessColumns = { {
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
	ProcessModel::ColFamilyMemory,
	ProcessModel::ColFamilyMemoryPercent,
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
#elif EE_PLATFORM == EE_PLATFORM_MACOS
constexpr std::array<size_t, 6> kUnavailableProcessColumns = { {
	ProcessModel::ColSharedMem,
	ProcessModel::ColGpuUsage,
	ProcessModel::ColGpuMemory,
	ProcessModel::ColDownload,
	ProcessModel::ColUpload,
	ProcessModel::ColTty,
} };
#elif EE_PLATFORM == EE_PLATFORM_BSD
constexpr std::array<size_t, 8> kUnavailableProcessColumns = { {
	ProcessModel::ColSharedMem,
	ProcessModel::ColGpuUsage,
	ProcessModel::ColGpuMemory,
	ProcessModel::ColDownload,
	ProcessModel::ColUpload,
	ProcessModel::ColTty,
	ProcessModel::ColIoRead,
	ProcessModel::ColIoWrite,
} };
#else
constexpr std::array<size_t, 0> kUnavailableProcessColumns{};
#endif

bool isProcessColumnSupported( size_t column ) {
	return std::find( kUnavailableProcessColumns.begin(), kUnavailableProcessColumns.end(),
					  column ) == kUnavailableProcessColumns.end();
}

String processColumnTooltip( UISceneNode* ui, size_t column ) {
	switch ( column ) {
		case ProcessModel::ColIcon:
			return ui->i18n( "eproc_tip_icon", "Application icon, when available." );
		case ProcessModel::ColName:
			return ui->i18n( "eproc_tip_name", "Process name reported by the operating system." );
		case ProcessModel::ColPid:
			return ui->i18n( "eproc_tip_pid", "Process identifier (PID)." );
		case ProcessModel::ColUsername:
			return ui->i18n( "eproc_tip_username", "User account that owns the process." );
		case ProcessModel::ColCpu:
			return ui->i18n( "eproc_tip_cpu", "CPU usage during the latest sampling interval." );
		case ProcessModel::ColMemory:
#if EE_PLATFORM == EE_PLATFORM_LINUX
			return ui->i18n(
				"eproc_tip_memory_linux",
				"Anonymous resident memory (RssAnon); falls back to RSS when unavailable." );
#elif EE_PLATFORM == EE_PLATFORM_WIN
			return ui->i18n(
				"eproc_tip_memory_windows",
				"Private committed memory; falls back to resident memory when unavailable." );
#else
			return ui->i18n( "eproc_tip_memory", "Resident memory of this process." );
#endif
		case ProcessModel::ColSharedMem:
			return ui->i18n( "eproc_tip_shared_memory",
							 "File-backed and shared resident memory (RssFile + RssShmem)." );
		case ProcessModel::ColGpuUsage:
			return ui->i18n( "eproc_tip_gpu_usage", "GPU engine usage reported for this process." );
		case ProcessModel::ColGpuMemory:
			return ui->i18n( "eproc_tip_gpu_memory", "GPU memory reported for this process." );
		case ProcessModel::ColDownload:
			return ui->i18n( "eproc_tip_download",
							 "Estimated network receive rate for this process." );
		case ProcessModel::ColUpload:
			return ui->i18n( "eproc_tip_upload", "Estimated network send rate for this process." );
		case ProcessModel::ColTotalMemory:
			return ui->i18n( "eproc_tip_resident_memory",
							 "Resident memory of this process (RSS or working set). Shared pages "
							 "can appear in multiple processes." );
		case ProcessModel::ColVirtualSize:
			return ui->i18n( "eproc_tip_virtual_size",
							 "Virtual address space reserved or mapped by this process." );
		case ProcessModel::ColCpuTime:
			return ui->i18n( "eproc_tip_cpu_time", "Total CPU time consumed by this process." );
		case ProcessModel::ColNiceness:
			return ui->i18n( "eproc_tip_niceness", "Scheduling nice value of this process." );
		case ProcessModel::ColRelativeStartTime:
			return ui->i18n( "eproc_tip_start_time", "Time elapsed since this process started." );
		case ProcessModel::ColTty:
			return ui->i18n( "eproc_tip_tty", "Controlling terminal of this process." );
		case ProcessModel::ColIoRead:
			return ui->i18n( "eproc_tip_io_read", "Bytes read by this process since it started." );
		case ProcessModel::ColIoWrite:
			return ui->i18n( "eproc_tip_io_write",
							 "Bytes written by this process since it started." );
		case ProcessModel::ColCommand:
			return ui->i18n( "eproc_tip_command",
							 "Executable path and arguments, when available." );
		case ProcessModel::ColThreads:
			return ui->i18n( "eproc_tip_threads", "Number of threads in this process." );
		case ProcessModel::ColMemoryPercent:
			return ui->i18n( "eproc_tip_memory_percent",
							 "Memory column as a percentage of total physical memory." );
		case ProcessModel::ColFamilyMemory:
#if EE_PLATFORM == EE_PLATFORM_LINUX
			return ui->i18n(
				"eproc_tip_family_memory_linux",
				"Sum of proportional resident memory (PSS) for this process and all live "
				"descendants. Shared pages are apportioned; blank if any member is unavailable." );
#else
			return ui->i18n( "eproc_tip_family_memory",
							 "Sum of the Memory column for this process and all live descendants. "
							 "Approximate application memory." );
#endif
		case ProcessModel::ColFamilyMemoryPercent:
			return ui->i18n( "eproc_tip_family_memory_percent",
							 "Family Memory as a percentage of total physical memory." );
		default:
			return {};
	}
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
	if ( state["version"] == kPreviousProcessTableStateVersion ) {
		if ( !state.contains( "widths" ) || !state["widths"].is_object() ||
			 !state["widths"].contains( "widths" ) || !state["widths"]["widths"].is_array() ||
			 state["widths"]["widths"].size() != kPreviousProcessColumnCount )
			return false;
		state["widths"]["widths"].push_back( 0 );
		state["widths"]["widths"].push_back( 0 );
		if ( !state.contains( "hidden_columns" ) || !state["hidden_columns"].is_array() )
			state["hidden_columns"] = nlohmann::json::array();
		state["hidden_columns"].push_back( ProcessModel::ColThreads );
		state["hidden_columns"].push_back( ProcessModel::ColMemoryPercent );
		if ( state.contains( "column_order" ) && state["column_order"].is_array() &&
			 state["column_order"].size() == kPreviousProcessColumnCount ) {
			state["column_order"].push_back( ProcessModel::ColThreads );
			state["column_order"].push_back( ProcessModel::ColMemoryPercent );
		}
		state["version"] = kPriorProcessTableStateVersion;
	}
	if ( state["version"] == kPriorProcessTableStateVersion ) {
		const auto addFamilyColumn = []( nlohmann::json& columns ) {
			if ( !columns.contains( "widths" ) || !columns["widths"].is_object() ||
				 !columns["widths"].contains( "widths" ) ||
				 !columns["widths"]["widths"].is_array() ||
				 columns["widths"]["widths"].size() != ProcessModel::ColFamilyMemory )
				return false;
			columns["widths"]["widths"].push_back( 0 );
			if ( !columns.contains( "hidden_columns" ) || !columns["hidden_columns"].is_array() )
				columns["hidden_columns"] = nlohmann::json::array();
			columns["hidden_columns"].push_back( ProcessModel::ColFamilyMemory );
			if ( columns.contains( "column_order" ) && columns["column_order"].is_array() &&
				 columns["column_order"].size() == ProcessModel::ColFamilyMemory )
				columns["column_order"].push_back( ProcessModel::ColFamilyMemory );
			return true;
		};
		if ( !addFamilyColumn( state ) )
			return false;
		if ( state.contains( "tree" ) && state["tree"].is_object() &&
			 !addFamilyColumn( state["tree"] ) )
			state.erase( "tree" );
		state["version"] = kFamilyProcessTableStateVersion;
	}
	if ( state["version"] != kFamilyProcessTableStateVersion )
		return false;
	const auto addFamilyPercentColumn = []( nlohmann::json& columns ) {
		if ( !columns.contains( "widths" ) || !columns["widths"].is_object() ||
			 !columns["widths"].contains( "widths" ) || !columns["widths"]["widths"].is_array() ||
			 columns["widths"]["widths"].size() != ProcessModel::ColFamilyMemoryPercent )
			return false;
		columns["widths"]["widths"].push_back( 0 );
		if ( !columns.contains( "hidden_columns" ) || !columns["hidden_columns"].is_array() )
			columns["hidden_columns"] = nlohmann::json::array();
		columns["hidden_columns"].push_back( ProcessModel::ColFamilyMemoryPercent );
		if ( columns.contains( "column_order" ) && columns["column_order"].is_array() &&
			 columns["column_order"].size() == ProcessModel::ColFamilyMemoryPercent ) {
			const auto& oldOrder = columns["column_order"];
			bool naturalOrder = true;
			for ( size_t i = 0; i < oldOrder.size(); ++i ) {
				if ( !oldOrder[i].is_number_integer() ||
					 oldOrder[i].get<Int64>() != static_cast<Int64>( i ) ) {
					naturalOrder = false;
					break;
				}
			}
			nlohmann::json order = nlohmann::json::array();
			if ( naturalOrder ) {
				for ( size_t column = 0; column < ProcessModel::ColFamilyMemoryPercent; ++column ) {
					if ( column != ProcessModel::ColCommand )
						order.push_back( column );
				}
				order.push_back( ProcessModel::ColFamilyMemoryPercent );
				order.push_back( ProcessModel::ColCommand );
			} else {
				bool inserted = false;
				for ( const auto& column : oldOrder ) {
					if ( column == ProcessModel::ColCommand ) {
						order.push_back( ProcessModel::ColFamilyMemoryPercent );
						inserted = true;
					}
					order.push_back( column );
				}
				if ( !inserted )
					order.push_back( ProcessModel::ColFamilyMemoryPercent );
			}
			columns["column_order"] = std::move( order );
		}
		return true;
	};
	if ( !addFamilyPercentColumn( state ) )
		return false;
	if ( state.contains( "tree" ) && state["tree"].is_object() &&
		 !addFamilyPercentColumn( state["tree"] ) )
		state.erase( "tree" );
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

std::string formatPerformanceRate( Int64 bytes ) {
	return bytes > 0 ? formatBytesPerSecond( bytes ) : "0 B/s";
}

Color themeColor( UISceneNode* ui, const char* name, Color fallback ) {
	const auto value = ui->getRoot()->getUIStyle()->getVariable( name );
	return value.isEmpty() ? fallback : Color::fromString( value.getValue() );
}

} // namespace

App::App() {
	mConfig = std::make_unique<AppConfig>( Sys::getConfigPath( "eproc" ) );
	mConfig->load();
}

App::~App() {
	// The worker may still publish a snapshot through mStagingMutex. Join it before member
	// destruction reaches the staging state and mutex.
	mThreadPool.reset();
}

int App::run( int argc, char* argv[] ) {
	args::ArgumentParser parser( "eproc" );
	args::HelpFlag help( parser, "help", "Display this help menu", { 'h', "help" } );
	args::ValueFlag<Float> pixelDensity( parser, "pixel-density",
										 "Set default application pixel density",
										 { 'd', "pixel-density" } );
	const std::unordered_map<std::string, FontHinting> fontHintingMap{
		{ "none", FontHinting::None },
		{ "slight", FontHinting::Slight },
		{ "full", FontHinting::Full } };
	args::MapFlag<std::string, FontHinting> fontHinting(
		parser, "font-hinting", "Font hinting mode (accepted values: none, slight, full)",
		{ "font-hinting" }, fontHintingMap, mFontHinting );
	const std::unordered_map<std::string, FontAntialiasing> fontAntialiasingMap{
		{ "none", FontAntialiasing::None },
		{ "grayscale", FontAntialiasing::Grayscale },
		{ "subpixel", FontAntialiasing::Subpixel } };
	args::MapFlag<std::string, FontAntialiasing> fontAntialiasing(
		parser, "font-antialiasing",
		"Font antialiasing mode (accepted values: none, grayscale, subpixel)",
		{ "font-antialiasing" }, fontAntialiasingMap, mFontAntialiasing );
	args::ValueFlag<std::string> prefersColorScheme(
		parser, "prefers-color-scheme",
		"Set the preferred color scheme (\"light\", \"dark\" or \"system\")",
		{ 'c', "prefers-color-scheme" } );
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
	mFontHinting = fontHinting.Get();
	mFontAntialiasing = fontAntialiasing.Get();
	if ( prefersColorScheme ) {
		const std::string& scheme = prefersColorScheme.Get();
		if ( scheme != "light" && scheme != "dark" && scheme != "system" ) {
			std::cerr << "Color scheme must be light, dark, or system\n";
			return EXIT_FAILURE;
		}
		mColorScheme = ColorSchemePreferences::fromStringExt( scheme );
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
	settings.fontHinting = mFontHinting;
	settings.fontAntialiasing = mFontAntialiasing;
	mApp = std::make_unique<UIApplication>( ws, settings, ctx );
	if ( mApp->getUI() && mApp->getWindow() ) {
		mApp->getWindow()->setTitle(
			mApp->getUI()->i18n( "eproc_window_title", "eproc - System Monitor" ) );
	}

	auto* ui = mApp->getUI();
	if ( !ui )
		return false;
	ui->setColorSchemePreference( mColorScheme );

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
		background-image: rectangle(solid, var(--item-hover));
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
		color: var(--disabled-color);
		tint: var(--disabled-color);
	}
	tableview::cell.eproc-process-column-icon,
	treeview::cell.eproc-process-column-icon {
		tint: white;
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
	.eproc-performance-sidebar { background-color: var(--tab-back); }
	.eproc-performance-card {
		background-color: var(--list-back);
		border: 1dp solid var(--button-border);
		border-radius: 5dp;
	}
	.eproc-performance-card:hover {
		border: 1dp solid var(--primary);
	}
	.eproc-performance-card-selected { border: 2dp solid var(--primary); }
	.eproc-performance-card.eproc-performance-card-selected:hover { border: 3dp solid var(--primary); }
	.eproc-performance-muted { color: var(--font-hint); }
	.eproc-performance-chart { background-color: var(--list-back); }
	.eproc-performance-preview {
		background-color: var(--list-back);
		border: 1dp solid var(--button-border);
		border-radius: 4dp;
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
			<hbox id="performance_area" lw="mp" lh="mp">
				<vbox class="eproc-performance-sidebar" lw="276dp" lh="mp" padding="8dp">
					<hbox id="perf_cpu_card" class="eproc-performance-card" lw="mp" lh="72dp" padding="5dp" margin-bottom="6dp">
						<Chart id="perf_cpu_preview" class="eproc-performance-preview" lw="80dp" lh="mp" margin-right="8dp" />
						<vbox lw="0" lw8="1" lh="mp"><TextView text="CPU" lw="mp" lh="wc" /><TextView id="perf_cpu_value" lw="mp" lh="wc" /></vbox>
					</hbox>
					<hbox id="perf_memory_card" class="eproc-performance-card" lw="mp" lh="72dp" padding="5dp" margin-bottom="6dp">
						<Chart id="perf_memory_preview" class="eproc-performance-preview" lw="80dp" lh="mp" margin-right="8dp" />
						<vbox lw="0" lw8="1" lh="mp"><TextView text="Memory" lw="mp" lh="wc" /><TextView id="perf_memory_value" lw="mp" lh="wc" /></vbox>
					</hbox>
					<hbox id="perf_gpu_card" class="eproc-performance-card" lw="mp" lh="72dp" padding="5dp" margin-bottom="6dp">
						<Chart id="perf_gpu_preview" class="eproc-performance-preview" lw="80dp" lh="mp" margin-right="8dp" />
						<vbox lw="0" lw8="1" lh="mp"><TextView text="GPU" lw="mp" lh="wc" /><TextView id="perf_gpu_value" lw="mp" lh="wc" /></vbox>
					</hbox>
					<hbox id="perf_network_card" class="eproc-performance-card" lw="mp" lh="72dp" padding="5dp">
						<Chart id="perf_network_preview" class="eproc-performance-preview" lw="80dp" lh="mp" margin-right="8dp" />
						<vbox lw="0" lw8="1" lh="mp"><TextView text="Network" lw="mp" lh="wc" /><TextView id="perf_network_value" lw="mp" lh="wc" /></vbox>
					</hbox>
				</vbox>
				<vbox lw="0" lw8="1" lh="mp" padding="14dp">
					<hbox lw="mp" lh="wc" margin-bottom="8dp">
						<TextView id="perf_title" lw="0" lw8="1" lh="wc" />
						<hbox id="perf_cpu_mode" lw="wc" lh="wc">
							<SelectButton id="perf_overall" text="Overall" lw="wc" lh="wc" margin-right="4dp" selected="true" />
							<SelectButton id="perf_per_core" text="Per core" lw="wc" lh="wc" />
						</hbox>
					</hbox>
					<TextView id="perf_summary" class="eproc-performance-muted" lw="mp" lh="wc" margin-bottom="8dp" />
					<Chart id="perf_detail_chart" class="eproc-performance-chart" lw="mp" lh="0" lw8="1" />
					<ScrollView id="perf_cores_scroll" lw="mp" lh="0" lw8="1" visible="false">
						<StackLayout id="perf_cores_stack" lw="mp" lh="wc" row-valign="top" />
					</ScrollView>
					<TextView id="perf_detail" lw="mp" lh="wc" margin-top="12dp" />
				</vbox>
			</hbox>
			<!-- Tab definitions -->
			<Tab id="tab_process_table" owns="process_table_area" />
			<Tab id="tab_performance" text="Performance" owns="performance_area" />
		</TabWidget>
	</vbox>
	)xml" );

	if ( !mRoot ) {
		return false;
	}

	setupUI();
	setupProcessTable();
	setupPerformance();

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
	if ( mTreeView ) {
		state["tree"] = serializeProcessColumns( *mTreeView, mTreeModel->columnCount() );
		state["tree"]["sort"]["column"] = mTreeModel->keyColumn();
		state["tree"]["sort"]["order"] = sortOrderName( mTreeModel->sortOrder() );
	}
	return state.dump();
}

static void restoreProcessSort( UIAbstractTableView& view, const nlohmann::json& state,
								size_t columnCount ) {
	if ( !state.contains( "sort" ) || !state["sort"].is_object() )
		return;
	const auto& sort = state["sort"];
	const int column = sort.contains( "column" ) && sort["column"].is_number_integer()
						   ? sort["column"].get<int>()
						   : -1;
	const std::string orderName = sort.contains( "order" ) && sort["order"].is_string()
									  ? sort["order"].get<std::string>()
									  : "none";
	SortOrder order = SortOrder::None;
	Model* model = view.getModel();
	if ( model && column >= 0 && static_cast<size_t>( column ) < columnCount &&
		 isProcessColumnSupported( static_cast<size_t>( column ) ) &&
		 parseSortOrder( orderName, order ) && order != SortOrder::None &&
		 model->isColumnSortable( column ) )
		view.sortByColumn( static_cast<size_t>( column ), order );
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
		 stateVersion == kPreviousProcessTableStateVersion ||
		 stateVersion == kPriorProcessTableStateVersion ||
		 stateVersion == kFamilyProcessTableStateVersion ) {
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
	restoreProcessSort( *mTableView, state, columnCount );

	if ( mTreeView && state.contains( "tree" ) && state["tree"].is_object() ) {
		restoreProcessColumns( *mTreeView, state["tree"], columnCount );
		restoreProcessSort( *mTreeView, state["tree"], columnCount );
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
	mTreeView = mRoot->find<UITreeView>( "process_tree" );
	mStatusText = mRoot->find<UITextView>( "process_count" );
	mCpuText = mRoot->find<UITextView>( "cpu_text" );
	mMemText = mRoot->find<UITextView>( "mem_text" );
	mSwapText = mRoot->find<UITextView>( "swap_text" );

	auto* ui = mApp->getUI();
	mRoot->find<UITab>( "tab_process_table" )
		->setText( ui->i18n( "eproc_process_table_tab", "Process Table" ) );
	mRoot->find<UITab>( "tab_performance" )
		->setText( ui->i18n( "eproc_performance_tab", "Performance" ) );
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

void App::setupPerformance() {
	constexpr std::array<const char*, 4> ids = { "cpu", "memory", "gpu", "network" };
	constexpr std::array<const char*, 4> names = { "CPU", "Memory", "GPU", "Network" };
	const Color primary = themeColor( mApp->getUI(), "--primary", Color( 49, 178, 224 ) );
	const Color gridColor =
		Color( themeColor( mApp->getUI(), "--separator", Color( 100, 113, 124 ) ), 45 );
	const std::array<Color, 4> colors = {
		primary,
		themeColor( mApp->getUI(), "--theme-warning", Color( 190, 151, 226 ) ),
		themeColor( mApp->getUI(), "--theme-success", Color( 119, 203, 135 ) ),
		themeColor( mApp->getUI(), "--font-highlight", Color( 91, 196, 223 ) ),
	};
	mPerformanceDetailChart = mRoot->find<UIChart>( "perf_detail_chart" );
	mPerformanceTitle = mRoot->find<UITextView>( "perf_title" );
	mPerformanceSummary = mRoot->find<UITextView>( "perf_summary" );
	mPerformanceDetail = mRoot->find<UITextView>( "perf_detail" );
	mCoreChartScroll = mRoot->find<UIWidget>( "perf_cores_scroll" );
	mCoreChartStack = mRoot->find<UIWidget>( "perf_cores_stack" );
	mCpuModeControls = mRoot->find<UIWidget>( "perf_cpu_mode" );
	mOverallButton = mRoot->find<UISelectButton>( "perf_overall" );
	mPerCoreButton = mRoot->find<UISelectButton>( "perf_per_core" );
	mOverallButton->onClick( [this]( const MouseEvent* ) { setPerCoreView( false ); } );
	mPerCoreButton->onClick( [this]( const MouseEvent* ) { setPerCoreView( true ); } );

	ChartStyle detailStyle = mPerformanceDetailChart->chartStyle();
	detailStyle.verticalGrid.mode = ChartGridMode::ScreenInterval;
	detailStyle.verticalGrid.spacing = 64.0;
	detailStyle.horizontalGrid.mode = ChartGridMode::AxisTicks;
	detailStyle.verticalGrid.color = gridColor;
	detailStyle.horizontalGrid.color = gridColor;
	detailStyle.leftMargin = 68.f;
	detailStyle.bottomMargin = 28.f;
	detailStyle.minimumAxisMargin = 46.f;
	mPerformanceDetailChart->setChartStyle( detailStyle );

	for ( size_t i = 0; i < ids.size(); ++i ) {
		auto& metric = mPerformanceMetrics[i];
		const std::string prefix = std::string( "perf_" ) + ids[i];
		metric.card = mRoot->find<UIWidget>( prefix + "_card" );
		metric.preview = mRoot->find<UIChart>( prefix + "_preview" );
		metric.value = mRoot->find<UITextView>( prefix + "_value" );
		metric.primary = std::make_shared<RingXYDataSource>( kPerformanceHistorySamples );
		auto& line = metric.preview->addLineSeries( names[i] );
		line.setDataSource( metric.primary );
		line.setColor( colors[i] );
		metric.preview->xAxis().setFormatter(
			[this]( double value, double ) { return formatPerformanceTime( value ); } );
		if ( i == 3 ) {
			metric.preview->yAxis().setFormatter( []( double value, double ) {
				if ( !std::isfinite( value ) || value <= 0 )
					return String( "0 B/s" );
				return String( formatPerformanceRate( static_cast<Int64>( std::min(
					value, static_cast<double>( std::numeric_limits<Int64>::max() ) ) ) ) );
			} );
		} else {
			metric.preview->yAxis().setFormatter(
				[]( double value, double ) { return String::format( "%.0f%%", value ); } );
		}
		if ( i == 3 ) {
			metric.secondary = std::make_shared<RingXYDataSource>( kPerformanceHistorySamples );
			auto& upload = metric.preview->addLineSeries( "Upload" );
			upload.setDataSource( metric.secondary );
			upload.setColor( themeColor( mApp->getUI(), "--theme-error", Color( 236, 115, 103 ) ) );
		}
		ChartStyle smallStyle = metric.preview->chartStyle();
		smallStyle.leftMargin = smallStyle.rightMargin = smallStyle.topMargin =
			smallStyle.bottomMargin = smallStyle.minimumAxisMargin = 2.f;
		smallStyle.tickLength = 0.f;
		smallStyle.axisColor = Color( 0, 0, 0, 0 );
		smallStyle.tickColor = Color( 0, 0, 0, 0 );
		smallStyle.tickLabelColor = Color( 0, 0, 0, 0 );
		metric.preview->setChartStyle( smallStyle );
		if ( i != 3 )
			metric.preview->setAxisRange( metric.preview->yAxis(), { 0.0, 100.0 } );
		metric.preview->setAxisRange( metric.preview->xAxis(),
									  { -kPerformanceVisibleSeconds, 0.0 } );
		// The chart and text are display-only. Let the card receive clicks anywhere inside it.
		for ( Node* child = metric.card->getFirstChild(); child; child = child->getNextNode() )
			child->writeNodeFlag( NODE_FLAG_OVER_FIND_ALLOWED, 0 );
		metric.card->onClick( [this, i]( const MouseEvent* ) { selectPerformanceMetric( i ); } );
		metric.value->setText( i < 2 ? "0%" : "Unavailable" );
	}
	selectPerformanceMetric( 0 );
}

String App::formatPerformanceTime( double value ) const {
	const double elapsed = mPerformanceClock.getElapsedTime().asSeconds() - value;
	if ( elapsed < 2.0 )
		return "Now";
	const int seconds = static_cast<int>( std::round( std::max( 0.0, elapsed ) ) );
	return String::format( "-%d:%02d", seconds / 60, seconds % 60 );
}

void App::selectPerformanceMetric( size_t metric ) {
	if ( metric >= mPerformanceMetrics.size() )
		return;
	mSelectedPerformanceMetric = metric;
	for ( size_t i = 0; i < mPerformanceMetrics.size(); ++i ) {
		auto* card = mPerformanceMetrics[i].card;
		if ( i == metric )
			card->addClass( "eproc-performance-card-selected" );
		else
			card->removeClass( "eproc-performance-card-selected" );
	}
	static const char* names[] = { "CPU", "Memory", "GPU", "Network" };
	mPerformanceTitle->setText( names[metric] );
	mPerformanceSummary->setText( mPerformanceSummaries[metric] );
	mPerformanceDetail->setText( mPerformanceDetails[metric] );
	mCpuModeControls->setVisible( metric == 0 && !mCoreHistory.empty() );
	mPerformanceDetailChart->setModel( mPerformanceMetrics[metric].preview->sharedModel() );
	if ( metric != 3 )
		mPerformanceDetailChart->setAxisRange( mPerformanceDetailChart->yAxis(), { 0.0, 100.0 } );
	const double now = mPerformanceClock.getElapsedTime().asSeconds();
	mPerformanceDetailChart->setAxisRange( mPerformanceDetailChart->xAxis(),
										   { now - kPerformanceVisibleSeconds, now } );
	mPerformanceDetailChart->setVisible( metric != 0 || !mPerCoreView );
	mCoreChartScroll->setVisible( metric == 0 && mPerCoreView );
}

void App::setPerCoreView( bool perCore ) {
	mPerCoreView = perCore && !mCoreHistory.empty();
	mConfig->performancePerCore = mPerCoreView;
	mOverallButton->setSelected( !mPerCoreView );
	mPerCoreButton->setSelected( mPerCoreView );
	if ( mPerCoreView && mCoreCharts.size() != mCoreHistory.size() )
		rebuildCoreCharts( mCoreHistory.size() );
	mPerformanceDetailChart->setVisible( !mPerCoreView );
	mCoreChartScroll->setVisible( mPerCoreView );
}

void App::rebuildCoreCharts( size_t count ) {
	mCoreChartStack->closeAllChildren();
	mCoreCharts.clear();
	mCoreLabels.clear();
	for ( size_t core = 0; core < count; ++core ) {
		auto* panel = mApp->getUI()->loadLayoutFromString(
			"<RelativeLayout lw=\"184dp\" lh=\"112dp\" margin-right=\"6dp\" "
			"margin-bottom=\"6dp\"><Chart id=\"core_chart\" lw=\"mp\" "
			"lh=\"mp\" /><TextView id=\"core_title\" lw=\"wc\" lh=\"wc\" "
			"lg=\"top|left\" margin-left=\"5dp\" margin-top=\"4dp\" />"
			"</RelativeLayout>",
			mCoreChartStack );
		auto* label = panel->find<UITextView>( "core_title" );
		label->setText( String::format( "CPU %zu", core ) );
		label->writeNodeFlag( NODE_FLAG_OVER_FIND_ALLOWED, 0 );
		mCoreLabels.push_back( label );
		auto* chart = panel->find<UIChart>( "core_chart" );
		chart->addClass( "eproc-performance-chart" );
		auto& line = chart->addLineSeries( String::format( "CPU %zu", core ) );
		line.setDataSource( mCoreHistory[core] );
		line.setColor( themeColor( mApp->getUI(), "--primary", Color( 49, 178, 224 ) ) );
		chart->xAxis().setFormatter( []( double, double ) { return String(); } );
		chart->yAxis().setFormatter( []( double, double ) { return String(); } );
		ChartStyle style = chart->chartStyle();
		style.leftMargin = 0.f;
		style.rightMargin = 0.f;
		style.topMargin = 0.f;
		style.bottomMargin = 0.f;
		style.minimumAxisMargin = 0.f;
		style.tickLength = 0.f;
		style.axisColor = Color( 0, 0, 0, 0 );
		style.tickColor = Color( 0, 0, 0, 0 );
		style.tickLabelColor = Color( 0, 0, 0, 0 );
		style.verticalGrid.mode = ChartGridMode::ScreenInterval;
		style.verticalGrid.spacing = 24.0;
		style.verticalGrid.color =
			Color( themeColor( mApp->getUI(), "--separator", Color( 100, 113, 124 ) ), 35 );
		style.horizontalGrid.mode = ChartGridMode::DataInterval;
		style.horizontalGrid.spacing = 25.0;
		style.horizontalGrid.color =
			Color( themeColor( mApp->getUI(), "--separator", Color( 100, 113, 124 ) ), 35 );
		chart->setChartStyle( style );
		chart->setAxisRange( chart->yAxis(), { 0.0, 100.0 } );
		const double now = mPerformanceClock.getElapsedTime().asSeconds();
		chart->setAxisRange( chart->xAxis(), { now - kPerformanceVisibleSeconds, now } );
		mCoreCharts.push_back( chart );
	}
}

void App::updatePerformance( const SystemInfo& sysInfo,
							 const std::vector<ProcessInfo>& processes ) {
	const double now = mPerformanceClock.getElapsedTime().asSeconds();
	const double memoryPercent =
		sysInfo.totalMemory > 0 ? 100.0 * sysInfo.getUsedMemoryKB() / sysInfo.totalMemory : 0.0;
	Int64 download = 0, upload = 0, gpuMemory = 0;
	int gpuUsage = 0;
	bool hasNetwork = false, hasGpu = false;
	for ( const auto& process : processes ) {
		if ( process.netDownload >= 0 ) {
			download += process.netDownload;
			hasNetwork = true;
		}
		if ( process.netUpload >= 0 ) {
			upload += process.netUpload;
			hasNetwork = true;
		}
		if ( process.gpuUsage >= 0 ) {
			gpuUsage += process.gpuUsage;
			hasGpu = true;
		}
		if ( process.gpuMemory >= 0 ) {
			gpuMemory += process.gpuMemory;
			hasGpu = true;
		}
	}
	mPerformanceMetrics[0].primary->append( { now, sysInfo.cpuUsage } );
	mPerformanceMetrics[1].primary->append( { now, memoryPercent } );
	if ( hasGpu )
		mPerformanceMetrics[2].primary->append(
			{ now, static_cast<double>( std::min( gpuUsage, 100 ) ) } );
	if ( hasNetwork ) {
		mPerformanceMetrics[3].primary->append( { now, static_cast<double>( download ) } );
		mPerformanceMetrics[3].secondary->append( { now, static_cast<double>( upload ) } );
	}
	mPerformanceMetrics[0].value->setText( String::format( "%.1f%%", sysInfo.cpuUsage ) );
	mPerformanceMetrics[1].value->setText( String::format( "%.1f%%", memoryPercent ) );
	mPerformanceMetrics[2].value->setText(
		hasGpu ? String::format( "%d%% tracked", std::min( gpuUsage, 100 ) ) : "Unavailable" );
	mPerformanceMetrics[3].value->setText(
		hasNetwork ? String::format( "%s down", formatPerformanceRate( download ).c_str() )
				   : "Unavailable" );

	if ( mCoreHistory.size() < sysInfo.coreCpuUsage.size() ) {
		mCoreHistory.reserve( sysInfo.coreCpuUsage.size() );
		while ( mCoreHistory.size() < sysInfo.coreCpuUsage.size() )
			mCoreHistory.push_back(
				std::make_shared<RingXYDataSource>( kPerformanceHistorySamples ) );
	}
	for ( size_t i = 0; i < sysInfo.coreCpuUsage.size(); ++i )
		mCoreHistory[i]->append( { now, sysInfo.coreCpuUsage[i] } );
	if ( mConfig->performancePerCore && !mPerCoreView && !mCoreHistory.empty() )
		setPerCoreView( true );
	for ( auto& metric : mPerformanceMetrics )
		metric.preview->setAxisRange( metric.preview->xAxis(),
									  { now - kPerformanceVisibleSeconds, now } );
	mPerformanceDetailChart->setAxisRange( mPerformanceDetailChart->xAxis(),
										   { now - kPerformanceVisibleSeconds, now } );
	for ( auto* chart : mCoreCharts )
		chart->setAxisRange( chart->xAxis(), { now - kPerformanceVisibleSeconds, now } );
	mCpuModeControls->setVisible( mSelectedPerformanceMetric == 0 && !mCoreHistory.empty() );
	if ( mPerCoreView && mCoreCharts.size() != mCoreHistory.size() )
		rebuildCoreCharts( mCoreHistory.size() );
	for ( size_t i = 0; i < mCoreLabels.size() && i < sysInfo.coreCpuUsage.size(); ++i )
		mCoreLabels[i]->setText(
			String::format( "CPU %zu    %.1f%%", i, sysInfo.coreCpuUsage[i] ) );

	mPerformanceSummaries[0] = String::format( "%.1f%% utilization", sysInfo.cpuUsage );
	mPerformanceDetails[0] =
		String::format( "%d logical processors    %zu processes    %.0f seconds up",
						sysInfo.cpuCount, processes.size(), sysInfo.uptimeSeconds );
	mPerformanceSummaries[1] = String::format( "%.1f%% physical memory", memoryPercent );
	mPerformanceDetails[1] =
		String::format( "In use  %s    Available  %s    Total  %s    Swap  %s / %s",
						formatKiBIEC( sysInfo.getUsedMemoryKB() ).c_str(),
						formatKiBIEC( sysInfo.availableMemory ).c_str(),
						formatKiBIEC( sysInfo.totalMemory ).c_str(),
						formatKiBIEC( sysInfo.getUsedSwapKB() ).c_str(),
						formatKiBIEC( sysInfo.totalSwap ).c_str() );
	mPerformanceSummaries[2] = "Tracked process GPU usage";
	mPerformanceDetails[2] =
		hasGpu ? String::format( "Usage  %d%%    Tracked GPU memory  %s", std::min( gpuUsage, 100 ),
								 formatKiBIEC( gpuMemory ).c_str() )
			   : "GPU data is unavailable on this system";
	mPerformanceSummaries[3] = "Tracked process network traffic";
	mPerformanceDetails[3] = hasNetwork ? String::format( "Download  %s    Upload  %s",
														  formatPerformanceRate( download ).c_str(),
														  formatPerformanceRate( upload ).c_str() )
										: "Network data is unavailable on this system";
	mPerformanceSummary->setText( mPerformanceSummaries[mSelectedPerformanceMetric] );
	mPerformanceDetail->setText( mPerformanceDetails[mSelectedPerformanceMetric] );
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
		view->moveColumn( ProcessModel::ColCommand, ProcessModel::ColCount - 1 );
		for ( size_t column = 0; column < ProcessModel::ColCount; ++column ) {
			if ( auto* header = view->getHeaderColumn( column ) )
				header->setTooltipText( processColumnTooltip( mApp->getUI(), column ) );
		}
		view->setColumnsHidden(
			std::vector<size_t>( kOptionalProcessColumns.begin(), kOptionalProcessColumns.end() ),
			true );
#if EE_PLATFORM == EE_PLATFORM_MACOS
		view->setColumnHidden( ProcessModel::ColTotalMemory, false );
#endif
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
		view->setRowHeight( PixelDensity::dpToPx( 18 ) );
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
	// guarantees no in-flight collect() can outlive mCollector. App's destructor explicitly
	// joins the pool before destroying the staging mutex that the worker uses.
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
	if ( mCollectInFlight.load() )
		return;
	if ( mCollectFamilyMemoryAfterRestore ) {
		mCollectFamilyMemoryAfterRestore = false;
		mDispatchClock.getElapsedTimeAndReset();
		collectAsync();
		return;
	}

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
	UIAbstractTableView* view = activeProcessView();
	mCollector->setCollectProportionalMemory(
		view && ( !view->isColumnHidden( ProcessModel::ColFamilyMemory ) ||
				  !view->isColumnHidden( ProcessModel::ColFamilyMemoryPercent ) ) );

	mThreadPool->run( [this] {
		std::vector<ProcessInfo> processes;
		SystemInfo sysInfo;

		if ( mCollector->collect( processes, sysInfo ) ) {
			Lock lock( mStagingMutex );
			mStagedProcesses = std::move( processes );
			mStagedSystemInfo = std::move( sysInfo );
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
		sysInfo = std::move( mStagedSystemInfo );
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

	if ( mPerformanceDetailChart )
		updatePerformance( sysInfo, processes );
	if ( mProcessModel )
		mProcessModel->applySnapshot( std::move( processes ), sysInfo );

	if ( !mProcessTableStateRestored && !mProcessTableStateRestoreScheduled && mApp &&
		 mApp->getUI() ) {
		mProcessTableStateRestoreScheduled = true;
		mApp->getUI()->runOnMainThread( [this] {
			if ( !mProcessTableStateRestored ) {
				restoreProcessTableState();
				mProcessTableStateRestored = true;
				UIAbstractTableView* view = activeProcessView();
				mCollectFamilyMemoryAfterRestore =
					view && ( !view->isColumnHidden( ProcessModel::ColFamilyMemory ) ||
							  !view->isColumnHidden( ProcessModel::ColFamilyMemoryPercent ) );
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
		mTreeExpansionInitialized = mTreeModel->rowCount() > 0;
		if ( mTreeExpansionInitialized )
			mTreeView->expandAll();
		return;
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

	if ( mSwapText ) {
		mSwapText->setVisible( sys.totalSwap >= 0 );
		if ( sys.totalSwap >= 0 ) {
			mSwapText->setText( String::format(
				mApp->getUI()->i18n( "eproc_swap_status_format", "Swap: %s / %s" ).toUtf8(),
				formatKiBIEC( sys.getUsedSwapKB() ).c_str(),
				formatKiBIEC( sys.totalSwap ).c_str() ) );
		}
	}
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
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_MACOS || \
	EE_PLATFORM == EE_PLATFORM_BSD
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
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_MACOS || \
	EE_PLATFORM == EE_PLATFORM_BSD
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

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_WIN || \
	EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_BSD
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
