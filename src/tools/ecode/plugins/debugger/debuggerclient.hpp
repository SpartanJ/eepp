#pragma once
#include "dap/protocol.hpp"
#include <string>

namespace ecode {

using namespace dap;

using SessionId = std::string;

class DebuggerClient {
  public:
	enum class State { None, Initializing, Initialized, Running, Terminated, Failed };

	using VariablesResponseCb =
		std::function<void( const int variablesReference, std::vector<Variable>&& vars )>;

	class Listener {
	  public:
		virtual void stateChanged( State, const SessionId& sessionId ) = 0;

		virtual void initialized( const SessionId& sessionId ) = 0;

		virtual void launched( const SessionId& sessionId ) = 0;

		virtual void configured( const SessionId& sessionId ) = 0;

		virtual void failed( const SessionId& sessionId ) = 0;

		virtual void debuggeeRunning( const SessionId& sessionId ) = 0;

		virtual void debuggeeTerminated( const SessionId& sessionId ) = 0;

		virtual void capabilitiesReceived( const Capabilities& capabilities ) = 0;

		virtual void debuggeeExited( int exitCode, const SessionId& sessionId ) = 0;

		virtual void debuggeeStopped( const StoppedEvent&, const SessionId& sessionId ) = 0;

		virtual void debuggeeContinued( const ContinuedEvent&, const SessionId& sessionId ) = 0;

		virtual void outputProduced( const Output& ) = 0;

		virtual void debuggingProcess( const ProcessInfo&, const SessionId& sessionId ) = 0;

		virtual void errorResponse( const std::string& command, const std::string& summary,
									const std::optional<Message>& message,
									const SessionId& sessionId ) = 0;

		virtual void threadChanged( const ThreadEvent&, const SessionId& sessionId ) = 0;

		virtual void moduleChanged( const ModuleEvent&, const SessionId& sessionId ) = 0;

		virtual void threads( std::vector<DapThread>&&, const SessionId& sessionId ) = 0;

		virtual void stackTrace( const int threadId, StackTraceInfo&&,
								 const SessionId& sessionId ) = 0;

		virtual void scopes( const int frameId, std::vector<Scope>&&,
							 const SessionId& sessionId ) = 0;

		virtual void variables( const int variablesReference, std::vector<Variable>&&,
								const SessionId& sessionId ) = 0;

		virtual void modules( ModulesInfo&&, const SessionId& sessionId ) = 0;

		virtual void serverDisconnected( const SessionId& sessionId ) = 0;

		virtual void sourceContent( const std::string& path, int reference = 0,
									const SourceContent& content = SourceContent(),
									const SessionId& sessionId = "" ) = 0;

		virtual void sourceBreakpoints( const std::string& path, int reference,
										const std::optional<std::vector<Breakpoint>>& breakpoints,
										const SessionId& sessionId ) = 0;

		virtual void breakpointChanged( const BreakpointEvent&, const SessionId& sessionId ) = 0;

		virtual void expressionEvaluated( const std::string& expression,
										  const std::optional<EvaluateInfo>&,
										  const SessionId& sessionId ) = 0;

		virtual void gotoTargets( const Source& source, const int line,
								  const std::vector<GotoTarget>& targets,
								  const SessionId& sessionId ) = 0;

	};

	virtual bool start() = 0;

	virtual bool started() const = 0;

	virtual bool resume( int threadId, bool singleThread = false ) = 0;

	virtual bool pause( int threadId ) = 0;

	virtual bool stepOver( int threadId, bool singleThread = false ) = 0;

	virtual bool goTo( int threadId, int targetId ) = 0;

	virtual bool stepInto( int threadId, bool singleThread = false ) = 0;

	virtual bool stepOut( int threadId, bool singleThread = false ) = 0;

	virtual bool terminate( bool restart ) = 0;

	virtual bool disconnect( bool terminateDebuggee, bool restart = false ) = 0;

	virtual bool threads() = 0;

	virtual bool stackTrace( int threadId, int startFrame = 0, int levels = 0 ) = 0;

	virtual bool scopes( int frameId ) = 0;

	virtual bool modules( int start, int count ) = 0;

	/** if responseCb is provided, response will not be sent to listeners */
	virtual bool variables( int variablesReference, Variable::Type filter = Variable::Type::Both,
							VariablesResponseCb responseCb = nullptr, int start = 0,
							int count = 0 ) = 0;

	virtual bool evaluate(
		const std::string& expression, const std::string& context, std::optional<int> frameId,
		std::function<void( const std::string& expression, const std::optional<EvaluateInfo>& )>
			cb = {} ) = 0;

	virtual bool isServerConnected() const = 0;

	virtual bool supportsTerminateRequest() const = 0;

	virtual bool supportsTerminateDebuggee() const = 0;

	virtual bool setBreakpoints( const std::string& path,
								 const std::vector<dap::SourceBreakpoint>& breakpoints,
								 bool sourceModified = false ) = 0;

	virtual bool setBreakpoints( const dap::Source& source,
								 const std::vector<dap::SourceBreakpoint>& breakpoints,
								 bool sourceModified = false ) = 0;

	virtual bool gotoTargets( const std::string& path, const int line,
							  const std::optional<int> column = std::nullopt ) = 0;

	virtual bool gotoTargets( const dap::Source& source, const int line,
							  const std::optional<int> column = std::nullopt ) = 0;

	virtual bool watch( const std::string& expression, std::optional<int> frameId ) = 0;

	virtual bool configurationDone( const SessionId& sessionId ) = 0;

	virtual void setSilent( bool silent ) = 0;

	void addListener( Listener* listener );

	void removeListener( Listener* listener );

	virtual ~DebuggerClient() {}

	virtual size_t sessionsActive() = 0;

  protected:
	virtual void setState( const State& state, const SessionId& sessionId = "" ) = 0;

	virtual void checkRunning( const SessionId& sessionId = "" ) = 0;

	std::vector<Listener*> mListeners;

	void stateChanged( State, const SessionId& sessionId = "" );
	void initialized( const SessionId& sessionId = "" );
	void debuggeeRunning( const SessionId& sessionId = "" );
	void debuggeeTerminated( const SessionId& sessionId = "" );
	void failed( const SessionId& sessionId = "" );
};

} // namespace ecode
