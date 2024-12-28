#include "debuggerclient.hpp"

namespace ecode {

void DebuggerClient::stateChanged( State state ) {
	for ( auto listener : mListeners )
		listener->stateChanged( state );
}

void DebuggerClient::initialized() {
	for ( auto listener : mListeners )
		listener->initialized();
}

void DebuggerClient::debuggeeRunning() {
	for ( auto listener : mListeners )
		listener->debuggeeRunning();
}

void DebuggerClient::debuggeeTerminated() {
	for ( auto listener : mListeners )
		listener->debuggeeRunning();
}

void DebuggerClient::failed() {
	for ( auto listener : mListeners )
		listener->failed();
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

void DebuggerClient::addListener( Listener* listener ) {
	mListeners.emplace_back( listener );
}

void DebuggerClient::removeListener( Listener* listener ) {
	mListeners.erase( std::find( mListeners.begin(), mListeners.end(), listener ) );
}

} // namespace ecode
