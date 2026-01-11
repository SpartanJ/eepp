#include "debuggerclient.hpp"

namespace ecode {

void DebuggerClient::stateChanged( State state, const SessionId& sessionId ) {
	for ( auto listener : mListeners )
		listener->stateChanged( state, sessionId );
}

void DebuggerClient::initialized( const SessionId& sessionId ) {
	for ( auto listener : mListeners )
		listener->initialized( sessionId );
}

void DebuggerClient::debuggeeRunning( const SessionId& sessionId ) {
	for ( auto listener : mListeners )
		listener->debuggeeRunning( sessionId );
}

void DebuggerClient::debuggeeTerminated( const SessionId& sessionId ) {
	for ( auto listener : mListeners )
		listener->debuggeeTerminated( sessionId );
}

void DebuggerClient::failed( const SessionId& sessionId ) {
	for ( auto listener : mListeners )
		listener->failed( sessionId );
}

void DebuggerClient::addListener( Listener* listener ) {
	mListeners.emplace_back( listener );
}

void DebuggerClient::removeListener( Listener* listener ) {
	mListeners.erase( std::find( mListeners.begin(), mListeners.end(), listener ) );
}

} // namespace ecode
