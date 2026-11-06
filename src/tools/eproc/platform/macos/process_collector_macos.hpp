#ifndef EPROC_PROCESS_COLLECTOR_MACOS_HPP
#define EPROC_PROCESS_COLLECTOR_MACOS_HPP

#include "../../process_collector.hpp"
#include "../posix/process_account.hpp"
#include "process_icon_resolver_macos.hpp"
#include "process_network_monitor_macos.hpp"
#include <array>
#include <chrono>
#include <unordered_map>
#include <vector>

namespace eproc {

class ProcessCollectorMacOS final : public ProcessCollector {
  public:
	bool collect( std::vector<ProcessInfo>& processes, SystemInfo& sysInfo ) override;
	bool supportsProgramsOnly() const override { return true; }

  private:
	void collectGpu( SystemInfo& sysInfo );

	struct TickEntry {
		Uint64 startTime{ 0 };
		Uint64 userTime{ 0 };
		Uint64 sysTime{ 0 };
		Uint32 pass{ 0 };
		bool pathQueried{ false };
		std::string processName;
		std::string commandPath;
		std::string iconPath;
	};
	struct NetworkCounter {
		Uint32 received{ 0 };
		Uint32 sent{ 0 };
		Uint32 pass{ 0 };
	};

	std::unordered_map<Int64, TickEntry> mProcessTicks;
	std::unordered_map<std::string, NetworkCounter> mNetworkCounters;
	ProcessAccounts mAccounts;
	ProcessIconResolverMacOS mIconResolver;
	ProcessNetworkMonitorMacOS mNetworkMonitor;
	std::chrono::steady_clock::time_point mPrevSampleTime;
	std::array<Uint32, 4> mPrevCpuTicks{};
	std::vector<std::array<Uint32, 4>> mPrevCoreCpuTicks;
	std::vector<float> mLastCoreCpuUsage;
	float mLastCpuUsage{ 0.f };
	Uint32 mPass{ 0 };
	bool mHasCpuSample{ false };
};

} // namespace eproc

#endif // EPROC_PROCESS_COLLECTOR_MACOS_HPP
