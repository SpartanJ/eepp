#include "bussocket.hpp"
#include <eepp/network/ipaddress.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/sys.hpp>

namespace ecode {

BusSocket::BusSocket( const Connection& connection ) : mConnection( connection ) {}

BusSocket::~BusSocket() {
	close();
}

bool BusSocket::start() {
	Clock clock;
	while ( clock.getElapsedTime() < Seconds( 3 ) ) {
		bool res = mSocket.connect( IpAddress( mConnection.host ), mConnection.port,
									Seconds( 1 ) ) == Socket::Status::Done;
		if ( res ) {
			setState( State::Running );
			return res;
		} else {
			Sys::sleep( Milliseconds( 50 ) );
		}
	}
	return false;
}

bool BusSocket::close() {
	if ( mState == State::Running ) {
		mSocket.disconnect();
		setState( State::Closed );
		return true;
	}

	return false;
}

void BusSocket::startAsyncRead( ReadFn readFn ) {
	mSocket.startAsyncRead( readFn );
}

size_t BusSocket::write( const char* buffer, const size_t& size ) {
	size_t sent = 0;
	mSocket.send( buffer, size, sent );
	return sent;
}

} // namespace ecode
