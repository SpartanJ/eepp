#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#include <winsock2.h>

#include "../../tools/eproc/platform/windows/performance_counters_windows.hpp"
#include "../../tools/eproc/platform/windows/process_collector_windows.hpp"
#include "utest.hpp"
#include <eepp/system/sys.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>

using namespace EE::System;

namespace eproc {

struct PerformanceCountersWindowsTestAccess {
	static bool process( std::wstring_view name, Int64& pid, std::wstring_view& engine ) {
		return PerformanceCountersWindows::processInstance( name, pid, engine );
	}

	static bool core( std::wstring_view name, unsigned& index ) {
		return PerformanceCountersWindows::coreInstance( name, index );
	}

	static void checkLayout() {
		static_assert( sizeof( PerformanceCountersWindows::GpuSample ) == 16 );
		static_assert( sizeof( PerformanceCountersWindows::CoreSample ) == 8 );
	}
};

struct ProcessNetworkMonitorWindowsTestAccess {
	static bool parse( const EVENT_RECORD& event, DWORD& pid, DWORD& bytes, bool& received ) {
		return ProcessNetworkMonitorWindows::parse( event, pid, bytes, received );
	}

	static void publish( ProcessNetworkMonitorWindows& monitor, Uint64 timestamp ) {
		monitor.mValidSample = true;
		monitor.mElapsedUs = 2'000'000;
		monitor.mSample[42] = { 4000, 2000, timestamp };
	}

	static void invalidate( ProcessNetworkMonitorWindows& monitor ) {
		monitor.mValidSample = false;
	}

	static bool running( const ProcessNetworkMonitorWindows& monitor ) { return monitor.mRunning; }

	static void retryNow( ProcessNetworkMonitorWindows& monitor ) { monitor.mRetryAt = {}; }

	static bool interruptSession( ProcessNetworkMonitorWindows& monitor ) {
		return ControlTraceW( 0, monitor.mProperties.name, &monitor.mProperties.trace,
							  EVENT_TRACE_CONTROL_STOP ) == ERROR_SUCCESS;
	}

	static bool stopsSession( ProcessNetworkMonitorWindows& monitor ) {
		const std::wstring name( monitor.mProperties.name );
		monitor.stop();
		ProcessNetworkMonitorWindows::Properties properties{};
		properties.trace.Wnode.BufferSize = sizeof( properties );
		properties.trace.LoggerNameOffset =
			offsetof( ProcessNetworkMonitorWindows::Properties, name );
		return !monitor.mReader.joinable() &&
			   ControlTraceW( 0, name.c_str(), &properties.trace, EVENT_TRACE_CONTROL_QUERY ) ==
				   ERROR_WMI_INSTANCE_NOT_FOUND;
	}

	static void checkLayout() {
		static_assert( sizeof( ProcessNetworkMonitorWindows::Counters ) == 24 );
		static_assert( sizeof( ProcessNetworkStatus ) == 1 );
#if defined( _WIN64 )
		// The status consumes existing padding before gpuMemory without growing the snapshot.
		static_assert( offsetof( SystemInfo, gpuMemory ) == offsetof( SystemInfo, gpuUsage ) + 8 );
#endif
	}
};

} // namespace eproc

using namespace eproc;

namespace {

// Exercise the caller's tracing permissions without changing the account, its groups or UAC.
// Preserve any prior impersonation so a failed assertion cannot leak a token into another test.
struct RestrictedNetworkToken {
	HANDLE token{ nullptr };
	HANDLE previous{ nullptr };
	bool impersonating{ false };
	bool administrator{ true };
	bool performanceLogUser{ false };

	explicit RestrictedNetworkToken( bool disablePerformanceLogUsers ) {
		if ( !OpenThreadToken( GetCurrentThread(),
							   TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_DUPLICATE, TRUE,
							   &previous ) &&
			 GetLastError() != ERROR_NO_TOKEN )
			return;
		HANDLE original = nullptr;
		if ( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &original ) )
			return;
		alignas( DWORD ) BYTE admin[SECURITY_MAX_SID_SIZE]{};
		alignas( DWORD ) BYTE performance[SECURITY_MAX_SID_SIZE]{};
		DWORD adminSize = sizeof( admin ), performanceSize = sizeof( performance );
		const bool sids =
			CreateWellKnownSid( WinBuiltinAdministratorsSid, nullptr, admin, &adminSize ) &&
			CreateWellKnownSid( WinBuiltinPerfLoggingUsersSid, nullptr, performance,
								&performanceSize );
		SID_AND_ATTRIBUTES disabled[] = { { admin, 0 }, { performance, 0 } };
		const bool created =
			sids && CreateRestrictedToken( original, DISABLE_MAX_PRIVILEGE,
										   disablePerformanceLogUsers ? 2 : 1, disabled, 0, nullptr,
										   0, nullptr, &token );
		CloseHandle( original );
		if ( !created || !ImpersonateLoggedOnUser( token ) )
			return;
		impersonating = true;
		BOOL member = TRUE;
		if ( CheckTokenMembership( nullptr, admin, &member ) )
			administrator = member;
		member = FALSE;
		if ( CheckTokenMembership( nullptr, performance, &member ) )
			performanceLogUser = member;
	}

	~RestrictedNetworkToken() {
		if ( impersonating ) {
			if ( previous )
				SetThreadToken( nullptr, previous );
			else
				RevertToSelf();
		}
		if ( previous )
			CloseHandle( previous );
		if ( token )
			CloseHandle( token );
	}
};

bool capturesLiveUdp( ProcessNetworkMonitorWindows& monitor ) {
	WSADATA data{};
	if ( WSAStartup( MAKEWORD( 2, 2 ), &data ) != 0 )
		return false;
	struct Socket {
		SOCKET value{ socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) };
		~Socket() {
			if ( value != INVALID_SOCKET )
				closesocket( value );
			WSACleanup();
		}
	} udp;
	if ( udp.value == INVALID_SOCKET )
		return false;
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
	if ( bind( udp.value, reinterpret_cast<sockaddr*>( &address ), sizeof( address ) ) != 0 )
		return false;
	int addressLength = sizeof( address );
	if ( getsockname( udp.value, reinterpret_cast<sockaddr*>( &address ), &addressLength ) != 0 )
		return false;
	const DWORD timeout = 2000;
	if ( setsockopt( udp.value, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>( &timeout ),
					 sizeof( timeout ) ) != 0 )
		return false;
	std::array<char, 1024> payload{};
	for ( int i = 0; i < 32; ++i ) {
		if ( sendto( udp.value, payload.data(), payload.size(), 0,
					 reinterpret_cast<sockaddr*>( &address ), sizeof( address ) ) != 1024 ||
			 recv( udp.value, payload.data(), payload.size(), 0 ) != 1024 )
			return false;
	}
	// Real-time ETW flushes its buffers once a second. Retry across that boundary without
	// imposing a timing requirement on CI scheduling or on the first consumer buffer.
	bool received = false, sent = false;
	for ( int i = 0; i < 4 && ( !received || !sent ); ++i ) {
		Sys::sleep( Milliseconds( 600 ) );
		monitor.sample();
		ProcessInfo process;
		process.pid = GetCurrentProcessId();
		monitor.apply( process, 1 );
		received = received || process.netDownload > 0;
		sent = sent || process.netUpload > 0;
	}
	return received && sent;
}

} // namespace

UTEST( EProcWindowsCounters, ParsesGpuProcessAndProcessorGroupInstances ) {
	Int64 pid = 0;
	std::wstring_view engine;
	ASSERT_TRUE( PerformanceCountersWindowsTestAccess::process(
		L"pid_123_luid_0x00000000_0x00001234_phys_0_eng_2_engtype_3D", pid, engine ) );
	EXPECT_EQ( pid, 123 );
	EXPECT_TRUE( engine == L"luid_0x00000000_0x00001234_phys_0_eng_2_engtype_3D" );
	ASSERT_TRUE( PerformanceCountersWindowsTestAccess::process(
		L"pid_4294967295_luid_0x00000000_0x00001234_phys_0", pid, engine ) );
	EXPECT_EQ( pid, 4294967295LL );
	for ( const auto* name :
		  { L"_Total", L"pid_0_luid_0", L"pid_-1_luid_0", L"pid_1_other", L"pid_4294967296_luid_0",
			L"pid_18446744073709551616_luid_0", L"pid__luid_0", L"pid_1" } )
		EXPECT_FALSE( PerformanceCountersWindowsTestAccess::process( name, pid, engine ) );
	unsigned index = 0;
	ASSERT_TRUE( PerformanceCountersWindowsTestAccess::core( L"0,10", index ) );
	EXPECT_EQ( index, 10u );
	ASSERT_TRUE( PerformanceCountersWindowsTestAccess::core( L"1,2", index ) );
	EXPECT_EQ( index, 66u );
	for ( const auto* name : { L"_Total", L"0,_Total", L"0,64", L"-1,2", L"65536,0", L"0,",
							   L"0,0,0", L"0,18446744073709551616" } )
		EXPECT_FALSE( PerformanceCountersWindowsTestAccess::core( name, index ) );
}

UTEST( EProcWindowsNetwork, ParsesTcpAndUdpPayloadOwnerForBothAddressFamilies ) {
	EVENT_RECORD event{};
	event.EventHeader.ProviderId = {
		0x7dd42a49, 0x5329, 0x4832, { 0x8d, 0xfd, 0x43, 0xd9, 0x79, 0x15, 0x3a, 0x88 } };
	event.EventHeader.ProcessId = 4; // Kernel worker, deliberately different from the socket owner.
	std::array<Uint8, 9> unaligned{};
	const DWORD fields[] = { 123, 4096 };
	std::memcpy( unaligned.data() + 1, fields, sizeof( fields ) );
	event.UserData = unaligned.data() + 1;
	event.UserDataLength = sizeof( fields );
	DWORD pid = 0, bytes = 0;
	bool received = false;
	for ( USHORT id : { 10, 11, 26, 27, 42, 43, 58, 59 } ) {
		event.EventHeader.EventDescriptor.Id = id;
		ASSERT_TRUE( ProcessNetworkMonitorWindowsTestAccess::parse( event, pid, bytes, received ) );
		EXPECT_EQ( pid, 123u );
		EXPECT_EQ( bytes, 4096u );
		EXPECT_EQ( received, ( id & 1 ) != 0 );
	}
	event.EventHeader.EventDescriptor.Id = 14; // Retransmits are not fresh application data.
	EXPECT_FALSE( ProcessNetworkMonitorWindowsTestAccess::parse( event, pid, bytes, received ) );
	event.EventHeader.EventDescriptor.Id = 10;
	event.UserDataLength = 7;
	EXPECT_FALSE( ProcessNetworkMonitorWindowsTestAccess::parse( event, pid, bytes, received ) );
	event.UserDataLength = sizeof( fields );
	event.EventHeader.EventDescriptor.Version = 1;
	EXPECT_FALSE( ProcessNetworkMonitorWindowsTestAccess::parse( event, pid, bytes, received ) );
	event.EventHeader.EventDescriptor.Version = 0;
	event.EventHeader.ProviderId.Data1 = 0;
	EXPECT_FALSE( ProcessNetworkMonitorWindowsTestAccess::parse( event, pid, bytes, received ) );
}

UTEST( EProcWindowsNetwork, RejectsRecycledPidsAndUnavailableSamples ) {
	ProcessNetworkMonitorWindows monitor;
	ProcessInfo process;
	process.pid = 42;
	monitor.apply( process, 100 );
	EXPECT_EQ( process.netDownload, -1 );
	EXPECT_EQ( process.netUpload, -1 );
	ProcessNetworkMonitorWindowsTestAccess::publish( monitor, 200 );
	monitor.apply( process, 100 );
	EXPECT_EQ( process.netDownload, 2000 );
	EXPECT_EQ( process.netUpload, 1000 );
	monitor.apply( process, 300 );
	EXPECT_EQ( process.netDownload, -1 );
	EXPECT_EQ( process.netUpload, -1 );
	process.pid = 43;
	monitor.apply( process, 100 );
	EXPECT_EQ( process.netDownload, 0 );
	EXPECT_EQ( process.netUpload, 0 );
	ProcessNetworkMonitorWindowsTestAccess::invalidate( monitor );
	monitor.apply( process, 100 );
	EXPECT_EQ( process.netDownload, -1 );
	EXPECT_EQ( process.netUpload, -1 );
}

UTEST( EProcWindowsCollector, CollectsCurrentProcessAndOptionalSystemCounters ) {
	ProcessCollectorWindows collector;
	std::vector<ProcessInfo> processes;
	SystemInfo system;
	ASSERT_TRUE( collector.collect( processes, system ) );
	ASSERT_TRUE( system.cpuCount > 0 );
	EXPECT_EQ( system.clockTicksPerSecond, 1'000'000 );
	EXPECT_TRUE( system.totalMemory > 0 );
	EXPECT_TRUE( collector.supportsProgramsOnly() );
	const auto pid = static_cast<Int64>( GetCurrentProcessId() );
	const auto current =
		std::find_if( processes.begin(), processes.end(),
					  [pid]( const ProcessInfo& process ) { return process.pid == pid; } );
	ASSERT_TRUE( current != processes.end() );
	EXPECT_TRUE( current->ownedByCurrentUser );
	EXPECT_TRUE( current->vmRSS > 0 );
	EXPECT_TRUE( current->startTime > 0 );
	EXPECT_FALSE( current->iconPath.empty() );
	const auto end = MonotonicClock::now() + Milliseconds( 250 );
	while ( MonotonicClock::now() < end ) {
	}
	system.gpuUsage = 1000;
	system.gpuMemory = -1000;
	ASSERT_TRUE( collector.collect( processes, system ) );
	EXPECT_TRUE( system.cpuUsage >= 0.f && system.cpuUsage <= 100.f );
	EXPECT_TRUE( system.gpuUsage >= -1 && system.gpuUsage <= 100 );
	EXPECT_TRUE( system.gpuMemory >= -1 );
	EXPECT_TRUE( system.networkDownload >= -1 );
	EXPECT_TRUE( system.networkUpload >= -1 );
	if ( !system.coreCpuUsage.empty() ) {
		EXPECT_EQ( system.coreCpuUsage.size(), static_cast<size_t>( system.cpuCount ) );
		for ( float usage : system.coreCpuUsage )
			EXPECT_TRUE( usage >= 0.f && usage <= 100.f );
	}
	const auto second =
		std::find_if( processes.begin(), processes.end(),
					  [pid]( const ProcessInfo& process ) { return process.pid == pid; } );
	ASSERT_TRUE( second != processes.end() );
	EXPECT_TRUE( second->getCpuForSort() > 0 );
}

UTEST( EProcWindowsNetwork, CapturesLiveUdpTrafficAndStopsItsSession ) {
	ProcessNetworkMonitorWindows monitor;
	monitor.sample();
	if ( !ProcessNetworkMonitorWindowsTestAccess::running( monitor ) )
		UTEST_SKIP( "Kernel-network ETW permissions unavailable" );
	EXPECT_TRUE( capturesLiveUdp( monitor ) );
	EXPECT_TRUE( monitor.status() == ProcessNetworkStatus::Available );
	EXPECT_TRUE( ProcessNetworkMonitorWindowsTestAccess::stopsSession( monitor ) );
}

UTEST( EProcWindowsNetwork, HonorsTracingPermissionsOfRestrictedCaller ) {
	RestrictedNetworkToken restricted( false );
	ASSERT_TRUE( restricted.impersonating );
	ASSERT_FALSE( restricted.administrator );
	if ( !restricted.performanceLogUser )
		UTEST_SKIP( "Account is not a member of Performance Log Users" );
	ProcessNetworkMonitorWindows monitor;
	monitor.sample();
	// Group membership permits session creation, but an individual provider's ACL can still
	// deny access. Neither outcome should be replaced with an administrator-token precheck.
	if ( monitor.status() == ProcessNetworkStatus::AccessDenied ) {
		EXPECT_TRUE( monitor.error() == ERROR_ACCESS_DENIED ||
					 monitor.error() == ERROR_PRIVILEGE_NOT_HELD );
		EXPECT_FALSE( ProcessNetworkMonitorWindowsTestAccess::running( monitor ) );
		return;
	}
	ASSERT_TRUE( monitor.status() == ProcessNetworkStatus::Starting );
	EXPECT_TRUE( capturesLiveUdp( monitor ) );
	EXPECT_TRUE( monitor.status() == ProcessNetworkStatus::Available );
	EXPECT_TRUE( ProcessNetworkMonitorWindowsTestAccess::stopsSession( monitor ) );
}

UTEST( EProcWindowsNetwork, DeniedTokenLeavesRatesUnavailableAndRetriesAfterPermissionsRecover ) {
	ProcessNetworkMonitorWindows monitor;
	{
		RestrictedNetworkToken restricted( true );
		ASSERT_TRUE( restricted.impersonating );
		ASSERT_FALSE( restricted.administrator );
		ASSERT_FALSE( restricted.performanceLogUser );
		monitor.sample();
		ASSERT_TRUE( monitor.status() == ProcessNetworkStatus::AccessDenied );
		EXPECT_TRUE( monitor.error() == ERROR_ACCESS_DENIED ||
					 monitor.error() == ERROR_PRIVILEGE_NOT_HELD );
		ProcessInfo process;
		process.pid = GetCurrentProcessId();
		process.netDownload = process.netUpload = 123;
		monitor.apply( process, 1 );
		EXPECT_EQ( process.netDownload, -1 );
		EXPECT_EQ( process.netUpload, -1 );
	}
	// Restored privileges must not trigger a restart on every sample during the backoff.
	monitor.sample();
	EXPECT_TRUE( monitor.status() == ProcessNetworkStatus::AccessDenied );
	ProcessNetworkMonitorWindowsTestAccess::retryNow( monitor );
	monitor.sample();
	if ( monitor.status() == ProcessNetworkStatus::AccessDenied )
		UTEST_SKIP( "Original token also lacks kernel-network ETW permissions" );
	EXPECT_TRUE( monitor.status() == ProcessNetworkStatus::Starting );
	EXPECT_TRUE( capturesLiveUdp( monitor ) );
	EXPECT_TRUE( ProcessNetworkMonitorWindowsTestAccess::stopsSession( monitor ) );
}

UTEST( EProcWindowsNetwork, RecoversFromAnInterruptedTraceSession ) {
	ProcessNetworkMonitorWindows monitor;
	monitor.sample();
	if ( !ProcessNetworkMonitorWindowsTestAccess::running( monitor ) )
		UTEST_SKIP( "Kernel-network ETW permissions unavailable" );
	ASSERT_TRUE( ProcessNetworkMonitorWindowsTestAccess::interruptSession( monitor ) );
	monitor.sample();
	EXPECT_TRUE( monitor.status() == ProcessNetworkStatus::Unavailable );
	ProcessNetworkMonitorWindowsTestAccess::retryNow( monitor );
	monitor.sample();
	ASSERT_TRUE( monitor.status() == ProcessNetworkStatus::Starting );
	EXPECT_TRUE( capturesLiveUdp( monitor ) );
	EXPECT_TRUE( ProcessNetworkMonitorWindowsTestAccess::stopsSession( monitor ) );
}

#endif
