#ifndef EPROC_PROCESS_NETWORK_MONITOR_WINDOWS_HPP
#define EPROC_PROCESS_NETWORK_MONITOR_WINDOWS_HPP

#include "../../process_collector.hpp"
#include <eepp/core/containers.hpp>
#include <eepp/system/monotonicclock.hpp>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <evntcons.h>
#include <evntrace.h>

#include <atomic>
#include <mutex>
#include <thread>

namespace eproc {

using namespace EE::System;

/** Owns a private real-time kernel-network ETW session. Access denied leaves rates unavailable;
 * it never attaches to, reconfigures, or stops a session owned by another application. */
class ProcessNetworkMonitorWindows {
  public:
	ProcessNetworkMonitorWindows() = default;

	~ProcessNetworkMonitorWindows();

	ProcessNetworkMonitorWindows( const ProcessNetworkMonitorWindows& ) = delete;

	ProcessNetworkMonitorWindows& operator=( const ProcessNetworkMonitorWindows& ) = delete;

	/** Starts lazily on the collection worker, then exchanges the completed accounting window. */
	void sample();

	/** creationTime is the process's FILETIME, used to reject events for a recycled PID. */
	void apply( ProcessInfo& process, Uint64 creationTime ) const;

	ProcessNetworkStatus status() const { return mStatus; }

	Uint32 error() const { return mError; }

  private:
	friend struct ProcessNetworkMonitorWindowsTestAccess;

	struct Counters {
		Uint64 received{ 0 };
		Uint64 sent{ 0 };
		Uint64 firstEvent{ 0 };
	};

	struct Properties {
		EVENT_TRACE_PROPERTIES trace{};
		WCHAR name[96]{};
	};

	void start();

	void stop();

	void fail( ULONG error, const char* operation );

	static void WINAPI onEvent( EVENT_RECORD* event );

	static bool parse( const EVENT_RECORD& event, DWORD& pid, DWORD& bytes, bool& received );

	Properties mProperties;
	TRACEHANDLE mSession{ 0 };
	TRACEHANDLE mTrace{ INVALID_PROCESSTRACE_HANDLE };
	std::thread mReader;
	std::mutex mMutex;
	UnorderedMap<Int64, Counters> mPending;
	UnorderedMap<Int64, Counters> mSample;
	MonotonicClock::TimePoint mPreviousSample;
	MonotonicClock::TimePoint mRetryAt;
	Int64 mElapsedUs{ 0 };
	ULONG mLostEvents{ 0 };
	ULONG mError{ ERROR_SUCCESS };
	std::atomic<ULONG> mTraceResult{ ERROR_SUCCESS };
	std::atomic<bool> mRunning{ false };
	ProcessNetworkStatus mStatus{ ProcessNetworkStatus::NotReported };
	bool mInitialized{ false };
	bool mValidSample{ false };
};

} // namespace eproc

#endif
