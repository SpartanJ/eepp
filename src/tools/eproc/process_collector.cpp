#include "process_collector.hpp"
#include <eepp/system/log.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX
#include "platform/linux/process_collector_linux.hpp"

#include <signal.h>
#include <sys/types.h>
#elif EE_PLATFORM == EE_PLATFORM_MACOS
#include "platform/macos/process_collector_macos.hpp"

#include <signal.h>
#include <sys/types.h>
#elif EE_PLATFORM == EE_PLATFORM_BSD && defined( __FreeBSD__ )
#include "platform/freebsd/process_collector_freebsd.hpp"

#include <signal.h>
#include <sys/types.h>
#elif EE_PLATFORM == EE_PLATFORM_WIN
#include "platform/windows/process_collector_windows.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_BSD && !defined( __FreeBSD__ )
#include <signal.h>
#include <sys/types.h>
#endif

using namespace EE::System;

namespace eproc {

std::unique_ptr<ProcessCollector> ProcessCollector::create() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	return std::make_unique<ProcessCollectorLinux>();
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return std::make_unique<ProcessCollectorMacOS>();
#elif EE_PLATFORM == EE_PLATFORM_BSD && defined( __FreeBSD__ )
	return std::make_unique<ProcessCollectorFreeBSD>();
#elif EE_PLATFORM == EE_PLATFORM_WIN
	return std::make_unique<ProcessCollectorWindows>();
#else
	Log::error( "eproc: no process collector available for this platform" );
	return nullptr;
#endif
}

bool sendProcessSignal( Int64 pid, int signal ) {
#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_MACOS || \
	EE_PLATFORM == EE_PLATFORM_BSD
	return pid > 0 && ::kill( static_cast<pid_t>( pid ), signal ) == 0;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	return signal == 9 && killProcess( pid );
#else
	Log::error( "eproc: sendProcessSignal is not implemented for this platform" );
	return false;
#endif
}

bool killProcess( Int64 pid ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( pid <= 0 || pid > MAXDWORD )
		return false;
	HANDLE process = OpenProcess( PROCESS_TERMINATE, FALSE, static_cast<DWORD>( pid ) );
	if ( !process )
		return false;
	const bool terminated = TerminateProcess( process, 1 ) != 0;
	CloseHandle( process );
	return terminated;
#else
	return sendProcessSignal( pid, 9 ); // SIGKILL
#endif
}

} // namespace eproc
