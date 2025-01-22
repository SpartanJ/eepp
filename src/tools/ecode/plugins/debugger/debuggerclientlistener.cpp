#include "debuggerclientlistener.hpp"
#include "../../notificationcenter.hpp"
#include "../../statusappoutputcontroller.hpp"
#include "debuggerplugin.hpp"
#include "eepp/window/clipboard.hpp"
#include "models/stackmodel.hpp"
#include "models/threadsmodel.hpp"
#include "models/variablesmodel.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/window/input.hpp>

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

DebuggerClientListener::DebuggerClientListener( DebuggerClient* client, DebuggerPlugin* plugin ) :
	mClient( client ),
	mPlugin( plugin ),
	mVariablesHolder( std::make_shared<VariablesHolder>( plugin->getUISceneNode() ) ) {
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

void DebuggerClientListener::initUI() {
	auto sdc = getStatusDebuggerController();

	sdc->createWidget();

	mPlugin->getManager()->getPluginContext()->getStatusAppOutputController()->initNewOutput(
		{}, false );

	sdc->clearConsoleBuffer();

	UISceneNode* sceneNode = mPlugin->getUISceneNode();

	if ( !mThreadsModel ) {
		mThreadsModel = std::make_shared<ThreadsModel>( std::vector<DapThread>{}, sceneNode );
		mThreadsModel->setCurrentThreadId( mCurrentThreadId );
	}

	if ( !mStackModel ) {
		mStackModel = std::make_shared<StackModel>( StackTraceInfo{}, sceneNode );
	}

	UITableView* uiThreads = sdc->getUIThreads();
	uiThreads->setModel( mThreadsModel );

	uiThreads->removeEventsOfType( Event::OnModelEvent );
	uiThreads->onModelEvent( [this]( const ModelEvent* modelEvent ) {
		if ( modelEvent->getModelEventType() == Abstract::ModelEventType::Open ) {
			auto model = static_cast<const ThreadsModel*>( modelEvent->getModel() );
			mClient->stackTrace( model->getThread( modelEvent->getModelIndex().row() ).id );
		}
	} );

	UITableView* uiStack = sdc->getUIStack();
	uiStack->setModel( mStackModel );
	uiStack->removeEventsOfType( Event::OnModelEvent );
	uiStack->onModelEvent( [this]( const ModelEvent* modelEvent ) {
		if ( modelEvent->getModelEventType() == Abstract::ModelEventType::Open ) {
			auto model = static_cast<const StackModel*>( modelEvent->getModel() );
			const auto& stack = model->getStack( modelEvent->getModelIndex().row() );
			changeScope( stack );
		}
	} );

	UITreeView* uiVariables = sdc->getUIVariables();
	uiVariables->setModel( mVariablesHolder->getModel() );
	uiVariables->removeEventsOfType( Event::OnModelEvent );
	uiVariables->onModelEvent( [this]( const ModelEvent* modelEvent ) {
		auto idx( modelEvent->getModelIndex() );
		if ( modelEvent->getModelEventType() == Abstract::ModelEventType::OpenTree ) {
			ModelVariableNode* node = static_cast<ModelVariableNode*>( idx.internalData() );
			mClient->variables( node->var.variablesReference );
			mVariablesHolder->saveExpandedState( idx );
		} else if ( modelEvent->getModelEventType() == Abstract::ModelEventType::CloseTree ) {
			mVariablesHolder->removeExpandedState( idx );
		} else if ( modelEvent->getModelEventType() == Abstract::ModelEventType::OpenMenu &&
					idx.isValid() ) {

			auto context = mPlugin->getPluginContext();
			UIPopUpMenu* menu = UIPopUpMenu::New();

			ModelVariableNode* node = static_cast<ModelVariableNode*>( idx.internalData() );
			Variable var( node->var );

			menu->add( context->i18n( "debugger_copy_variable_value", "Copy Value" ),
					   context->findIcon( "copy" ) )
				->setId( "debugger_copy_variable_value" );

			menu->add( context->i18n( "debugger_copy_variable_name", "Copy Name" ),
					   context->findIcon( "copy" ) )
				->setId( "debugger_copy_variable_name" );

			if ( var.type ) {
				menu->add( context->i18n( "debugger_copy_variable_type", "Copy Type" ),
						   context->findIcon( "copy" ) )
					->setId( "debugger_copy_variable_type" );
			}

			if ( var.evaluateName ) {
				menu->add( context->i18n( "debugger_copy_variable_evaluate_name",
										  "Copy Evaluate Name" ),
						   context->findIcon( "copy" ) )
					->setId( "debugger_copy_variable_evaluate_name" );
			}

			if ( var.memoryReference ) {
				menu->add( context->i18n( "debugger_copy_variable_memory_reference",
										  "Copy Memory Reference" ),
						   context->findIcon( "copy" ) )
					->setId( "debugger_copy_variable_memory_reference" );
			}

			menu->on( Event::OnItemClicked, [this, var = std::move( var )]( const Event* event ) {
				UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
				std::string id( item->getId() );
				if ( id == "debugger_copy_variable_value" ) {
					mPlugin->getUISceneNode()->getWindow()->getClipboard()->setText( var.value );
				} else if ( id == "debugger_copy_variable_name" ) {
					mPlugin->getUISceneNode()->getWindow()->getClipboard()->setText( var.name );
				} else if ( id == "debugger_copy_variable_type" ) {
					mPlugin->getUISceneNode()->getWindow()->getClipboard()->setText( *var.type );
				} else if ( id == "debugger_copy_variable_evaluate_name" ) {
					mPlugin->getUISceneNode()->getWindow()->getClipboard()->setText(
						*var.evaluateName );
				} else if ( id == "debugger_copy_variable_memory_reference" ) {
					mPlugin->getUISceneNode()->getWindow()->getClipboard()->setText(
						*var.memoryReference );
				}
			} );

			Vector2f pos( context->getWindow()->getInput()->getMousePos().asFloat() );
			menu->nodeToWorldTranslation( pos );
			UIMenu::findBestMenuPos( pos, menu );
			menu->setPixelsPosition( pos );
			menu->show();
		}
	} );

	mPlugin->setUIDebuggingState( StatusDebuggerController::State::Running );
}

void DebuggerClientListener::stateChanged( DebuggerClient::State state ) {
	if ( state == DebuggerClient::State::Initializing ) {
		mPlugin->getManager()->getUISceneNode()->runOnMainThread( [this] { initUI(); } );
	}
}

void DebuggerClientListener::sendBreakpoints() {
	Lock l( mPlugin->mBreakpointsMutex );
	for ( const auto& fileBps : mPlugin->mBreakpoints ) {
		std::string path( fileBps.first );
		if ( isRemote() ) {
			FileSystem::filePathRemoveBasePath( mPlugin->mProjectPath, path );
			path = "/" + path;
		}
		mClient->setBreakpoints( path, fromSet( fileBps.second ) );
	}
}

void DebuggerClientListener::initialized() {
	sendBreakpoints();
}

void DebuggerClientListener::launched() {}

void DebuggerClientListener::configured() {}

void DebuggerClientListener::failed() {
	mPlugin->exitDebugger();
	resetState();
}

void DebuggerClientListener::debuggeeRunning() {}

void DebuggerClientListener::debuggeeTerminated() {
	mPlugin->exitDebugger();
	resetState();
}

void DebuggerClientListener::capabilitiesReceived( const Capabilities& /*capabilities*/ ) {}

void DebuggerClientListener::resetState() {
	mStoppedData = {};
	mCurrentScopePos = {};
	mCurrentFrameId = 0;
	if ( mThreadsModel )
		mThreadsModel->resetThreads();
	if ( mStackModel )
		mStackModel->resetStack();
	mVariablesHolder->clear();
}

void DebuggerClientListener::debuggeeExited( int /*exitCode*/ ) {
	mPlugin->exitDebugger();
	resetState();
}

void DebuggerClientListener::debuggeeStopped( const StoppedEvent& event ) {
	Log::debug( "DebuggerClientListener::debuggeeStopped: reason %s", event.reason );

	if ( "exception" == event.reason ) {
		if ( event.description ) {
			mPlugin->getPluginContext()->getNotificationCenter()->addNotification(
				String::format( mPlugin
									->i18n( "debuggee_exception_triggered_desc",
											"Debuggee triggered an exception: %s" )
									.toUtf8(),
								*event.description ) );
		} else {
			mPlugin->getPluginContext()->getNotificationCenter()->addNotification( mPlugin->i18n(
				"debuggee_exception_triggered", "Debuggee triggered an exception" ) );
		}
	}

	mCurrentThreadId = mStoppedData->threadId ? *mStoppedData->threadId : 1;
	mStoppedData = event;

	if ( mPausedToRefreshBreakpoints ) {
		mPlugin->sendPendingBreakpoints();
		mPausedToRefreshBreakpoints = false;
		return;
	}

	changeThread( mStoppedData->threadId ? *mStoppedData->threadId : 1 );

	mClient->threads();
	mClient->stackTrace( mCurrentThreadId );

	UISceneNode* sceneNode = mPlugin->getUISceneNode();
	sceneNode->runOnMainThread( [sceneNode] { sceneNode->getWindow()->raise(); } );

	mPlugin->setUIDebuggingState( StatusDebuggerController::State::Paused );

	auto sdc = getStatusDebuggerController();
	if ( sdc )
		sdc->getWidget()->runOnMainThread( [sdc] { sdc->show(); } );
}

void DebuggerClientListener::debuggeeContinued( const ContinuedEvent& ) {
	resetState();

	UISceneNode* sceneNode = mPlugin->getUISceneNode();
	sceneNode->runOnMainThread( [sceneNode] { sceneNode->getRoot()->invalidateDraw(); } );

	mPlugin->setUIDebuggingState( StatusDebuggerController::State::Running );
}

void DebuggerClientListener::outputProduced( const Output& output ) {
	if ( Output::Category::Stdout == output.category ||
		 Output::Category::Stderr == output.category ) {
		mPlugin->getPluginContext()->getStatusAppOutputController()->insertBuffer( output.output );
	} else if ( Output::Category::Console == output.category ) {
		auto buffer = output.output;
		auto sdc = getStatusDebuggerController();
		if ( sdc == nullptr || sdc->getUIConsole() == nullptr ) {
			mPlugin->getUISceneNode()->runOnMainThread(
				[this, buffer = std::move( buffer )]() mutable {
					mPlugin->initStatusDebuggerController();
					auto sdc = getStatusDebuggerController();
					sdc->insertConsoleBuffer( std::move( buffer ) );
				} );
			return;
		} else {
			sdc->insertConsoleBuffer( std::move( buffer ) );
		}
	}
}

void DebuggerClientListener::debuggingProcess( const ProcessInfo& ) {}

void DebuggerClientListener::errorResponse( const std::string& command, const std::string& summary,
											const std::optional<Message>& /*message*/ ) {
	if ( command == "evaluate" )
		return;

	if ( command == "launch" )
		failed();

	mPlugin->getPluginContext()->getNotificationCenter()->addNotification( summary, Seconds( 5 ),
																		   true );
}

void DebuggerClientListener::threadChanged( const ThreadEvent& ) {}

void DebuggerClientListener::moduleChanged( const ModuleEvent& ) {}

void DebuggerClientListener::threads( std::vector<DapThread>&& threads ) {
	std::sort( threads.begin(), threads.end(),
			   []( const DapThread& a, const DapThread& b ) { return a.id < b.id; } );

	mThreadsModel->setThreads( std::move( threads ) );
}

void DebuggerClientListener::changeScope( const StackFrame& f ) {
	mCurrentFrameId = f.id;
	mClient->scopes( f.id );

	if ( mStackModel )
		mStackModel->setCurrentScopeId( f.id );

	if ( !f.source )
		return;

	TextRange range{ { f.line - 1, f.column }, { f.line - 1, f.column } };
	std::string path( f.source->path );

	mPlugin->getUISceneNode()->runOnMainThread(
		[this, path, range] { mPlugin->getPluginContext()->focusOrLoadFile( path, range ); } );

	mCurrentScopePos = { f.source->path, f.line };

	auto sdc = getStatusDebuggerController();
	if ( sdc && sdc->getUIStack() )
		sdc->getUIStack()->setSelection( mStackModel->index( f.id ) );
}

void DebuggerClientListener::changeThread( int id ) {
	mCurrentThreadId = id;
	if ( mThreadsModel )
		mThreadsModel->setCurrentThreadId( id );

	auto sdc = getStatusDebuggerController();
	if ( sdc && sdc->getUIThreads() )
		sdc->getUIThreads()->setSelection( mThreadsModel->fromThreadId( id ) );
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

	for ( const auto& expression : mPlugin->mExpressions ) {
		mClient->evaluate(
			expression, "watch", getCurrentFrameId(),
			[this, expression]( const std::string&, const std::optional<EvaluateInfo>& info ) {
				Variable var;
				var.evaluateName = expression;
				var.name = std::move( expression );
				if ( info ) {
					var.value = info->result;
					var.type = info->type;
					var.variablesReference = info->variablesReference;
					var.indexedVariables = info->indexedVariables;
					var.namedVariables = info->namedVariables;
					var.memoryReference = info->memoryReference;
				}
				mPlugin->mExpressionsHolder->upsertRootChild( std::move( var ) );
				ExpandedState::Location location{ mCurrentScopePos->first, mCurrentScopePos->second,
												  mCurrentFrameId };
				mPlugin->mExpressionsHolder->restoreExpandedState(
					location, mClient, getStatusDebuggerController()->getUIExpressions(), true );
			} );
	}
}

void DebuggerClientListener::scopes( const int /*frameId*/, std::vector<Scope>&& scopes ) {
	if ( scopes.empty() )
		return;

	mVariablesHolder->clear();

	for ( const auto& scope : scopes ) {
		auto child = std::make_shared<ModelVariableNode>( scope.name, scope.variablesReference );
		mVariablesHolder->addChild( child );
		if ( ( !mPlugin->mFetchRegisters && scope.name == "Registers" ) ||
			 ( !mPlugin->mFetchGlobals && scope.name == "Globals" ) )
			continue;
		mClient->variables( scope.variablesReference );
	}

	for ( auto& scope : scopes )
		mScopeRef[scope.variablesReference] = std::move( scope );

	auto sdc = getStatusDebuggerController();
	if ( nullptr == sdc )
		return;
	auto uiVars = sdc->getUIVariables();
	if ( uiVars ) {
		uiVars->runOnMainThread( [this, uiVars] {
			auto model = uiVars->getModel();
			size_t total = model->rowCount();
			for ( size_t i = 0; i < total; i++ ) {
				auto index = model->index( i, uiVars->getMainColumn() );
				ModelVariableNode* node = static_cast<ModelVariableNode*>( index.internalData() );
				if ( ( !mPlugin->mFetchRegisters && node->var.name == "Registers" ) ||
					 ( !mPlugin->mFetchGlobals && node->var.name == "Globals" ) )
					continue;
				uiVars->tryOpenModelIndex( index );
			}
		} );
	}
}

void DebuggerClientListener::variables( const int variablesReference,
										std::vector<Variable>&& vars ) {
	mVariablesHolder->addVariables( variablesReference, std::move( vars ) );

	auto scopeIt = mScopeRef.find( variablesReference );
	if ( mCurrentScopePos && scopeIt != mScopeRef.end() ) {
		ExpandedState::Location location{ mCurrentScopePos->first, mCurrentScopePos->second,
										  mCurrentFrameId };
		mVariablesHolder->restoreExpandedState( location, mClient,
												getStatusDebuggerController()->getUIVariables() );
	}
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

bool DebuggerClientListener::isRemote() const {
	return mIsRemote;
}

bool DebuggerClientListener::isStopped() const {
	return mStoppedData ? true : false;
}

std::optional<StoppedEvent> DebuggerClientListener::getStoppedData() const {
	return mStoppedData;
}

void DebuggerClientListener::setPausedToRefreshBreakpoints() {
	mPausedToRefreshBreakpoints = true;
}

int DebuggerClientListener::getCurrentThreadId() const {
	return mCurrentThreadId;
}

int DebuggerClientListener::getCurrentFrameId() const {
	return mCurrentFrameId;
}

std::optional<std::pair<std::string, int>> DebuggerClientListener::getCurrentScopePos() const {
	return mCurrentScopePos;
}

void DebuggerClientListener::setIsRemote( bool isRemote ) {
	mIsRemote = isRemote;
}

StatusDebuggerController* DebuggerClientListener::getStatusDebuggerController() const {
	return mPlugin->getStatusDebuggerController();
}

} // namespace ecode
