#include "debuggerclientlistener.hpp"
#include "../../notificationcenter.hpp"
#include "../../statusappoutputcontroller.hpp"
#include "debuggerplugin.hpp"
#include "models/stackmodel.hpp"
#include "models/threadsmodel.hpp"
#include "models/variablesmodel.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/input.hpp>

namespace ecode {

static constexpr auto SCOPES_UI_HASH = String::hash( "DebuggerClientListener::scopes::ui" );

std::vector<SourceBreakpoint>
DebuggerClientListener::fromSet( const UnorderedSet<SourceBreakpointStateful>& set ) {
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
	resetState( true );
	auto sdc = getStatusDebuggerController();
	if ( !mPlugin->isShuttingDown() && sdc ) {
		if ( sdc->getUIThreads() )
			sdc->getUIThreads()->removeEventsOfType( Event::OnModelEvent );
		if ( sdc->getUIStack() )
			sdc->getUIStack()->removeEventsOfType( Event::OnModelEvent );
		mPlugin->setUIDebuggingState( StatusDebuggerController::State::NotStarted );
	}
}

void DebuggerClientListener::createAndShowVariableMenu( ModelIndex idx ) {
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
		menu->add( context->i18n( "debugger_copy_variable_evaluate_name", "Copy Evaluate Name" ),
				   context->findIcon( "copy" ) )
			->setId( "debugger_copy_variable_evaluate_name" );
	}

	if ( var.memoryReference ) {
		menu->add(
				context->i18n( "debugger_copy_variable_memory_reference", "Copy Memory Reference" ),
				context->findIcon( "copy" ) )
			->setId( "debugger_copy_variable_memory_reference" );
	}

	menu->add( context->i18n( "debugger_value_viewer", "Value Viewer" ),
			   context->findIcon( "eye" ) )
		->setId( "debugger_value_viewer" );

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
			mPlugin->getUISceneNode()->getWindow()->getClipboard()->setText( *var.evaluateName );
		} else if ( id == "debugger_copy_variable_memory_reference" ) {
			mPlugin->getUISceneNode()->getWindow()->getClipboard()->setText( *var.memoryReference );
		} else if ( id == "debugger_value_viewer" ) {
			static constexpr auto VALUE_VIEWER_LAYOUT = R"html(
					<window id="process_picker" lw="250dp" lh="250dp" padding="4dp" window-flags="default|ephemeral">
						<vbox lw="mp" lh="mp">
							<TextEdit id="value_input" lw="mp" lh="0dp" lw8="1" wordwrap="true" />
						</vbox>
					</window>
					)html";
			UIWindow* win = mPlugin->getUISceneNode()
								->loadLayoutFromString( VALUE_VIEWER_LAYOUT )
								->asType<UIWindow>();
			win->setTitle( String::format( "%s:", var.name ) );
			UITextEdit* input = win->find( "value_input" )->asType<UITextEdit>();
			input->setText( var.value );
			win->center();
			win->showWhenReady();
		}
	} );

	menu->showOverMouseCursor();
}

void DebuggerClientListener::initUI() {
	auto sdc = getStatusDebuggerController();

	if ( sdc == nullptr )
		return;

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
			createAndShowVariableMenu( idx );
		}
	} );

	mPlugin->setUIDebuggingState( StatusDebuggerController::State::Running );
}

void DebuggerClientListener::stateChanged( DebuggerClient::State state, const SessionId& ) {
	if ( state == DebuggerClient::State::Initializing ) {
		mPlugin->getManager()->getUISceneNode()->runOnMainThread( [this] { initUI(); } );
	}
}

void DebuggerClientListener::sendBreakpoints() {
	Lock l( mPlugin->mBreakpointsMutex );
	for ( const auto& fileBps : mPlugin->mBreakpoints ) {
		if ( isRemote() && !mLocalRoot.empty() && !mRemoteRoot.empty() &&
			 String::startsWith( fileBps.first, mLocalRoot ) ) {
			auto remoteRoot = mRemoteRoot;
			auto localRoot = mLocalRoot;
			FileSystem::dirAddSlashAtEnd( localRoot );
			FileSystem::dirAddSlashAtEnd( remoteRoot );
			auto remotePath = fileBps.first;
			FileSystem::filePathRemoveBasePath( mLocalRoot, remotePath );
			remotePath = remoteRoot + remotePath;
			mClient->setBreakpoints( remotePath, fromSet( fileBps.second ) );
		} else {
			mClient->setBreakpoints( fileBps.first, fromSet( fileBps.second ) );
		}
	}
}

const std::string& DebuggerClientListener::localRoot() const {
	return mLocalRoot;
}

void DebuggerClientListener::setLocalRoot( const std::string& newLocalRoot ) {
	mLocalRoot = newLocalRoot;
}

const std::string& DebuggerClientListener::remoteRoot() const {
	return mRemoteRoot;
}

void DebuggerClientListener::setRemoteRoot( const std::string& newRemoteRoot ) {
	mRemoteRoot = newRemoteRoot;
}

bool DebuggerClientListener::isUnstableFrameId() const {
	return mUnstableFrameId;
}

void DebuggerClientListener::setUnstableFrameId( bool unstableFrameId ) {
	mUnstableFrameId = unstableFrameId;
}

void DebuggerClientListener::initialized( const SessionId& ) {
	sendBreakpoints();
}

void DebuggerClientListener::launched( const SessionId& ) {}

void DebuggerClientListener::configured( const SessionId& ) {}

void DebuggerClientListener::failed( const SessionId& ) {
	mPlugin->exitDebugger();
	resetState( true );
}

void DebuggerClientListener::debuggeeRunning( const SessionId& ) {}

void DebuggerClientListener::debuggeeTerminated( const SessionId& ) {
	if ( mClient->sessionsActive() == 0 ) {
		mPlugin->exitDebugger();
		resetState( true );
	}
}

void DebuggerClientListener::capabilitiesReceived( const Capabilities& /*capabilities*/ ) {}

void DebuggerClientListener::resetState( bool full ) {
	Lock l( mMutex );
	mStoppedData = {};
	mCurrentScopePos = {};
	mCurrentFrameId = 0;
	if ( mThreadsModel )
		mThreadsModel->resetThreads();
	if ( mStackModel )
		mStackModel->resetStack();
	mVariablesHolder->clear( full );
	mScopeRef.clear();
}

void DebuggerClientListener::debuggeeExited( int /*exitCode*/, const SessionId& ) {
	mPlugin->exitDebugger();
	resetState( true );
}

void DebuggerClientListener::debuggeeStopped( const StoppedEvent& event, const SessionId& ) {
	Log::debug( "DebuggerClientListener::debuggeeStopped: reason %s", event.reason );

	for ( auto& [editor, _] : mPlugin->mEditors ) {
		if ( editor->getTooltip() )
			editor->getTooltip()->hide();
	}

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

	int threadId = event.threadId.value_or( 1 );
	{
		Lock l( mMutex );
		mStoppedData = event;
		mCurrentThreadId = threadId;
	}

	if ( mPausedToRefreshBreakpoints ) {
		mPlugin->sendPendingBreakpoints();
		mPausedToRefreshBreakpoints = false;
		return;
	}

	changeThread( threadId );

	mClient->threads();
	mClient->stackTrace( threadId );

	UISceneNode* sceneNode = mPlugin->getUISceneNode();
	sceneNode->runOnMainThread( [sceneNode] { sceneNode->getWindow()->raise(); } );

	mPlugin->setUIDebuggingState( StatusDebuggerController::State::Paused );

	auto sdc = getStatusDebuggerController();
	if ( sdc )
		sdc->getWidget()->runOnMainThread( [sdc] { sdc->show(); } );
}

void DebuggerClientListener::debuggeeContinued( const ContinuedEvent&, const SessionId& ) {
	resetState( false );

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

void DebuggerClientListener::debuggingProcess( const ProcessInfo& info, const SessionId& ) {
	mProcessInfo = info;
}

void DebuggerClientListener::errorResponse( const std::string& command, const std::string& summary,
											const std::optional<Message>& /*message*/,
											const SessionId& sessionId ) {
	if ( command == "evaluate" )
		return;

	if ( command == "launch" )
		failed( sessionId );

	mPlugin->getPluginContext()->getNotificationCenter()->addNotification( summary, Seconds( 5 ),
																		   true );
}

void DebuggerClientListener::threadChanged( const ThreadEvent&, const std::string& ) {}

void DebuggerClientListener::moduleChanged( const ModuleEvent&, const std::string& ) {}

void DebuggerClientListener::threads( std::vector<DapThread>&& threads, const SessionId& ) {
	std::sort( threads.begin(), threads.end(),
			   []( const DapThread& a, const DapThread& b ) { return a.id < b.id; } );

	mThreadsModel->setThreads( std::move( threads ) );
}

void DebuggerClientListener::changeScope( const StackFrame& f ) {
	{
		Lock l( mMutex );
		mCurrentFrameId = f.id;
		if ( f.source ) {
			mCurrentScopePos = { f.source->path, f.line };
		} else {
			mCurrentScopePos.reset();
		}
	}
	mClient->scopes( f.id );

	if ( mStackModel )
		mStackModel->setCurrentScopeId( f.id );

	if ( !f.source )
		return;

	TextRange range{ { f.line - 1, f.column - 1 }, { f.line - 1, f.column - 1 } };
	std::string path( f.source->path );

	mPlugin->getUISceneNode()->runOnMainThread(
		[this, path, range] { mPlugin->getPluginContext()->focusOrLoadFile( path, range ); } );

	auto sdc = getStatusDebuggerController();
	if ( sdc && sdc->getUIStack() )
		sdc->getUIStack()->setSelection( mStackModel->index( f.id ) );
}

void DebuggerClientListener::changeThread( int id ) {
	{
		Lock l( mMutex );
		mCurrentThreadId = id;
	}
	if ( mThreadsModel )
		mThreadsModel->setCurrentThreadId( id );

	auto sdc = getStatusDebuggerController();
	if ( sdc && sdc->getUIThreads() )
		sdc->getUIThreads()->setSelection( mThreadsModel->fromThreadId( id ) );
}

void DebuggerClientListener::stackTrace( const int threadId, StackTraceInfo&& stack,
										 const SessionId& ) {
	changeThread( threadId );

	for ( const auto& f : stack.stackFrames ) {
		// Jump to the first stack frame that can be read
		if ( f.source ) {
			changeScope( f );
			break;
		}
	}

	mStackModel->setStack( std::move( stack ) );

	evaluateExpressions();
}

void DebuggerClientListener::evaluateExpression( const std::string& expression ) {
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
			ExpandedState::Location location;
			{
				Lock l( mMutex );
				if ( !mCurrentScopePos.has_value() )
					return;
				location = { mCurrentScopePos->first, mCurrentScopePos->second, mCurrentFrameId };
			}
			mPlugin->mExpressionsHolder->restoreExpandedState(
				location, mClient, getStatusDebuggerController()->getUIExpressions(), true,
				mUnstableFrameId );
		} );
}

void DebuggerClientListener::evaluateExpressions() {
	for ( const auto& expression : mPlugin->mExpressions )
		evaluateExpression( expression );
}

void DebuggerClientListener::scopes( const int /*frameId*/, std::vector<Scope>&& scopes,
									 const SessionId& ) {
	if ( scopes.empty() )
		return;

	mVariablesHolder->clear();

	for ( const auto& scope : scopes ) {
		auto child = std::make_shared<ModelVariableNode>( scope.name, scope.variablesReference );
		mVariablesHolder->addChild( child );
		if ( ( !mPlugin->mFetchRegisters && scope.name == "Registers" ) ||
			 ( !mPlugin->mFetchGlobals && ( scope.name == "Global" || scope.name == "Globals" ) ) )
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
		uiVars->clearViewMetadata();
		uiVars->removeActionsByTag( SCOPES_UI_HASH );
		uiVars->runOnMainThread(
			[this, uiVars] {
				// Reset selection given that the VariablesModel can end up doing an use-after-free
				uiVars->getSelection().clear();
				auto model = uiVars->getModel();
				size_t total = model->rowCount();
				for ( size_t i = 0; i < total; i++ ) {
					auto index = model->index( i, uiVars->getMainColumn() );
					if ( !index.isValid() )
						continue;
					ModelVariableNode* node =
						static_cast<ModelVariableNode*>( index.internalData() );
					if ( ( !mPlugin->mFetchRegisters && node->var.name == "Registers" ) ||
						 ( !mPlugin->mFetchGlobals &&
						   ( node->var.name == "Global" || node->var.name == "Globals" ) ) )
						continue;
					uiVars->tryOpenModelIndex( index );
				}
			},
			Seconds( 0 ), SCOPES_UI_HASH );
	}
}

void DebuggerClientListener::variables( const int variablesReference, std::vector<Variable>&& vars,
										const SessionId& ) {
	mVariablesHolder->addVariables( variablesReference, std::move( vars ) );

	ExpandedState::Location location;
	bool scopeFound = false;

	{
		Lock l( mMutex );
		if ( !mCurrentScopePos.has_value() )
			return;
		location = { mCurrentScopePos->first, mCurrentScopePos->second, mCurrentFrameId };
		scopeFound = mScopeRef.find( variablesReference ) != mScopeRef.end();
	}

	if ( scopeFound ) {
		mVariablesHolder->restoreExpandedState( location, mClient,
												getStatusDebuggerController()->getUIVariables(),
												false, mUnstableFrameId );
	}
}

void DebuggerClientListener::modules( ModulesInfo&&, const SessionId& ) {}

void DebuggerClientListener::serverDisconnected( const SessionId& ) {}

void DebuggerClientListener::sourceContent( const std::string& /*path*/, int /*reference*/,
											const SourceContent& /*content*/, const SessionId& ) {}

void DebuggerClientListener::sourceBreakpoints(
	const std::string& /*path*/, int /*reference*/,
	const std::optional<std::vector<Breakpoint>>& /*breakpoints*/, const SessionId& ) {}

void DebuggerClientListener::breakpointChanged( const BreakpointEvent&, const SessionId& ) {}

void DebuggerClientListener::expressionEvaluated( const std::string& /*expression*/,
												  const std::optional<EvaluateInfo>&,
												  const SessionId& ) {}

void DebuggerClientListener::gotoTargets( const Source& /*source*/, const int /*line*/,
										  const std::vector<GotoTarget>& /*targets*/,
										  const SessionId& ) {}

bool DebuggerClientListener::isRemote() const {
	return mIsRemote;
}

bool DebuggerClientListener::isStopped() const {
	Lock l( mMutex );
	return mStoppedData ? true : false;
}

std::optional<StoppedEvent> DebuggerClientListener::getStoppedData() const {
	Lock l( mMutex );
	return mStoppedData;
}

void DebuggerClientListener::setPausedToRefreshBreakpoints() {
	mPausedToRefreshBreakpoints = true;
}

int DebuggerClientListener::getCurrentThreadId() const {
	Lock l( mMutex );
	return mCurrentThreadId;
}

int DebuggerClientListener::getCurrentFrameId() const {
	Lock l( mMutex );
	return mCurrentFrameId;
}

std::optional<std::pair<std::string, int>> DebuggerClientListener::getCurrentScopePos() const {
	Lock l( mMutex );
	return mCurrentScopePos;
}

int DebuggerClientListener::getCurrentScopePosLine() const {
	Lock l( mMutex );
	return mCurrentScopePos.has_value() ? mCurrentScopePos->second : -1;
}

bool DebuggerClientListener::isCurrentScopePos( const std::string& filePath, int index ) const {
	Lock l( mMutex );
	return mCurrentScopePos.has_value() && index == mCurrentScopePos->second &&
		   mCurrentScopePos->first == filePath;
}

bool DebuggerClientListener::isCurrentScopePos( const std::string& filePath ) const {
	Lock l( mMutex );
	return mCurrentScopePos.has_value() && mCurrentScopePos->first == filePath;
}

void DebuggerClientListener::setIsRemote( bool isRemote ) {
	mIsRemote = isRemote;
}

StatusDebuggerController* DebuggerClientListener::getStatusDebuggerController() const {
	return mPlugin->getStatusDebuggerController();
}

} // namespace ecode
