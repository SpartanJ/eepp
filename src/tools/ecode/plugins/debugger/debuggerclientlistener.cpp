#include "debuggerclientlistener.hpp"
#include "../../statusappoutputcontroller.hpp"
#include "debuggerplugin.hpp"

namespace ecode {

std::vector<SourceBreakpoint>
DebuggerClientListener::fromSet( const EE::UnorderedSet<SourceBreakpointStateful>& set ) {
	std::vector<SourceBreakpoint> bps;
	bps.reserve( set.size() );
	for ( const auto& bp : set )
		if ( bp.enabled )
			bps.emplace_back( bp );
	return bps;
}

using i18nFn = std::function<String( const std::string&, const String& )>;

class ThreadsModel : public Model {
  public:
	ThreadsModel( const std::vector<Thread>& threads, i18nFn fn ) :
		mThreads( threads ), mi18nFn( std::move( fn ) ) {}

	virtual size_t rowCount( const ModelIndex& ) const { return mThreads.size(); }
	virtual size_t columnCount( const ModelIndex& ) const { return 1; }

	virtual std::string columnName( const size_t& colIdx ) const {
		switch ( colIdx ) {
			case 0:
				return mi18nFn( "thread_id", "Thread ID" );
		}
		return "";
	}

	virtual Variant data( const ModelIndex& modelIndex, ModelRole role ) const {
		if ( role == ModelRole::Display && modelIndex.column() == 0 ) {
			return Variant( String::format( "#%d (%s)", mThreads[modelIndex.row()].id,
											mThreads[modelIndex.row()].name.c_str() ) );
		}
		return {};
	}

	void setThreads( std::vector<Thread>&& threads ) {
		{
			Lock l( mResourceLock );
			mThreads = std::move( threads );
		}
		invalidate();
	}

	void resetThreads() {

		{
			Lock l( mResourceLock );
			mThreads = {};
		}
		invalidate();
	}

	const Thread& getThread( size_t index ) const {
		Lock l( mResourceLock );
		eeASSERT( index < mThreads.size() );
		return mThreads[index];
	}

	ModelIndex fromThreadId( int id ) {
		Lock l( mResourceLock );
		for ( size_t i = 0; i < mThreads.size(); i++ ) {
			const Thread& thread = mThreads[i];
			if ( thread.id == id )
				return index( i );
		}
		return {};
	}

  protected:
	std::vector<Thread> mThreads;
	i18nFn mi18nFn;
};

class StackModel : public Model {
  public:
	StackModel( StackTraceInfo&& stack, i18nFn fn ) :
		mStack( std::move( stack ) ), mi18nFn( std::move( fn ) ) {}

	virtual size_t rowCount( const ModelIndex& ) const {
		Lock l( mResourceLock );
		return mStack.stackFrames.size();
	}

	virtual size_t columnCount( const ModelIndex& ) const { return 5; }

	virtual std::string columnName( const size_t& colIdx ) const {
		switch ( colIdx ) {
			case 0:
				return mi18nFn( "id", "ID" );
			case 1:
				return mi18nFn( "name", "Name" );
			case 2:
				return mi18nFn( "source_name", "Source Name" );
			case 3:
				return mi18nFn( "source_path", "Source Path" );
			case 4:
				return mi18nFn( "line", "Line" );
		}
		return "";
	}

	virtual Variant data( const ModelIndex& modelIndex, ModelRole role ) const {
		Lock l( mResourceLock );
		if ( role == ModelRole::Display ) {
			switch ( modelIndex.column() ) {
				case 0:
					return Variant( String::toString( mStack.stackFrames[modelIndex.row()].id ) );
				case 1:
					return Variant( mStack.stackFrames[modelIndex.row()].name.c_str() );
				case 2:
					return mStack.stackFrames[modelIndex.row()].source
							   ? Variant( mStack.stackFrames[modelIndex.row()].source->name )
							   : Variant();
				case 3:
					return mStack.stackFrames[modelIndex.row()].source
							   ? Variant( mStack.stackFrames[modelIndex.row()].source->path )
							   : Variant();
				case 4:
					return Variant( String::toString( mStack.stackFrames[modelIndex.row()].line ) );
				case 5:
					return Variant(
						String::toString( mStack.stackFrames[modelIndex.row()].column ) );
			}
		}
		return {};
	}

	void setStack( StackTraceInfo&& stack ) {
		{
			Lock l( mResourceLock );
			mStack = std::move( stack );
		}
		invalidate();
	}

	void resetStack() {

		{
			Lock l( mResourceLock );
			mStack = {};
		}
		invalidate();
	}

	const StackFrame& getStack( size_t index ) const {
		Lock l( mResourceLock );
		eeASSERT( index < mStack.stackFrames.size() );
		return mStack.stackFrames[index];
	}

  protected:
	StackTraceInfo mStack;
	i18nFn mi18nFn;
};

DebuggerClientListener::DebuggerClientListener( DebuggerClient* client, DebuggerPlugin* plugin ) :
	mClient( client ), mPlugin( plugin ) {
	eeASSERT( mClient && mPlugin );
}

DebuggerClientListener::~DebuggerClientListener() {
	resetState();
	auto sdc = getStatusDebuggerController();
	if ( !mPlugin->isShuttingDown() && sdc ) {
		if ( sdc->getUIThreads() )
			sdc->getUIThreads()->removeEventsOfType( Event::OnModelEvent );
		if ( sdc->getUIStack() )
			sdc->getUIStack()->removeEventsOfType( Event::OnModelEvent );
		mPlugin->setUIDebuggingState( StatusDebuggerController::State::NotStarted );
	}
}

void DebuggerClientListener::stateChanged( DebuggerClient::State state ) {
	if ( state == DebuggerClient::State::Initializing ) {
		mPlugin->getManager()->getUISceneNode()->runOnMainThread( [this] {
			getStatusDebuggerController()->createWidget();

			mPlugin->getManager()
				->getPluginContext()
				->getStatusAppOutputController()
				->initNewOutput( {}, false );

			UISceneNode* sceneNode = mPlugin->getUISceneNode();

			if ( !mThreadsModel ) {
				mThreadsModel = std::make_shared<ThreadsModel>(
					std::vector<Thread>{}, [sceneNode]( const auto& key, const auto& val ) {
						return sceneNode->i18n( key, val );
					} );
			}

			if ( !mStackModel ) {
				mStackModel = std::make_shared<StackModel>(
					StackTraceInfo{}, [sceneNode]( const auto& key, const auto& val ) {
						return sceneNode->i18n( key, val );
					} );
			}

			UITableView* uiThreads = getStatusDebuggerController()->getUIThreads();
			uiThreads->setModel( mThreadsModel );

			uiThreads->removeEventsOfType( Event::OnModelEvent );
			uiThreads->onModelEvent( [this]( const ModelEvent* modelEvent ) {
				if ( modelEvent->getModelEventType() == Abstract::ModelEventType::Open ) {
					auto model = static_cast<const ThreadsModel*>( modelEvent->getModel() );
					mClient->stackTrace( model->getThread( modelEvent->getModelIndex().row() ).id );
				}
			} );

			UITableView* uiStack = getStatusDebuggerController()->getUIStack();
			uiStack->setModel( mStackModel );
			uiStack->removeEventsOfType( Event::OnModelEvent );
			uiStack->onModelEvent( [this]( const ModelEvent* modelEvent ) {
				if ( modelEvent->getModelEventType() == Abstract::ModelEventType::Open ) {
					auto model = static_cast<const StackModel*>( modelEvent->getModel() );
					const auto& stack = model->getStack( modelEvent->getModelIndex().row() );
					changeScope( stack );
				}
			} );

			mPlugin->setUIDebuggingState( StatusDebuggerController::State::Running );
		} );
	}
}

void DebuggerClientListener::initialized() {
	Lock l( mPlugin->mBreakpointsMutex );
	for ( const auto& fileBps : mPlugin->mBreakpoints )
		mClient->setBreakpoints( fileBps.first, fromSet( fileBps.second ) );
}

void DebuggerClientListener::launched() {}

void DebuggerClientListener::configured() {}

void DebuggerClientListener::failed() {}

void DebuggerClientListener::debuggeeRunning() {}

void DebuggerClientListener::debuggeeTerminated() {}

void DebuggerClientListener::capabilitiesReceived( const Capabilities& /*capabilities*/ ) {}

void DebuggerClientListener::resetState() {
	mStoppedData = {};
	mCurrentScopePos = {};
	if ( mThreadsModel )
		mThreadsModel->resetThreads();
	if ( mStackModel )
		mStackModel->resetStack();
	mScope.clear();
}

void DebuggerClientListener::debuggeeExited( int /*exitCode*/ ) {
	mPlugin->exitDebugger();
	resetState();
}

void DebuggerClientListener::debuggeeStopped( const StoppedEvent& event ) {
	Log::debug( "DebuggerClientListener::debuggeeStopped: reason %s", event.reason );

	mStoppedData = event;
	changeThread( mStoppedData->threadId ? *mStoppedData->threadId : 1 );

	if ( mPausedToRefreshBreakpoints ) {
		mPlugin->sendPendingBreakpoints();
		mClient->resume( 1 );
		mPausedToRefreshBreakpoints = false;
		return;
	}

	mClient->threads();
	mClient->stackTrace( mCurrentThreadId );

	UISceneNode* sceneNode = mPlugin->getUISceneNode();
	sceneNode->runOnMainThread( [sceneNode] { sceneNode->getWindow()->raise(); } );

	mPlugin->setUIDebuggingState( StatusDebuggerController::State::Paused );
}

void DebuggerClientListener::debuggeeContinued( const ContinuedEvent& ) {
	resetState();

	UISceneNode* sceneNode = mPlugin->getUISceneNode();
	sceneNode->runOnMainThread( [sceneNode] { sceneNode->invalidateDraw(); } );

	mPlugin->setUIDebuggingState( StatusDebuggerController::State::Running );
}

void DebuggerClientListener::outputProduced( const Output& output ) {
	if ( Output::Category::Stdout == output.category ||
		 Output::Category::Stderr == output.category ) {
		mPlugin->getPluginContext()->getStatusAppOutputController()->insertBuffer( output.output );
	}
}

void DebuggerClientListener::debuggingProcess( const ProcessInfo& ) {}

void DebuggerClientListener::errorResponse( const std::string& /*summary*/,
											const std::optional<Message>& /*message*/ ) {}

void DebuggerClientListener::threadChanged( const ThreadEvent& ) {}

void DebuggerClientListener::moduleChanged( const ModuleEvent& ) {}

void DebuggerClientListener::threads( std::vector<Thread>&& threads ) {
	mThreadsModel->setThreads( std::move( threads ) );
}

void DebuggerClientListener::changeScope( const StackFrame& f ) {
	mClient->scopes( f.id );

	if ( !f.source )
		return;

	TextRange range{ { f.line - 1, f.column }, { f.line - 1, f.column } };
	std::string path( f.source->path );

	mPlugin->getUISceneNode()->runOnMainThread(
		[this, path, range] { mPlugin->getPluginContext()->focusOrLoadFile( path, range ); } );

	mCurrentScopePos = { f.source->path, f.line };

	if ( getStatusDebuggerController() && getStatusDebuggerController()->getUIStack() )
		getStatusDebuggerController()->getUIStack()->setSelection( mStackModel->index( f.id ) );
}

void DebuggerClientListener::changeThread( int id ) {
	mCurrentThreadId = id;
	if ( getStatusDebuggerController() && getStatusDebuggerController()->getUIThreads() ) {
		getStatusDebuggerController()->getUIThreads()->setSelection(
			mThreadsModel->fromThreadId( id ) );
	}
}

void DebuggerClientListener::stackTrace( const int threadId, StackTraceInfo&& stack ) {
	changeThread( threadId );

	for ( const auto& f : stack.stackFrames ) {
		// Jump to the first stack frame that can be read
		if ( f.source ) {
			changeScope( f );
			break;
		}
	}

	mStackModel->setStack( std::move( stack ) );
}

void DebuggerClientListener::scopes( const int /*frameId*/, std::vector<Scope>&& scopes ) {
	if ( !scopes.empty() ) {
		for ( const auto& scope : scopes ) {
			ModelScope mscope;
			mscope.name = scope.name;
			mscope.variablesReference = scope.variablesReference;
			mScope.emplace_back( std::move( mscope ) );
			mClient->variables( scope.variablesReference );
		}
	}
}

void DebuggerClientListener::variables( const int variablesReference,
										std::vector<Variable>&& vars ) {
	auto scopeIt =
		std::find_if( mScope.begin(), mScope.end(), [variablesReference]( const ModelScope& cur ) {
			return cur.variablesReference == variablesReference;
		} );
	if ( scopeIt == mScope.end() )
		return;
	scopeIt->variables = vars;
}

void DebuggerClientListener::modules( ModulesInfo&& ) {}

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

bool DebuggerClientListener::isStopped() const {
	return mStoppedData ? true : false;
}

std::optional<StoppedEvent> DebuggerClientListener::getStoppedData() const {
	return mStoppedData;
}

int DebuggerClientListener::getCurrentThreadId() const {
	return mCurrentThreadId;
}

std::optional<std::pair<std::string, int>> DebuggerClientListener::getCurrentScopePos() const {
	return mCurrentScopePos;
}

StatusDebuggerController* DebuggerClientListener::getStatusDebuggerController() const {
	return mPlugin->getStatusDebuggerController();
}

} // namespace ecode
