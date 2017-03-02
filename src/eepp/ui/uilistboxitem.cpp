#include <eepp/ui/uilistboxitem.hpp>
#include <eepp/ui/uilistbox.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIListBoxItem * UIListBoxItem::New() {
	return eeNew( UIListBoxItem, () );
}

UIListBoxItem::UIListBoxItem() :
	UITextBox()
{
	applyDefaultTheme();
}

UIListBoxItem::~UIListBoxItem() {
	if ( UIManager::instance()->getFocusControl() == this )
		mParentCtrl->setFocus();

	if ( UIManager::instance()->getOverControl() == this )
		UIManager::instance()->setOverControl( mParentCtrl );
}

Uint32 UIListBoxItem::getType() const {
	return UI_TYPE_LISTBOXITEM;
}

bool UIListBoxItem::isType( const Uint32& type ) const {
	return UIListBoxItem::getType() == type ? true : UITextBox::isType( type );
}

void UIListBoxItem::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "listboxitem" );
}

Uint32 UIListBoxItem::onMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTONS_LRM ) {
		reinterpret_cast<UIListBox*> ( getParent()->getParent() )->itemClicked( this );

		select();
	}

	return 1;
}

void UIListBoxItem::select() {
	UIListBox * LBParent = reinterpret_cast<UIListBox*> ( getParent()->getParent() );

	bool wasSelected = 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );

	if ( LBParent->isMultiSelect() ) {
		if ( !wasSelected ) {
			setSkinState( UISkinState::StateSelected );

			mControlFlags |= UI_CTRL_FLAG_SELECTED;

			LBParent->mSelected.push_back( LBParent->getItemIndex( this ) );

			LBParent->onSelected();
		} else {
			mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

			LBParent->mSelected.remove( LBParent->getItemIndex( this ) );
		}
	} else {
		setSkinState( UISkinState::StateSelected );

		mControlFlags |= UI_CTRL_FLAG_SELECTED;

		LBParent->mSelected.clear();
		LBParent->mSelected.push_back( LBParent->getItemIndex( this ) );

		if ( !wasSelected ) {
			LBParent->onSelected();
		}
	}
}

void UIListBoxItem::update() {
	UITextBox::update();

	if ( mEnabled && mVisible ) {
		UIListBox * LBParent 	= reinterpret_cast<UIListBox*> ( getParent()->getParent() );
		Uint32 Flags 			= UIManager::instance()->getInput()->getClickTrigger();

		if ( isMouseOver() ) {
			if ( Flags & EE_BUTTONS_WUWD && LBParent->getVerticalScrollBar()->isVisible() ) {
				LBParent->getVerticalScrollBar()->getSlider()->manageClick( Flags );
			}
		}
	}
}

Uint32 UIListBoxItem::onMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	UIControl::onMouseExit( Pos, Flags );

	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		setSkinState( UISkinState::StateSelected );

	return 1;
}

void UIListBoxItem::unselect() {
	if ( mControlFlags & UI_CTRL_FLAG_SELECTED )
		mControlFlags &= ~UI_CTRL_FLAG_SELECTED;

	setSkinState( UISkinState::StateNormal );
}

bool UIListBoxItem::isSelected() const {
	return 0 != ( mControlFlags & UI_CTRL_FLAG_SELECTED );
}

void UIListBoxItem::onStateChange() {
	UIListBox * LBParent = reinterpret_cast<UIListBox*> ( getParent()->getParent() );

	if ( isSelected() && mSkinState->getState() != UISkinState::StateSelected ) {
		setSkinState( UISkinState::StateSelected );
	}

	if ( mSkinState->getState() == UISkinState::StateSelected ) {
		setFontColor( LBParent->getFontSelectedColor() );
	} else if ( mSkinState->getState() == UISkinState::StateMouseEnter ) {
		setFontColor( LBParent->getFontOverColor() );
	} else {
		setFontColor( LBParent->getFontColor() );
	}
}

}}
