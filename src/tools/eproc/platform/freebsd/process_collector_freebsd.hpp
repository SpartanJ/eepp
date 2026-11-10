#ifndef EPROC_PROCESS_COLLECTOR_FREEBSD_HPP
#define EPROC_PROCESS_COLLECTOR_FREEBSD_HPP

#include "../../process_collector.hpp"
#include "../posix/process_account.hpp"
#include "../posix/process_icon_resolver.hpp"
#include <eepp/system/monotonicclock.hpp>
#include <unordered_map>
#include <vector>

namespace eproc {

using namespace EE::System;

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
	MonotonicClock::TimePoint mPrevSampleTime;
	std::vector<long> mCoreCpuTicks;
	std::vector<long> mPrevCoreCpuTicks;
	std::vector<float> mLastCoreCpuUsage;
	Uint64 mPrevTotalCpu{ 0 };
	Uint64 mPrevIdleCpu{ 0 };
	float mLastCpuUsage{ 0.f };
	Uint32 mPass{ 0 };
};

} // namespace eproc

#endif // EPROC_PROCESS_COLLECTOR_FREEBSD_HPP
