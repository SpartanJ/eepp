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

void StatusDebuggerController::createContainer() {
	if ( mContainer )
		return;
	const auto XML = R"xml(
	<TabWidget id="app_debugger" layout_width="mp" layout_height="mp">
		<Splitter id="debugger_threads_and_stack" layout_width="mp" lh="mp" splitter-partition="15%">
			<TableView id="debugger_threads" layout_width="mp" layout_height="mp" />
			<TableView id="debugger_stack" layout_width="mp" layout_height="mp" />
		</Splitter>
		<TableView id="debugger_breakpoints" layout_width="mp" layout_height="mp" />
		<Tab text="@string(threads_and_stack, Threads & Stack)" owns="debugger_threads_and_stack" />
		<Tab text="@string(breakpoints, Breakpoints)" owns="debugger_breakpoints" />
	</TabWidget>
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

	mUIThreads->setAutoExpandOnSingleColumn( true );

	mUIStack->setAutoColumnsWidth( true );
	mUIStack->setMainColumn( 1 );

	mUIBreakpoints->setAutoColumnsWidth( true );
	mUIBreakpoints->setMainColumn( 1 );
}

} // namespace ecode
