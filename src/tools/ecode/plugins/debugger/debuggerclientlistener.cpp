#include "debuggerclientlistener.hpp"
#include "../../statusappoutputcontroller.hpp"
#include "debuggerplugin.hpp"

namespace ecode {

static std::vector<SourceBreakpoint> fromSet( const UnorderedSet<SourceBreakpointStateful>& set ) {
	std::vector<SourceBreakpoint> bps;
	bps.reserve( set.size() );
	for ( const auto& bp : set )
		if ( bp.enabled )
			bps.emplace_back( bp );
	return bps;
}

DebuggerClientListener::DebuggerClientListener( DebuggerClient* client, DebuggerPlugin* plugin ) :
	mClient( client ), mPlugin( plugin ) {
	eeASSERT( mClient && mPlugin );
}

void DebuggerClientListener::stateChanged( DebuggerClient::State state ) {
	if ( state == DebuggerClient::State::Initializing ) {
		mPlugin->getManager()->getUISceneNode()->runOnMainThread( [this] {
			getStatusDebuggerController()->createWidget();

			mPlugin->getManager()
				->getPluginContext()
				->getStatusAppOutputController()
				->initNewOutput( {}, false );
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

void DebuggerClientListener::debuggeeExited( int /*exitCode*/ ) {
	mPlugin->exitDebugger();
}

void DebuggerClientListener::debuggeeStopped( const StoppedEvent& event ) {
	Log::debug( "DebuggerClientListener::debuggeeStopped: reason %s", event.reason );
	mClient->threads();
	if ( event.threadId )
		mClient->stackTrace( *event.threadId );
}

void DebuggerClientListener::debuggeeContinued( const ContinuedEvent& ) {}

void DebuggerClientListener::outputProduced( const Output& output ) {
	if ( Output::Category::Stdout == output.category ||
		 Output::Category::Stderr == output.category ) {
		mPlugin->getManager()->getPluginContext()->getStatusAppOutputController()->insertBuffer(
			output.output );
	}
}

void DebuggerClientListener::debuggingProcess( const ProcessInfo& ) {}

void DebuggerClientListener::errorResponse( const std::string& /*summary*/,
											const std::optional<Message>& /*message*/ ) {}

void DebuggerClientListener::threadChanged( const ThreadEvent& ) {}

void DebuggerClientListener::moduleChanged( const ModuleEvent& ) {}

class ThreadsModel : public Model {
  public:
	ThreadsModel( const std::vector<Thread>& threads ) : mThreads( threads ) {}
	virtual size_t rowCount( const ModelIndex& ) const { return mThreads.size(); }
	virtual size_t columnCount( const ModelIndex& ) const { return 2; }

	virtual std::string columnName( const size_t& colIdx ) const {
		return colIdx == 0 ? "ID" : "Name";
	}

	virtual Variant data( const ModelIndex& modelIndex, ModelRole role ) const {
		if ( role == ModelRole::Display ) {
			return modelIndex.column() == 0
					   ? Variant( String::toString( mThreads[modelIndex.row()].id ) )
					   : Variant( mThreads[modelIndex.row()].name.c_str() );
		}
		return {};
	}

  protected:
	std::vector<Thread> mThreads;
};

void DebuggerClientListener::threads( const std::vector<Thread>& threads ) {
	getStatusDebuggerController()->getUIThreads()->setModel(
		std::make_shared<ThreadsModel>( threads ) );
}

class StackModel : public Model {
  public:
	StackModel( const StackTraceInfo& stack ) : mStack( stack ) {}
	virtual size_t rowCount( const ModelIndex& ) const { return mStack.stackFrames.size(); }
	virtual size_t columnCount( const ModelIndex& ) const { return 5; }

	virtual std::string columnName( const size_t& colIdx ) const {
		switch ( colIdx ) {
			case 0:
				return "ID";
			case 1:
				return "Name";
			case 2:
				return "Source Name";
			case 3:
				return "Source Path";
			case 4:
				return "Line";
			case 5:
				return "Column";
		}
		return "";
	}

	virtual Variant data( const ModelIndex& modelIndex, ModelRole role ) const {
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

  protected:
	StackTraceInfo mStack;
};

void DebuggerClientListener::stackTrace( const int /*threadId*/, const StackTraceInfo& stack ) {
	getStatusDebuggerController()->getUIStack()->setModel( std::make_shared<StackModel>( stack ) );

	if ( !stack.stackFrames.empty() ) {
		mClient->scopes( stack.stackFrames[0].id );
	}
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

StatusDebuggerController* DebuggerClientListener::getStatusDebuggerController() const {

	auto debuggerElement =
		mPlugin->getManager()->getPluginContext()->getStatusBar()->getStatusBarElement(
			"status_app_debugger" );

	if ( !debuggerElement )
		return nullptr;
	return static_cast<StatusDebuggerController*>( debuggerElement.get() );
}

} // namespace ecode
