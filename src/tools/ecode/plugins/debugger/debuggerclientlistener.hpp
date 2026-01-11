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
	fromSet( const UnorderedSet<SourceBreakpointStateful>& set );

	DebuggerClientListener( DebuggerClient* client, DebuggerPlugin* plugin );

	virtual ~DebuggerClientListener();

	void stateChanged( DebuggerClient::State, const SessionId& sessionId );

	void initialized( const SessionId& sessionId );

	void launched( const SessionId& sessionId );

	void configured( const SessionId& sessionId );

	void failed( const SessionId& sessionId );

	void debuggeeRunning( const SessionId& sessionId );

	void debuggeeTerminated( const SessionId& sessionId );

	void capabilitiesReceived( const Capabilities& capabilities );

	void debuggeeExited( int exitCode, const SessionId& sessionId );

	void debuggeeStopped( const StoppedEvent&, const SessionId& sessionId );

	void debuggeeContinued( const ContinuedEvent&, const SessionId& sessionId );

	void outputProduced( const Output& );
	void debuggingProcess( const ProcessInfo&, const SessionId& sessionId );

	void errorResponse( const std::string& command, const std::string& summary,
						const std::optional<Message>& message, const SessionId& sessionId );

	void threadChanged( const ThreadEvent&, const SessionId& sessionId );

	void moduleChanged( const ModuleEvent&, const SessionId& sessionId );

	void threads( std::vector<DapThread>&&, const SessionId& sessionId );

	void stackTrace( const int threadId, StackTraceInfo&&, const SessionId& sessionId );

	void scopes( const int frameId, std::vector<Scope>&&, const SessionId& sessionId );

	void variables( const int variablesReference, std::vector<Variable>&&,
					const SessionId& sessionId );

	void modules( ModulesInfo&&, const SessionId& sessionId );

	void serverDisconnected( const SessionId& sessionId );

	void sourceContent( const std::string& path, int reference,
						const SourceContent& content = SourceContent(),
						const SessionId& sessionId = "" );

	void sourceBreakpoints( const std::string& path, int reference,
							const std::optional<std::vector<Breakpoint>>& breakpoints,
							const SessionId& sessionId );

	void breakpointChanged( const BreakpointEvent&, const SessionId& sessionId );

	void expressionEvaluated( const std::string& expression, const std::optional<EvaluateInfo>&,
							  const SessionId& sessionId );

	void gotoTargets( const Source& source, const int line, const std::vector<GotoTarget>& targets,
					  const SessionId& sessionId );

	void evaluateExpression( const std::string& expression );

	void evaluateExpressions();

	bool isRemote() const;

	bool isStopped() const;

	std::optional<StoppedEvent> getStoppedData() const;

	void setPausedToRefreshBreakpoints();

	int getCurrentThreadId() const;

	int getCurrentFrameId() const;

	std::optional<std::pair<std::string, int>> getCurrentScopePos() const;

	int getCurrentScopePosLine() const;

	bool isCurrentScopePos( const std::string& filePath ) const;

	bool isCurrentScopePos( const std::string& filePath, int index ) const;

	void setIsRemote( bool isRemote );

	void sendBreakpoints();

	const ProcessInfo& getProcessInfo() const { return mProcessInfo; }

	const std::string& localRoot() const;

	void setLocalRoot( const std::string& newLocalRoot );

	const std::string& remoteRoot() const;

	void setRemoteRoot( const std::string& newRemoteRoot );

	bool isUnstableFrameId() const;

	void setUnstableFrameId( bool unstableFrameId );

	void createAndShowVariableMenu( ModelIndex idx );

  protected:
	mutable Mutex mMutex;
	DebuggerClient* mClient{ nullptr };
	DebuggerPlugin* mPlugin{ nullptr };
	std::optional<StoppedEvent> mStoppedData;
	std::optional<std::pair<std::string, int>> mCurrentScopePos;
	bool mPausedToRefreshBreakpoints{ false };
	bool mIsRemote{ false };
	bool mUnstableFrameId{ false };
	int mCurrentThreadId{ 1 };
	int mCurrentFrameId{ 0 };
	std::shared_ptr<ThreadsModel> mThreadsModel;
	std::shared_ptr<StackModel> mStackModel;
	std::shared_ptr<VariablesHolder> mVariablesHolder;
	std::unordered_map<int, Scope> mScopeRef;
	ProcessInfo mProcessInfo;
	std::string mLocalRoot;
	std::string mRemoteRoot;

	StatusDebuggerController* getStatusDebuggerController() const;

	void resetState( bool full );

	void changeScope( const StackFrame& f );

	void changeThread( int id );

	void initUI();
};

} // namespace ecode
