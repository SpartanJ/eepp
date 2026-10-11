#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX
#include "../../tools/eproc/platform/linux/process_collector_linux.hpp"
#include "utest.hpp"
#include <unistd.h>

namespace eproc {

struct ProcessCollectorLinuxTestAccess {
	static void sample( ProcessCollectorLinux& collector, ProcessInfo& proc, Uint32 pass,
						bool enabled ) {
		collector.mPass = pass;
		collector.setCollectProportionalMemory( enabled );
		collector.collectProcessPss( proc );
	}

	static void seed( ProcessCollectorLinux& collector, const ProcessInfo& proc, Int64 value,
					  Uint32 pass ) {
		collector.mProcessPss[proc.pid] = { value, proc.startTime, pass };
	}

	static bool cached( const ProcessCollectorLinux& collector, Int64 pid ) {
		return collector.mProcessPss.find( pid ) != collector.mProcessPss.end();
	}

	static void prune( ProcessCollectorLinux& collector ) { collector.pruneCaches(); }
};

} // namespace eproc

using namespace eproc;
using Access = ProcessCollectorLinuxTestAccess;

UTEST( EProcLinuxCollector, HiddenPassesPreserveOnlyLiveProcessIncarnations ) {
	ProcessCollectorLinux collector;
	ProcessInfo proc;
	proc.pid = 1000000000; // Outside Linux's PID range; an accidental procfs read must fail.
	proc.startTime = 42;
	proc.vmRSS = 100;
	EXPECT_FALSE( Access::cached( collector, proc.pid ) );
	Access::seed( collector, proc, 123, 1 );
	for ( Uint32 pass = 2; pass <= 60; ++pass )
		Access::sample( collector, proc, pass, false );
	Access::prune( collector );
	ASSERT_TRUE( Access::cached( collector, proc.pid ) );
	Access::sample( collector, proc, 61, true ); // Different refresh bucket from PID.
	EXPECT_EQ( proc.vmPSS, 123 );
	++proc.startTime;
	Access::sample( collector, proc, 62, false );
	EXPECT_FALSE( Access::cached( collector, proc.pid ) );
	Access::sample( collector, proc, 63, true );
	EXPECT_EQ( proc.vmPSS, -1 );
	Access::seed( collector, proc, 123, 63 );
	ProcessInfo other;
	other.pid = proc.pid + 1;
	other.startTime = 43;
	other.vmRSS = 100;
	Access::sample( collector, other, 120, false );
	Access::prune( collector );
	EXPECT_FALSE( Access::cached( collector, proc.pid ) );
}

UTEST( EProcLinuxCollector, ZeroRssAvoidsProcfsAndBucketRefreshStillRuns ) {
	ProcessCollectorLinux collector;
	ProcessInfo proc;
	proc.pid = 1000000000;
	proc.startTime = 42;
	proc.vmRSS = 0;
	Access::sample( collector, proc, 1, true );
	EXPECT_EQ( proc.vmPSS, 0 );
	EXPECT_TRUE( Access::cached( collector, proc.pid ) );
	proc.vmRSS = 100;
	Access::seed( collector, proc, 123, 1 );
	Access::sample( collector, proc, 2, true );
	EXPECT_EQ( proc.vmPSS, 123 );
	Access::sample( collector, proc, 5, true );
	EXPECT_EQ( proc.vmPSS, -1 ); // Scheduled refresh fails for this nonexistent PID.
}

UTEST( EProcLinuxCollector, FirstEnabledSnapshotIncludesCurrentProcessPss ) {
	ProcessCollectorLinux collector;
	collector.setCollectProportionalMemory( true );
	std::vector<ProcessInfo> processes;
	SystemInfo system;
	ASSERT_TRUE( collector.collectInitial( processes, system ) );
	const auto current =
		std::find_if( processes.begin(), processes.end(),
					  []( const ProcessInfo& proc ) { return proc.pid == getpid(); } );
	ASSERT_TRUE( current != processes.end() );
	EXPECT_TRUE( current->vmPSS > 0 );
	EXPECT_TRUE( current->startTime > 0 );
	ASSERT_TRUE( Access::cached( collector, current->pid ) );
	ProcessInfo cached;
	cached.pid = current->pid;
	cached.startTime = current->startTime;
	cached.vmRSS = current->vmRSS;
	// Stay outside this PID's refresh bucket: a miss would read procfs again.
	const Uint32 pass = static_cast<Uint32>( current->pid ) % 5 == 2 ? 3 : 2;
	Access::sample( collector, cached, pass, true );
	EXPECT_EQ( cached.vmPSS, current->vmPSS );
}
UTEST( EProcLinuxCollector, PublishesBaseBeforeReturningCompletedMemorySamples ) {
	ProcessCollectorLinux collector;
	collector.setCollectProportionalMemory( true );
	std::vector<ProcessInfo> processes, base;
	std::vector<ProportionalMemorySample> memory;
	SystemInfo system, baseSystem;
	bool published = false;
	ASSERT_TRUE( collector.collectInitial(
		processes, system,
		[&]( std::vector<ProcessInfo>&& snapshot, SystemInfo&& info ) {
			published = true;
			base = std::move( snapshot );
			baseSystem = std::move( info );
		},
		&memory ) );
	ASSERT_TRUE( published );
	EXPECT_TRUE( processes.empty() );
	EXPECT_TRUE( baseSystem.totalMemory > 0 );
	const auto current = std::find_if(
		base.begin(), base.end(), []( const ProcessInfo& proc ) { return proc.pid == getpid(); } );
	ASSERT_TRUE( current != base.end() );
	EXPECT_EQ( current->vmPSS, -1 );
	ASSERT_TRUE(
		std::is_sorted( memory.begin(), memory.end(),
						[]( const auto& lhs, const auto& rhs ) { return lhs.pid < rhs.pid; } ) );
	const auto sample = std::find_if( memory.begin(), memory.end(),
									  []( const auto& sample ) { return sample.pid == getpid(); } );
	ASSERT_TRUE( sample != memory.end() );
	EXPECT_EQ( sample->startTime, current->startTime );
	EXPECT_TRUE( sample->valueKB > 0 );
	EXPECT_TRUE( Access::cached( collector, current->pid ) );
}

UTEST( EProcLinuxCollector, HiddenStartupDoesNotPublishPartialSnapshotOrCollectPss ) {
	ProcessCollectorLinux collector;
	std::vector<ProcessInfo> processes;
	std::vector<ProportionalMemorySample> memory;
	SystemInfo system;
	bool published = false;
	ASSERT_TRUE( collector.collectInitial(
		processes, system, [&]( std::vector<ProcessInfo>&&, SystemInfo&& ) { published = true; },
		&memory ) );
	EXPECT_FALSE( published );
	EXPECT_TRUE( memory.empty() );
	const auto current =
		std::find_if( processes.begin(), processes.end(),
					  []( const ProcessInfo& proc ) { return proc.pid == getpid(); } );
	ASSERT_TRUE( current != processes.end() );
	EXPECT_EQ( current->vmPSS, -1 );
	EXPECT_FALSE( Access::cached( collector, current->pid ) );
}

#endif
