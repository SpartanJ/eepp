#ifndef EPROC_PROCESS_COLLECTOR_FREEBSD_HPP
#define EPROC_PROCESS_COLLECTOR_FREEBSD_HPP

#include "../../process_collector.hpp"
#include "../posix/process_account.hpp"
#include "../posix/process_icon_resolver.hpp"
#include <chrono>
#include <unordered_map>

namespace eproc {

class ProcessCollectorFreeBSD final : public ProcessCollector {
  public:
	bool collect( std::vector<ProcessInfo>& processes, SystemInfo& sysInfo ) override;

  private:
	struct TickEntry {
		std::string iconPath;
		Uint64 startTime{ 0 };
		Uint64 userTime{ 0 };
		Uint64 sysTime{ 0 };
		Uint32 pass{ 0 };
	};

	std::unordered_map<Int64, TickEntry> mProcessTicks;
	ProcessAccounts mAccounts;
	ProcessIconResolver mIconResolver;
	std::chrono::steady_clock::time_point mPrevSampleTime;
	Uint64 mPrevTotalCpu{ 0 };
	Uint64 mPrevIdleCpu{ 0 };
	float mLastCpuUsage{ 0.f };
	Uint32 mPass{ 0 };
};

} // namespace eproc

#endif // EPROC_PROCESS_COLLECTOR_FREEBSD_HPP
