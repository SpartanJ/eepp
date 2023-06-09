#include "uistatusbar.hpp"
#include "ecode.hpp"
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/window.hpp>

namespace ecode {

UIStatusBar* UIStatusBar::New() {
	return eeNew( UIStatusBar, () );
}

UIStatusBar::UIStatusBar() :
	UILinearLayout( "statusbar", UIOrientation::Horizontal ),
	WidgetCommandExecuter( getUISceneNode()->getWindow()->getInput() ) {}

void UIStatusBar::updateState() {
	forEachChild( [this]( Node* node ) {
		if ( node->isWidget() && String::startsWith( node->getId(), "status_" ) ) {
			auto widget = node->asType<UIWidget>();
			widget->removeClass( "selected" );
			if ( nullptr == widget->getTooltip() ) {
				std::string name( widget->getId() );
				String::replaceAll( name, "_", "-" );
				auto kb = mApp->getKeybind( "toggle-" + name );
				if ( !kb.empty() )
					widget->setTooltipText( kb );
			}
		}
	} );

	getParent()->forEachChild( [this]( Node* node ) {
		UIWidget* but = find<UIWidget>( "status_" + node->getId() );
		if ( but && but != this ) {
			if ( node->isVisible() ) {
				but->addClass( "selected" );
			} else {
				but->removeClass( "selected" );
			}
		}
	} );

	if ( mApp->getMainSplitter() && mApp->getMainSplitter()->getLastWidget() ) {
		UIWidget* widget = mApp->getMainSplitter()->getLastWidget();
		UIWidget* but = find<UIWidget>( "status_" + widget->getId() );
		if ( but && but != this ) {
			if ( widget->isVisible() ) {
				but->addClass( "selected" );
			} else {
				but->removeClass( "selected" );
			}
		}
	}
}

Uint32 UIStatusBar::onMessage( const NodeMessage* msg ) {
	if ( !isVisible() || nullptr == mApp || msg->getMsg() != NodeMessage::MouseClick ||
		 0 == ( msg->getFlags() & EE_BUTTON_LMASK ) || !msg->getSender()->isWidget() )
		return 0;

	UIWidget* widget = msg->getSender()->asType<UIWidget>();

	if ( !widget->hasClass( "status_but" ) )
		return 0;

	int ret = 0;

	if ( widget->getId() == "status_locate_bar" ) {
		mApp->getUniversalLocator()->toggleLocateBar();
		ret = 1;
	} else if ( widget->getId() == "status_global_search_bar" ) {
		mApp->getGlobalSearchController()->toggleGlobalSearchBar();
		ret = 1;
	} else if ( widget->getId() == "status_terminal" ) {
		mApp->getStatusTerminalController()->toggle();
		ret = 1;
	} else if ( widget->getId() == "status_build_output" ) {
		mApp->getStatusBuildOutputController()->toggle();
		ret = 1;
	}

	if ( ret )
		updateState();
	return ret;
}

void UIStatusBar::setApp( App* app ) {
	mApp = app;
	updateState();
}

void UIStatusBar::onVisibilityChange() {
	UILinearLayout::onVisibilityChange();
	if ( isVisible() )
		updateState();
}

void UIStatusBar::onChildCountChange( Node* node, const bool& removed ) {
	UILinearLayout::onChildCountChange( node, removed );
	if ( mApp )
		updateState();
}

} // namespace ecode
