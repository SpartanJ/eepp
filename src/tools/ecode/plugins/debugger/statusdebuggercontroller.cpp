#include "statusdebuggercontroller.hpp"
#include "../plugincontextprovider.hpp"

namespace ecode {

StatusDebuggerController::StatusDebuggerController( UISplitter* mainSplitter,
													UISceneNode* uiSceneNode,
													PluginContextProvider* pluginContext ) :
	StatusBarElement( mainSplitter, uiSceneNode, pluginContext ) {}

UIWidget* StatusDebuggerController::getWidget() {
	return mContainer;
}

UIWidget* StatusDebuggerController::createWidget() {
	if ( nullptr == mContainer )
		createContainer();
	return mContainer;
}

UITableView* StatusDebuggerController::getUIThreads() {
	return mUIThreads;
}

UITableView* StatusDebuggerController::getUIStack() {
	return mUIStack;
}

UITableView* StatusDebuggerController::getUIBreakpoints() {
	return mUIBreakpoints;
}

void StatusDebuggerController::setDebuggingState( State state ) {
	if ( !mContainer )
		return;

	mUISceneNode->runOnMainThread( [this, state] {
		mUIButStart->setVisible( state == State::NotStarted )
			->setEnabled( state == State::NotStarted );
		mUIButStop->setVisible( state != State::NotStarted )
			->setEnabled( state != State::NotStarted );
		mUIButContinue->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Paused );
		mUIButPause->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Running );
		mUIButStepOver->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Paused );
		mUIButStepInto->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Paused );
		mUIButStepOut->setVisible( state != State::NotStarted )
			->setEnabled( state == State::Paused );
	} );
}

void StatusDebuggerController::createContainer() {
	if ( mContainer )
		return;
	const auto XML = R"xml(
	<style>
	#app_debugger_buttons > PushButton:disabled {
		tint: var(--disabled-color);
	}
	</style>
	<hbox id="app_debugger" class="vertical_bar" lw="mp" lh="mp" visible="false">
		<TabWidget lw="0" lw8="1" lh="mp">
			<Splitter id="debugger_threads_and_stack" layout_width="mp" lh="mp" splitter-partition="15%">
				<TableView id="debugger_threads" layout_width="mp" layout_height="mp" />
				<TableView id="debugger_stack" layout_width="mp" layout_height="mp" />
			</Splitter>
			<TableView id="debugger_breakpoints" layout_width="mp" layout_height="mp" />
			<Tab text="@string(threads_and_stack, Threads & Stack)" owns="debugger_threads_and_stack" />
			<Tab text="@string(breakpoints, Breakpoints)" owns="debugger_breakpoints" />
		</TabWidget>
		<vbox id="app_debugger_buttons" lw="16dp" lh="mp">
			<PushButton id="app_debugger_start" lw="mp" icon="icon(debug-start, 12dp)" tooltip="@string(start, Start)" />
			<PushButton id="app_debugger_stop" lw="mp" icon="icon(debug-stop, 12dp)" tooltip="@string(stop, Stop)" />
			<PushButton id="app_debugger_continue" lw="mp" icon="icon(debug-continue, 12dp)" tooltip="@string(continue, Continue)" />
			<PushButton id="app_debugger_pause" lw="mp" icon="icon(debug-pause, 12dp)" tooltip="@string(pause, Pause)" />
			<PushButton id="app_debugger_step_over" lw="mp" icon="icon(debug-step-over, 12dp)" tooltip="@string(step_over, Step Over)" />
			<PushButton id="app_debugger_step_into" lw="mp" icon="icon(debug-step-into, 12dp)" tooltip="@string(step_into, Step Into)" />
			<PushButton id="app_debugger_step_out" lw="mp" icon="icon(debug-step-out, 12dp)" tooltip="@string(step_out, Step Out)" />
		</vbox>
	</hbox>
	)xml";

	if ( mMainSplitter->getLastWidget() != nullptr ) {
		mMainSplitter->getLastWidget()->setVisible( false );
		mMainSplitter->getLastWidget()->setParent( mUISceneNode );
	}

	mContainer = mContext->getUISceneNode()
					 ->loadLayoutFromString( XML, mMainSplitter )
					 ->asType<UILinearLayout>();

	mContainer->bind( "debugger_threads_and_stack", mUIThreadsSplitter );
	mContainer->bind( "debugger_threads", mUIThreads );
	mContainer->bind( "debugger_stack", mUIStack );
	mContainer->bind( "debugger_breakpoints", mUIBreakpoints );
	mContainer->bind( "app_debugger_start", mUIButStart );
	mContainer->bind( "app_debugger_stop", mUIButStop );
	mContainer->bind( "app_debugger_pause", mUIButPause );
	mContainer->bind( "app_debugger_continue", mUIButContinue );
	mContainer->bind( "app_debugger_step_over", mUIButStepOver );
	mContainer->bind( "app_debugger_step_into", mUIButStepInto );
	mContainer->bind( "app_debugger_step_out", mUIButStepOut );

	setDebuggingState( State::NotStarted );

	mUIButStart->onClick(
		[this]( auto ) { mContext->runCommand( "debugger-continue-interrupt" ); } );

	mUIButContinue->onClick(
		[this]( auto ) { mContext->runCommand( "debugger-continue-interrupt" ); } );

	mUIButPause->onClick(
		[this]( auto ) { mContext->runCommand( "debugger-continue-interrupt" ); } );

	mUIButStop->onClick( [this]( auto ) { mContext->runCommand( "debugger-stop" ); } );

	mUIButStepOver->onClick( [this]( auto ) { mContext->runCommand( "debugger-step-over" ); } );

	mUIButStepInto->onClick( [this]( auto ) { mContext->runCommand( "debugger-step-into" ); } );

	mUIButStepOut->onClick( [this]( auto ) { mContext->runCommand( "debugger-step-out" ); } );

	mUIThreads->setAutoExpandOnSingleColumn( true );

	mUIStack->setAutoColumnsWidth( true );
	mUIStack->setMainColumn( 1 );

	mUIBreakpoints->setAutoColumnsWidth( true );
	mUIBreakpoints->setMainColumn( 1 );
}

} // namespace ecode
