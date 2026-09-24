#ifdef __FreeBSD__
#include "process_collector_freebsd.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <limits>
#include <sys/dkstat.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/user.h>
#include <unistd.h>

namespace eproc {

namespace {

constexpr Int64 kMicrosecondsPerSecond = 1'000'000;

template <typename T> bool readSysctl( const char* name, T& value ) {
	size_t size = sizeof( value );
	return sysctlbyname( name, &value, &size, nullptr, 0 ) == 0 && size == sizeof( value );
}

Uint64 microseconds( const timeval& time ) {
	return static_cast<Uint64>( time.tv_sec ) * kMicrosecondsPerSecond + time.tv_usec;
}

int cpuPercent( Uint64 elapsed, Int64 wallMicroseconds ) {
	if ( wallMicroseconds <= 0 )
		return 0;
	const double percent = static_cast<double>( elapsed ) * 100.0 / wallMicroseconds;
	return static_cast<int>(
		std::min( percent, static_cast<double>( std::numeric_limits<int>::max() ) ) );
}

std::string executablePath( pid_t pid ) {
	int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, pid };
	char path[MAXPATHLEN];
	size_t length = sizeof( path );
	if ( sysctl( mib, 4, path, &length, nullptr, 0 ) != 0 || length <= 1 ||
		 length > sizeof( path ) )
		return {};
	path[std::min( length - 1, sizeof( path ) - 1 )] = '\0';
	return path;
}

} // namespace

bool ProcessCollectorFreeBSD::collect( std::vector<ProcessInfo>& processes, SystemInfo& sysInfo ) {
	const auto sampledAt = std::chrono::steady_clock::now();
	const Int64 elapsedUs =
		mPrevSampleTime.time_since_epoch().count() == 0
			? 0
			: std::chrono::duration_cast<std::chrono::microseconds>( sampledAt - mPrevSampleTime )
				  .count();
	mPrevSampleTime = sampledAt;
	++mPass;
	processes.clear();
	sysInfo = {};
	sysInfo.clockTicksPerSecond = kMicrosecondsPerSecond;
	sysInfo.totalSwap = -1;
	sysInfo.freeSwap = -1;

	int cpuCount = 0;
	if ( readSysctl( "hw.ncpu", cpuCount ) && cpuCount > 0 )
		sysInfo.cpuCount = cpuCount;
	uint64_t totalBytes = 0;
	if ( readSysctl( "hw.physmem", totalBytes ) )
		sysInfo.totalMemory = static_cast<Int64>( totalBytes / 1024 );
	unsigned int freePages = 0;
	unsigned int cachePages = 0;
	if ( readSysctl( "vm.stats.vm.v_free_count", freePages ) ) {
		const Uint64 pageKiB = static_cast<Uint64>( getpagesize() ) / 1024;
		sysInfo.freeMemory = freePages * pageKiB;
		// The inactive page count gives a cheap approximation of reclaimable memory.
		readSysctl( "vm.stats.vm.v_inactive_count", cachePages );
		const Int64 available =
			static_cast<Int64>( ( freePages + static_cast<Uint64>( cachePages ) ) * pageKiB );
		sysInfo.availableMemory =
			sysInfo.totalMemory > 0 ? std::min( sysInfo.totalMemory, available ) : available;
	}
	// FreeBSD swap accounting needs swap device enumeration; leave both fields unavailable here.

	std::array<long, CPUSTATES> cpuTicks{};
	if ( readSysctl( "kern.cp_time", cpuTicks ) ) {
		Uint64 total = 0;
		for ( long ticks : cpuTicks )
			total += static_cast<Uint64>( ticks );
		const Uint64 idle = static_cast<Uint64>( cpuTicks[CP_IDLE] );
		if ( mPrevTotalCpu && total > mPrevTotalCpu && idle >= mPrevIdleCpu ) {
			const Uint64 delta = total - mPrevTotalCpu;
			mLastCpuUsage = static_cast<float>( std::min(
				100.0, 100.0 *
						   static_cast<double>( delta - std::min( delta, idle - mPrevIdleCpu ) ) /
						   delta ) );
		}
		mPrevTotalCpu = total;
		mPrevIdleCpu = idle;
	}
	sysInfo.cpuUsage = mLastCpuUsage;

	timeval bootTime{};
	readSysctl( "kern.boottime", bootTime );
	timeval now{};
	gettimeofday( &now, nullptr );
	sysInfo.uptimeSeconds =
		bootTime.tv_sec > 0 ? std::max( 0.0, static_cast<double>( now.tv_sec - bootTime.tv_sec ) +
												 ( now.tv_usec - bootTime.tv_usec ) / 1e6 )
							: 0.0;

	int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL };
	size_t bytes = 0;
	if ( sysctl( mib, 3, nullptr, &bytes, nullptr, 0 ) != 0 )
		return false;
	std::vector<kinfo_proc> entries;
	bool fetched = false;
	for ( int attempt = 0; attempt < 3; ++attempt ) {
		entries.resize( bytes / sizeof( kinfo_proc ) + 64 );
		size_t capacity = entries.size() * sizeof( kinfo_proc );
		if ( sysctl( mib, 3, entries.data(), &capacity, nullptr, 0 ) == 0 ) {
			entries.resize( capacity / sizeof( kinfo_proc ) );
			fetched = true;
			break;
		}
		if ( errno != ENOMEM )
			break;
		bytes = capacity + 64 * sizeof( kinfo_proc );
	}
	if ( !fetched )
		return false;

	processes.reserve( entries.size() );
	const uid_t currentUid = getuid();
	const Uint64 bootUs = microseconds( bootTime );
	const Uint64 pageKiB = static_cast<Uint64>( getpagesize() ) / 1024;
	for ( const kinfo_proc& info : entries ) {
		if ( info.ki_pid <= 0 )
			continue;
		ProcessInfo proc;
		proc.pid = info.ki_pid;
		proc.parentPid = info.ki_ppid;
		proc.name = info.ki_comm;
		proc.uid = info.ki_ruid;
		proc.euid = info.ki_uid;
		proc.suid = info.ki_svuid;
		const auto& account = mAccounts.get( static_cast<uid_t>( proc.uid ) );
		proc.username = account.name;
		proc.canLogin = account.canLogin;
		proc.euidCanLogin = mAccounts.get( static_cast<uid_t>( proc.euid ) ).canLogin;
		proc.ownedByCurrentUser = proc.uid == currentUid || proc.euid == currentUid;
		proc.systemProcess = proc.uid == 0 || ( !account.name.empty() && !account.canLogin );
		proc.userProcess = !proc.systemProcess && !account.name.empty();
		proc.numThreads = info.ki_numthreads;
		proc.niceLevel = info.ki_nice;
		proc.vmSize = static_cast<Int64>( info.ki_size / 1024 );
		proc.vmRSS = static_cast<Int64>( info.ki_rssize * pageKiB );
		proc.userTime = static_cast<Int64>( microseconds( info.ki_rusage.ru_utime ) );
		proc.sysTime = static_cast<Int64>( microseconds( info.ki_rusage.ru_stime ) );
		const Uint64 startUs = microseconds( info.ki_start );
		proc.startTime =
			bootTime.tv_sec > 0 && startUs >= bootUs ? static_cast<Int64>( startUs - bootUs ) : 0;
		proc.commandLine = proc.name;

		const Uint64 user = static_cast<Uint64>( proc.userTime );
		const Uint64 kernel = static_cast<Uint64>( proc.sysTime );
		TickEntry& previous = mProcessTicks[proc.pid];
		if ( previous.pass == mPass - 1 && previous.startTime == startUs &&
			 user >= previous.userTime && kernel >= previous.sysTime ) {
			proc.userUsage = cpuPercent( user - previous.userTime, elapsedUs );
			proc.sysUsage = cpuPercent( kernel - previous.sysTime, elapsedUs );
		}
		if ( previous.pass == 0 || previous.startTime != startUs ) {
			previous.iconPath = mIconResolver.iconFor( executablePath( info.ki_pid ), proc.name );
		}
		proc.iconPath = previous.iconPath;
		previous.startTime = startUs;
		previous.userTime = user;
		previous.sysTime = kernel;
		previous.pass = mPass;
		processes.emplace_back( std::move( proc ) );
	}
	for ( auto it = mProcessTicks.begin(); it != mProcessTicks.end(); ) {
		if ( it->second.pass != mPass )
			it = mProcessTicks.erase( it );
		else
			++it;
	}
	return true;
}

} // namespace eproc
#endif // __FreeBSD__
