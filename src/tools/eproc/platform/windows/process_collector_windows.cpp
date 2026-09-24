#include "process_collector_windows.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <psapi.h>
#include <tlhelp32.h>

#include <algorithm>
#include <array>
#include <climits>
#include <cwchar>
#include <limits>

namespace eproc {

namespace {

constexpr Uint64 kMicrosecondsPerSecond = 1000000;
constexpr Uint32 kCacheSweepInterval = 60;

class ScopedHandle {
  public:
	explicit ScopedHandle( HANDLE value = nullptr ) : mValue( value ) {}

	~ScopedHandle() {
		if ( valid() )
			CloseHandle( mValue );
	}

	ScopedHandle( const ScopedHandle& ) = delete;
	ScopedHandle& operator=( const ScopedHandle& ) = delete;

	bool valid() const { return mValue && mValue != INVALID_HANDLE_VALUE; }

	HANDLE get() const { return mValue; }

	void reset( HANDLE value ) {
		if ( valid() )
			CloseHandle( mValue );
		mValue = value;
	}

  private:
	HANDLE mValue;
};

Uint64 fileTimeValue( const FILETIME& value ) {
	return ( static_cast<Uint64>( value.dwHighDateTime ) << 32 ) | value.dwLowDateTime;
}

std::string utf8( const WCHAR* text, int length ) {
	if ( !text || length <= 0 )
		return {};
	const int bytes = WideCharToMultiByte( CP_UTF8, 0, text, length, nullptr, 0, nullptr, nullptr );
	if ( bytes <= 0 )
		return {};
	std::string result( static_cast<size_t>( bytes ), '\0' );
	if ( WideCharToMultiByte( CP_UTF8, 0, text, length, result.data(), bytes, nullptr, nullptr ) !=
		 bytes )
		return {};
	return result;
}

bool tokenUser( HANDLE token, std::vector<Uint64>& storage, TOKEN_USER*& user ) {
	DWORD bytes = 0;
	GetTokenInformation( token, TokenUser, nullptr, 0, &bytes );
	if ( bytes < sizeof( TOKEN_USER ) )
		return false;
	storage.resize( ( bytes + sizeof( Uint64 ) - 1 ) / sizeof( Uint64 ) );
	if ( !GetTokenInformation( token, TokenUser, storage.data(),
							   static_cast<DWORD>( storage.size() * sizeof( Uint64 ) ), &bytes ) )
		return false;
	user = reinterpret_cast<TOKEN_USER*>( storage.data() );
	return user->User.Sid && IsValidSid( user->User.Sid );
}

Int64 positiveDelta( Uint64 current, Uint64 previous ) {
	return current >= previous ? static_cast<Int64>( current - previous ) : 0;
}

int cpuPercent( Uint64 delta, Int64 elapsedMicroseconds ) {
	if ( elapsedMicroseconds <= 0 )
		return 0;
	const double percent = static_cast<double>( delta / 10 ) * 100.0 / elapsedMicroseconds;
	// userUsage and sysUsage are added in the common model.
	return static_cast<int>( std::min( percent, static_cast<double>( INT_MAX / 2 ) ) );
}

} // namespace

ProcessCollectorWindows::ProcessCollectorWindows() {
	HANDLE rawToken = nullptr;
	if ( OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &rawToken ) ) {
		ScopedHandle currentToken( rawToken );
		TOKEN_USER* user = nullptr;
		// TokenUser's SID pointer remains valid while this vector is unchanged.
		if ( !tokenUser( currentToken.get(), mCurrentUserToken, user ) )
			mCurrentUserToken.clear();
	}
}

void ProcessCollectorWindows::readIdentity( void* processHandle, ProcessInfo& process ) {
	HANDLE rawToken = nullptr;
	if ( !OpenProcessToken( static_cast<HANDLE>( processHandle ), TOKEN_QUERY, &rawToken ) )
		return;
	ScopedHandle token( rawToken );
	// Most TokenUser results fit on the stack; only unusually large SIDs allocate.
	std::array<Uint64, 32> stackStorage;
	std::vector<Uint64> overflowStorage;
	TOKEN_USER* user = nullptr;
	DWORD bytes = 0;
	if ( GetTokenInformation( token.get(), TokenUser, stackStorage.data(), sizeof( stackStorage ),
							  &bytes ) ) {
		user = reinterpret_cast<TOKEN_USER*>( stackStorage.data() );
	} else {
		if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes < sizeof( TOKEN_USER ) )
			return;
		overflowStorage.resize( ( bytes + sizeof( Uint64 ) - 1 ) / sizeof( Uint64 ) );
		if ( !GetTokenInformation( token.get(), TokenUser, overflowStorage.data(),
								   static_cast<DWORD>( overflowStorage.size() * sizeof( Uint64 ) ),
								   &bytes ) )
			return;
		user = reinterpret_cast<TOKEN_USER*>( overflowStorage.data() );
	}
	if ( !user->User.Sid || !IsValidSid( user->User.Sid ) )
		return;

	const PSID sid = user->User.Sid;
	if ( !mCurrentUserToken.empty() ) {
		const auto* own = reinterpret_cast<const TOKEN_USER*>( mCurrentUserToken.data() );
		process.ownedByCurrentUser = EqualSid( sid, own->User.Sid ) != 0;
	}
	process.systemProcess = IsWellKnownSid( sid, WinLocalSystemSid ) ||
							IsWellKnownSid( sid, WinLocalServiceSid ) ||
							IsWellKnownSid( sid, WinNetworkServiceSid );
	process.userProcess = !process.systemProcess;

	const DWORD sidLength = GetLengthSid( sid );
	std::string sidKey( static_cast<const char*>( sid ), sidLength );
	auto found = mAccountNames.find( sidKey );
	if ( found == mAccountNames.end() ) {
		DWORD nameLength = 0, domainLength = 0;
		SID_NAME_USE kind;
		LookupAccountSidW( nullptr, sid, nullptr, &nameLength, nullptr, &domainLength, &kind );
		std::string account;
		if ( nameLength ) {
			std::vector<WCHAR> name( nameLength );
			std::vector<WCHAR> domain( std::max<DWORD>( domainLength, 1 ) );
			if ( LookupAccountSidW( nullptr, sid, name.data(), &nameLength, domain.data(),
									&domainLength, &kind ) ) {
				account = utf8( name.data(), static_cast<int>( nameLength ) );
				if ( domainLength )
					account =
						utf8( domain.data(), static_cast<int>( domainLength ) ) + "\\" + account;
			}
		}
		found = mAccountNames.emplace( std::move( sidKey ), std::move( account ) ).first;
	}
	process.username = found->second;
}

bool ProcessCollectorWindows::collect( std::vector<ProcessInfo>& processes, SystemInfo& sysInfo ) {
	++mPass;
	if ( mPass == 0 )
		++mPass;
	const auto sampledAt = std::chrono::steady_clock::now();
	const Int64 elapsedMicroseconds =
		mPrevSampleTime.time_since_epoch().count() == 0
			? 0
			: std::chrono::duration_cast<std::chrono::microseconds>( sampledAt - mPrevSampleTime )
				  .count();
	mPrevSampleTime = sampledAt;

	SYSTEM_INFO system;
	GetNativeSystemInfo( &system );
	sysInfo.cpuCount = static_cast<int>( std::max<DWORD>( 1, system.dwNumberOfProcessors ) );
	sysInfo.clockTicksPerSecond = kMicrosecondsPerSecond;
	sysInfo.uptimeSeconds = static_cast<double>( GetTickCount64() ) / 1000.0;

	FILETIME idle, kernel, user;
	if ( GetSystemTimes( &idle, &kernel, &user ) ) {
		const Uint64 idleNow = fileTimeValue( idle );
		const Uint64 kernelNow = fileTimeValue( kernel );
		const Uint64 userNow = fileTimeValue( user );
		const Uint64 deltaKernel = positiveDelta( kernelNow, mPrevSystemKernel );
		const Uint64 deltaUser = positiveDelta( userNow, mPrevSystemUser );
		const Uint64 deltaIdle = positiveDelta( idleNow, mPrevSystemIdle );
		const Uint64 deltaTotal = deltaKernel + deltaUser;
		if ( mPrevSystemKernel && deltaTotal > deltaIdle && elapsedMicroseconds >= 200000 )
			mLastCpuUsage = static_cast<float>( deltaTotal - deltaIdle ) * 100.f / deltaTotal;
		mPrevSystemIdle = idleNow;
		mPrevSystemKernel = kernelNow;
		mPrevSystemUser = userNow;
	}
	sysInfo.cpuUsage = mLastCpuUsage;

	MEMORYSTATUSEX memory{};
	memory.dwLength = sizeof( memory );
	if ( GlobalMemoryStatusEx( &memory ) ) {
		sysInfo.totalMemory = static_cast<Int64>( memory.ullTotalPhys / 1024 );
		sysInfo.availableMemory = static_cast<Int64>( memory.ullAvailPhys / 1024 );
		sysInfo.freeMemory = sysInfo.availableMemory;
		// Windows reports pagefile/commit capacity here, not Linux-style swap usage.
		sysInfo.totalSwap = static_cast<Int64>( memory.ullTotalPageFile / 1024 );
		sysInfo.freeSwap = static_cast<Int64>( memory.ullAvailPageFile / 1024 );
	}

	FILETIME nowFileTime;
	GetSystemTimeAsFileTime( &nowFileTime );
	const Uint64 now100ns = fileTimeValue( nowFileTime );
	const Uint64 uptimeMicroseconds = GetTickCount64() * 1000;

	ScopedHandle snapshot( CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 ) );
	if ( !snapshot.valid() )
		return false;
	processes.clear();
	PROCESSENTRY32W entry{};
	entry.dwSize = sizeof( entry );
	if ( !Process32FirstW( snapshot.get(), &entry ) )
		return false;
	do {
		ProcessInfo process;
		process.pid = static_cast<Int64>( entry.th32ProcessID );
		process.parentPid = static_cast<Int64>( entry.th32ParentProcessID );
		process.name = utf8( entry.szExeFile, static_cast<int>( wcslen( entry.szExeFile ) ) );
		process.numThreads = static_cast<int>( entry.cntThreads );

		ScopedHandle handle( OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ,
										  FALSE, entry.th32ProcessID ) );
		if ( !handle.valid() )
			handle.reset(
				OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID ) );
		if ( handle.valid() ) {
			FILETIME creation, exit, kernelTime, userTime;
			if ( GetProcessTimes( handle.get(), &creation, &exit, &kernelTime, &userTime ) ) {
				const Uint64 creation100ns = fileTimeValue( creation );
				const Uint64 user100ns = fileTimeValue( userTime );
				const Uint64 kernel100ns = fileTimeValue( kernelTime );
				process.userTime = static_cast<Int64>( user100ns / 10 );
				process.sysTime = static_cast<Int64>( kernel100ns / 10 );
				const Uint64 ageMicroseconds =
					now100ns >= creation100ns ? ( now100ns - creation100ns ) / 10 : 0;
				TickEntry& previous = mProcessTicks[process.pid];
				const bool sameProcess =
					creation100ns != 0 && previous.creationTime == creation100ns;
				process.startTime =
					sameProcess ? previous.startTime
								: static_cast<Int64>( ageMicroseconds < uptimeMicroseconds
														  ? uptimeMicroseconds - ageMicroseconds
														  : 0 );
				if ( sameProcess && previous.pass == mPass - 1 && elapsedMicroseconds >= 200000 ) {
					process.userUsage = cpuPercent( positiveDelta( user100ns, previous.userTime ),
													elapsedMicroseconds );
					process.sysUsage = cpuPercent(
						positiveDelta( kernel100ns, previous.kernelTime ), elapsedMicroseconds );
				}
				previous = { user100ns, kernel100ns, creation100ns, process.startTime, mPass };
			}

			PROCESS_MEMORY_COUNTERS_EX counters{};
			if ( GetProcessMemoryInfo( handle.get(),
									   reinterpret_cast<PROCESS_MEMORY_COUNTERS*>( &counters ),
									   sizeof( counters ) ) ) {
				if ( counters.WorkingSetSize )
					process.vmRSS = static_cast<Int64>( counters.WorkingSetSize / 1024 );
				// PrivateUsage is private commit, not private resident memory on Linux.
				if ( counters.PrivateUsage )
					process.vmURSS = static_cast<Int64>( counters.PrivateUsage / 1024 );
			}

			IO_COUNTERS io{};
			if ( GetProcessIoCounters( handle.get(), &io ) ) {
				process.ioReadBytes = static_cast<Int64>( io.ReadTransferCount );
				process.ioWriteBytes = static_cast<Int64>( io.WriteTransferCount );
			}

			std::array<WCHAR, 4096> path;
			DWORD pathLength = static_cast<DWORD>( path.size() );
			if ( QueryFullProcessImageNameW( handle.get(), 0, path.data(), &pathLength ) )
				process.commandLine = utf8( path.data(), static_cast<int>( pathLength ) );

			readIdentity( handle.get(), process );
		}
		if ( process.commandLine.empty() )
			process.commandLine = process.name;
		processes.push_back( std::move( process ) );
	} while ( Process32NextW( snapshot.get(), &entry ) );

	if ( ( mPass % kCacheSweepInterval ) == 0 ) {
		for ( auto it = mProcessTicks.begin(); it != mProcessTicks.end(); ) {
			if ( it->second.pass != mPass )
				it = mProcessTicks.erase( it );
			else
				++it;
		}
	}
	return true;
}

} // namespace eproc
