#include "uistatusbar.hpp"
#include "globalsearchcontroller.hpp"
#include "plugins/plugincontextprovider.hpp"
#include "statusappoutputcontroller.hpp"
#include "statusbuildoutputcontroller.hpp"
#include "statusterminalcontroller.hpp"
#include "universallocator.hpp"
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/window.hpp>

namespace ecode {

std::unordered_map<std::string, std::string> UIStatusBar::getDefaultKeybindings() {
	return { { "alt+shift+9", "statusbar-panel-expand-contract-toggle" } };
}

StatusBarElement::StatusBarElement( UISplitter* mainSplitter, UISceneNode* uiSceneNode,
									PluginContextProvider* app ) :
	mMainSplitter( mainSplitter ),
	mUISceneNode( uiSceneNode ),
	mContext( app ),
	mSplitter( mContext->getSplitter() ) {}

void StatusBarElement::toggle() {
	if ( nullptr == getWidget() ) {
		show();
		return;
	}

	if ( mMainSplitter->getLastWidget() != nullptr ) {
		if ( mMainSplitter->getLastWidget() == getWidget() ) {
			hide();
		} else {
			show();
		}
	} else {
		show();
	}
}

void StatusBarElement::hide() {
	if ( getWidget() && getWidget()->isVisible() ) {
		getWidget()->setParent( mUISceneNode );
		getWidget()->setVisible( false );
		mContext->getStatusBar()->updateState();
		if ( mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
	}
}

void StatusBarElement::show() {
	if ( nullptr == getWidget() ) {
		mMainSplitter->updateLayout();
		createWidget();
		if ( nullptr == getWidget() )
			return;
		getWidget()->setVisible( false );
	}

	if ( !getWidget()->isVisible() ) {
		mContext->hideLocateBar();
		mContext->hideSearchBar();
		mContext->hideGlobalSearchBar();
		if ( mMainSplitter->getLastWidget() != nullptr ) {
			mMainSplitter->getLastWidget()->setVisible( false );
			mMainSplitter->getLastWidget()->setParent( mUISceneNode );
		}
		getWidget()->setParent( mMainSplitter );
		getWidget()->setVisible( true );
		getWidget()->setFocus();
		mContext->getStatusBar()->updateState();
	}
}

UIStatusBar* UIStatusBar::New() {
	return eeNew( UIStatusBar, () );
}

UIStatusBar::UIStatusBar() :
	UILinearLayout( "statusbar", UIOrientation::Horizontal ), WidgetCommandExecuter( getInput() ) {}

void UIStatusBar::updateState() {
	forEachChild( [this]( Node* node ) {
		if ( node->isWidget() && String::startsWith( node->getId(), "status_" ) ) {
			auto widget = node->asType<UIWidget>();
			widget->removeClass( "selected" );
			if ( nullptr == widget->getTooltip() ) {
				std::string name( widget->getId() );
				String::replaceAll( name, "_", "-" );
				auto kb = mContext->getKeybind( "toggle-" + name );
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

	if ( mContext->getMainSplitter() && mContext->getMainSplitter()->getLastWidget() ) {
		UIWidget* widget = mContext->getMainSplitter()->getLastWidget();
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
	if ( !isVisible() || nullptr == mContext || msg->getMsg() != NodeMessage::MouseClick ||
		 0 == ( msg->getFlags() & EE_BUTTON_LMASK ) || !msg->getSender()->isWidget() )
		return 0;

	UIWidget* widget = msg->getSender()->asType<UIWidget>();

	if ( !widget->hasClass( "status_but" ) )
		return 0;

	int ret = 0;

	if ( widget->getId() == "status_locate_bar" ) {
		mContext->getUniversalLocator()->toggleLocateBar();
		ret = 1;
	} else if ( widget->getId() == "status_global_search_bar" ) {
		mContext->getGlobalSearchController()->toggleGlobalSearchBar();
		ret = 1;
	} else if ( widget->getId() == "status_terminal_panel" ) {
		mContext->getStatusTerminalController()->toggle();
		ret = 1;
	} else if ( widget->getId() == "status_build_output" ) {
		mContext->getStatusBuildOutputController()->toggle();
		ret = 1;
	} else if ( widget->getId() == "status_app_output" ) {
		mContext->getStatusAppOutputController()->toggle();
		ret = 1;
	} else {
		auto elemIt = mElements.find( widget->getId() );
		if ( elemIt != mElements.end() ) {
			elemIt->second.second->toggle();
			ret = 1;
		}
	}

	if ( ret )
		updateState();
	return ret;
}

void UIStatusBar::setPluginContextProvider( PluginContextProvider* app ) {
	mContext = app;
	mPanelContractedPartition = mContext->getConfig().windowState.statusBarPartition;
	updateState();
}

StyleSheetLength UIStatusBar::getPanelContractedPartition() const {
	if ( isPanelExpanded() )
		return mPanelContractedPartition;
	return mContext->getMainSplitter()->getSplitPartition();
}

std::shared_ptr<StatusBarElement> UIStatusBar::getStatusBarElement( const std::string& id ) const {
	auto elemIt = mElements.find( id );
	if ( elemIt != mElements.end() )
		return elemIt->second.second;
	return {};
}

void UIStatusBar::onVisibilityChange() {
	UILinearLayout::onVisibilityChange();
	if ( isVisible() )
		updateState();
}

void UIStatusBar::onChildCountChange( Node* node, const bool& removed ) {
	UILinearLayout::onChildCountChange( node, removed );
	if ( mContext )
		updateState();
}

UIPushButton* UIStatusBar::insertStatusBarElement( std::string id, const String& text,
												   const std::string& icon,
												   std::shared_ptr<StatusBarElement> element ) {
	auto elemIt = mElements.find( id );
	UIPushButton* button = nullptr;
	if ( elemIt != mElements.end() ) {
		button = elemIt->second.first;
		mElements.erase( elemIt );
	} else {
		button = UIPushButton::New();
		button->beginAttributesTransaction();
		button->setText( text );
		button->setParent( this )->setId( id );
		button->setClass( "status_but" );
		UIWidget* statusSep = findByClass( "status_sep" );
		if ( statusSep )
			button->toPosition( statusSep->getNodeIndex() );
		button->applyProperty(
			StyleSheetProperty( "icon", icon, false, StyleSheetSelectorRule::SpecificityInline ) );
		button->endAttributesTransaction();
	}

	mElements[id] = { button, element };
	return button;
}

void UIStatusBar::removeStatusBarElement( const std::string& id ) {
	auto elemIt = mElements.find( id );
	if ( elemIt != mElements.end() ) {
		elemIt->second.first->close();
		mElements.erase( elemIt );
	}
}

void UIStatusBar::hideAllElements() {
	for ( auto& [_, el] : mElements )
		el.second->hide();
}

bool UIStatusBar::isPanelExpanded() const {
	return mPanelExpanded;
}

void UIStatusBar::expandPanel() {
	mPanelExpanded = true;
	mPanelContractedPartition = mContext->getMainSplitter()->getSplitPartition();
	auto allBtns =
		mContext->getUISceneNode()->getRoot()->findAllByClass( "expand_status_bar_panel" );
	for ( auto btn : allBtns )
		btn->addClass( "expanded" );
	mContext->getMainSplitter()->setSplitPartition( StyleSheetLength( 48, StyleSheetLength::Dp ) );
}

void UIStatusBar::contractPanel() {
	mPanelExpanded = false;
	mContext->getMainSplitter()->setSplitPartition( mPanelContractedPartition );
	auto allBtns =
		mContext->getUISceneNode()->getRoot()->findAllByClass( "expand_status_bar_panel" );
	for ( auto btn : allBtns )
		btn->removeClass( "expanded" );
}

void UIStatusBar::togglePanelExpansion() {
	if ( isPanelExpanded() ) {
		contractPanel();
	} else {
		expandPanel();
	}
}

void UIStatusBar::registerStatusBarPanel( WidgetCommandExecuter* container, UIWidget* widget ) {
	if ( !container )
		return;

	container->setCommand( "statusbar-panel-expand-contract-toggle",
						   [this] { togglePanelExpansion(); } );
	container->getKeyBindings().addKeybindsStringUnordered( mContext->getStatusBarKeybindings() );

	if ( widget ) {
		auto expCntPanelBtn =
			widget->findByClass( "expand_status_bar_panel" )->asType<UIPushButton>();
		if ( expCntPanelBtn ) {
			expCntPanelBtn->setTooltipText(
				String::format( "%s (%s)", expCntPanelBtn->getTooltipText().toUtf8(),
								container->getKeyBindings().getCommandKeybindString(
									"statusbar-panel-expand-contract-toggle" ) ) );
			expCntPanelBtn->onClick( [container]( auto event ) {
				container->execute( "statusbar-panel-expand-contract-toggle" );
			} );
		}
	}
}

} // namespace ecode
