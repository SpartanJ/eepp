#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIDropDownList * UIDropDownList::New() {
	return eeNew( UIDropDownList, () );
}

UIDropDownList::UIDropDownList() :
	UITextInput(),
	mListBox( NULL ),
	mFriendCtrl( NULL )
{
	clipEnable();
	setFlags( UI_AUTO_SIZE | UI_AUTO_PADDING );
	unsetFlags( UI_TEXT_SELECTION_ENABLED );

	UITheme * theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != theme ) {
		mStyleConfig = theme->getDropDownListStyleConfig();
	}

	setAllowEditing( false );

	applyDefaultTheme();

	mListBox = UIListBox::New();
	mListBox->setSize( mDpSize.getWidth(), mStyleConfig.MaxNumVisibleItems * mDpSize.getHeight() );
	mListBox->setEnabled( false );
	mListBox->setVisible( false );

	mListBox->addEventListener( Event::OnWidgetFocusLoss, cb::Make1( this, &UIDropDownList::onListBoxFocusLoss ) );
	mListBox->addEventListener( Event::OnItemSelected	, cb::Make1( this, &UIDropDownList::onItemSelected ) );
	mListBox->addEventListener( Event::OnItemClicked, cb::Make1( this, &UIDropDownList::onItemClicked ) );
	mListBox->addEventListener( Event::OnItemKeyDown, cb::Make1( this, &UIDropDownList::onItemKeyDown ) );
	mListBox->addEventListener( Event::KeyDown		, cb::Make1( this, &UIDropDownList::onItemKeyDown ) );
	mListBox->addEventListener( Event::OnControlClear, cb::Make1( this, &UIDropDownList::onControlClear ) );
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
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "dropdownlist" );

	onThemeLoaded();
}

void UIDropDownList::onSizeChange() {
	UIWidget::onSizeChange();

	autoSizeControl();
}

void UIDropDownList::autoSizeControl() {
	if ( ( mFlags & UI_AUTO_SIZE || 0 == mDpSize.getHeight() ) && 0 != getSkinSize().getHeight() ) {
		setSize( mDpSize.x, getSkinSize().getHeight() );
	}
}

void UIDropDownList::onThemeLoaded() {
	autoPadding();

	autoSizeControl();
}

void UIDropDownList::setFriendControl( UINode * friendCtrl ) {
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
			mListBox->setParent( NULL != mFriendCtrl ? mFriendCtrl->getParent() : getWindowContainer() );
		else
			mListBox->setParent( mSceneNode );

		mListBox->toFront();

		Vector2f Pos( mDpPos.x, mDpPos.y + mDpSize.getHeight() );

		if ( mStyleConfig.PopUpToMainControl ) {
			getParent()->nodeToWorld( Pos );
			Pos = PixelDensity::pxToDp( Pos );
		} else if ( NULL != mFriendCtrl ) {
			Pos = Vector2f( mFriendCtrl->getPosition().x, mFriendCtrl->getPosition().y + mFriendCtrl->getSize().getHeight() );
		} else {
			Node * ParentLoop = getParent();
			Node * rp = getWindowContainer();
			while ( rp != ParentLoop ) {
				Pos += ParentLoop->getPosition();
				ParentLoop = ParentLoop->getParent();
			}
		}

		mListBox->setPosition( Pos );

		if ( mListBox->getCount() ) {
			Rectf tPadding = mListBox->getContainerPadding();

			Float sliderValue = mListBox->getVerticalScrollBar()->getValue();

			if ( mStyleConfig.MaxNumVisibleItems < mListBox->getCount() )
				mListBox->setSize( NULL != mFriendCtrl ? mFriendCtrl->getSize().getWidth() : mDpSize.getWidth(), (Int32)( mStyleConfig.MaxNumVisibleItems * mListBox->getRowHeight() ) + tPadding.Top + tPadding.Bottom );
			else {
				mListBox->setSize( NULL != mFriendCtrl ? mFriendCtrl->getSize().getWidth() : mDpSize.getWidth(), (Int32)( mListBox->getCount() * mListBox->getRowHeight() ) + tPadding.Top + tPadding.Bottom );
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

	mListBox->setSize( mDpSize.getWidth(), mStyleConfig.MaxNumVisibleItems * mDpSize.getHeight() );
}

UIDropDownListStyleConfig UIDropDownList::getStyleConfig() const {
	return mStyleConfig;
}

void UIDropDownList::setStyleConfig(const UIDropDownListStyleConfig & styleConfig) {
	mStyleConfig = styleConfig;

	mListBox->setFontStyleConfig( mStyleConfig );
	setMaxNumVisibleItems( mStyleConfig.MaxNumVisibleItems );
}

void UIDropDownList::onControlClear( const Event * Event ) {
	setText( "" );
}

void UIDropDownList::onItemKeyDown( const Event * Event ) {
	const KeyEvent * KEvent = reinterpret_cast<const KeyEvent*> ( Event );

	if ( KEvent->getKeyCode() == KEY_RETURN )
		onItemClicked( Event );
}

void UIDropDownList::onListBoxFocusLoss( const Event * Event ) {
	if ( NULL == getEventDispatcher() )
		return;

	bool frienIsFocus = NULL != mFriendCtrl && mFriendCtrl == getEventDispatcher()->getFocusControl();
	bool isChildFocus = isChild( getEventDispatcher()->getFocusControl() );

	if ( getEventDispatcher()->getFocusControl() != this && !isChildFocus && !frienIsFocus ) {
		hide();
	}
}

void UIDropDownList::onItemClicked( const Event * Event ) {
	hide();
	setFocus();
}

void UIDropDownList::onItemSelected( const Event * Event ) {
	setText( mListBox->getItemSelectedText() );

	NodeMessage Msg( this, NodeMessage::Selected, mListBox->getItemSelectedIndex() );
	messagePost( &Msg );

	sendCommonEvent( Event::OnItemSelected );
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

void UIDropDownList::update( const Time& time ) {
	if ( mEnabled && mVisible && NULL != getEventDispatcher() ) {
		if ( isMouseOver() ) {
			Uint32 Flags 			= getEventDispatcher()->getClickTrigger();

			if ( Flags & EE_BUTTONS_WUWD ) {
				if ( Flags & EE_BUTTON_WUMASK ) {
					mListBox->selectPrev();
				} else if ( Flags & EE_BUTTON_WDMASK ) {
					mListBox->selectNext();
				}
			}
		}
	}

	UITextInput::update( time );
}

Uint32 UIDropDownList::onKeyDown( const KeyEvent &Event ) {
	mListBox->onKeyDown( Event );

	return UITextInput::onKeyDown( Event );
}

void UIDropDownList::destroyListBox() {
	if ( !SceneManager::instance()->isShootingDown() ) {
		mListBox->close();
	}
}

void UIDropDownList::loadFromXmlNode(const pugi::xml_node & node) {
	beginPropertiesTransaction();

	UITextInput::loadFromXmlNode( node );

	mListBox->loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "popuptomaincontrol" == name ) {
			setPopUpToMainControl( ait->as_bool() );
		} else if ( "maxnumvisibleitems" == name ) {
			setMaxNumVisibleItems( ait->as_uint() );
		}
	}

	endPropertiesTransaction();
}

}}
