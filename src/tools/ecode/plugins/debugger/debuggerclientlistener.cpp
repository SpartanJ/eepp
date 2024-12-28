#include "debuggerclientlistener.hpp"
#include "debuggerplugin.hpp"

namespace ecode {

DebuggerClientListener::DebuggerClientListener( DebuggerClient* client, DebuggerPlugin* plugin ) :
	mClient( client ), mPlugin( plugin ) {
	eeASSERT( mClient && mPlugin );
}

void DebuggerClientListener::stateChanged( DebuggerClient::State ) {}

void DebuggerClientListener::initialized() {
	// mClient->setBreakpoints( "/home/programming/eepp/src/tools/ecode/ecode.cpp",
	// 						 { SourceBreakpoint( 4116 ) } );
}

void DebuggerClientListener::launched() {}

void DebuggerClientListener::configured() {}

void DebuggerClientListener::failed() {}

void DebuggerClientListener::debuggeeRunning() {}

void DebuggerClientListener::debuggeeTerminated() {}

void DebuggerClientListener::capabilitiesReceived( const Capabilities& /*capabilities*/ ) {}

void DebuggerClientListener::debuggeeExited( int /*exitCode*/ ) {
	mPlugin->exitDebugger();
}

void DebuggerClientListener::debuggeeStopped( const StoppedEvent& event ) {
	Log::warning( "DebuggerClientListener::debuggeeStopped: reason %s", event.reason );
	// if ( event.threadId ) {
	// 	mClient->stackTrace( *event.threadId );
	// }
}

void DebuggerClientListener::debuggeeContinued( const ContinuedEvent& ) {}

void DebuggerClientListener::outputProduced( const Output& ) {}

void DebuggerClientListener::debuggingProcess( const ProcessInfo& ) {}

void DebuggerClientListener::errorResponse( const std::string& /*summary*/,
											const std::optional<Message>& /*message*/ ) {}

void DebuggerClientListener::threadChanged( const ThreadEvent& ) {}

void DebuggerClientListener::moduleChanged( const ModuleEvent& ) {}

void DebuggerClientListener::threads( const std::vector<Thread>& /*threads*/ ) {}

void DebuggerClientListener::stackTrace( const int /*threadId*/, const StackTraceInfo& stack ) {
	// if ( !stack.stackFrames.empty() ) {
	// 	mClient->scopes( stack.stackFrames[0].id );
	// }
}

void DebuggerClientListener::scopes( const int /*frameId**/, const std::vector<Scope>& scopes ) {
	// if ( !scopes.empty() ) {
	// 	mClient->variables( scopes[0].variablesReference );
	// }
}

void DebuggerClientListener::variables( const int /*variablesReference*/,
										const std::vector<Variable>& vars ) {
	// if ( !vars.empty() ) {
	// 	mClient->resume( 1 );
	// }
}

void DebuggerClientListener::modules( const ModulesInfo& ) {}

void DebuggerClientListener::serverDisconnected() {}

void DebuggerClientListener::sourceContent( const std::string& /*path*/, int /*reference*/,
											const SourceContent& /*content*/ ) {}

void DebuggerClientListener::sourceBreakpoints(
	const std::string& /*path*/, int /*reference*/,
	const std::optional<std::vector<Breakpoint>>& /*breakpoints*/ ) {}

void DebuggerClientListener::breakpointChanged( const BreakpointEvent& ) {}

void DebuggerClientListener::expressionEvaluated( const std::string& /*expression*/,
												  const std::optional<EvaluateInfo>& ) {}
void DebuggerClientListener::gotoTargets( const Source& /*source*/, const int /*line*/,
										  const std::vector<GotoTarget>& /*targets*/ ) {}

} // namespace ecode
