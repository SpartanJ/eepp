#ifndef EPROC_PROCESS_COLLECTOR_WINDOWS_HPP
#define EPROC_PROCESS_COLLECTOR_WINDOWS_HPP

#include "../../process_collector.hpp"
#include <chrono>
#include <unordered_map>

namespace eproc {

class ProcessCollectorWindows final : public ProcessCollector {
  public:
	ProcessCollectorWindows();

	bool collect( std::vector<ProcessInfo>& processes, SystemInfo& sysInfo ) override;

  private:
	struct TickEntry {
		Uint64 userTime{ 0 };
		Uint64 kernelTime{ 0 };
		Uint64 creationTime{ 0 };
		Int64 startTime{ 0 };
		Uint32 pass{ 0 };
	};

	std::unordered_map<Int64, TickEntry> mProcessTicks;
	std::unordered_map<std::string, std::string> mAccountNames;
	std::vector<Uint64> mCurrentUserToken;
	Uint64 mPrevSystemIdle{ 0 };
	Uint64 mPrevSystemKernel{ 0 };
	Uint64 mPrevSystemUser{ 0 };
	std::chrono::steady_clock::time_point mPrevSampleTime;
	float mLastCpuUsage{ 0.f };
	Uint32 mPass{ 0 };

	void readIdentity( void* processHandle, ProcessInfo& process );
};

} // namespace eproc

#endif // EPROC_PROCESS_COLLECTOR_WINDOWS_HPP
