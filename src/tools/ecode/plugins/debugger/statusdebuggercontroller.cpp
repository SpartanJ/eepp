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
	<TabWidget id="app_debugger" layout_width="match_parent" layout_height="0dp" layout_weight="1">
		<TableView id="debugger_stack" layout_width="mp" layout_height="mp" />
		<TableView id="debugger_threads" layout_width="mp" layout_height="mp" />
		<TableView id="debugger_breakpoints" layout_width="mp" layout_height="mp" />
		<Tab text="@string(stack, Stack)" owns="debugger_stack" />
		<Tab text="@string(threads, Threads)" owns="debugger_threads" />
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

	mContainer->bind( "debugger_threads", mUIThreads );
	mContainer->bind( "debugger_stack", mUIStack );
	mContainer->bind( "debugger_breakpoints", mUIBreakpoints );

	mContainer->on( Event::OnSizeChange, [this]( const Event* event ) {
		if ( !mContainer->isVisible() || mContainer->getSize().getWidth() == 0.f )
			return;

		const Float width = mContainer->getPixelsSize().getWidth();

		mUIThreads->setColumnWidth( 0, width * 0.1 );
		mUIThreads->setColumnWidth( 1, eefloor( width * 0.88 ) );

		mUIStack->setColumnWidth( 0, width * 0.05 );
		mUIStack->setColumnWidth( 1, width * 0.3 );
		mUIStack->setColumnWidth( 2, width * 0.15 );
		mUIStack->setColumnWidth( 3, eefloor( width * 0.3 ) );
		mUIStack->setColumnWidth( 4, width * 0.08 );
		mUIStack->setColumnWidth( 5, width * 0.08 );

		mUIBreakpoints->setColumnWidth( 0, width * 0.1 );
		mUIBreakpoints->setColumnWidth( 1, eefloor( width * 0.7 ) );
		mUIBreakpoints->setColumnWidth( 2, eefloor( width * 0.1 ) );

		mContainer->removeEventListener( event->getCallbackId() );
	} );
}

} // namespace ecode
