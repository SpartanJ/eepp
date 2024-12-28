#include "debuggerclientlistener.hpp"

namespace ecode {

void DebuggerClientListener::stateChanged( DebuggerClient::State ) {}

void DebuggerClientListener::initialized() {}

void DebuggerClientListener::launched() {}

void DebuggerClientListener::failed() {}

void DebuggerClientListener::debuggeeRunning() {}

void DebuggerClientListener::debuggeeTerminated() {}

void DebuggerClientListener::capabilitiesReceived( const Capabilities& capabilities ) {}

void DebuggerClientListener::debuggeeExited( int exitCode ) {}

void DebuggerClientListener::debuggeeStopped( const StoppedEvent& ) {}

void DebuggerClientListener::debuggeeContinued( const ContinuedEvent& ) {}

void DebuggerClientListener::outputProduced( const Output& ) {}

void DebuggerClientListener::debuggingProcess( const ProcessInfo& ) {}

void DebuggerClientListener::errorResponse( const std::string& summary,
											const std::optional<Message>& message ) {}

void DebuggerClientListener::threadChanged( const ThreadEvent& ) {}

void DebuggerClientListener::moduleChanged( const ModuleEvent& ) {}

void DebuggerClientListener::threads( const std::vector<Thread>& ) {}

void DebuggerClientListener::stackTrace( const int threadId, const StackTraceInfo& ) {}

void DebuggerClientListener::scopes( const int frameId, const std::vector<Scope>& ) {}

void DebuggerClientListener::variables( const int variablesReference,
										const std::vector<Variable>& ) {}

void DebuggerClientListener::modules( const ModulesInfo& ) {}

void DebuggerClientListener::serverDisconnected() {}

void DebuggerClientListener::sourceContent( const std::string& path, int reference,
											const SourceContent& content ) {}

void DebuggerClientListener::sourceBreakpoints(
	const std::string& path, int reference,
	const std::optional<std::vector<Breakpoint>>& breakpoints ) {}

void DebuggerClientListener::breakpointChanged( const BreakpointEvent& ) {}

void DebuggerClientListener::expressionEvaluated( const std::string& expression,
												  const std::optional<EvaluateInfo>& ) {}
void DebuggerClientListener::gotoTargets( const Source& source, const int line,

										  const std::vector<GotoTarget>& targets ) {}

} // namespace ecode
