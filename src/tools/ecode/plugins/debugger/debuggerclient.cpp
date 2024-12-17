#include "debuggerclient.hpp"

namespace ecode {

void DebuggerClient::stateChanged( State state ) {
	if ( mObserver )
		mObserver->stateChanged( state );
}

void DebuggerClient::initialized() {
	if ( mObserver )
		mObserver->initialized();
}

void DebuggerClient::debuggeeRunning() {
	if ( mObserver )
		mObserver->debuggeeRunning();
}

void DebuggerClient::debuggeeTerminated() {
	if ( mObserver )
		mObserver->debuggeeRunning();
}

void DebuggerClient::failed() {
	if ( mObserver )
		mObserver->failed();
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
