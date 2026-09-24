#include "process_collector_macos.hpp"

#include <algorithm>
#include <cmath>
#include <libproc.h>
#include <limits>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <sys/proc.h>
#include <sys/proc_info.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <unistd.h>

namespace eproc {

namespace {

constexpr Int64 kMicrosecondsPerSecond = 1'000'000;
static_assert( CPU_STATE_MAX == 4 );

template <typename T> bool readSysctl( const char* name, T& value ) {
	size_t size = sizeof( value );
	return sysctlbyname( name, &value, &size, nullptr, 0 ) == 0 && size == sizeof( value );
}

int cpuPercent( Uint64 elapsed, Int64 wallMicroseconds ) {
	if ( wallMicroseconds <= 0 )
		return 0;
	const double percent = static_cast<double>( elapsed ) * 100.0 / wallMicroseconds;
	return static_cast<int>(
		std::min( percent, static_cast<double>( std::numeric_limits<int>::max() ) ) );
}

Uint64 machTicksToMicroseconds( Uint64 ticks ) {
	static const mach_timebase_info_data_t timebase = [] {
		mach_timebase_info_data_t value{};
		mach_timebase_info( &value );
		return value;
	}();
	if ( timebase.denom == 0 )
		return 0;
	return static_cast<Uint64>( static_cast<unsigned __int128>( ticks ) * timebase.numer /
								( static_cast<Uint64>( timebase.denom ) * 1000 ) );
}

} // namespace

bool ProcessCollectorMacOS::collect( std::vector<ProcessInfo>& processes, SystemInfo& sysInfo ) {
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

	int cpuCount = 0;
	if ( readSysctl( "hw.logicalcpu", cpuCount ) && cpuCount > 0 )
		sysInfo.cpuCount = cpuCount;

	uint64_t totalBytes = 0;
	if ( readSysctl( "hw.memsize", totalBytes ) )
		sysInfo.totalMemory = static_cast<Int64>( totalBytes / 1024 );

	const mach_port_t host = mach_host_self();
	vm_size_t pageSize = 0;
	if ( host_page_size( host, &pageSize ) == KERN_SUCCESS ) {
		vm_statistics64_data_t vm{};
		mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
		if ( host_statistics64( host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>( &vm ),
								&count ) == KERN_SUCCESS ) {
			const Uint64 pageKiB = pageSize / 1024;
			sysInfo.freeMemory = static_cast<Int64>( vm.free_count * pageKiB );
			// Approximate reclaimable memory; this is not Linux MemAvailable.
			const Uint64 available = ( static_cast<Uint64>( vm.free_count ) + vm.inactive_count +
									   vm.speculative_count ) *
									 pageKiB;
			sysInfo.availableMemory = static_cast<Int64>(
				sysInfo.totalMemory > 0 ? std::min<Uint64>( sysInfo.totalMemory, available )
										: available );
		}
	}

	host_cpu_load_info_data_t cpu{};
	mach_msg_type_number_t cpuInfoCount = HOST_CPU_LOAD_INFO_COUNT;
	if ( host_statistics( host, HOST_CPU_LOAD_INFO, reinterpret_cast<host_info_t>( &cpu ),
						  &cpuInfoCount ) == KERN_SUCCESS ) {
		Uint64 totalDelta = 0;
		Uint64 idleDelta = 0;
		for ( int state = 0; state < CPU_STATE_MAX; ++state ) {
			const Uint32 ticks = cpu.cpu_ticks[state];
			if ( mHasCpuSample ) {
				// host_cpu_load_info uses 32-bit counters, so subtract modulo 2^32.
				const Uint32 delta = ticks - mPrevCpuTicks[state];
				totalDelta += delta;
				if ( state == CPU_STATE_IDLE )
					idleDelta = delta;
			}
			mPrevCpuTicks[state] = ticks;
		}
		if ( totalDelta > 0 )
			mLastCpuUsage = static_cast<float>(
				100.0 * static_cast<double>( totalDelta - idleDelta ) / totalDelta );
		mHasCpuSample = true;
	}
	sysInfo.cpuUsage = mLastCpuUsage;
	mach_port_deallocate( mach_task_self(), host );

	xsw_usage swap{};
	if ( readSysctl( "vm.swapusage", swap ) ) {
		sysInfo.totalSwap = static_cast<Int64>( swap.xsu_total / 1024 );
		sysInfo.freeSwap = static_cast<Int64>( swap.xsu_avail / 1024 );
	}
	struct timeval bootTime {};
	size_t bootSize = sizeof( bootTime );
	int bootName[] = { CTL_KERN, KERN_BOOTTIME };
	if ( sysctl( bootName, 2, &bootTime, &bootSize, nullptr, 0 ) == 0 ) {
		struct timeval now {};
		gettimeofday( &now, nullptr );
		sysInfo.uptimeSeconds = std::max( 0.0, static_cast<double>( now.tv_sec - bootTime.tv_sec ) +
												   ( now.tv_usec - bootTime.tv_usec ) / 1e6 );
	}

	const int pidCount = proc_listallpids( nullptr, 0 );
	if ( pidCount <= 0 )
		return false;
	std::vector<pid_t> pids( static_cast<size_t>( pidCount ) + 128 );
	const int listed =
		proc_listallpids( pids.data(), static_cast<int>( pids.size() * sizeof( pid_t ) ) );
	if ( listed <= 0 )
		return false;
	processes.reserve( static_cast<size_t>( listed ) );
	const uid_t currentUid = getuid();
	for ( int i = 0; i < listed; ++i ) {
		const pid_t pid = pids[i];
		if ( pid <= 0 )
			continue;
		proc_taskallinfo info{};
		if ( proc_pidinfo( pid, PROC_PIDTASKALLINFO, 0, &info, sizeof( info ) ) != sizeof( info ) )
			continue;

		const auto& bsd = info.pbsd;
		const auto& task = info.ptinfo;
		ProcessInfo proc;
		proc.pid = pid;
		proc.parentPid = bsd.pbi_ppid;
		proc.name = bsd.pbi_name[0] ? bsd.pbi_name : bsd.pbi_comm;
		proc.uid = bsd.pbi_ruid;
		proc.euid = bsd.pbi_uid;
		proc.suid = bsd.pbi_svuid;
		const auto& account = mAccounts.get( static_cast<uid_t>( proc.uid ) );
		proc.username = account.name;
		proc.canLogin = account.canLogin;
		proc.euidCanLogin = mAccounts.get( static_cast<uid_t>( proc.euid ) ).canLogin;
		proc.ownedByCurrentUser = proc.uid == currentUid || proc.euid == currentUid;
		proc.systemProcess = proc.uid == 0 || ( bsd.pbi_flags & PROC_FLAG_SYSTEM ) != 0 ||
							 ( !account.name.empty() && !account.canLogin );
		proc.userProcess = !proc.systemProcess && !account.name.empty();
		switch ( bsd.pbi_status ) {
			case SRUN:
				proc.status = ProcessStatus::Running;
				break;
			case SSLEEP:
				proc.status = ProcessStatus::Sleeping;
				break;
			case SZOMB:
				proc.status = ProcessStatus::Zombie;
				break;
			case SSTOP:
				proc.status = ProcessStatus::Stopped;
				break;
			default:
				break;
		}
		proc.numThreads = task.pti_threadnum;
		proc.niceLevel = bsd.pbi_nice;
		proc.vmSize = static_cast<Int64>( task.pti_virtual_size / 1024 );
		proc.vmRSS = static_cast<Int64>( task.pti_resident_size / 1024 );
		// proc_taskinfo CPU times use Mach absolute ticks, not nanoseconds.
		proc.userTime = static_cast<Int64>( machTicksToMicroseconds( task.pti_total_user ) );
		proc.sysTime = static_cast<Int64>( machTicksToMicroseconds( task.pti_total_system ) );
		const Uint64 startUs = bsd.pbi_start_tvsec * kMicrosecondsPerSecond + bsd.pbi_start_tvusec;
		const Uint64 bootUs =
			static_cast<Uint64>( bootTime.tv_sec ) * kMicrosecondsPerSecond + bootTime.tv_usec;
		proc.startTime =
			bootTime.tv_sec > 0 && startUs >= bootUs ? static_cast<Int64>( startUs - bootUs ) : 0;
		TickEntry& previous = mProcessTicks[pid];
		if ( previous.startTime != startUs || previous.processName != proc.name ) {
			previous.commandPath.clear();
			previous.iconPath.clear();
			previous.pathQueried = false;
			previous.processName = proc.name;
		}
		if ( !previous.pathQueried ) {
			char path[PROC_PIDPATHINFO_MAXSIZE]{};
			if ( proc_pidpath( pid, path, sizeof( path ) ) > 0 ) {
				previous.commandPath = path;
				previous.iconPath = mIconResolver.resolve( previous.commandPath );
			}
			previous.pathQueried = true;
		}
		proc.commandLine = previous.commandPath.empty() ? proc.name : previous.commandPath;
		proc.iconPath = previous.iconPath;

		rusage_info_v2 usage{};
		if ( proc_pid_rusage( pid, RUSAGE_INFO_V2, reinterpret_cast<rusage_info_t*>( &usage ) ) ==
			 0 ) {
			proc.vmURSS = static_cast<Int64>( usage.ri_phys_footprint / 1024 );
			proc.ioReadBytes = static_cast<Int64>( usage.ri_diskio_bytesread );
			proc.ioWriteBytes = static_cast<Int64>( usage.ri_diskio_byteswritten );
		}

		const Uint64 user = static_cast<Uint64>( proc.userTime );
		const Uint64 kernel = static_cast<Uint64>( proc.sysTime );
		if ( previous.pass == mPass - 1 && previous.startTime == startUs &&
			 user >= previous.userTime && kernel >= previous.sysTime ) {
			proc.userUsage = cpuPercent( user - previous.userTime, elapsedUs );
			proc.sysUsage = cpuPercent( kernel - previous.sysTime, elapsedUs );
		}
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
