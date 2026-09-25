#include <eepp/config.hpp>
#if EE_PLATFORM == EE_PLATFORM_MACOS
#define Rect AppleRect
#include <ImageIO/ImageIO.h>
#undef Rect
#endif
#include "process_model.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <eepp/graphics/glyphdrawable.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/ui/abstract/uiabstractview.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <fstream>
#include <string_view>

namespace eproc {

namespace {

const char* processColumnClass( size_t column ) {
	switch ( column ) {
		case ProcessModel::ColIcon:
			return "eproc-process-column-icon";
		case ProcessModel::ColName:
			return "eproc-process-column-name";
		case ProcessModel::ColPid:
			return "eproc-process-column-pid";
		case ProcessModel::ColUsername:
			return "eproc-process-column-username";
		case ProcessModel::ColCpu:
			return "eproc-process-column-cpu";
		case ProcessModel::ColThreads:
			return "eproc-process-column-threads";
		case ProcessModel::ColMemoryPercent:
			return "eproc-process-column-memory-percent";
		case ProcessModel::ColFamilyMemoryPercent:
			return "eproc-process-column-memory-percent";
		case ProcessModel::ColMemory:
			return "eproc-process-column-memory";
		case ProcessModel::ColFamilyMemory:
			return "eproc-process-column-memory";
		case ProcessModel::ColSharedMem:
			return "eproc-process-column-shared-memory";
		case ProcessModel::ColGpuUsage:
			return "eproc-process-column-gpu-usage";
		case ProcessModel::ColGpuMemory:
			return "eproc-process-column-gpu-memory";
		case ProcessModel::ColDownload:
			return "eproc-process-column-download";
		case ProcessModel::ColUpload:
			return "eproc-process-column-upload";
		case ProcessModel::ColCommand:
			return "eproc-process-column-command";
		case ProcessModel::ColTotalMemory:
			return "eproc-process-column-total-memory";
		case ProcessModel::ColVirtualSize:
			return "eproc-process-column-virtual-size";
		case ProcessModel::ColCpuTime:
			return "eproc-process-column-cpu-time";
		case ProcessModel::ColNiceness:
			return "eproc-process-column-niceness";
		case ProcessModel::ColRelativeStartTime:
			return "eproc-process-column-relative-start-time";
		case ProcessModel::ColTty:
			return "eproc-process-column-tty";
		case ProcessModel::ColIoRead:
			return "eproc-process-column-io-read";
		case ProcessModel::ColIoWrite:
			return "eproc-process-column-io-write";
		default:
			return "eproc-process-column-base";
	}
}

} // namespace

ProcessModel::ProcessModel( UISceneNode* ui ) : mUI( ui ) {}

ProcessModel::~ProcessModel() {}

size_t ProcessModel::rowCount( const ModelIndex& ) const {
	return mFilteredProcesses.size();
}

size_t ProcessModel::columnCount( const ModelIndex& ) const {
	return ColCount;
}

std::string ProcessModel::columnName( const size_t& column ) const {
	switch ( column ) {
		case ColIcon:
			return "";
		case ColName:
			return mUI->i18n( "eproc_column_name", "Name" ).toUtf8();
		case ColPid:
			return mUI->i18n( "eproc_column_pid", "PID" ).toUtf8();
		case ColUsername:
			return mUI->i18n( "eproc_column_username", "Username" ).toUtf8();
		case ColCpu:
			return mUI->i18n( "eproc_column_cpu", "CPU %" ).toUtf8();
		case ColThreads:
			return mUI->i18n( "eproc_column_threads", "Threads" ).toUtf8();
		case ColMemoryPercent:
			return mUI->i18n( "eproc_column_memory_percent", "Memory %" ).toUtf8();
		case ColMemory:
			return mUI->i18n( "eproc_column_memory", "Memory" ).toUtf8();
		case ColFamilyMemory:
			return mUI->i18n( "eproc_column_family_memory", "Family Memory" ).toUtf8();
		case ColFamilyMemoryPercent:
			return mUI->i18n( "eproc_column_family_memory_percent", "Family Memory %" ).toUtf8();
		case ColSharedMem:
			return mUI->i18n( "eproc_column_shared_memory", "Shared Mem" ).toUtf8();
		case ColGpuUsage:
			return mUI->i18n( "eproc_column_gpu_usage", "GPU Usage" ).toUtf8();
		case ColGpuMemory:
			return mUI->i18n( "eproc_column_gpu_memory", "GPU Memory" ).toUtf8();
		case ColDownload:
			return mUI->i18n( "eproc_column_download", "Download" ).toUtf8();
		case ColUpload:
			return mUI->i18n( "eproc_column_upload", "Upload" ).toUtf8();
		case ColCommand:
			return mUI->i18n( "eproc_column_command", "Command" ).toUtf8();
		case ColTotalMemory:
			return mUI->i18n( "eproc_column_resident_memory", "Resident Memory" ).toUtf8();
		case ColVirtualSize:
			return mUI->i18n( "eproc_column_virtual_size", "Virtual Size" ).toUtf8();
		case ColCpuTime:
			return mUI->i18n( "eproc_column_cpu_time", "CPU Time" ).toUtf8();
		case ColNiceness:
			return mUI->i18n( "eproc_column_niceness", "Niceness" ).toUtf8();
		case ColRelativeStartTime:
			return mUI->i18n( "eproc_column_relative_start_time", "Relative Start Time" ).toUtf8();
		case ColTty:
			return mUI->i18n( "eproc_column_tty", "TTY" ).toUtf8();
		case ColIoRead:
			return mUI->i18n( "eproc_column_io_read", "IO Read" ).toUtf8();
		case ColIoWrite:
			return mUI->i18n( "eproc_column_io_write", "IO Write" ).toUtf8();
		default:
			return {};
	}
}

Variant ProcessModel::data( const ModelIndex& index, ModelRole role ) const {
	const std::string EMPTY = "";

	if ( role == ModelRole::Class ) {
		const char* cls = processColumnClass( index.column() );
		return cls ? Variant( cls ) : Variant();
	}

	if ( role == ModelRole::Icon ) {
		// The icon lives in its own column so that every icon lines up, instead of padding the
		// name text.
		const auto* proc = getProcessByRow( index.row() );
		if ( !proc || index.column() != ColIcon )
			return Variant();
		return Variant( !proc->iconPath.empty() ? iconFor( proc->iconPath ) : proc->windowIcon );
	}

	if ( role == ModelRole::Sort ) {
		const auto* proc = getProcessByRow( index.row() );
		if ( !proc )
			return Variant();
		switch ( index.column() ) {
			case ColName:
				return Variant::fromRef( proc->name );
			case ColUsername:
				return Variant::fromRef( proc->username );
			case ColCommand:
				return Variant::fromRef( proc->commandLine );
			case ColPid:
				return Variant( static_cast<Int64>( proc->pid ) );
			case ColCpu:
				return Variant( static_cast<Int64>( proc->getCpuForSort() ) );
			case ColThreads:
				return Variant( static_cast<Int64>( proc->numThreads ) );
			case ColMemoryPercent:
				return Variant( static_cast<Int64>( proc->getMemoryForSort() ) );
			case ColMemory:
				return Variant( static_cast<Int64>( proc->getMemoryForSort() ) );
			case ColFamilyMemory:
			case ColFamilyMemoryPercent:
				return Variant( proc->familyMemoryKB );
			case ColSharedMem:
				return Variant( static_cast<Int64>( proc->sharedMem ) );
			case ColGpuUsage:
				return Variant( static_cast<Int64>( proc->gpuUsage ) );
			case ColGpuMemory:
				return Variant( static_cast<Int64>( proc->gpuMemory ) );
			case ColDownload:
				return Variant( static_cast<Int64>( proc->netDownload ) );
			case ColUpload:
				return Variant( static_cast<Int64>( proc->netUpload ) );
			case ColTotalMemory:
				return Variant( static_cast<Int64>( proc->vmRSS ) );
			case ColVirtualSize:
				return Variant( static_cast<Int64>( proc->vmSize ) );
			case ColCpuTime:
				return Variant( static_cast<Int64>( proc->userTime ) + proc->sysTime );
			case ColNiceness:
				return Variant( static_cast<Int64>( proc->niceLevel ) );
			case ColRelativeStartTime:
				return Variant( static_cast<Int64>( proc->startTime ) );
			case ColTty:
				return Variant( static_cast<Int64>( proc->ttyNr ) );
			case ColIoRead:
				return Variant( static_cast<Int64>( proc->ioReadBytes ) );
			case ColIoWrite:
				return Variant( static_cast<Int64>( proc->ioWriteBytes ) );
			default:
				break;
		}
	}

	if ( role != ModelRole::Display )
		return Variant();

	const auto* proc = getProcessByRow( index.row() );
	if ( !proc )
		return Variant();

	switch ( index.column() ) {
		case ColName:
			return Variant::fromRef( proc->name );
		case ColPid:
			return Variant( String::toString( static_cast<Int64>( proc->pid ) ) );
		case ColUsername:
			return Variant::fromRef( proc->username );
		case ColCpu:
			return displayedCpuUsage( *proc ) > 0
					   ? Variant( std::to_string( displayedCpuUsage( *proc ) ) + "%" )
					   : Variant( EMPTY );
		case ColThreads:
			return Variant( String::toString( static_cast<Int64>( proc->numThreads ) ) );
		case ColMemoryPercent:
			return mSystemInfo.totalMemory > 0 && proc->getMemoryForSort() >= 0
					   ? Variant( String::format( "%.1f%%", proc->getMemoryForSort() * 100.0 /
																mSystemInfo.totalMemory ) )
					   : Variant( EMPTY );
		case ColMemory:
			return Variant( proc->formatMemory() );
		case ColFamilyMemory:
			return Variant( formatKiB( proc->familyMemoryKB ) );
		case ColFamilyMemoryPercent:
			return mSystemInfo.totalMemory > 0 && proc->familyMemoryKB >= 0
					   ? Variant( String::format( "%.1f%%", proc->familyMemoryKB * 100.0 /
																mSystemInfo.totalMemory ) )
					   : Variant( EMPTY );
		case ColSharedMem:
			return Variant( proc->formatSharedMem() );
		case ColGpuUsage:
			return Variant( proc->formatGpuUsage() );
		case ColGpuMemory:
			return Variant( proc->formatGpuMemory() );
		case ColDownload:
			return Variant( proc->formatDownload() );
		case ColUpload:
			return Variant( proc->formatUpload() );
		case ColCommand:
			return Variant::fromRef( proc->commandLine );
		case ColTotalMemory:
			return Variant( formatKiB( proc->vmRSS ) );
		case ColVirtualSize:
			return Variant( formatKiB( proc->vmSize ) );
		case ColCpuTime:
			return Variant( proc->formatCpuTime( mSystemInfo.clockTicksPerSecond ) );
		case ColNiceness:
			return Variant( String::toString( static_cast<Int64>( proc->niceLevel ) ) );
		case ColRelativeStartTime:
			return Variant( proc->formatRelativeStartTime( mSystemInfo.uptimeSeconds,
														   mSystemInfo.clockTicksPerSecond ) );
		case ColTty:
			return Variant::fromRef( proc->tty );
		case ColIoRead:
			return Variant( formatBytes( proc->ioReadBytes ) );
		case ColIoWrite:
			return Variant( formatBytes( proc->ioWriteBytes ) );
		default:
			return Variant( EMPTY );
	}
}

ModelIndex ProcessModel::index( int row, int column, const ModelIndex& ) const {
	return createIndex( row, column );
}

void ProcessModel::buildFamilyMemory( std::vector<ProcessInfo>& processes ) {
	const size_t count = processes.size();
	mFamilyRowForPid.clear();
	mFamilyRowForPid.reserve( count );
	mFamilyParents.assign( count, -1 );
	mFamilyPendingChildren.assign( count, 0 );
	mFamilyQueue.clear();
	mFamilyQueue.reserve( count );
	for ( size_t row = 0; row < count; ++row ) {
		auto& process = processes[row];
		mFamilyRowForPid.emplace( process.pid, static_cast<int>( row ) );
#if EE_PLATFORM == EE_PLATFORM_LINUX
		// Zombies have no address space, so smaps_rollup is unavailable but their PSS is zero.
		process.familyMemoryKB = process.vmPSS >= 0 ? process.vmPSS : process.vmRSS == 0 ? 0 : -1;
#else
		process.familyMemoryKB = process.getMemoryForSort();
#endif
	}
	for ( size_t row = 0; row < count; ++row ) {
		const auto& process = processes[row];
		if ( process.parentPid <= 0 || process.parentPid == process.pid )
			continue;
		auto parent = mFamilyRowForPid.find( process.parentPid );
		if ( parent == mFamilyRowForPid.end() )
			continue;
		mFamilyParents[row] = parent->second;
		++mFamilyPendingChildren[parent->second];
	}
	for ( size_t row = 0; row < count; ++row ) {
		if ( mFamilyPendingChildren[row] == 0 )
			mFamilyQueue.push_back( row );
	}
	for ( size_t head = 0; head < mFamilyQueue.size(); ++head ) {
		const size_t row = mFamilyQueue[head];
		const int parentRow = mFamilyParents[row];
		if ( parentRow < 0 )
			continue;
		auto& family = processes[parentRow].familyMemoryKB;
		const Int64 childMemory = processes[row].familyMemoryKB;
		family = family >= 0 && childMemory >= 0 ? family + childMemory : -1;
		if ( --mFamilyPendingChildren[parentRow] == 0 )
			mFamilyQueue.push_back( static_cast<size_t>( parentRow ) );
	}
	// A malformed parent cycle cannot produce a meaningful family total.
	for ( size_t row = 0; row < count; ++row ) {
		if ( mFamilyPendingChildren[row] > 0 )
			processes[row].familyMemoryKB = -1;
	}
}

void ProcessModel::applySnapshot( std::vector<ProcessInfo>&& processes,
								  const SystemInfo& sysInfo ) {
	buildFamilyMemory( processes );
	// Keep processes that disappeared from the latest snapshot for one more update. This mirrors
	// ksysguard's Ended state and is especially useful when a process exits between two refreshes:
	// its last known row remains visible, but is marked as ended by the view.
	UnorderedMap<Int64, Int64> currentStartTimes;
	currentStartTimes.reserve( processes.size() );
	for ( const auto& process : processes )
		currentStartTimes[process.pid] = process.startTime;

	std::vector<ProcessInfo> endedProcesses;
	endedProcesses.reserve( mProcesses.size() );
	for ( auto& previous : mProcesses ) {
		// An ended process was already shown during the previous update. Do not keep it for a
		// second update.
		if ( previous.status == ProcessStatus::Ended )
			continue;

		auto current = currentStartTimes.find( previous.pid );
		const bool processStillExists = current != currentStartTimes.end() &&
										( previous.startTime == 0 || current->second == 0 ||
										  previous.startTime == current->second );
		if ( processStillExists )
			continue;

		previous.status = ProcessStatus::Ended;
		endedProcesses.emplace_back( std::move( previous ) );
	}

	processes.reserve( processes.size() + endedProcesses.size() );
	for ( auto& process : endedProcesses )
		processes.emplace_back( std::move( process ) );

	mProcesses = std::move( processes );
	mSystemInfo = sysInfo;
	applyFilters();
	onModelUpdate();
}

void ProcessModel::setFilter( FilterMode mode ) {
	mFilterMode = mode;
	applyFilters();
	onModelUpdate();
}

void ProcessModel::setTextFilter( const std::string& text ) {
	mTextRegex.reset();
	mTextLiteral.clear();

	if ( !text.empty() ) {
		// Compiled here rather than per row: the filter runs over every process on every pass.
		// useCache is false because the pattern changes on each keystroke: the cache is
		// least-recently-used and shared with the syntax definitions, so caching every typed prefix
		// would evict the patterns the tokenizer is working from.
		// AllowFallback keeps the default engine behaviour: a pattern PCRE2 rejects is retried
		// with Oniguruma before it is treated as invalid.
		auto regex = std::make_unique<RegEx>(
			text, RegEx::Options::Utf | RegEx::Options::AllowFallback | RegEx::Options::Caseless,
			false );
		if ( regex->isValid() ) {
			mTextRegex = std::move( regex );
		} else {
			// An unfinished pattern matches nothing, which would blank the table while the user is
			// still typing it, so it is searched for literally instead.
			mTextLiteral = text;
			std::transform( mTextLiteral.begin(), mTextLiteral.end(), mTextLiteral.begin(),
							[]( unsigned char c ) { return std::tolower( c ); } );
		}
	}

	applyFilters();
	onModelUpdate();
}

// Case-insensitive substring test that does not allocate.
static bool containsIgnoreCase( std::string_view haystack, std::string_view lowerNeedle ) {
	if ( lowerNeedle.empty() )
		return true;
	if ( haystack.size() < lowerNeedle.size() )
		return false;

	for ( size_t i = 0; i + lowerNeedle.size() <= haystack.size(); ++i ) {
		size_t j = 0;
		for ( ; j < lowerNeedle.size(); ++j ) {
			if ( std::tolower( static_cast<unsigned char>( haystack[i + j] ) ) != lowerNeedle[j] )
				break;
		}
		if ( j == lowerNeedle.size() )
			return true;
	}
	return false;
}

bool ProcessModel::matchesText( const ProcessInfo& proc ) const {
	if ( !mTextRegex && mTextLiteral.empty() )
		return true;

	// The original lets the user search by PID too. The digits are formatted into a stack buffer
	// because this runs for every process on every pass while a filter is active.
	char pidBuffer[24];
	int pidLength =
		snprintf( pidBuffer, sizeof( pidBuffer ), "%lld", static_cast<long long>( proc.pid ) );
	size_t pidSize = pidLength > 0 ? static_cast<size_t>( pidLength ) : 0;

	if ( mTextRegex ) {
		// Each column is matched on its own, so an anchored pattern applies to every one of them.
		return mTextRegex->matches( proc.name ) || mTextRegex->matches( proc.commandLine ) ||
			   mTextRegex->matches( proc.username ) ||
			   mTextRegex->matches( pidBuffer, 0, nullptr, pidSize );
	}

	const std::string_view pid( pidBuffer, pidSize );
	return containsIgnoreCase( proc.name, mTextLiteral ) ||
		   containsIgnoreCase( proc.commandLine, mTextLiteral ) ||
		   containsIgnoreCase( proc.username, mTextLiteral ) ||
		   containsIgnoreCase( pid, mTextLiteral );
}

bool ProcessModel::accepts( const ProcessInfo& proc ) const {
	switch ( mFilterMode ) {
		case AllProcesses:
		case AllProcessesInTreeForm:
			return true;

		case SystemProcesses:
			return proc.systemProcess;

		case UserProcesses:
			return proc.userProcess;

		case OwnProcesses:
			return proc.ownedByCurrentUser;

		case ProgramsOnly:
			// A controlling terminal also belongs to shells and short-lived commands. Keep only
			// processes that own a managed or embedded GUI window, including XEmbed tray icons.
			return mGuiPids.count( proc.pid ) != 0;

		default:
			return true;
	}
}

void ProcessModel::applyFilters() {
	mFilteredProcesses.clear();
	mFilteredProcesses.reserve( mProcesses.size() );
	mTextMatchedPids.clear();
	if ( mFilterMode == AllProcessesInTreeForm && ( mTextRegex || !mTextLiteral.empty() ) ) {
		UnorderedMap<Int64, ProcessInfo*> byPid;
		byPid.reserve( mProcesses.size() );
		for ( auto& process : mProcesses )
			byPid.emplace( process.pid, &process );
		UnorderedSet<ProcessInfo*> visible;
		visible.reserve( mProcesses.size() );
		for ( auto& process : mProcesses ) {
			if ( !matchesText( process ) )
				continue;
			mTextMatchedPids.push_back( process.pid );
			for ( ProcessInfo* ancestor = &process; ancestor; ) {
				if ( !visible.insert( ancestor ).second )
					break;
				auto parent = byPid.find( ancestor->parentPid );
				ancestor =
					parent != byPid.end() && parent->second != ancestor ? parent->second : nullptr;
			}
		}
		for ( auto& process : mProcesses ) {
			if ( visible.count( &process ) )
				mFilteredProcesses.push_back( &process );
		}
		return;
	}

	for ( auto& proc : mProcesses ) {
		if ( !accepts( proc ) || !matchesText( proc ) )
			continue;

		mFilteredProcesses.push_back( &proc );
	}
}

void ProcessModel::setGuiWindowPids( UnorderedSet<Int64>&& pids ) {
	mGuiPids = std::move( pids );
}

// nanosvg substitutes its own default (white) fill for the SVG constructs it cannot parse, so an
// unsupported icon rasterizes to a blank white square. Such a result is reported as "no icon"
// rather than drawn as a misleading box.
static bool isBlankIcon( const Image& image ) {
	const Uint8* pixels = image.getPixelsPtr();
	const unsigned int channels = image.getChannels();
	const unsigned int width = image.getWidth();
	const unsigned int height = image.getHeight();

	if ( !pixels || width == 0 || height == 0 || channels < 3 )
		return false; // no pixel access: trust the image rather than dropping the icon

	for ( unsigned int y = 0; y < height; ++y ) {
		for ( unsigned int x = 0; x < width; ++x ) {
			const Uint8* pixel = pixels + ( static_cast<size_t>( y ) * width + x ) * channels;
			if ( channels == 4 && pixel[3] < 8 )
				continue; // fully transparent
			if ( pixel[0] < 240 || pixel[1] < 240 || pixel[2] < 240 )
				return false; // real content
		}
	}

	return true;
}

// The original renders 16px icons in the name column.
static constexpr int kIconSizeDp = 16;

#if EE_PLATFORM == EE_PLATFORM_MACOS
static Image loadMacIcon( const std::string& path, Uint32 iconPx ) {
	Image image;
	CFURLRef url = CFURLCreateFromFileSystemRepresentation(
		kCFAllocatorDefault, reinterpret_cast<const UInt8*>( path.data() ),
		static_cast<CFIndex>( path.size() ), false );
	if ( !url )
		return image;
	CGImageSourceRef source = CGImageSourceCreateWithURL( url, nullptr );
	CFRelease( url );
	if ( !source )
		return image;
	const int maxSize = static_cast<int>( iconPx );
	CFNumberRef size = CFNumberCreate( kCFAllocatorDefault, kCFNumberIntType, &maxSize );
	const void* keys[] = { kCGImageSourceCreateThumbnailFromImageAlways,
						   kCGImageSourceThumbnailMaxPixelSize };
	const void* values[] = { kCFBooleanTrue, size };
	CFDictionaryRef options =
		CFDictionaryCreate( kCFAllocatorDefault, keys, values, 2, &kCFTypeDictionaryKeyCallBacks,
							&kCFTypeDictionaryValueCallBacks );
	CGImageRef thumbnail = CGImageSourceCreateThumbnailAtIndex( source, 0, options );
	CFRelease( options );
	CFRelease( size );
	CFRelease( source );
	if ( !thumbnail )
		return image;
	CFMutableDataRef data = CFDataCreateMutable( kCFAllocatorDefault, 0 );
	if ( data ) {
		CGImageDestinationRef destination =
			CGImageDestinationCreateWithData( data, CFSTR( "public.png" ), 1, nullptr );
		if ( destination ) {
			CGImageDestinationAddImage( destination, thumbnail, nullptr );
			if ( CGImageDestinationFinalize( destination ) ) {
				image = Image( CFDataGetBytePtr( data ),
							   static_cast<unsigned int>( CFDataGetLength( data ) ), 4 );
			}
			CFRelease( destination );
		}
		CFRelease( data );
	}
	CGImageRelease( thumbnail );
	return image;
}
#endif

DrawablePtr ProcessModel::iconFor( const std::string& path ) const {
	auto it = mIconCache.find( path );
	if ( it != mIconCache.end() )
		return it->second;

	// Loading is done once per icon file: the table asks for this on every cell refresh, and
	// re-loading the texture each time would be pathological.
	const Uint32 iconPx = static_cast<Uint32>( PixelDensity::dpToPxI( kIconSizeDp ) );
	Image image;

#if EE_PLATFORM == EE_PLATFORM_MACOS
	if ( String::endsWith( path, ".icns" ) ) {
		image = loadMacIcon( path, iconPx );
	} else
#endif
		if ( String::endsWith( path, ".svg" ) || String::endsWith( path, ".svgz" ) ) {
		// Scalable icons are rasterized before resizing, so the same high-quality image resampler
		// is used for both SVG and bitmap icons.
		std::ifstream file( path, std::ios::binary );
		std::string svg( ( std::istreambuf_iterator<char>( file ) ),
						 std::istreambuf_iterator<char>() );
		if ( !svg.empty() ) {
			Image::FormatConfiguration format;
			int width = 0, height = 0, channels = 0;
			if ( Image::getInfoFromMemory( reinterpret_cast<const unsigned char*>( svg.data() ),
										   svg.size(), &width, &height, &channels, format ) ) {
				format.svgScale( iconPx / static_cast<Float>( eemax( width, height ) ) );
				image = Image( reinterpret_cast<const Uint8*>( svg.data() ),
							   static_cast<unsigned int>( svg.size() ), 4, format );
			}
		}
	} else {
		image = Image( path, 4 );
	}

	// Decode at the final pixel size and use Lanczos resampling for bitmap icons. This avoids
	// relying on the GPU's texture filtering to reduce large process icons at draw time.
	if ( image.getPixelsPtr() && image.getWidth() > 0 && image.getHeight() > 0 ) {
		if ( image.getWidth() != iconPx || image.getHeight() != iconPx )
			image.resize( iconPx, iconPx, Image::RESAMPLER_LANCZOS4 );

		if ( isBlankIcon( image ) )
			image = Image();
	}

	TexturePtr texture;
	if ( image.getPixelsPtr() && image.getWidth() > 0 && image.getHeight() > 0 ) {
		texture = TextureFactory::instance()->loadFromPixels(
			image.getPixelsPtr(), image.getWidth(), image.getHeight(), image.getChannels(), false,
			Texture::ClampMode::ClampToEdge, false, false, path );
	}

	// The table draws a ModelRole::Icon drawable at its own size, so the icon is built already
	// scaled to the row height; at native resolution it would overflow the row.
	DrawablePtr drawable;
	if ( texture ) {
		// GlyphDrawable feeds the source rect to quadsSetTexCoord as u/v, so the rect is
		// normalized: (0,0,1,1) selects the whole texture.
		Rect srcRect( 0, 0, 1, 1 );
		auto* glyph = GlyphDrawable::New( texture, srcRect, Sizef( iconPx, iconPx ), path );
		glyph->setDrawMode( GlyphDrawable::DrawMode::Image );
		glyph->setGlyphRenderMode( GlyphRenderMode::Color );
		glyph->setPixelDensity( PixelDensity::getPixelDensity() );
		drawable = DrawablePtr( glyph );
	}

	mIconCache.emplace( path, drawable );
	return drawable;
}

const ProcessInfo* ProcessModel::getProcessByRow( int row ) const {
	if ( row < 0 || static_cast<size_t>( row ) >= mFilteredProcesses.size() )
		return nullptr;
	return mFilteredProcesses[row];
}

int ProcessModel::rowForPid( Int64 pid ) const {
	for ( size_t i = 0; i < mFilteredProcesses.size(); ++i ) {
		if ( mFilteredProcesses[i]->pid == pid )
			return static_cast<int>( i );
	}
	return -1;
}

ProcessTreeModel::ProcessTreeModel( std::shared_ptr<ProcessModel> source ) :
	mSource( std::move( source ) ) {
	mSource->registerClient( this );
	rebuild();
}

ProcessTreeModel::~ProcessTreeModel() {
	mSource->unregisterClient( this );
}

void ProcessTreeModel::onModelUpdated( unsigned ) {
	rebuild();
	onModelUpdate();
}

void ProcessTreeModel::rebuild() {
	mRoots.clear();
	mNodeForPid.clear();
	const size_t count = mSource->visibleCount();
	mNodes.resize( count );
	for ( auto& node : mNodes ) {
		node.children.clear();
		node.parent = -1;
		node.rowInParent = -1;
	}
	mRoots.reserve( count );
	mNodeForPid.reserve( count );
	for ( size_t row = 0; row < count; ++row ) {
		const ProcessInfo* process = mSource->getProcessByRow( static_cast<int>( row ) );
		mNodes[row].sourceRow = static_cast<int>( row );
		if ( process )
			mNodeForPid.emplace( process->pid, static_cast<int>( row ) );
	}
	for ( size_t row = 0; row < count; ++row ) {
		const ProcessInfo* process = mSource->getProcessByRow( static_cast<int>( row ) );
		int parent = -1;
		if ( process && process->parentPid != process->pid ) {
			auto found = mNodeForPid.find( process->parentPid );
			if ( found != mNodeForPid.end() ) {
				parent = found->second;
				for ( int ancestor = parent; ancestor >= 0; ancestor = mNodes[ancestor].parent ) {
					if ( ancestor == static_cast<int>( row ) ) {
						parent = -1;
						break;
					}
				}
			}
		}
		Node& node = mNodes[row];
		node.parent = parent;
		if ( parent >= 0 ) {
			auto& siblings = mNodes[parent].children;
			node.rowInParent = static_cast<int>( siblings.size() );
			siblings.push_back( static_cast<int>( row ) );
		} else {
			node.rowInParent = static_cast<int>( mRoots.size() );
			mRoots.push_back( static_cast<int>( row ) );
		}
	}
	sortChildren();
}

void ProcessTreeModel::sortChildren() {
	if ( mSortColumn < 0 || mSortOrder == SortOrder::None )
		return;
	const auto compare = [this]( int left, int right ) {
		const Variant a =
			mSource->data( mSource->index( mNodes[left].sourceRow, mSortColumn ), ModelRole::Sort );
		const Variant b = mSource->data( mSource->index( mNodes[right].sourceRow, mSortColumn ),
										 ModelRole::Sort );
		if ( a.isStdStringLike() && b.isStdStringLike() ) {
			const auto aText = a.asStdStringView();
			const auto bText = b.asStdStringView();
			for ( size_t i = 0; i < std::min( aText.size(), bText.size() ); ++i ) {
				const int lhs = std::tolower( static_cast<unsigned char>( aText[i] ) );
				const int rhs = std::tolower( static_cast<unsigned char>( bText[i] ) );
				if ( lhs != rhs )
					return lhs < rhs ? -1 : 1;
			}
			if ( aText.size() == bText.size() )
				return 0;
			return aText.size() < bText.size() ? -1 : 1;
		}
		if ( a < b )
			return -1;
		return b < a ? 1 : 0;
	};
	const auto sortSiblings = [this, &compare]( std::vector<int>& siblings ) {
		if ( siblings.size() > 1 ) {
			std::sort( siblings.begin(), siblings.end(), [&]( int left, int right ) {
				const int result = compare( left, right );
				if ( result == 0 )
					return mNodes[left].sourceRow < mNodes[right].sourceRow;
				return mSortOrder == SortOrder::Ascending ? result < 0 : result > 0;
			} );
		}
		for ( size_t row = 0; row < siblings.size(); ++row )
			mNodes[siblings[row]].rowInParent = static_cast<int>( row );
	};
	sortSiblings( mRoots );
	for ( auto& node : mNodes )
		sortSiblings( node.children );
}

void ProcessTreeModel::sort( const size_t& column, const SortOrder& order ) {
	if ( column >= columnCount() || !isColumnSortable( column ) || order == SortOrder::None )
		return;
	struct ViewSelection {
		UIAbstractView* view;
		std::vector<ModelIndex> indexes;
	};
	std::vector<ViewSelection> selections;
	forEachView( [&]( UIAbstractView* view ) {
		auto indexes = view->getSelection().indexes();
		if ( !indexes.empty() )
			selections.push_back( { view, std::move( indexes ) } );
	} );
	mSortColumn = static_cast<int>( column );
	mSortOrder = order;
	sortChildren();
	for ( auto& selection : selections ) {
		for ( auto& index : selection.indexes ) {
			const Node& node = *static_cast<const Node*>( index.internalData() );
			index = createIndex( node.rowInParent, static_cast<int>( index.column() ),
								 const_cast<Node*>( &node ) );
		}
		selection.view->getSelection().set( selection.indexes, false );
		selection.view->notifySelectionChange();
	}
	onModelUpdate( UpdateFlag::DontInvalidateIndexes );
}

size_t ProcessTreeModel::rowCount( const ModelIndex& parent ) const {
	if ( !parent.isValid() )
		return mRoots.size();
	if ( parent.model() != this || !parent.internalData() )
		return 0;
	return static_cast<const Node*>( parent.internalData() )->children.size();
}

size_t ProcessTreeModel::columnCount( const ModelIndex& ) const {
	return mSource->columnCount();
}

std::string ProcessTreeModel::columnName( const size_t& column ) const {
	return mSource->columnName( column );
}

ModelIndex ProcessTreeModel::index( int row, int column, const ModelIndex& parent ) const {
	if ( row < 0 || column < 0 || static_cast<size_t>( column ) >= columnCount() )
		return {};
	const auto* children = &mRoots;
	if ( parent.isValid() ) {
		if ( parent.model() != this || !parent.internalData() )
			return {};
		children = &static_cast<const Node*>( parent.internalData() )->children;
	}
	if ( static_cast<size_t>( row ) >= children->size() )
		return {};
	return createIndex( row, column, const_cast<Node*>( &mNodes[( *children )[row]] ) );
}

ModelIndex ProcessTreeModel::parentIndex( const ModelIndex& index ) const {
	if ( index.model() != this || !index.internalData() )
		return {};
	const Node& node = *static_cast<const Node*>( index.internalData() );
	if ( node.parent < 0 )
		return {};
	const Node& parent = mNodes[node.parent];
	return createIndex( parent.rowInParent, treeColumn(), const_cast<Node*>( &parent ) );
}

Variant ProcessTreeModel::data( const ModelIndex& index, ModelRole role ) const {
	if ( index.model() != this || !index.internalData() )
		return {};
	const Node& node = *static_cast<const Node*>( index.internalData() );
	return mSource->data( mSource->index( node.sourceRow, index.column() ), role );
}

ModelIndex ProcessTreeModel::indexForPid( Int64 pid, int column ) const {
	auto found = mNodeForPid.find( pid );
	if ( found == mNodeForPid.end() || column < 0 ||
		 static_cast<size_t>( column ) >= columnCount() )
		return {};
	const Node& node = mNodes[found->second];
	return createIndex( node.rowInParent, column, const_cast<Node*>( &node ) );
}

const ProcessInfo* ProcessTreeModel::processForIndex( const ModelIndex& index ) const {
	if ( index.model() != this || !index.internalData() )
		return nullptr;
	return mSource->getProcessByRow( static_cast<const Node*>( index.internalData() )->sourceRow );
}

} // namespace eproc
