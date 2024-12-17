#pragma once
#include "dap/protocol.hpp"
#include <cstddef>
#include <string>

namespace ecode {

using namespace dap;

class DebuggerClient {
  public:
	enum class State { None, Initializing, Initialized, Running, Terminated, Failed };

	class Observer {
	  public:
		virtual void stateChanged( State ) = 0;
		virtual void initialized() = 0;
		virtual void launched() = 0;
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
		virtual void threads( const std::vector<Thread>& ) = 0;
		virtual void stackTrace( const int threadId, const StackTraceInfo& ) = 0;
		virtual void scopes( const int frameId, const std::vector<Scope>& ) = 0;
		virtual void variables( const int variablesReference, const std::vector<Variable>& ) = 0;
		virtual void modules( const ModulesInfo& ) = 0;
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

	virtual bool hasBreakpoint( const std::string& path, size_t line ) = 0;

	virtual bool addBreakpoint( const std::string& path, size_t line ) = 0;

	virtual bool removeBreakpoint( const std::string& path, size_t line ) = 0;

	virtual bool start() = 0;

	virtual bool attach() = 0;

	virtual bool started() const = 0;

	virtual bool cont( int threadId ) = 0;

	virtual bool pause( int threadId ) = 0;

	virtual bool next( int threadId ) = 0;

	virtual bool goTo( int threadId, int targetId ) = 0;

	virtual bool stepInto( int threadId ) = 0;

	virtual bool stepOver( int threadId ) = 0;

	virtual bool stepOut( int threadId ) = 0;

	virtual bool halt() = 0;

	virtual bool terminate() = 0;

	virtual bool stopped() = 0;

	virtual bool completed() = 0;

  protected:
	void setState( const State& state );

	State mState{ State::None };
	bool mLaunched{ false };
	bool mConfigured{ false };
	Observer* mObserver{ nullptr };

	void checkRunning();

	void stateChanged( State );
	void initialized();
	void debuggeeRunning();
	void debuggeeTerminated();
	void failed();
};

} // namespace ecode
