#include "process_network_monitor_windows.hpp"

#include <cstddef>
#include <cstring>
#include <cwchar>
#include <eepp/system/log.hpp>
#include <iterator>
#include <limits>

namespace eproc {

namespace {

// Microsoft-Windows-Kernel-Network, IPv4/IPv6 TCP and UDP send/receive events.
constexpr GUID kNetworkProvider = {
	0x7dd42a49, 0x5329, 0x4832, { 0x8d, 0xfd, 0x43, 0xd9, 0x79, 0x15, 0x3a, 0x88 } };
constexpr int kRetrySeconds = 30;

} // namespace

ProcessNetworkMonitorWindows::~ProcessNetworkMonitorWindows() {
	stop();
}

void ProcessNetworkMonitorWindows::start() {
	mInitialized = true;
	mProperties = {};
	mTraceResult = ERROR_SUCCESS;
	mLostEvents = 0;
	mValidSample = false;
	mPreviousSample = {};
	mStatus = ProcessNetworkStatus::Starting;
	auto& properties = mProperties.trace;
	properties.Wnode.BufferSize = sizeof( mProperties );
	properties.Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	// System FILETIME timestamps let apply() compare events with GetProcessTimes directly.
	properties.Wnode.ClientContext = 2;
	properties.LogFileMode = EVENT_TRACE_REAL_TIME_MODE | EVENT_TRACE_NO_PER_PROCESSOR_BUFFERING;
	properties.BufferSize = 64;
	properties.MinimumBuffers = 2;
	properties.MaximumBuffers = 64;
	properties.FlushTimer = 1;
	properties.LoggerNameOffset = offsetof( Properties, name );
	swprintf( mProperties.name, std::size( mProperties.name ), L"eproc-network-%lu-%p",
			  GetCurrentProcessId(), static_cast<void*>( this ) );
	ULONG status = StartTraceW( &mSession, mProperties.name, &properties );
	if ( status != ERROR_SUCCESS ) {
		mSession = 0;
		fail( status, "starting the network trace" );
		return;
	}
	status = EnableTraceEx2( mSession, &kNetworkProvider, EVENT_CONTROL_CODE_ENABLE_PROVIDER,
							 TRACE_LEVEL_INFORMATION, 0x30, 0, 0, nullptr );
	if ( status != ERROR_SUCCESS ) {
		fail( status, "enabling the kernel-network provider" );
		stop();
		return;
	}
	EVENT_TRACE_LOGFILEW log{};
	log.LoggerName = mProperties.name;
	log.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
	log.EventRecordCallback = &ProcessNetworkMonitorWindows::onEvent;
	log.Context = this;
	mTrace = OpenTraceW( &log );
	if ( mTrace == INVALID_PROCESSTRACE_HANDLE ) {
		fail( GetLastError(), "opening the network trace" );
		stop();
		return;
	}
	mError = ERROR_SUCCESS;
	mRunning = true;
	mReader = std::thread( [this] {
		mTraceResult = ProcessTrace( &mTrace, 1, nullptr, nullptr );
		mRunning = false;
	} );
}

void ProcessNetworkMonitorWindows::fail( ULONG error, const char* operation ) {
	// Do not turn lack of permission into an elevation requirement: Performance Log Users and
	// provider ACLs can grant tracing access to an ordinary desktop token.
	if ( error != mError ) {
		Log::warning( "eproc: %s failed (Windows error %lu); per-process network data is "
					  "unavailable. Collection will retry automatically.",
					  operation, error );
	}
	mError = error;
	mStatus = error == ERROR_ACCESS_DENIED || error == ERROR_PRIVILEGE_NOT_HELD
				  ? ProcessNetworkStatus::AccessDenied
				  : ProcessNetworkStatus::Unavailable;
	mValidSample = false;
	mRetryAt = MonotonicClock::now() + Seconds( kRetrySeconds );
}

void ProcessNetworkMonitorWindows::stop() {
	if ( mSession ) {
		ControlTraceW( 0, mProperties.name, &mProperties.trace, EVENT_TRACE_CONTROL_STOP );
		mSession = 0;
	}
	if ( mTrace != INVALID_PROCESSTRACE_HANDLE ) {
		CloseTrace( mTrace );
		if ( mReader.joinable() )
			mReader.join();
		mTrace = INVALID_PROCESSTRACE_HANDLE;
	}
	mRunning = false;
}

bool ProcessNetworkMonitorWindows::parse( const EVENT_RECORD& event, DWORD& pid, DWORD& bytes,
										  bool& received ) {
	if ( !IsEqualGUID( event.EventHeader.ProviderId, kNetworkProvider ) ||
		 event.EventHeader.EventDescriptor.Version != 0 || !event.UserData ||
		 event.UserDataLength < 2 * sizeof( DWORD ) )
		return false;
	switch ( event.EventHeader.EventDescriptor.Id ) {
		case 10: // TCP IPv4 send
		case 26: // TCP IPv6 send
		case 42: // UDP IPv4 send
		case 58: // UDP IPv6 send
			received = false;
			break;
		case 11:
		case 27:
		case 43:
		case 59:
			received = true;
			break;
		default:
			return false;
	}
	// The event payload identifies the process that owns the socket. EventHeader.ProcessId
	// can name a kernel worker instead, particularly for receive events. The version-0 manifest
	// starts with two UInt32s for every supported event, independent of pointer size/protocol.
	DWORD fields[2];
	std::memcpy( fields, event.UserData, sizeof( fields ) );
	pid = fields[0];
	bytes = fields[1];
	return pid != 0;
}

void WINAPI ProcessNetworkMonitorWindows::onEvent( EVENT_RECORD* event ) {
	DWORD pid = 0, bytes = 0;
	bool received = false;
	if ( !event || !event->UserContext || !parse( *event, pid, bytes, received ) )
		return;
	auto* monitor = static_cast<ProcessNetworkMonitorWindows*>( event->UserContext );
	const Uint64 timestamp = static_cast<Uint64>( event->EventHeader.TimeStamp.QuadPart );
	std::lock_guard<std::mutex> lock( monitor->mMutex );
	auto& counters = monitor->mPending[pid];
	if ( counters.firstEvent == 0 || timestamp < counters.firstEvent )
		counters.firstEvent = timestamp;
	if ( received )
		counters.received += bytes;
	else
		counters.sent += bytes;
}

void ProcessNetworkMonitorWindows::sample() {
	const auto now = MonotonicClock::now();
	if ( mInitialized && !mRunning &&
		 ( mStatus == ProcessNetworkStatus::Starting ||
		   mStatus == ProcessNetworkStatus::Available ||
		   mStatus == ProcessNetworkStatus::DataLost ) ) {
		const ULONG result = mTraceResult;
		fail( result == ERROR_SUCCESS ? ERROR_WMI_INSTANCE_NOT_FOUND : result,
			  "consuming the network trace" );
		stop();
	}
	if ( !mInitialized || ( !mRunning && now >= mRetryAt ) ) {
		// A finished consumer can still own a session and a joinable thread. Release both
		// before restarting; only this collector's session is ever controlled.
		stop();
		mPending.clear();
		mSample.clear();
		start();
	}
	mElapsedUs = mPreviousSample == MonotonicClock::TimePoint{}
					 ? 0
					 : ( now - mPreviousSample ).asMicroseconds();
	mPreviousSample = now;
	mSample.clear();
	{
		std::lock_guard<std::mutex> lock( mMutex );
		mPending.swap( mSample );
	}
	mValidSample = mRunning && mElapsedUs > 0;
	if ( mValidSample ) {
		Properties status{};
		status.trace.Wnode.BufferSize = sizeof( status );
		status.trace.LoggerNameOffset = offsetof( Properties, name );
		const ULONG result =
			ControlTraceW( 0, mProperties.name, &status.trace, EVENT_TRACE_CONTROL_QUERY );
		if ( result != ERROR_SUCCESS ) {
			fail( result, "querying the network trace" );
			stop();
		} else {
			const ULONG lost = status.trace.EventsLost + status.trace.RealTimeBuffersLost;
			mValidSample = lost == mLostEvents;
			mLostEvents = lost;
			mStatus =
				mValidSample ? ProcessNetworkStatus::Available : ProcessNetworkStatus::DataLost;
		}
	}
}

void ProcessNetworkMonitorWindows::apply( ProcessInfo& process, Uint64 creationTime ) const {
	process.netDownload = process.netUpload = -1;
	if ( !mValidSample || creationTime == 0 )
		return;
	auto it = mSample.find( process.pid );
	if ( it == mSample.end() ) {
		process.netDownload = 0;
		process.netUpload = 0;
		return;
	}
	// Drop a mixed window if a PID was reused, including delayed events delivered by ETW.
	if ( it->second.firstEvent < creationTime )
		return;
	const auto rate = [this]( Uint64 bytes ) {
		const double value = static_cast<double>( bytes ) * 1'000'000.0 / mElapsedUs;
		return value < static_cast<double>( std::numeric_limits<Int64>::max() )
				   ? static_cast<Int64>( value )
				   : std::numeric_limits<Int64>::max();
	};
	process.netDownload = rate( it->second.received );
	process.netUpload = rate( it->second.sent );
}

} // namespace eproc
