#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIDropDownList::UIDropDownList( UIDropDownList::CreateParams& Params ) :
	UITextInput( Params ),
	mListBox( Params.ListBox ),
	mFriendCtrl( NULL ),
	mMinNumVisibleItems( Params.MinNumVisibleItems ),
	mPopUpToMainControl( Params.PopUpToMainControl )
{
	setAllowEditing( false );

	applyDefaultTheme();

	if ( NULL == mListBox ) {
		Uint32 flags = UI_CLIP_ENABLE | UI_AUTO_PADDING;

		if ( Params.Flags & UI_TOUCH_DRAG_ENABLED )
			flags |= UI_TOUCH_DRAG_ENABLED;

		if ( Params.Flags & UI_TEXT_SELECTION_ENABLED )
			flags |= UI_TEXT_SELECTION_ENABLED;

		UITheme * Theme = UIThemeManager::instance()->getDefaultTheme();

		if ( NULL != Theme ) {
			mListBox = Theme->createListBox( NULL, Sizei( mSize.getWidth(), mMinNumVisibleItems * mSize.getHeight() ),Vector2i(), flags );
		} else {
			UIListBox::CreateParams LBParams;
			LBParams.Size 				= Sizei( mSize.getWidth(), mMinNumVisibleItems * mSize.getHeight() );
			LBParams.Flags 				= flags;
			LBParams.FontSelectedColor	= ColorA( 255, 255, 255, 255 );
			mListBox = eeNew( UIListBox, ( LBParams ) );
		}
	}

	mListBox->setEnabled( false );
	mListBox->setVisible( false );

	mListBox->addEventListener( UIEvent::EventOnComplexControlFocusLoss, cb::Make1( this, &UIDropDownList::onListBoxFocusLoss ) );
	mListBox->addEventListener( UIEvent::EventOnItemSelected	, cb::Make1( this, &UIDropDownList::onItemSelected ) );
	mListBox->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &UIDropDownList::onItemClicked ) );
	mListBox->addEventListener( UIEvent::EventOnItemKeyDown, cb::Make1( this, &UIDropDownList::onItemKeyDown ) );
	mListBox->addEventListener( UIEvent::EventKeyDown		, cb::Make1( this, &UIDropDownList::onItemKeyDown ) );
	mListBox->addEventListener( UIEvent::EventOnControlClear, cb::Make1( this, &UIDropDownList::onControlClear ) );
}

UIDropDownList::UIDropDownList() :
	UITextInput(),
	mListBox( NULL ),
	mFriendCtrl( NULL ),
	mMinNumVisibleItems( 10 ),
	mPopUpToMainControl( false )
{
	setFlags( UI_CLIP_ENABLE | UI_AUTO_SIZE | UI_AUTO_PADDING );

	setAllowEditing( false );

	applyDefaultTheme();

	mListBox = eeNew( UIListBox, () );
	mListBox->setSize( mSize.getWidth(), mMinNumVisibleItems * mSize.getHeight() );
	mListBox->setEnabled( false );
	mListBox->setVisible( false );

	mListBox->addEventListener( UIEvent::EventOnComplexControlFocusLoss, cb::Make1( this, &UIDropDownList::onListBoxFocusLoss ) );
	mListBox->addEventListener( UIEvent::EventOnItemSelected	, cb::Make1( this, &UIDropDownList::onItemSelected ) );
	mListBox->addEventListener( UIEvent::EventOnItemClicked, cb::Make1( this, &UIDropDownList::onItemClicked ) );
	mListBox->addEventListener( UIEvent::EventOnItemKeyDown, cb::Make1( this, &UIDropDownList::onItemKeyDown ) );
	mListBox->addEventListener( UIEvent::EventKeyDown		, cb::Make1( this, &UIDropDownList::onItemKeyDown ) );
	mListBox->addEventListener( UIEvent::EventOnControlClear, cb::Make1( this, &UIDropDownList::onControlClear ) );
}

UIDropDownList::~UIDropDownList() {
	destroyListBox();
}

Uint32 UIDropDownList::getType() const {
	return UI_TYPE_DROPDOWNLIST;
}

bool UIDropDownList::isType( const Uint32& type ) const {
	return UIDropDownList::getType() == type ? true : UITextInput::isType( type );
}

void UIDropDownList::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "dropdownlist" );

	doAftersetTheme();
}

void UIDropDownList::onSizeChange() {
	UIComplexControl::onSizeChange();

	autoSizeControl();
}

void UIDropDownList::autoSizeControl() {
	if ( mFlags & UI_AUTO_SIZE || 0 == mSize.getHeight() ) {
		setSize( mSize.x, getSkinSize().getHeight() );
	}
}

void UIDropDownList::doAftersetTheme() {
	autoPadding();

	autoSizeControl();
}

void UIDropDownList::setFriendControl( UIControl * friendCtrl ) {
	mFriendCtrl = friendCtrl;
}

void UIDropDownList::autoSize() {
}

UIListBox * UIDropDownList::getListBox() const {
	return mListBox;
}

Uint32 UIDropDownList::onMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( ( Flags & EE_BUTTON_LMASK ) && NULL == mFriendCtrl )
		showList();

	if ( NULL != mFriendCtrl ) {
		UITextInput::onMouseClick( Pos, Flags );
	}

	return 1;
}

void UIDropDownList::showList() {
	if ( !mListBox->isVisible() ) {
		if ( !mPopUpToMainControl )
			mListBox->setParent( NULL != mFriendCtrl ? mFriendCtrl->getParent() : getParent() );
		else
			mListBox->setParent( UIManager::instance()->getMainControl() );

		mListBox->toFront();

		Vector2i Pos( mPos.x, mPos.y + mSize.getHeight() );

		if ( mPopUpToMainControl ) {
			getParent()->controlToWorld( Pos );
			Pos = PixelDensity::pxToDpI( Pos );
		} else if ( NULL != mFriendCtrl ) {
			Pos = Vector2i( mFriendCtrl->getPosition().x, mFriendCtrl->getPosition().y + mFriendCtrl->getSize().getHeight() );
		}

		mListBox->setPosition( Pos );

		if ( mListBox->getCount() ) {
			Recti tPadding = mListBox->getContainerPadding();

			Float sliderValue = mListBox->getVerticalScrollBar()->getValue();

			if ( mMinNumVisibleItems < mListBox->getCount() )
				mListBox->setSize( NULL != mFriendCtrl ? mFriendCtrl->getSize().getWidth() : mSize.getWidth(), (Int32)( mMinNumVisibleItems * mListBox->getRowHeight() ) + tPadding.Top + tPadding.Bottom );
			else {
				mListBox->setSize( NULL != mFriendCtrl ? mFriendCtrl->getSize().getWidth() : mSize.getWidth(), (Int32)( mListBox->getCount() * mListBox->getRowHeight() ) + tPadding.Top + tPadding.Bottom );
			}

			mListBox->updateQuad();

			Rectf aabb( mListBox->getPolygon().toAABB() );
			Recti aabbi( aabb.Left, aabb.Top, aabb.Right, aabb.Bottom );

			if ( !UIManager::instance()->getMainControl()->getScreenRect().contains( aabbi ) ) {
				Pos = Vector2i( mPos.x, mPos.y );

				if ( mPopUpToMainControl ) {
					getParent()->controlToWorld( Pos );
					Pos = PixelDensity::pxToDpI( Pos );
				}

				Pos.y -= mListBox->getSize().getHeight();

				mListBox->setPosition( Pos );
			}

			mListBox->getVerticalScrollBar()->setValue( sliderValue );

			show();

			mListBox->setFocus();
		}
	} else {
		hide();
	}
}

bool UIDropDownList::getPopUpToMainControl() const {
	return mPopUpToMainControl;
}

void UIDropDownList::setPopUpToMainControl(bool popUpToMainControl) {
	mPopUpToMainControl = popUpToMainControl;
}

Uint32 UIDropDownList::getMinNumVisibleItems() const {
	return mMinNumVisibleItems;
}

void UIDropDownList::setMinNumVisibleItems(const Uint32 & minNumVisibleItems) {
	mMinNumVisibleItems = minNumVisibleItems;
}

void UIDropDownList::onControlClear( const UIEvent * Event ) {
	setText( "" );
}

void UIDropDownList::onItemKeyDown( const UIEvent * Event ) {
	const UIEventKey * KEvent = reinterpret_cast<const UIEventKey*> ( Event );

	if ( KEvent->getKeyCode() == KEY_RETURN )
		onItemClicked( Event );
}

void UIDropDownList::onListBoxFocusLoss( const UIEvent * Event ) {
	bool frienIsFocus = NULL != mFriendCtrl && mFriendCtrl == UIManager::instance()->getFocusControl();
	bool isChildFocus = isChild( UIManager::instance()->getFocusControl() );

	if ( UIManager::instance()->getFocusControl() != this && !isChildFocus && !frienIsFocus ) {
		hide();
	}
}

void UIDropDownList::onItemClicked( const UIEvent * Event ) {
	hide();
	setFocus();
}

void UIDropDownList::onItemSelected( const UIEvent * Event ) {
	setText( mListBox->getItemSelectedText() );

	UIMessage Msg( this, UIMessage::MsgSelected, mListBox->getItemSelectedIndex() );
	messagePost( &Msg );

	sendCommonEvent( UIEvent::EventOnItemSelected );
}

void UIDropDownList::show() {
	mListBox->setEnabled( true );
	mListBox->setVisible( true );

	if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
		mListBox->startAlphaAnim( 255.f == mListBox->getAlpha() ? 0.f : mListBox->getAlpha(), 255.f, UIThemeManager::instance()->getControlsFadeInTime() );
	}
}

void UIDropDownList::hide() {
	if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
		mListBox->disableFadeOut( UIThemeManager::instance()->getControlsFadeOutTime() );
	} else {
		mListBox->setEnabled( false );
		mListBox->setVisible( false );
	}
}

void UIDropDownList::update() {
	if ( mEnabled && mVisible ) {
		if ( isMouseOver() ) {
			Uint32 Flags 			= UIManager::instance()->getInput()->getClickTrigger();

			if ( Flags & EE_BUTTONS_WUWD ) {
				if ( Flags & EE_BUTTON_WUMASK ) {
					mListBox->selectPrev();
				} else if ( Flags & EE_BUTTON_WDMASK ) {
					mListBox->selectNext();
				}
			}
		}
	}

	UITextInput::update();
}

Uint32 UIDropDownList::onKeyDown( const UIEventKey &Event ) {
	mListBox->onKeyDown( Event );

	return UITextInput::onKeyDown( Event );
}

void UIDropDownList::destroyListBox() {
	if ( !UIManager::instance()->isShootingDown() ) {
		mListBox->close();
	}
}

}}
