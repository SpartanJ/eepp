#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uiskinstate.hpp>

namespace EE { namespace UI {

UIListBoxItem* UIListBoxItem::New() {
	return eeNew( UIListBoxItem, () );
}

UIListBoxItem* UIListBoxItem::NewWithTag( const std::string& tag ) {
	return eeNew( UIListBoxItem, ( tag ) );
}

UIListBoxItem::UIListBoxItem() : UIListBoxItem( "listbox::item" ) {}

UIListBoxItem::UIListBoxItem( const std::string& tag ) : UITextView( tag ) {
	setLayoutSizePolicy( SizePolicy::Fixed, SizePolicy::Fixed );
	applyDefaultTheme();
}

UIListBoxItem::~UIListBoxItem() {
	EventDispatcher* eventDispatcher = getEventDispatcher();

	if ( NULL != eventDispatcher ) {
		if ( eventDispatcher->getFocusNode() == this )
			mParentNode->setFocus();

		if ( eventDispatcher->getMouseOverNode() == this )
			eventDispatcher->setMouseOverNode( mParentNode );
	}
}

Uint32 UIListBoxItem::getType() const {
	return UI_TYPE_LISTBOXITEM;
}

bool UIListBoxItem::isType( const Uint32& type ) const {
	return UIListBoxItem::getType() == type ? true : UITextView::isType( type );
}

void UIListBoxItem::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "listboxitem" );

	onThemeLoaded();
}

Uint32 UIListBoxItem::onMouseUp( const Vector2i& Pos, const Uint32& Flags ) {
	UITextView::onMouseUp( Pos, Flags );

	if ( mEnabled && mVisible ) {
		UIListBox* LBParent = getParent()->getParent()->asType<UIListBox>();

		if ( Flags & EE_BUTTONS_WUWD && LBParent->getVerticalScrollBar()->isVisible() ) {
			// Manage click can delete _this_
			LBParent->getVerticalScrollBar()->getSlider()->manageClick( Flags );
		}
	}

	return 1;
}

Uint32 UIListBoxItem::onMouseClick( const Vector2i& Pos, const Uint32& Flags ) {
	if ( Flags & EE_BUTTONS_LRM ) {
		UIListBox* LBParent = getParent()->getParent()->asType<UIListBox>();

		LBParent->itemClicked( this );

		select();

		if ( !LBParent->isMultiSelect() )
			setFocus();
	}

	return UITextView::onMouseClick( Pos, Flags );
}

void UIListBoxItem::select() {
	UIListBox* LBParent = getParent()->getParent()->asType<UIListBox>();

	bool wasSelected = 0 != ( mNodeFlags & NODE_FLAG_SELECTED );

	if ( LBParent->isMultiSelect() ) {
		if ( !wasSelected ) {
			pushState( UIState::StateSelected );

			mNodeFlags |= NODE_FLAG_SELECTED;

			LBParent->mSelected.push_back( LBParent->getItemIndex( this ) );

			LBParent->onSelected();
		} else {
			popState( UIState::StateSelected );

			mNodeFlags &= ~NODE_FLAG_SELECTED;

			auto found = std::find( LBParent->mSelected.begin(), LBParent->mSelected.end(),
									LBParent->getItemIndex( this ) );
			if ( found != LBParent->mSelected.end() )
				LBParent->mSelected.erase( found );
			LBParent->sendCommonEvent( Event::OnSelectionChanged );
		}
	} else {
		pushState( UIState::StateSelected );

		mNodeFlags |= NODE_FLAG_SELECTED;

		if ( !LBParent->mSelected.empty() &&
			 NULL != LBParent->mItems[LBParent->mSelected.front()] &&
			 LBParent->getItemIndex( this ) != LBParent->mSelected.front() ) {
			LBParent->mItems[LBParent->mSelected.front()]->unselect();
		}
		LBParent->mSelected.clear();
		LBParent->mSelected.push_back( LBParent->getItemIndex( this ) );

		if ( !wasSelected ) {
			LBParent->onSelected();
		} else {
			LBParent->sendCommonEvent( Event::OnSelectionChanged );
		}
	}
}

Uint32 UIListBoxItem::onMouseLeave( const Vector2i& Pos, const Uint32& Flags ) {
	UIWidget::onMouseLeave( Pos, Flags );

	if ( mNodeFlags & NODE_FLAG_SELECTED )
		pushState( UIState::StateSelected );

	return 1;
}

void UIListBoxItem::unselect() {
	if ( mNodeFlags & NODE_FLAG_SELECTED )
		mNodeFlags &= ~NODE_FLAG_SELECTED;

	popState( UIState::StateSelected );
}

bool UIListBoxItem::isSelected() const {
	return 0 != ( mNodeFlags & NODE_FLAG_SELECTED );
}

void UIListBoxItem::onStateChange() {
	if ( isSelected() && NULL != mSkinState && mSkinState->getState() != UIState::StateSelected ) {
		pushState( UIState::StateSelected, false );
	}

	UITextView::onStateChange();
}

}} // namespace EE::UI
