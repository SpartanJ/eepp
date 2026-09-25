#ifndef EPROC_PROCESS_COLLECTOR_HPP
#define EPROC_PROCESS_COLLECTOR_HPP

#include "process_info.hpp"
#include <memory>
#include <vector>

namespace eproc {

struct SystemInfo {
	Int64 totalMemory{ 0 };
	Int64 freeMemory{ 0 };
	Int64 availableMemory{ 0 };
	Int64 totalSwap{ 0 };
	Int64 freeSwap{ 0 };
	int cpuCount{ 1 };
	float cpuUsage{ 0.f };
	// Per-logical-CPU usage, indexed by CPU number when the platform provides it.
	std::vector<float> coreCpuUsage;
	// Number of units in ProcessInfo::{userTime,sysTime,startTime} per second.
	Int64 clockTicksPerSecond{ 100 };
	double uptimeSeconds{ 0.0 };

	Int64 getTotalMemoryKB() const { return totalMemory; }
	Int64 getUsedMemoryKB() const { return totalMemory - availableMemory; }
	Int64 getUsedSwapKB() const { return totalSwap - freeSwap; }
};

class ProcessCollector {
  public:
	virtual ~ProcessCollector() = default;

	/** Collects every process and the system-wide counters into the given buffers.
	 *  Not thread-safe: one instance must never be used concurrently, because per-process CPU
	 *  usage is derived from the delta against the previous sample held by the instance.
	 *  @return true on success. */
	virtual bool collect( std::vector<ProcessInfo>& processes, SystemInfo& sysInfo ) = 0;

	/** Enables the optional, more expensive proportional-memory reading where available. */
	virtual void setCollectProportionalMemory( bool ) {}

	virtual bool supportsProgramsOnly() const { return false; }

	/** Returns the collector for the running OS, or nullptr when the platform is unsupported. */
	static std::unique_ptr<ProcessCollector> create();

  protected:
	ProcessCollector() = default;
};

/** Sends a process signal (e.g. SIGTERM 15, SIGKILL 9) to @p pid. Returns true on success. */
bool sendProcessSignal( Int64 pid, int signal );

/** Sends SIGKILL to @p pid. Returns true on success. */
bool killProcess( Int64 pid );

} // namespace eproc

#endif // EPROC_PROCESS_COLLECTOR_HPP
