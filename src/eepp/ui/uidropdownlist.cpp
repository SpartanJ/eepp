#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIDropDownList * UIDropDownList::New() {
	return eeNew( UIDropDownList, () );
}

UIDropDownList::UIDropDownList() :
	UITextInput(),
	mListBox( NULL ),
	mFriendCtrl( NULL )
{
	setFlags( UI_CLIP_ENABLE | UI_AUTO_SIZE | UI_AUTO_PADDING );
	unsetFlags( UI_TEXT_SELECTION_ENABLED );

	UITheme * theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != theme ) {
		mStyleConfig = theme->getDropDownListStyleConfig();
	}

	setAllowEditing( false );

	applyDefaultTheme();

	mListBox = UIListBox::New();
	mListBox->setSize( mSize.getWidth(), mStyleConfig.MaxNumVisibleItems * mSize.getHeight() );
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

	onThemeLoaded();
}

void UIDropDownList::onSizeChange() {
	UIWidget::onSizeChange();

	autoSizeControl();
}

void UIDropDownList::autoSizeControl() {
	if ( mFlags & UI_AUTO_SIZE || 0 == mSize.getHeight() ) {
		setSize( mSize.x, getSkinSize().getHeight() );
	}
}

void UIDropDownList::onThemeLoaded() {
	autoPadding();

	autoSizeControl();
}

void UIDropDownList::setFriendControl( UIControl * friendCtrl ) {
	mFriendCtrl = friendCtrl;
}

void UIDropDownList::onAutoSize() {
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
		if ( !mStyleConfig.PopUpToMainControl )
			mListBox->setParent( NULL != mFriendCtrl ? mFriendCtrl->getParent() : getParent() );
		else
			mListBox->setParent( UIManager::instance()->getMainControl() );

		mListBox->toFront();

		Vector2i Pos( mPos.x, mPos.y + mSize.getHeight() );

		if ( mStyleConfig.PopUpToMainControl ) {
			getParent()->controlToWorld( Pos );
			Pos = PixelDensity::pxToDpI( Pos );
		} else if ( NULL != mFriendCtrl ) {
			Pos = Vector2i( mFriendCtrl->getPosition().x, mFriendCtrl->getPosition().y + mFriendCtrl->getSize().getHeight() );
		}

		mListBox->setPosition( Pos );

		if ( mListBox->getCount() ) {
			Recti tPadding = mListBox->getContainerPadding();

			Float sliderValue = mListBox->getVerticalScrollBar()->getValue();

			if ( mStyleConfig.MaxNumVisibleItems < mListBox->getCount() )
				mListBox->setSize( NULL != mFriendCtrl ? mFriendCtrl->getSize().getWidth() : mSize.getWidth(), (Int32)( mStyleConfig.MaxNumVisibleItems * mListBox->getRowHeight() ) + tPadding.Top + tPadding.Bottom );
			else {
				mListBox->setSize( NULL != mFriendCtrl ? mFriendCtrl->getSize().getWidth() : mSize.getWidth(), (Int32)( mListBox->getCount() * mListBox->getRowHeight() ) + tPadding.Top + tPadding.Bottom );
			}

			mListBox->updateQuad();

			Rectf aabb( mListBox->getPolygon().toAABB() );
			Recti aabbi( aabb.Left, aabb.Top, aabb.Right, aabb.Bottom );

			if ( !UIManager::instance()->getMainControl()->getScreenRect().contains( aabbi ) ) {
				Pos = Vector2i( mPos.x, mPos.y );

				if ( mStyleConfig.PopUpToMainControl ) {
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
	return mStyleConfig.PopUpToMainControl;
}

void UIDropDownList::setPopUpToMainControl(bool popUpToMainControl) {
	mStyleConfig.PopUpToMainControl = popUpToMainControl;
}

Uint32 UIDropDownList::getMaxNumVisibleItems() const {
	return mStyleConfig.MaxNumVisibleItems;
}

void UIDropDownList::setMaxNumVisibleItems(const Uint32 & maxNumVisibleItems) {
	mStyleConfig.MaxNumVisibleItems = maxNumVisibleItems;

	mListBox->setSize( mSize.getWidth(), mStyleConfig.MaxNumVisibleItems * mSize.getHeight() );
}

DropDownListStyleConfig UIDropDownList::getStyleConfig() const {
	return mStyleConfig;
}

void UIDropDownList::setStyleConfig(const DropDownListStyleConfig & styleConfig) {
	mStyleConfig = styleConfig;

	mListBox->setFontStyleConfig( mStyleConfig );
	setMaxNumVisibleItems( mStyleConfig.MaxNumVisibleItems );
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
