#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIDropDownList * UIDropDownList::New() {
	return eeNew( UIDropDownList, () );
}

UIDropDownList::UIDropDownList() :
	UITextInput( "dropdownlist" ),
	mListBox( NULL ),
	mFriendCtrl( NULL )
{
	clipEnable();
	setFlags( UI_AUTO_SIZE | UI_AUTO_PADDING );
	unsetFlags( UI_TEXT_SELECTION_ENABLED );

	setAllowEditing( false );

	applyDefaultTheme();

	mListBox = UIListBox::NewWithTag( "dropdownlist::listbox" );
	mListBox->setSize( getSize().getWidth(), mStyleConfig.MaxNumVisibleItems * getSize().getHeight() );
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
	onAutoSize();

	UITextView::onSizeChange();
}

void UIDropDownList::onThemeLoaded() {
	autoPadding();

	onAutoSize();
}

void UIDropDownList::setFriendControl( UINode * friendCtrl ) {
	mFriendCtrl = friendCtrl;
}

void UIDropDownList::onAutoSize() {
	if ( mLayoutHeightRule == LayoutSizeRule::WrapContent ) {
		setInternalPixelsHeight( PixelDensity::dpToPxI( getSkinSize().getHeight() ) + mRealPadding.Top + mRealPadding.Bottom );
	} else if ( ( mFlags & UI_AUTO_SIZE || 0 == getSize().getHeight() ) && 0 != getSkinSize().getHeight() ) {
		setInternalHeight( getSkinSize().getHeight() );
	}
}

UIListBox * UIDropDownList::getListBox() const {
	return mListBox;
}

Uint32 UIDropDownList::onMouseUp( const Vector2i& Pos, const Uint32& Flags ) {
	if ( mEnabled && mVisible && isMouseOver() ) {
		if ( Flags & EE_BUTTONS_WUWD ) {
			if ( Flags & EE_BUTTON_WUMASK ) {
				mListBox->selectPrev();
			} else if ( Flags & EE_BUTTON_WDMASK ) {
				if ( mListBox->getItemSelectedIndex() != eeINDEX_NOT_FOUND ) {
					mListBox->selectNext();
				} else {
					mListBox->setSelected( 0 );
				}
			}
		}
	}

	return UITextInput::onMouseUp( Pos, Flags );
}

Uint32 UIDropDownList::onMouseClick( const Vector2i& Pos, const Uint32& Flags ) {
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
			mListBox->setParent( getWindowContainer() );
		else
			mListBox->setParent( mSceneNode );

		mListBox->toFront();

		Vector2f Pos( mDpPos.x, mDpPos.y + getSize().getHeight() );

		if ( mStyleConfig.PopUpToMainControl ) {
			getParent()->nodeToWorld( Pos );
			Pos = PixelDensity::pxToDp( Pos );
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

			if ( mStyleConfig.MaxNumVisibleItems < mListBox->getCount() ) {
				mListBox->setSize( NULL != mFriendCtrl ? mFriendCtrl->getSize().getWidth() : getSize().getWidth(), (Int32)( mStyleConfig.MaxNumVisibleItems * mListBox->getRowHeight() ) + tPadding.Top + tPadding.Bottom );
			} else {
				mListBox->setSize( NULL != mFriendCtrl ? mFriendCtrl->getSize().getWidth() : getSize().getWidth(), (Int32)( mListBox->getCount() * mListBox->getRowHeight() ) + tPadding.Top + tPadding.Bottom );
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

	mListBox->setSize( getSize().getWidth(), mStyleConfig.MaxNumVisibleItems * getSize().getHeight() );
}

const UIDropDownList::StyleConfig& UIDropDownList::getStyleConfig() const {
	return mStyleConfig;
}

void UIDropDownList::setStyleConfig( const StyleConfig& styleConfig ) {
	mStyleConfig = styleConfig;

	setMaxNumVisibleItems( mStyleConfig.MaxNumVisibleItems );
	setPopUpToMainControl( mStyleConfig.PopUpToMainControl );
}

void UIDropDownList::onControlClear( const Event * ) {
	setText( "" );
}

void UIDropDownList::onItemKeyDown( const Event * Event ) {
	const KeyEvent * KEvent = reinterpret_cast<const KeyEvent*> ( Event );

	if ( KEvent->getKeyCode() == KEY_RETURN )
		onItemClicked( Event );
}

void UIDropDownList::onListBoxFocusLoss( const Event * ) {
	if ( NULL == getEventDispatcher() )
		return;

	bool frienIsFocus = NULL != mFriendCtrl && mFriendCtrl == getEventDispatcher()->getFocusControl();
	bool isChildFocus = isChild( getEventDispatcher()->getFocusControl() );

	if ( getEventDispatcher()->getFocusControl() != this && !isChildFocus && !frienIsFocus ) {
		hide();
	}
}

void UIDropDownList::onItemClicked( const Event * ) {
	hide();
	setFocus();
}

void UIDropDownList::onItemSelected( const Event * ) {
	setText( mListBox->getItemSelectedText() );

	NodeMessage Msg( this, NodeMessage::Selected, mListBox->getItemSelectedIndex() );
	messagePost( &Msg );

	sendCommonEvent( Event::OnItemSelected );
}

void UIDropDownList::show() {
	mListBox->setEnabled( true );
	mListBox->setVisible( true );

	if ( NULL != getUISceneNode() &&  getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
		mListBox->runAction( Actions::Sequence::New(
								 Actions::Fade::New( 255.f == mListBox->getAlpha() ? 0.f : mListBox->getAlpha(), 255.f, getUISceneNode()->getUIThemeManager()->getControlsFadeOutTime() ),
								 Actions::Spawn::New( Actions::Enable::New(), Actions::Visible::New( true ) ) ) );
	}
}

void UIDropDownList::hide() {
	if ( NULL != getUISceneNode() &&  getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
		mListBox->runAction( Actions::Sequence::New( Actions::FadeOut::New( getUISceneNode()->getUIThemeManager()->getControlsFadeOutTime() ),
													 Actions::Spawn::New( Actions::Disable::New(), Actions::Visible::New( false ) ) ) );
	} else {
		mListBox->setEnabled( false );
		mListBox->setVisible( false );
	}
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

bool UIDropDownList::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) ) return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::PopUpToMainControl:
			setPopUpToMainControl( attribute.asBool() );
			break;
		case PropertyId::MaxVisibleItems:
			setMaxNumVisibleItems( attribute.asUint() );
			break;
		case PropertyId::SelectedIndex:
		case PropertyId::SelectedText:
		case PropertyId::ScrollBarType:
		case PropertyId::RowHeight:
		case PropertyId::VScrollMode:
		case PropertyId::HScrollMode:
			return mListBox->applyProperty( attribute );
		default:
			return UITextInput::applyProperty( attribute );
	}

	return true;
}

std::string UIDropDownList::getPropertyString( const PropertyDefinition* propertyDef ) {
	if ( NULL == propertyDef ) return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::PopUpToMainControl:
			return mStyleConfig.PopUpToMainControl ? "true" : "false";
		case PropertyId::MaxVisibleItems:
			return String::toStr( mStyleConfig.MaxNumVisibleItems );
		case PropertyId::SelectedIndex:
		case PropertyId::SelectedText:
		case PropertyId::ScrollBarType:
		case PropertyId::RowHeight:
		case PropertyId::VScrollMode:
		case PropertyId::HScrollMode:
			return mListBox->getPropertyString( propertyDef );
		default:
			return UITextInput::getPropertyString( propertyDef );
	}
}

void UIDropDownList::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	mListBox->loadItemsFromXmlNode( node );

	UITextInput::loadFromXmlNode( node );

	endAttributesTransaction();
}

}}
