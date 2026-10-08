#include "uirightpanel.hpp"
#include "appconfig.hpp"

namespace ecode {

using namespace EE;
using namespace EE::UI;

UIRightPanel::UIRightPanel( UISplitter* splitter, UILayout* container, AppConfig* config ) :
	mSplitter( splitter ), mContainer( container ), mConfig( config ) {}

UILayout* UIRightPanel::registerPanel( const std::string& id ) {
	auto found = mPanels.find( id );
	if ( found != mPanels.end() )
		return found->second.layout;
	if ( !mContainer )
		return nullptr;
	auto layout = UIRelativeLayout::New();
	layout->setId( id );
	layout->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
	layout->setVisible( false )->setEnabled( false )->setParent( mContainer );
	mPanels.emplace( id, Panel{ layout, false } );
	return layout;
}

void UIRightPanel::unregisterPanel( const std::string& id ) {
	auto found = mPanels.find( id );
	if ( found == mPanels.end() )
		return;
	if ( found->second.layout )
		found->second.layout->close();
	mPanels.erase( found );
	updateVisibility();
}

void UIRightPanel::setPanelVisible( const std::string& id, bool visible ) {
	auto found = mPanels.find( id );
	if ( found == mPanels.end() )
		return;
	if ( visible ) {
		for ( auto& panel : mPanels ) {
			if ( panel.first != id && panel.second.visible ) {
				panel.second.visible = false;
				if ( panel.second.layout )
					panel.second.layout->setVisible( false )->setEnabled( false );
			}
		}
	}
	if ( found->second.visible == visible )
		return;
	found->second.visible = visible;
	if ( found->second.layout )
		found->second.layout->setVisible( visible )->setEnabled( visible );
	updateVisibility();
}

bool UIRightPanel::isPanelVisible( const std::string& id ) const {
	auto found = mPanels.find( id );
	return found != mPanels.end() && found->second.visible;
}

void UIRightPanel::saveState() {
	if ( mSplitter && mConfig &&
		 mSplitter->getSplitPartition().getUnit() == StyleSheetLength::Percentage &&
		 mSplitter->getSplitPartition().getValue() < 100.f )
		mConfig->windowState.rightPanelPartition = mSplitter->getSplitPartition().toString();
}

UISplitter* UIRightPanel::getSplitter() const {
	return mSplitter;
}

UILayout* UIRightPanel::getContainer() const {
	return mContainer;
}

void UIRightPanel::updateVisibility() {
	if ( !mSplitter || !mConfig )
		return;
	bool visible = false;
	for ( const auto& panel : mPanels ) {
		if ( panel.second.visible ) {
			visible = true;
			break;
		}
	}
	if ( visible == mVisible )
		return;
	mVisible = visible;
	if ( visible ) {
		auto partition = mConfig->windowState.rightPanelPartition;
		if ( partition.empty() || partition == "100%" )
			partition = "75%";
		mSplitter->setSplitPartition( StyleSheetLength( partition ) );
	} else {
		saveState();
		mSplitter->setSplitPartition( StyleSheetLength( "100%" ) );
	}
}

} // namespace ecode
