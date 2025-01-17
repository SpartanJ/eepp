#include "busprocess.hpp"

namespace ecode {

BusProcess::BusProcess( const Command& command ) : mCommand( command ), mProcess() {}

BusProcess::~BusProcess() {
	close();
}

bool BusProcess::start() {
	bool res = mCommand.arguments.empty()
				   ? mProcess.create( mCommand.command,
									  Process::getDefaultOptions() | Process::Options::EnableAsync |
										  Process::Options::CombinedStdoutStderr,
									  mCommand.environment )
				   : mProcess.create( mCommand.command, mCommand.arguments,
									  Process::getDefaultOptions() | Process::Options::EnableAsync |
										  Process::Options::CombinedStdoutStderr,
									  mCommand.environment );
	if ( res )
		setState( State::Running );

	return res;
}

bool BusProcess::close() {
	if ( mState == State::Running ) {
		bool res = mProcess.kill();
		if ( res )
			setState( State::Closed );
	}
	return false;
}

void BusProcess::startAsyncRead( ReadFn readFn ) {
	mProcess.startAsyncRead( readFn, readFn );
}

size_t BusProcess::write( const char* buffer, const size_t& size ) {
	return mProcess.write( buffer, size );
}

} // namespace ecode
