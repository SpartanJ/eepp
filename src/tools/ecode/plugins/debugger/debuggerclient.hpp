#pragma once
#include "dap/protocol.hpp"
#include <string>

namespace ecode {

using namespace dap;

class DebuggerClient {
  public:
	enum class State { None, Initializing, Initialized, Running, Terminated, Failed };

	class Listener {
	  public:
		virtual void stateChanged( State ) = 0;
		virtual void initialized() = 0;
		virtual void launched() = 0;
		virtual void configured() = 0;
		virtual void failed() = 0;
		virtual void debuggeeRunning() = 0;
		virtual void debuggeeTerminated() = 0;

		virtual void capabilitiesReceived( const Capabilities& capabilities ) = 0;
		virtual void debuggeeExited( int exitCode ) = 0;
		virtual void debuggeeStopped( const StoppedEvent& ) = 0;
		virtual void debuggeeContinued( const ContinuedEvent& ) = 0;
		virtual void outputProduced( const Output& ) = 0;
		virtual void debuggingProcess( const ProcessInfo& ) = 0;
		virtual void errorResponse( const std::string& summary,
									const std::optional<Message>& message ) = 0;
		virtual void threadChanged( const ThreadEvent& ) = 0;
		virtual void moduleChanged( const ModuleEvent& ) = 0;
		virtual void threads( std::vector<DapThread>&& ) = 0;
		virtual void stackTrace( const int threadId, StackTraceInfo&& ) = 0;
		virtual void scopes( const int frameId, std::vector<Scope>&& ) = 0;
		virtual void variables( const int variablesReference, std::vector<Variable>&& ) = 0;
		virtual void modules( ModulesInfo&& ) = 0;
		virtual void serverDisconnected() = 0;
		virtual void sourceContent( const std::string& path, int reference = 0,
									const SourceContent& content = SourceContent() ) = 0;
		virtual void
		sourceBreakpoints( const std::string& path, int reference,
						   const std::optional<std::vector<Breakpoint>>& breakpoints ) = 0;
		virtual void breakpointChanged( const BreakpointEvent& ) = 0;
		virtual void expressionEvaluated( const std::string& expression,
										  const std::optional<EvaluateInfo>& ) = 0;
		virtual void gotoTargets( const Source& source, const int line,
								  const std::vector<GotoTarget>& targets ) = 0;
	};

	State state() const { return mState; }

	virtual bool start() = 0;

	virtual bool started() const = 0;

	virtual bool resume( int threadId, bool singleThread = false ) = 0;

	virtual bool pause( int threadId ) = 0;

	virtual bool stepOver( int threadId, bool singleThread = false ) = 0;

	virtual bool goTo( int threadId, int targetId ) = 0;

	virtual bool stepInto( int threadId, bool singleThread = false ) = 0;

	virtual bool stepOut( int threadId, bool singleThread = false ) = 0;

	virtual bool terminate( bool restart ) = 0;

	virtual bool disconnect( bool restart = false ) = 0;

	virtual bool threads() = 0;

	virtual bool stackTrace( int threadId, int startFrame = 0, int levels = 0 ) = 0;

	virtual bool scopes( int frameId ) = 0;

	virtual bool modules( int start, int count ) = 0;

	virtual bool variables( int variablesReference, Variable::Type filter = Variable::Type::Both,
							int start = 0, int count = 0 ) = 0;

	virtual bool evaluate( const std::string& expression, const std::string& context,
						   std::optional<int> frameId ) = 0;

	virtual bool isServerConnected() const = 0;

	virtual bool supportsTerminate() const = 0;

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

	virtual bool configurationDone() = 0;

	void addListener( Listener* listener );

	void removeListener( Listener* listener );

	virtual ~DebuggerClient() {}

  protected:
	void setState( const State& state );

	State mState{ State::None };
	bool mLaunched{ false };
	bool mConfigured{ false };
	std::vector<Listener*> mListeners;

	void checkRunning();

	void stateChanged( State );
	void initialized();
	void debuggeeRunning();
	void debuggeeTerminated();
	void failed();
};

} // namespace ecode
