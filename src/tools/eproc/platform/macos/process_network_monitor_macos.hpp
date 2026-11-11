#ifndef EPROC_PROCESS_NETWORK_MONITOR_MACOS_HPP
#define EPROC_PROCESS_NETWORK_MONITOR_MACOS_HPP

#include <eepp/core/core.hpp>
#include <eepp/system/monotonicclock.hpp>
#include <mutex>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../../process_info.hpp"

namespace eproc {

using namespace EE::System;

/** Reads nettop's unprivileged per-process byte counters in a background thread. */
class ProcessNetworkMonitorMacOS {
  public:
	ProcessNetworkMonitorMacOS();
	~ProcessNetworkMonitorMacOS();
	ProcessNetworkMonitorMacOS( const ProcessNetworkMonitorMacOS& ) = delete;
	ProcessNetworkMonitorMacOS& operator=( const ProcessNetworkMonitorMacOS& ) = delete;

	void applyRates( std::vector<ProcessInfo>& processes ) const;
	static bool parseRow( std::string_view line, Int64& pid, Uint64& received, Uint64& sent );

  private:
	struct Counters {
		Uint64 received{ 0 };
		Uint64 sent{ 0 };
	};
	struct Rates {
		Int64 received{ 0 };
		Int64 sent{ 0 };
	};

	void readLoop();
	void publish( std::unordered_map<Int64, Counters>& current,
				  MonotonicClock::TimePoint sampledAt );

	mutable std::mutex mMutex;
	std::unordered_map<Int64, Counters> mPrevious;
	std::unordered_map<Int64, Rates> mRates;
	std::unordered_map<Int64, Rates> mNextRates;
	std::thread mReader;
	MonotonicClock::TimePoint mPreviousSample;
	int mMaster{ -1 };
	int mChildPid{ -1 };
};

} // namespace eproc

#endif // EPROC_PROCESS_NETWORK_MONITOR_MACOS_HPP
