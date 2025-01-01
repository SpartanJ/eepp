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

void StatusDebuggerController::createContainer() {
	if ( mContainer )
		return;
	const auto XML = R"xml(
	<hbox id="app_debugger" lw="mp" lh="mp" visible="false">
	</hbox>
	)xml";

	if ( mMainSplitter->getLastWidget() != nullptr ) {
		mMainSplitter->getLastWidget()->setVisible( false );
		mMainSplitter->getLastWidget()->setParent( mUISceneNode );
	}

	mContainer = mContext->getUISceneNode()
					 ->loadLayoutFromString( XML, mMainSplitter )
					 ->asType<UILinearLayout>();
}

} // namespace ecode
