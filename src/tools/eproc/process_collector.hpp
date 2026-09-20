#ifndef EPROC_PROCESS_COLLECTOR_HPP
#define EPROC_PROCESS_COLLECTOR_HPP

#include "process_info.hpp"
#include <memory>
#include <vector>

namespace eproc {

struct SystemInfo {
	long totalMemory{ 0 };
	long freeMemory{ 0 };
	long availableMemory{ 0 };
	long totalSwap{ 0 };
	long freeSwap{ 0 };
	int cpuCount{ 1 };
	float cpuUsage{ 0.f };
	long clockTicksPerSecond{ 100 };
	double uptimeSeconds{ 0.0 };

	long getTotalMemoryKB() const { return totalMemory; }
	long getUsedMemoryKB() const { return totalMemory - availableMemory; }
	long getUsedSwapKB() const { return totalSwap - freeSwap; }
};

class ProcessCollector {
  public:
	virtual ~ProcessCollector() = default;

	/** Collects every process and the system-wide counters into the given buffers.
	 *  Not thread-safe: one instance must never be used concurrently, because per-process CPU
	 *  usage is derived from the delta against the previous sample held by the instance.
	 *  @return true on success. */
	virtual bool collect( std::vector<ProcessInfo>& processes, SystemInfo& sysInfo ) = 0;

	/** Returns the collector for the running OS, or nullptr when the platform is unsupported. */
	static std::unique_ptr<ProcessCollector> create();

  protected:
	ProcessCollector() = default;
};

/** Sends a process signal (e.g. SIGTERM 15, SIGKILL 9) to @p pid. Returns true on success. */
bool sendProcessSignal( long pid, int signal );

/** Sends SIGKILL to @p pid. Returns true on success. */
bool killProcess( long pid );

} // namespace eproc

#endif // EPROC_PROCESS_COLLECTOR_HPP
