#include "busprocess.hpp"

namespace ecode {

BusProcess::BusProcess( const Command& command ) :
	mProcess( command.command, command.arguments,
			  Process::getDefaultOptions() | Process::Options::EnableAsync |
				  Process::Options::CombinedStdoutStderr,
			  command.environment ) {}

void BusProcess::startAsyncRead( ReadFn readFn ) {
	mProcess.startAsyncRead( readFn, readFn );
}

size_t BusProcess::write( const char* buffer, const size_t& size ) {
	return mProcess.write( buffer, size );
}

} // namespace ecode
