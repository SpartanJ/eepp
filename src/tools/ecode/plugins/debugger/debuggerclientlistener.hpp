#pragma once
#include "debuggerclient.hpp"
#include "statusdebuggercontroller.hpp"

namespace ecode {

class DebuggerPlugin;
class ThreadsModel;
class StackModel;
class VariablesModel;
class VariablesHolder;
struct ModelVariableNode;

class DebuggerClientListener : public DebuggerClient::Listener {
  public:
	static std::vector<SourceBreakpoint>
	fromSet( const EE::UnorderedSet<SourceBreakpointStateful>& set );

	DebuggerClientListener( DebuggerClient* client, DebuggerPlugin* plugin );

	virtual ~DebuggerClientListener();

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
	void errorResponse( const std::string& command, const std::string& summary,
						const std::optional<Message>& message );
	void threadChanged( const ThreadEvent& );
	void moduleChanged( const ModuleEvent& );
	void threads( std::vector<DapThread>&& );
	void stackTrace( const int threadId, StackTraceInfo&& );
	void scopes( const int frameId, std::vector<Scope>&& );
	void variables( const int variablesReference, std::vector<Variable>&& );
	void modules( ModulesInfo&& );
	void serverDisconnected();
	void sourceContent( const std::string& path, int reference,
						const SourceContent& content = SourceContent() );
	void sourceBreakpoints( const std::string& path, int reference,
							const std::optional<std::vector<Breakpoint>>& breakpoints );
	void breakpointChanged( const BreakpointEvent& );
	void expressionEvaluated( const std::string& expression, const std::optional<EvaluateInfo>& );
	void gotoTargets( const Source& source, const int line,
					  const std::vector<GotoTarget>& targets );

	bool isStopped() const;

	std::optional<StoppedEvent> getStoppedData() const;

	void setPausedToRefreshBreakpoints() { mPausedToRefreshBreakpoints = true; }

	int getCurrentThreadId() const;

	int getCurrentFrameId() const;

	std::optional<std::pair<std::string, int>> getCurrentScopePos() const;

  protected:
	DebuggerClient* mClient{ nullptr };
	DebuggerPlugin* mPlugin{ nullptr };
	std::optional<StoppedEvent> mStoppedData;
	std::optional<std::pair<std::string, int>> mCurrentScopePos;
	bool mPausedToRefreshBreakpoints{ false };
	int mCurrentThreadId{ 1 };
	int mCurrentFrameId{ 0 };
	std::shared_ptr<ThreadsModel> mThreadsModel;
	std::shared_ptr<StackModel> mStackModel;
	std::shared_ptr<VariablesHolder> mVariablesHolder;

	StatusDebuggerController* getStatusDebuggerController() const;

	void resetState();

	void changeScope( const StackFrame& f );

	void changeThread( int id );

	void initUI();

};

} // namespace ecode
