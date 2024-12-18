#include "debuggerclient.hpp"

namespace ecode {

void DebuggerClient::stateChanged( State state ) {
	for ( auto client : mClients )
		client->stateChanged( state );
}

void DebuggerClient::initialized() {
	for ( auto client : mClients )
		client->initialized();
}

void DebuggerClient::debuggeeRunning() {
	for ( auto client : mClients )
		client->debuggeeRunning();
}

void DebuggerClient::debuggeeTerminated() {
	for ( auto client : mClients )
		client->debuggeeRunning();
}

void DebuggerClient::failed() {
	for ( auto client : mClients )
		client->failed();
}

void DebuggerClient::setState( const State& state ) {
	if ( state != mState ) {
		mState = state;
		stateChanged( mState );

		switch ( mState ) {
			case State::Initialized:
				initialized();
				checkRunning();
				break;
			case State::Running:
				debuggeeRunning();
				break;
			case State::Terminated:
				debuggeeTerminated();
				break;
			case State::Failed:
				failed();
				break;
			default:;
		}
	}
}

void DebuggerClient::checkRunning() {
	if ( mLaunched && mConfigured && mState == State::Initialized ) {
		setState( State::Running );
	}
}

} // namespace ecode
