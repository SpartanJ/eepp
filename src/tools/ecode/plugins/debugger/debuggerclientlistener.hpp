#pragma once
#include "debuggerclient.hpp"
#include "statusdebuggercontroller.hpp"

namespace ecode {

class DebuggerPlugin;

class DebuggerClientListener : public DebuggerClient::Listener {
  public:
	DebuggerClientListener( DebuggerClient* client, DebuggerPlugin* plugin );

	void stateChanged( DebuggerClient::State );
	void initialized();
	void launched();
	void configured();
	void failed();
	void debuggeeRunning();
	void debuggeeTerminated();

	void capabilitiesReceived( const Capabilities& capabilities );
	void debuggeeExited( int exitCode );
	void debuggeeStopped( const StoppedEvent& );
	void debuggeeContinued( const ContinuedEvent& );
	void outputProduced( const Output& );
	void debuggingProcess( const ProcessInfo& );
	void errorResponse( const std::string& summary, const std::optional<Message>& message );
	void threadChanged( const ThreadEvent& );
	void moduleChanged( const ModuleEvent& );
	void threads( const std::vector<Thread>& );
	void stackTrace( const int threadId, const StackTraceInfo& );
	void scopes( const int frameId, const std::vector<Scope>& );
	void variables( const int variablesReference, const std::vector<Variable>& );
	void modules( const ModulesInfo& );
	void serverDisconnected();
	void sourceContent( const std::string& path, int reference,
						const SourceContent& content = SourceContent() );
	void sourceBreakpoints( const std::string& path, int reference,
							const std::optional<std::vector<Breakpoint>>& breakpoints );
	void breakpointChanged( const BreakpointEvent& );
	void expressionEvaluated( const std::string& expression, const std::optional<EvaluateInfo>& );
	void gotoTargets( const Source& source, const int line,
					  const std::vector<GotoTarget>& targets );

  protected:
	DebuggerClient* mClient{ nullptr };
	DebuggerPlugin* mPlugin{ nullptr };

	StatusDebuggerController* getStatusDebuggerController() const;
};

} // namespace ecode
