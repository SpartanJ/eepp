#ifndef EPROC_PROCESS_COLLECTOR_WINDOWS_HPP
#define EPROC_PROCESS_COLLECTOR_WINDOWS_HPP

#include "../../process_collector.hpp"
#include "performance_counters_windows.hpp"
#include "process_network_monitor_windows.hpp"
#include <eepp/core/containers.hpp>
#include <eepp/system/monotonicclock.hpp>
#include <unordered_map>

namespace eproc {

using namespace EE::System;

class ProcessCollectorWindows final : public ProcessCollector {
  public:
	ProcessCollectorWindows();

	bool collect( std::vector<ProcessInfo>& processes, SystemInfo& sysInfo ) override;

	bool supportsProgramsOnly() const override { return true; }

  private:
	struct TickEntry {
		Uint64 userTime{ 0 };
		Uint64 kernelTime{ 0 };
		Uint64 creationTime{ 0 };
		Int64 startTime{ 0 };
		Uint32 pass{ 0 };
	};

	struct NetworkCounter {
		Uint64 received{ 0 };
		Uint64 sent{ 0 };
		Uint32 pass{ 0 };
	};
	static_assert( sizeof( NetworkCounter ) == 24 );

	std::unordered_map<Int64, TickEntry> mProcessTicks;
	std::unordered_map<std::string, std::string> mAccountNames;
	std::vector<Uint64> mCurrentUserToken;
	UnorderedMap<Uint64, NetworkCounter> mNetworkCounters;
	PerformanceCountersWindows mPerformance;
	ProcessNetworkMonitorWindows mNetworkMonitor;
	Uint64 mPrevSystemIdle{ 0 };
	Uint64 mPrevSystemKernel{ 0 };
	Uint64 mPrevSystemUser{ 0 };
	MonotonicClock::TimePoint mPrevSampleTime;
	float mLastCpuUsage{ 0.f };
	Uint32 mPass{ 0 };

	void readIdentity( void* processHandle, ProcessInfo& process );

	void readNetwork( SystemInfo& system, Int64 elapsedMicroseconds );
};

} // namespace eproc

#endif // EPROC_PROCESS_COLLECTOR_WINDOWS_HPP
