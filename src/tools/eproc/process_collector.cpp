#include "process_collector.hpp"
#include <eepp/system/log.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX
#include "platform/linux/process_collector_linux.hpp"

#include <signal.h>
#include <sys/types.h>
#endif

using namespace EE::System;

namespace eproc {

std::unique_ptr<ProcessCollector> ProcessCollector::create() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	return std::make_unique<ProcessCollectorLinux>();
#else
	Log::error( "eproc: no process collector available for this platform" );
	return nullptr;
#endif
}

bool sendProcessSignal( long pid, int signal ) {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	return ::kill( static_cast<pid_t>( pid ), signal ) == 0;
#else
	Log::error( "eproc: sendProcessSignal is not implemented for this platform" );
	return false;
#endif
}

bool killProcess( long pid ) {
	return sendProcessSignal( pid, 9 ); // SIGKILL
}

} // namespace eproc
