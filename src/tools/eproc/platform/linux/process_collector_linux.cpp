#include "process_collector_linux.hpp"

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

bool ProcessCollectorLinux::readCpuTimes( long long& idle, long long& total ) {
	idle = 0;
	total = 0;

	char buffer[kProcFileCapacity];
	size_t length = 0;
	if ( !readProcFile( "/proc/stat", buffer, sizeof( buffer ), length ) )
		return false;

	long long user = 0, nice = 0, system = 0, idleVal = 0, iowait = 0, irq = 0, softirq = 0,
			  steal = 0;
	// The leading "cpu" aggregate line has at least the first four counters on every kernel.
	if ( sscanf( buffer, "cpu %lld %lld %lld %lld %lld %lld %lld %lld", &user, &nice, &system,
				 &idleVal, &iowait, &irq, &softirq, &steal ) < 4 )
		return false;

	idle = idleVal + iowait;
	total = user + nice + system + idleVal + iowait + irq + softirq + steal;
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

	if ( readCpuTimes( idle, total ) ) {
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
}

} // namespace eproc
