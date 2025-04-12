#include "busprocess.hpp"
#include "bussocket.hpp"
#include "bussocketprocess.hpp"

namespace ecode {

BusSocketProcess::BusSocketProcess( const Command& command, const Connection& connection ) :
	mCommand( command ),
	mConnection( connection ),
	mProcess( std::make_unique<BusProcess>( command ) ),
	mSocket( std::make_unique<BusSocket>( connection ) ) {}

BusSocketProcess::~BusSocketProcess() {
	mSocket.reset();
	mProcess.reset();
}

bool BusSocketProcess::start() {
	if ( !mCommand.isValid() || !mConnection.isValid() )
		return false;
	bool res = mProcess->start() && mSocket->start();
	if ( res )
		mState = State::Running;
	return res;
}

bool BusSocketProcess::close() {
	if ( mState == State::Running ) {
		if ( mSocket->state() == State::Running )
			mSocket->close();
		if ( mProcess->state() == State::Running )
			mProcess->close();
		mState = State::Closed;
		return true;
	}

	return false;
}

void BusSocketProcess::startAsyncRead( ReadFn readFn ) {
	mSocket->startAsyncRead( readFn );
}

size_t BusSocketProcess::write( const char* buffer, const size_t& size ) {
	return mSocket->write( buffer, size );
	;
}

} // namespace ecode
