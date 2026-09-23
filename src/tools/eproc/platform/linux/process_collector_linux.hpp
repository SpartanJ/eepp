#ifndef EPROC_PROCESS_COLLECTOR_LINUX_HPP
#define EPROC_PROCESS_COLLECTOR_LINUX_HPP

#include "../../process_collector.hpp"
#include "gpu_reader_drm.hpp"
#include "gpu_reader_nvidia.hpp"
#include "process_icon_resolver.hpp"
#include "process_network_monitor.hpp"

namespace eproc {

class ProcessCollectorLinux : public ProcessCollector {
  public:
	ProcessCollectorLinux();
	~ProcessCollectorLinux() override;

	bool collect( std::vector<ProcessInfo>& processes, SystemInfo& sysInfo ) override;

  private:
	// Cached per-uid identity, since passwd lookups can hit NSS.
	struct UserInfo {
		std::string name;
		bool canLogin{ false };
	};

	// Per-pid values tagged with the pass they were last seen in, so exited processes can be
	// pruned without rebuilding (and reallocating every node of) the map on each pass. The start
	// time pins an entry to one process incarnation, because a recycled pid would otherwise
	// inherit the figures of the process that died.
	struct TickEntry {
		long long ticks{ 0 };
		long long startTime{ 0 };
		Uint32 pass{ 0 };
	};

	struct IconEntry {
		std::string path;
		long long startTime{ 0 };
		Uint32 pass{ 0 };
	};

	// KiB per memory page (e.g. 4 for a 4096-byte page size)
	long mPageSizeKb{ 4 };
	int mProcessorCount{ 1 };
	// Kernel clock ticks per second, used to size the minimum meaningful sampling window.
	long mJiffiesPerSecond{ 100 };

	// Previous CPU totals for delta calculation
	long long mPrevTotalCpu{ 0 };
	long long mPrevIdleCpu{ 0 };
	// Machine-wide jiffies that must elapse before a CPU delta is trusted (~200ms across cores).
	long long mMinSampleJiffies{ 0 };
	float mLastCpuUsage{ 0.f };

	UnorderedMap<long, TickEntry> mProcessTicks;
	UnorderedMap<long, IconEntry> mProcessIcons;
	Uint32 mPass{ 0 };

	// Reused between passes so the GPU queries do not allocate a fresh map every second.
	UnorderedMap<long, int> mGpuUsage;
	UnorderedMap<long, long> mGpuMemory;

	bool readCpuTimes( long long& idle, long long& total );
	bool readProcessStat( ProcessInfo& proc, const char* statLine );
	void readProcessStatus( ProcessInfo& proc, const char* content, size_t length );
	void readProcessCmdline( ProcessInfo& proc, long pid );
	void readProcessIO( ProcessInfo& proc, long pid );
	void readSystemMemory( SystemInfo& sysInfo );
	void readSystemUptime( SystemInfo& sysInfo );
	void resolveUser( ProcessInfo& proc );
	void pruneCaches();

	/** Cached passwd lookup for a uid (name + login-shell capability). */
	const UserInfo& userInfo( long uid );

	UnorderedMap<long, UserInfo> mUserCache;

	// Both run on the collection worker. Icons are resolved through the XDG desktop index (cached
	// internally), and GPU figures come from NVML when an NVIDIA driver is present.
	ProcessIconResolver mIconResolver;
	NvidiaGpuReader mGpuReader;
	DrmGpuReader mDrmGpuReader;
	ProcessNetworkMonitor mNetworkMonitor;
};

} // namespace eproc

#endif // EPROC_PROCESS_COLLECTOR_LINUX_HPP
