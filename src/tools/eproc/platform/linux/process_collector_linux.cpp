#include "process_collector_linux.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <thread>

#include <eepp/core/string.hpp>
#include <eepp/system/fileinfo.hpp>

#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

using namespace EE::System;

namespace eproc {

namespace {

// /proc pseudo-files are read several times per process on every pass. Building the paths with
// string concatenation and reading them through std::ifstream allocated a path string plus the
// stream's own multi-kilobyte buffer per file; formatting into a stack buffer and issuing a single
// read(2) keeps the heap untouched.

constexpr size_t kProcPathCapacity = 64;
constexpr size_t kProcFileCapacity = 8192;

/** How often (in passes) the per-pid caches are swept of processes that exited. A sweep walks the
 *  whole map, so it is amortised over many passes instead of running on every one. */
constexpr Uint32 kCacheSweepInterval = 60;
// Four short-lived PSS workers finish the measured cold scan within UI initialization;
// one/two workers exceeded the startup budget. No extra PSS workers survive this scan.
constexpr size_t kStartupPssWorkers = 4;
// smaps_rollup walks page tables. Refresh one PID bucket per pass to spread that work across
// collections, while reading uncached values immediately.
constexpr Uint32 kPssSampleInterval = 5;

/** Formats "/proc/<pid>/<leaf>" into @p out. Returns the length, or 0 on overflow. */
inline size_t formatProcPath( char* out, size_t capacity, long pid, const char* leaf ) {
	int written = snprintf( out, capacity, "/proc/%ld/%s", pid, leaf );
	return written > 0 && static_cast<size_t>( written ) < capacity ? static_cast<size_t>( written )
																	: 0;
}

/** Reads a small pseudo-file into @p out, NUL-terminated. Larger files are truncated, which is
 *  fine for the fields parsed here. */
inline bool readProcFile( const char* path, char* out, size_t capacity, size_t& length ) {
	length = 0;

	int fd = ::open( path, O_RDONLY | O_CLOEXEC );
	if ( fd < 0 )
		return false;

	ssize_t readBytes = ::read( fd, out, capacity - 1 );
	::close( fd );

	if ( readBytes <= 0 )
		return false;

	length = static_cast<size_t>( readBytes );
	out[length] = '\0';
	return true;
}

/** Skips the whitespace that follows a "Label:" separator. */
inline const char* skipSeparator( std::string_view line ) {
	size_t colon = line.find( ':' );
	if ( colon == std::string_view::npos )
		return nullptr;

	const char* p = line.data() + colon + 1;
	const char* end = line.data() + line.size();
	while ( p < end && ( *p == ' ' || *p == '\t' ) )
		++p;
	return p;
}

// /proc files pad values with spaces (meminfo) or tabs (status); the parsers below tolerate either
// separator so one implementation reads both formats.

/** Returns the first integer following the ':' of a "Label:   value" line, or 0 when absent. */
inline long parseLabeledLong( std::string_view line ) {
	const char* p = skipSeparator( line );
	return p ? strtol( p, nullptr, 10 ) : 0;
}

/** Reads up to @p count whitespace-separated integers following the ':' of a "Label: v v v" line.
 *  Used for the status "Uid:" line, which carries the real, effective, saved and fs ids. */
inline void parseLabeledLongs( std::string_view line, long* out, int count ) {
	const char* p = skipSeparator( line );
	if ( !p )
		return;

	const char* end = line.data() + line.size();
	for ( int i = 0; i < count; ++i ) {
		while ( p < end && ( *p == ' ' || *p == '\t' ) )
			++p;
		if ( p >= end )
			return;

		long value = 0;
		auto result = std::from_chars( p, end, value );
		if ( result.ec != std::errc() )
			return;

		out[i] = value;
		p = result.ptr;
	}
}

/** Returns the trimmed text following the ':' of a "Label:   value" line, as a view into it. */
inline std::string_view parseLabeledString( std::string_view line ) {
	const char* p = skipSeparator( line );
	if ( !p )
		return {};

	std::string_view value( p, line.data() + line.size() - p );
	return String::trim( value, " \t\r\n" );
}

std::string ttyName( long ttyNumber ) {
	if ( ttyNumber == 0 )
		return {};

	const unsigned long device = static_cast<unsigned long>( ttyNumber );
	const unsigned int major = static_cast<unsigned int>( ( device >> 8 ) & 0xff );
	const unsigned int minor = static_cast<unsigned int>( device & 0xff );
	char buffer[32];
	if ( major == 136 )
		snprintf( buffer, sizeof( buffer ), "pts/%u", minor );
	else if ( major == 4 )
		snprintf( buffer, sizeof( buffer ), minor < 64 ? "tty/%u" : "ttyS/%u",
				  minor < 64 ? minor : minor - 64 );
	else
		return {};
	return buffer;
}

/** Resolves the executable behind a pid into @p out. Returns an empty string for kernel threads
 *  and for processes the caller may not inspect. The " (deleted)" suffix readlink adds for an
 *  unlinked executable is left in place: ProcessIconResolver strips it when matching names. */
std::string readExePath( long pid, char* out, size_t capacity ) {
	char linkPath[kProcPathCapacity];
	if ( 0 == formatProcPath( linkPath, sizeof( linkPath ), pid, "exe" ) )
		return {};

	ssize_t length = readlink( linkPath, out, capacity - 1 );
	if ( length <= 0 )
		return {};

	return std::string( out, static_cast<size_t>( length ) );
}

// Startup workers use only stat and smaps_rollup. They never access collector caches.
struct StartupPssSample {
	Int64 startTime{ -1 };
	Int64 rssPages{ 0 };
	Int64 value{ -1 };
	long pid{ 0 };
};

bool readProcessMemoryIdentity( long pid, Int64& startTime, Int64& rssPages ) {
	char path[kProcPathCapacity];
	char buffer[kProcFileCapacity];
	size_t length = 0;
	if ( 0 == formatProcPath( path, sizeof( path ), pid, "stat" ) ||
		 !readProcFile( path, buffer, sizeof( buffer ), length ) )
		return false;
	// comm can contain spaces and parentheses; fields after the final ')' start with state (3).
	const char* p = strrchr( buffer, ')' );
	if ( !p || p[1] != ' ' )
		return false;
	p += 2;
	for ( int field = 3; field < 22; ++field ) {
		while ( *p && *p != ' ' )
			++p;
		while ( *p == ' ' )
			++p;
		if ( !*p )
			return false;
	}
	const char* end = buffer + length;
	auto start = std::from_chars( p, end, startTime );
	if ( start.ec != std::errc() || start.ptr == end || *start.ptr != ' ' )
		return false;
	p = start.ptr;
	while ( *p == ' ' )
		++p;
	// vsize (23) is irrelevant; RSS (24) is the cheap zero-memory shortcut.
	while ( *p && *p != ' ' )
		++p;
	while ( *p == ' ' )
		++p;
	auto rss = std::from_chars( p, end, rssPages );
	return rss.ec == std::errc() && startTime >= 0 && rssPages >= 0;
}

Int64 readProcessPss( long pid ) {
	char path[kProcPathCapacity];
	char buffer[kProcFileCapacity];
	size_t length = 0;
	if ( 0 == formatProcPath( path, sizeof( path ), pid, "smaps_rollup" ) ||
		 !readProcFile( path, buffer, sizeof( buffer ), length ) )
		return -1;
	const char* line = strstr( buffer, "\nPss:" );
	return line ? parseLabeledLong( line + 1 ) : -1;
}

void collectStartupPss( std::vector<StartupPssSample>& samples ) {
	DIR* procDir = opendir( "/proc" );
	if ( !procDir )
		return;
	struct dirent* entry;
	while ( ( entry = readdir( procDir ) ) != nullptr ) {
		char* end = nullptr;
		const long pid = strtol( entry->d_name, &end, 10 );
		if ( *end != '\0' || pid <= 0 )
			continue;
		StartupPssSample sample;
		sample.pid = pid;
		if ( readProcessMemoryIdentity( pid, sample.startTime, sample.rssPages ) )
			samples.push_back( sample );
	}
	closedir( procDir );

	// Measured cold scans have a long tail of large resident processes. Start those first so
	// their page-table walks overlap the cheap jobs instead of delaying the final join.
	std::sort( samples.begin(), samples.end(),
			   []( const auto& lhs, const auto& rhs ) { return lhs.rssPages > rhs.rssPages; } );

	std::atomic<size_t> nextJob{ 0 };
	const auto work = [&] {
		for ( ;; ) {
			const size_t index = nextJob.fetch_add( 1, std::memory_order_relaxed );
			if ( index >= samples.size() )
				break;
			auto& sample = samples[index];
			sample.value = sample.rssPages == 0 ? 0 : readProcessPss( sample.pid );
			// Check again after the accounting read: an exit/reused PID during the read must
			// not attach a different incarnation's PSS to this sample.
			Int64 startTime = 0, rssPages = 0;
			if ( !readProcessMemoryIdentity( sample.pid, startTime, rssPages ) ||
				 startTime != sample.startTime )
				sample.startTime = -1;
		}
	};
	// The sampler thread is itself one worker. All helpers join before samples are merged.
	std::array<std::jthread, kStartupPssWorkers - 1> workers;
	for ( auto& worker : workers )
		worker = std::jthread( work );
	work();
}

// ksysguard treats an account as a system account when its shell cannot be used to log in.
bool isLoginShell( const char* shell ) {
	if ( !shell || !*shell )
		return false;
	const char* base = strrchr( shell, '/' );
	base = base ? base + 1 : shell;
	return strcmp( base, "nologin" ) != 0 && strcmp( base, "false" ) != 0;
}

} // namespace

ProcessCollectorLinux::ProcessCollectorLinux() {
	mPageSizeKb = sysconf( _SC_PAGESIZE ) / 1024;
	mProcessorCount = sysconf( _SC_NPROCESSORS_ONLN );
	if ( mProcessorCount < 1 )
		mProcessorCount = 1;

	mJiffiesPerSecond = sysconf( _SC_CLK_TCK );
	if ( mJiffiesPerSecond < 1 )
		mJiffiesPerSecond = 100;

	// Require roughly a fifth of a second of machine-wide time before trusting a CPU delta.
	mMinSampleJiffies = static_cast<long long>( mProcessorCount ) * mJiffiesPerSecond / 5;
}

ProcessCollectorLinux::~ProcessCollectorLinux() {}

bool ProcessCollectorLinux::readCpuTimes( long long& idle, long long& total, SystemInfo& sysInfo ) {
	idle = 0;
	total = 0;

	FILE* file = fopen( "/proc/stat", "r" );
	if ( !file )
		return false;

	char buffer[256];
	if ( !fgets( buffer, sizeof( buffer ), file ) ) {
		fclose( file );
		return false;
	}
	long long user = 0, nice = 0, system = 0, idleVal = 0, iowait = 0, irq = 0, softirq = 0,
			  steal = 0;
	// The leading "cpu" aggregate line has at least the first four counters on every kernel.
	if ( sscanf( buffer, "cpu %lld %lld %lld %lld %lld %lld %lld %lld", &user, &nice, &system,
				 &idleVal, &iowait, &irq, &softirq, &steal ) < 4 ) {
		fclose( file );
		return false;
	}

	idle = idleVal + iowait;
	total = user + nice + system + idleVal + iowait + irq + softirq + steal;
	while ( fgets( buffer, sizeof( buffer ), file ) && buffer[0] == 'c' && buffer[1] == 'p' &&
			buffer[2] == 'u' && buffer[3] >= '0' && buffer[3] <= '9' ) {
		unsigned int index = 0;
		user = nice = system = idleVal = iowait = irq = softirq = steal = 0;
		if ( sscanf( buffer, "cpu%u %lld %lld %lld %lld %lld %lld %lld %lld", &index, &user, &nice,
					 &system, &idleVal, &iowait, &irq, &softirq, &steal ) < 5 ||
			 index >= 4096 )
			continue;
		if ( index >= mCoreCpuSamples.size() )
			mCoreCpuSamples.resize( index + 1 );
		auto& previous = mCoreCpuSamples[index];
		const long long coreIdle = idleVal + iowait;
		const long long coreTotal = user + nice + system + idleVal + iowait + irq + softirq + steal;
		const long long delta = coreTotal - previous.total;
		if ( previous.total > 0 && delta >= mJiffiesPerSecond / 5 && coreIdle >= previous.idle )
			previous.usage =
				static_cast<float>( delta - ( coreIdle - previous.idle ) ) / delta * 100.f;
		previous.idle = coreIdle;
		previous.total = coreTotal;
	}
	fclose( file );
	sysInfo.coreCpuUsage.resize( mCoreCpuSamples.size() );
	for ( size_t i = 0; i < mCoreCpuSamples.size(); ++i )
		sysInfo.coreCpuUsage[i] = mCoreCpuSamples[i].usage;
	return true;
}

// Parses /proc/[pid]/stat — extracts: name, state, ppid, utime, stime, nice, num_threads,
// vsize (bytes), rss (pages). Returns false if the line can't be parsed.
bool ProcessCollectorLinux::readProcessStat( ProcessInfo& proc, const char* statLine ) {
	const char* p = statLine;

	// pid
	while ( *p && *p != ' ' )
		p++;
	if ( !*p )
		return false;
	p++; // skip space

	// comm — may contain spaces/parens, so find the last ')'
	if ( *p != '(' )
		return false;
	p++;
	const char* end = strrchr( p, ')' );
	if ( !end || *( end + 1 ) != ' ' )
		return false;
	proc.name.assign( p, end - p );
	p = end + 2; // skip ') '

	// state
	if ( !*p )
		return false;
	switch ( *p ) {
		case 'R':
			proc.status = ProcessStatus::Running;
			break;
		case 'S':
			proc.status = ProcessStatus::Sleeping;
			break;
		case 'D':
			proc.status = ProcessStatus::DiskSleep;
			break;
		case 'Z':
			proc.status = ProcessStatus::Zombie;
			break;
		case 'T':
		case 't':
			proc.status = ProcessStatus::Stopped;
			break;
		case 'W':
			proc.status = ProcessStatus::Paging;
			break;
		default:
			proc.status = ProcessStatus::Other;
			break;
	}
	p++; // past state char
	if ( *p )
		p++; // past space

	// Now we're at field 4 (1-indexed): ppid
	// Fields: ppid(4) pgrp(5) session(6) tty_nr(7) tpgid(8) flags(9)
	//         minflt(10) cminflt(11) majflt(12) cmajflt(13) utime(14) stime(15)
	//         cutime(16) cstime(17) priority(18) nice(19) num_threads(20)
	//         itrealvalue(21) starttime(22) vsize(23) rss(24)
	auto skipField = [&]() {
		while ( *p && *p != ' ' )
			p++;
		if ( *p )
			p++;
	};

	// ppid (field 4)
	proc.parentPid = strtol( p, nullptr, 10 );
	skipField();

	// pgrp(5), session(6)
	skipField();
	skipField();

	// tty_nr (field 7) — the controlling terminal; 0 means none. Used by the "Programs Only"
	// filter, which mirrors ksysguard's tty test.
	proc.ttyNr = strtol( p, nullptr, 10 );
	skipField();

	// tpgid(8), flags(9)
	skipField();
	skipField();

	// minflt(10), cminflt(11), majflt(12), cmajflt(13) — skip 4 fields
	for ( int i = 0; i < 4; i++ )
		skipField();

	// utime (field 14)
	proc.userTime = strtol( p, nullptr, 10 );
	skipField();

	// stime (field 15)
	proc.sysTime = strtol( p, nullptr, 10 );
	skipField();

	// cutime(16), cstime(17), priority(18) — skip 3 fields
	for ( int i = 0; i < 3; i++ )
		skipField();

	// nice (field 19)
	proc.niceLevel = strtol( p, nullptr, 10 );
	skipField();

	// num_threads (field 20)
	proc.numThreads = strtol( p, nullptr, 10 );
	skipField();

	// itrealvalue (field 21) — skip
	skipField();

	// starttime (field 22) — the process's start, in clock ticks since boot
	proc.startTime = strtoll( p, nullptr, 10 );
	skipField();

	// vsize (field 23) — bytes
	proc.vmSize = strtol( p, nullptr, 10 ) / 1024;
	skipField();

	// rss (field 24) — pages; mPageSizeKb is already KiB per page
	if ( *p )
		proc.vmRSS = strtol( p, nullptr, 10 ) * mPageSizeKb;

	proc.tty = ttyName( proc.ttyNr );

	return true;
}

void ProcessCollectorLinux::readProcessStatus( ProcessInfo& proc, const char* content,
											   size_t length ) {
	String::readBySeparator( std::string_view( content, length ), [&proc]( std::string_view line ) {
		if ( String::startsWith( line, "Uid:" ) ) {
			long ids[4] = { 0, 0, 0, 0 };
			parseLabeledLongs( line, ids, 4 );
			proc.uid = ids[0];
			proc.euid = ids[1];
			proc.suid = ids[2];
			proc.fsuid = ids[3];
		} else if ( String::startsWith( line, "TracerPid:" ) ) {
			proc.tracerPid = parseLabeledLong( line );
		} else if ( String::startsWith( line, "VmRSS:" ) ) {
			long rss = parseLabeledLong( line );
			if ( rss > 0 )
				proc.vmRSS = rss;
		} else if ( String::startsWith( line, "RssFile:" ) ) {
			if ( !proc.hasSharedInfo )
				proc.sharedMem = 0;
			proc.sharedMem += parseLabeledLong( line );
			proc.hasSharedInfo = true;
		} else if ( String::startsWith( line, "RssShmem:" ) ) {
			if ( !proc.hasSharedInfo )
				proc.sharedMem = 0;
			proc.sharedMem += parseLabeledLong( line );
			proc.hasSharedInfo = true;
		} else if ( String::startsWith( line, "Name:" ) ) {
			if ( proc.name.empty() )
				proc.name.assign( parseLabeledString( line ) );
		}
	} );
}

void ProcessCollectorLinux::readProcessCmdline( ProcessInfo& proc, long pid ) {
	char path[kProcPathCapacity];
	if ( 0 == formatProcPath( path, sizeof( path ), pid, "cmdline" ) )
		return;

	int fd = ::open( path, O_RDONLY | O_CLOEXEC );
	if ( fd < 0 )
		return;

	char buffer[kProcFileCapacity];
	proc.commandLine.clear();
	for ( ;; ) {
		ssize_t bytes = ::read( fd, buffer, sizeof( buffer ) );
		if ( bytes <= 0 )
			break;
		proc.commandLine.append( buffer, static_cast<size_t>( bytes ) );
	}
	::close( fd );

	if ( proc.commandLine.empty() )
		return;

	// Linux comm is limited to 15 bytes. As in ksysguard, extend a truncated name from argv[0]
	// only when its basename begins with the kernel name; wrappers and renamed processes retain
	// their comm instead. Inspect argv[0] before replacing the NUL argument separators below.
	if ( proc.name.size() == 15 ) {
		const size_t firstSeparator = proc.commandLine.find( '\0' );
		const std::string_view argv0( proc.commandLine.data(), firstSeparator == std::string::npos
																   ? proc.commandLine.size()
																   : firstSeparator );
		const size_t lastSlash = argv0.rfind( '/' );
		const std::string_view basename =
			argv0.substr( lastSlash == std::string_view::npos ? 0 : lastSlash + 1 );
		if ( basename.size() > proc.name.size() && basename.starts_with( proc.name ) )
			proc.name.assign( basename );
	}

	// argv entries are NUL separated. Replace the separators with spaces to reconstruct the
	// command line as ksysguard6 shows it in its command column (and for the clipboard).
	for ( char& character : proc.commandLine ) {
		if ( character == '\0' )
			character = ' ';
	}
	while ( !proc.commandLine.empty() && proc.commandLine.back() == ' ' )
		proc.commandLine.pop_back();
}

void ProcessCollectorLinux::readProcessIO( ProcessInfo& proc, long pid ) {
	char path[kProcPathCapacity];
	if ( 0 == formatProcPath( path, sizeof( path ), pid, "io" ) )
		return;

	char buffer[kProcFileCapacity];
	size_t length = 0;
	if ( !readProcFile( path, buffer, sizeof( buffer ), length ) )
		return;

	String::readBySeparator( std::string_view( buffer, length ), [&proc]( std::string_view line ) {
		const char* value = nullptr;
		if ( String::startsWith( line, "read_bytes:" ) ) {
			value = skipSeparator( line );
			if ( value )
				proc.ioReadBytes = strtoll( value, nullptr, 10 );
		} else if ( String::startsWith( line, "write_bytes:" ) ) {
			value = skipSeparator( line );
			if ( value )
				proc.ioWriteBytes = strtoll( value, nullptr, 10 );
		}
	} );
}

void ProcessCollectorLinux::readSystemMemory( SystemInfo& sysInfo ) {
	char buffer[kProcFileCapacity];
	size_t length = 0;
	if ( !readProcFile( "/proc/meminfo", buffer, sizeof( buffer ), length ) )
		return;

	String::readBySeparator( std::string_view( buffer, length ),
							 [&sysInfo]( std::string_view line ) {
								 if ( String::startsWith( line, "MemTotal:" ) ) {
									 sysInfo.totalMemory = parseLabeledLong( line );
								 } else if ( String::startsWith( line, "MemFree:" ) ) {
									 sysInfo.freeMemory = parseLabeledLong( line );
								 } else if ( String::startsWith( line, "MemAvailable:" ) ) {
									 sysInfo.availableMemory = parseLabeledLong( line );
								 } else if ( String::startsWith( line, "SwapTotal:" ) ) {
									 sysInfo.totalSwap = parseLabeledLong( line );
								 } else if ( String::startsWith( line, "SwapFree:" ) ) {
									 sysInfo.freeSwap = parseLabeledLong( line );
								 }
							 } );
}

void ProcessCollectorLinux::readSystemUptime( SystemInfo& sysInfo ) {
	char buffer[kProcFileCapacity];
	size_t length = 0;
	if ( !readProcFile( "/proc/uptime", buffer, sizeof( buffer ), length ) )
		return;

	char* end = nullptr;
	const double uptime = strtod( buffer, &end );
	if ( end != buffer && uptime >= 0 )
		sysInfo.uptimeSeconds = uptime;
}

void ProcessCollectorLinux::resolveUser( ProcessInfo& proc ) {
	// Insert first, then read: an insertion can rehash the cache, so no reference may be held
	// across a lookup for a second uid.
	userInfo( proc.uid );
	if ( proc.euid != proc.uid )
		userInfo( proc.euid );

	const UserInfo& real = mUserCache.at( proc.uid );
	proc.username = real.name;
	proc.canLogin = real.canLogin;
	proc.euidCanLogin = proc.euid == proc.uid ? real.canLogin : mUserCache.at( proc.euid ).canLogin;
	const Int64 own = static_cast<Int64>( getuid() );
	proc.ownedByCurrentUser =
		proc.uid == own || proc.euid == own || proc.suid == own || proc.fsuid == own;
	proc.systemProcess = proc.uid < 100 || !proc.canLogin;
	proc.userProcess =
		( proc.uid >= 100 && proc.canLogin ) || ( proc.euid >= 100 && proc.euidCanLogin );
}

const ProcessCollectorLinux::UserInfo& ProcessCollectorLinux::userInfo( long uid ) {
	auto it = mUserCache.find( uid );
	if ( it != mUserCache.end() )
		return it->second;

	UserInfo info;
	struct passwd* pw = getpwuid( static_cast<uid_t>( uid ) );
	if ( pw ) {
		info.name = pw->pw_name ? pw->pw_name : std::to_string( uid );
		info.canLogin = isLoginShell( pw->pw_shell );
	} else {
		info.name = std::to_string( uid );
		info.canLogin = false;
	}

	return mUserCache.emplace( uid, std::move( info ) ).first->second;
}

bool ProcessCollectorLinux::collectInitial( std::vector<ProcessInfo>& processes,
											SystemInfo& sysInfo,
											const InitialSnapshotCallback& publishBase,
											std::vector<ProportionalMemorySample>* memory ) {
	if ( !mCollectProportionalMemory )
		return collect( processes, sysInfo );

	// Overlap the cold PSS scan with the normal snapshot and with UI initialization. Workers
	// write only private samples; the collector cache remains exclusively owned by this thread.
	std::vector<StartupPssSample> samples;
	std::jthread sampler( [&samples] { collectStartupPss( samples ); } );
	mCollectProportionalMemory = false;
	const bool collected = collect( processes, sysInfo );
	mCollectProportionalMemory = true;
	if ( !collected )
		return false; // sampler's destructor still joins all startup workers.

	const bool publishEarly = publishBase && memory;
	if ( publishEarly ) {
		// Keep only numeric identities/RSS-zero markers for the later merge. The large process
		// snapshot and its strings move directly to the UI; no snapshot copy is needed.
		memory->clear();
		memory->reserve( processes.size() );
		for ( const auto& proc : processes )
			memory->push_back( { proc.pid, proc.startTime, proc.vmRSS == 0 ? 0 : -1 } );
		publishBase( std::move( processes ), std::move( sysInfo ) );
	}
	sampler.join();

	std::sort( samples.begin(), samples.end(),
			   []( const auto& lhs, const auto& rhs ) { return lhs.pid < rhs.pid; } );
	const auto merge = [&]( Int64 pid, Int64 startTime, bool zeroRss ) -> Int64 {
		const auto sample =
			std::lower_bound( samples.begin(), samples.end(), pid,
							  []( const auto& sample, Int64 pid ) { return sample.pid < pid; } );
		if ( sample == samples.end() || sample->pid != pid || sample->startTime != startTime )
			return -1;
		// Seed the ordinary bucket cache, so the next pass does not repeat the full scan.
		PssEntry& pss = mProcessPss[pid];
		pss.value = zeroRss ? 0 : sample->value;
		pss.startTime = startTime;
		pss.alivePass = mPass;
		return pss.value;
	};
	if ( publishEarly ) {
		for ( auto& sample : *memory )
			sample.valueKB = merge( sample.pid, sample.startTime, sample.valueKB == 0 );
		std::sort( memory->begin(), memory->end(),
				   []( const auto& lhs, const auto& rhs ) { return lhs.pid < rhs.pid; } );
	} else {
		for ( auto& proc : processes )
			proc.vmPSS = merge( proc.pid, proc.startTime, proc.vmRSS == 0 );
	}
	return true;
}

bool ProcessCollectorLinux::collect( std::vector<ProcessInfo>& processes, SystemInfo& sysInfo ) {
	// 0 is the "never seen" sentinel for the per-pid caches below.
	++mPass;
	if ( mPass == 0 )
		++mPass;

	// CPU usage needs two samples separated by a real interval. A sub-jiffy window quantises into
	// noise (a few busy jiffies out of a few total reads as ~100%), and the very first sample has
	// no reference point, so both keep the last known value instead of inventing one.
	long long idle = 0, total = 0;
	long long deltaTotal = 0;
	long long deltaIdle = 0;

	if ( readCpuTimes( idle, total, sysInfo ) ) {
		if ( mPrevTotalCpu > 0 && total > mPrevTotalCpu ) {
			deltaTotal = total - mPrevTotalCpu;
			deltaIdle = idle - mPrevIdleCpu;
		}
		mPrevTotalCpu = total;
		mPrevIdleCpu = idle;
	}

	const bool haveSample = deltaTotal >= mMinSampleJiffies;
	if ( haveSample )
		mLastCpuUsage = static_cast<float>( deltaTotal - deltaIdle ) / deltaTotal * 100.f;
	sysInfo.cpuUsage = mLastCpuUsage;
	sysInfo.cpuCount = mProcessorCount;

	// System memory
	readSystemMemory( sysInfo );
	sysInfo.clockTicksPerSecond = mJiffiesPerSecond;
	readSystemUptime( sysInfo );

	// GPU figures are per-PID and driver-provided, so query them once per pass and apply below.
	// The maps are members so the per-pass queries do not reallocate them.
	mGpuUsage.clear();
	mGpuMemory.clear();
	if ( mGpuReader.isAvailable() )
		mGpuReader.query( mGpuUsage, mGpuMemory );
	mDrmGpuReader.beginSample();

	// Network packet capture runs continuously in its own thread; this refresh only joins the
	// current procfs socket ownership with the endpoints seen by that capture thread.
	mNetworkMonitor.refreshMapping();

	DIR* procDir = opendir( "/proc" );
	if ( !procDir )
		return false;

	processes.clear();

	struct dirent* entry;
	while ( ( entry = readdir( procDir ) ) != nullptr ) {
		// Check if directory entry is a PID
		char* endptr = nullptr;
		long pid = strtol( entry->d_name, &endptr, 10 );
		if ( *endptr != '\0' || pid <= 0 )
			continue;

		// /proc reports DT_DIR, but other filesystems may return DT_UNKNOWN; verify those.
		if ( entry->d_type == DT_UNKNOWN ) {
			char dirPath[kProcPathCapacity];
			int written = snprintf( dirPath, sizeof( dirPath ), "/proc/%ld", pid );
			if ( written <= 0 || !FileInfo( dirPath ).isDirectory() )
				continue;
		} else if ( entry->d_type != DT_DIR ) {
			continue;
		}

		ProcessInfo proc;
		proc.pid = pid;

		char path[kProcPathCapacity];
		char buffer[kProcFileCapacity];
		size_t length = 0;

		// /proc/[pid]/stat — name, state, ppid, utime, stime, nice, threads, vsize, rss
		if ( 0 == formatProcPath( path, sizeof( path ), pid, "stat" ) ||
			 !readProcFile( path, buffer, sizeof( buffer ), length ) )
			continue;

		if ( !readProcessStat( proc, buffer ) )
			continue;

		// /proc/[pid]/status — uid variants, VmRSS, the shared breakdown and TracerPid
		if ( 0 != formatProcPath( path, sizeof( path ), pid, "status" ) &&
			 readProcFile( path, buffer, sizeof( buffer ), length ) )
			readProcessStatus( proc, buffer, length );

		// ksysguard's Memory column is the process's private memory: resident memory minus the
		// pages it shares with other processes. Only derived when the kernel reported the shared
		// breakdown; otherwise vmURSS stays -1 and the display falls back to RSS.
		if ( proc.hasSharedInfo && proc.vmRSS >= 0 )
			proc.vmURSS = proc.vmRSS - proc.sharedMem;

		collectProcessPss( proc );

		// CPU usage delta against the previous pass for this PID. Entries are updated in place and
		// swept periodically, so the steady state does not allocate. A pid whose start time
		// changed is a different process that reused the number, so it starts from scratch.
		long long curTicks = proc.userTime + proc.sysTime;
		TickEntry& tickEntry = mProcessTicks[pid];
		const bool sameProcess = tickEntry.startTime == proc.startTime;
		const bool hadPrevious = sameProcess && tickEntry.pass == mPass - 1;
		long long deltaTicks = hadPrevious ? curTicks - tickEntry.ticks : 0;
		tickEntry.ticks = curTicks;
		tickEntry.startTime = proc.startTime;
		tickEntry.pass = mPass;

		if ( haveSample && deltaTicks > 0 ) {
			// deltaTotal spans every core, so dividing by the core count turns it into elapsed
			// wall-clock ticks: the result is percent of a single core, matching ksysguard.
			double wallTicks = static_cast<double>( deltaTotal ) / mProcessorCount;
			proc.userUsage = static_cast<int>( deltaTicks * 100.0 / wallTicks );
		}

		// Read command line
		readProcessCmdline( proc, pid );

		// Read actual storage I/O totals. Processes without permission to expose this file keep the
		// default -1 values, so the corresponding optional columns remain empty.
		readProcessIO( proc, pid );

		// Resolve username and login capability from uid
		resolveUser( proc );

		auto usageIt = mGpuUsage.find( pid );
		if ( usageIt != mGpuUsage.end() )
			proc.gpuUsage = usageIt->second;

		auto memoryIt = mGpuMemory.find( pid );
		if ( memoryIt != mGpuMemory.end() )
			proc.gpuMemory = memoryIt->second;
		if ( proc.gpuUsage < 0 || proc.gpuMemory < 0 ) {
			int drmUsage = -1;
			long drmMemory = -1;
			mDrmGpuReader.query( pid, proc.startTime, drmUsage, drmMemory );
			if ( proc.gpuUsage < 0 )
				proc.gpuUsage = drmUsage;
			if ( proc.gpuMemory < 0 )
				proc.gpuMemory = drmMemory;
		}

		// Icons are resolved once per process, not once per pass: the executable behind a pid does
		// not change, and the resolver walks the desktop index on a miss. A pid that was reused by
		// a new process is a miss, so it does not keep the dead process's icon.
		auto iconIt = mProcessIcons.find( pid );
		if ( iconIt != mProcessIcons.end() && iconIt->second.startTime != proc.startTime )
			iconIt = mProcessIcons.end();

		if ( iconIt == mProcessIcons.end() ) {
			char exeBuffer[PATH_MAX];
			IconEntry iconEntry;
			iconEntry.path = mIconResolver.iconFor(
				readExePath( pid, exeBuffer, sizeof( exeBuffer ) ), proc.name );
			iconEntry.startTime = proc.startTime;
			iconEntry.pass = mPass;
			iconIt = mProcessIcons.insert_or_assign( pid, std::move( iconEntry ) ).first;
		} else {
			iconIt->second.pass = mPass;
		}
		proc.iconPath = iconIt->second.path;

		processes.push_back( std::move( proc ) );
	}

	closedir( procDir );

	// Sweeping is amortised: entries of exited processes only cost memory until the next sweep.
	if ( ( mPass % kCacheSweepInterval ) == 0 )
		pruneCaches();

	mNetworkMonitor.applyRates( processes );

	return true;
}

void ProcessCollectorLinux::collectProcessPss( ProcessInfo& proc ) {
	auto pssIt = mProcessPss.find( proc.pid );
	if ( pssIt != mProcessPss.end() ) {
		if ( pssIt->second.startTime != proc.startTime ) {
			// A reused PID must never inherit the previous incarnation's cached PSS,
			// including when collection is disabled.
			mProcessPss.erase( pssIt );
		} else {
			// Liveness is independent of refresh policy. Keep live values while hidden.
			pssIt->second.alivePass = mPass;
		}
	}
	if ( mCollectProportionalMemory ) {
		PssEntry& pss = mProcessPss[proc.pid];
		if ( proc.vmRSS == 0 ) {
			// Zombies/kernel threads have no resident userspace memory to account for.
			pss.value = 0;
			pss.startTime = proc.startTime;
		} else if ( pss.startTime != proc.startTime || pss.value < 0 ||
					static_cast<Uint32>( proc.pid ) % kPssSampleInterval ==
						mPass % kPssSampleInterval ) {
			pss.value = readProcessPss( proc.pid );
			pss.startTime = proc.startTime;
		}
		pss.alivePass = mPass;
		proc.vmPSS = pss.value;
	}
}

void ProcessCollectorLinux::pruneCaches() {
	for ( auto it = mProcessTicks.begin(); it != mProcessTicks.end(); ) {
		if ( it->second.pass != mPass )
			it = mProcessTicks.erase( it );
		else
			++it;
	}

	for ( auto it = mProcessIcons.begin(); it != mProcessIcons.end(); ) {
		if ( it->second.pass != mPass )
			it = mProcessIcons.erase( it );
		else
			++it;
	}
	for ( auto it = mProcessPss.begin(); it != mProcessPss.end(); ) {
		if ( it->second.alivePass != mPass )
			it = mProcessPss.erase( it );
		else
			++it;
	}
}

} // namespace eproc
