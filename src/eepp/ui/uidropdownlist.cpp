#include <eepp/scene/actions/actions.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIDropDownList* UIDropDownList::NewWithTag( const std::string& tag ) {
	return eeNew( UIDropDownList, ( tag ) );
}

UIDropDownList* UIDropDownList::New() {
	return eeNew( UIDropDownList, () );
}

UIDropDownList::UIDropDownList( const std::string& tag ) :
	UITextInput( tag ), mListBox( NULL ), mFriendCtrl( NULL ) {
	clipEnable();
	setFlags( UI_AUTO_SIZE | UI_AUTO_PADDING );
	unsetFlags( UI_TEXT_SELECTION_ENABLED );

	setAllowEditing( false );

	applyDefaultTheme();

	mListBox = UIListBox::NewWithTag( mTag + "::listbox" );
	mListBox->setSize( getSize().getWidth(),
					   mStyleConfig.MaxNumVisibleItems * getSize().getHeight() );
	mListBox->setEnabled( false );
	mListBox->setVisible( false );
	// This will force to change the parent when shown, and force the CSS style reload.
	mListBox->setParent( this );

	mListBox->addEventListener( Event::OnWidgetFocusLoss,
								cb::Make1( this, &UIDropDownList::onListBoxFocusLoss ) );
	mListBox->addEventListener( Event::OnItemSelected,
								cb::Make1( this, &UIDropDownList::onItemSelected ) );
	mListBox->addEventListener( Event::OnItemClicked,
								cb::Make1( this, &UIDropDownList::onItemClicked ) );
	mListBox->addEventListener( Event::OnItemKeyDown,
								cb::Make1( this, &UIDropDownList::onItemKeyDown ) );
	mListBox->addEventListener( Event::KeyDown, cb::Make1( this, &UIDropDownList::onItemKeyDown ) );
	mListBox->addEventListener( Event::OnControlClear,
								cb::Make1( this, &UIDropDownList::onControlClear ) );
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

void UIDropDownList::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "dropdownlist" );

	onThemeLoaded();
}

void UIDropDownList::onSizeChange() {
	onAutoSize();

	UITextInput::onSizeChange();
}

void UIDropDownList::onThemeLoaded() {
	autoPadding();

	onAutoSize();
}

void UIDropDownList::setFriendControl( UINode* friendCtrl ) {
	mFriendCtrl = friendCtrl;
}

void UIDropDownList::onAutoSize() {
	Float max = eemax<Float>( PixelDensity::dpToPxI( getSkinSize().getHeight() ), getTextHeight() );

	if ( mHeightPolicy == SizePolicy::WrapContent ) {
		setInternalPixelsHeight( eeceil( max + mRealPadding.Top + mRealPadding.Bottom ) );
	} else if ( ( ( mFlags & UI_AUTO_SIZE ) || 0 == getSize().getHeight() ) && max > 0 ) {
		setInternalPixelsHeight( eeceil( max ) );
	}
}

UIListBox* UIDropDownList::getListBox() const {
	return mListBox;
}

Uint32 UIDropDownList::onMouseUp( const Vector2i& Pos, const Uint32& Flags ) {
	if ( mEnabled && mVisible && isMouseOver() && NULL != mListBox ) {
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
	if ( NULL == mListBox )
		return;

	if ( !mListBox->isVisible() ) {
		if ( !mStyleConfig.PopUpToRoot )
			mListBox->setParent( getWindowContainer() );
		else
			mListBox->setParent( getUISceneNode()->getRoot() );

		mListBox->toFront();

		Vector2f Pos( mDpPos.x, mDpPos.y + getSize().getHeight() );

		if ( mStyleConfig.PopUpToRoot ) {
			getParent()->nodeToWorld( Pos );
			Pos = PixelDensity::pxToDp( Pos );
		} else {
			Node* ParentLoop = getParent();
			Node* rp = getWindowContainer();
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
				mListBox->setSize(
					NULL != mFriendCtrl ? mFriendCtrl->getSize().getWidth() : getSize().getWidth(),
					( Int32 )( mStyleConfig.MaxNumVisibleItems * mListBox->getRowHeight() ) +
						tPadding.Top + tPadding.Bottom );
			} else {
				mListBox->setSize( NULL != mFriendCtrl ? mFriendCtrl->getSize().getWidth()
													   : getSize().getWidth(),
								   ( Int32 )( mListBox->getCount() * mListBox->getRowHeight() ) +
									   tPadding.Top + tPadding.Bottom );
			}

			mListBox->getVerticalScrollBar()->setValue( sliderValue );

			show();

			mListBox->setFocus();
		}
	} else {
		hide();
	}
}

bool UIDropDownList::getPopUpToRoot() const {
	return mStyleConfig.PopUpToRoot;
}

void UIDropDownList::setPopUpToRoot( bool popUpToRoot ) {
	mStyleConfig.PopUpToRoot = popUpToRoot;
}

Uint32 UIDropDownList::getMaxNumVisibleItems() const {
	return mStyleConfig.MaxNumVisibleItems;
}

void UIDropDownList::setMaxNumVisibleItems( const Uint32& maxNumVisibleItems ) {
	if ( maxNumVisibleItems != mStyleConfig.MaxNumVisibleItems ) {
		mStyleConfig.MaxNumVisibleItems = maxNumVisibleItems;

		if ( NULL != mListBox )
			mListBox->setSize( getSize().getWidth(),
							   mStyleConfig.MaxNumVisibleItems * mListBox->getRowHeight() );
	}
}

const UIDropDownList::StyleConfig& UIDropDownList::getStyleConfig() const {
	return mStyleConfig;
}

void UIDropDownList::setStyleConfig( const StyleConfig& styleConfig ) {
	mStyleConfig = styleConfig;

	setMaxNumVisibleItems( mStyleConfig.MaxNumVisibleItems );
	setPopUpToRoot( mStyleConfig.PopUpToRoot );
}

void UIDropDownList::onControlClear( const Event* ) {
	setText( "" );
}

void UIDropDownList::onItemKeyDown( const Event* Event ) {
	const KeyEvent* KEvent = reinterpret_cast<const KeyEvent*>( Event );

	if ( KEvent->getKeyCode() == KEY_RETURN )
		onItemClicked( Event );
}

void UIDropDownList::onListBoxFocusLoss( const Event* ) {
	if ( NULL == getEventDispatcher() )
		return;

	bool frienIsFocus = NULL != mFriendCtrl && mFriendCtrl == getEventDispatcher()->getFocusNode();
	bool isChildFocus = isChild( getEventDispatcher()->getFocusNode() );

	if ( getEventDispatcher()->getFocusNode() != this && !isChildFocus && !frienIsFocus ) {
		hide();
	}
}

void UIDropDownList::onItemClicked( const Event* ) {
	hide();
	setFocus();
}

void UIDropDownList::onItemSelected( const Event* ) {
	setText( mListBox->getItemSelectedText() );

	NodeMessage Msg( this, NodeMessage::Selected, mListBox->getItemSelectedIndex() );
	messagePost( &Msg );

	sendCommonEvent( Event::OnItemSelected );
}

void UIDropDownList::show() {
	if ( NULL == mListBox )
		return;

	mListBox->setEnabled( true );
	mListBox->setVisible( true );

	if ( NULL != getUISceneNode() &&
		 getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
		mListBox->runAction( Actions::Sequence::New(
			Actions::Fade::New( 255.f == mListBox->getAlpha() ? 0.f : mListBox->getAlpha(), 255.f,
								getUISceneNode()->getUIThemeManager()->getControlsFadeOutTime() ),
			Actions::Spawn::New( Actions::Enable::New(), Actions::Visible::New( true ) ) ) );
	}
}

void UIDropDownList::hide() {
	if ( NULL == mListBox )
		return;

	if ( NULL != getUISceneNode() &&
		 getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
		mListBox->runAction( Actions::Sequence::New(
			Actions::FadeOut::New(
				getUISceneNode()->getUIThemeManager()->getControlsFadeOutTime() ),
			Actions::Spawn::New( Actions::Disable::New(), Actions::Visible::New( false ) ) ) );
	} else {
		mListBox->setEnabled( false );
		mListBox->setVisible( false );
	}
}

Uint32 UIDropDownList::onMouseOver( const Vector2i& position, const Uint32& flags ) {
	if ( getParent()->isType( UI_TYPE_COMBOBOX ) ) {
		return UITextInput::onMouseOver( position, flags );
	} else {
		return UITextView::onMouseOver( position, flags );
	}
}

Uint32 UIDropDownList::onMouseLeave( const Vector2i& position, const Uint32& flags ) {
	if ( getParent()->isType( UI_TYPE_COMBOBOX ) ) {
		return UITextInput::onMouseLeave( position, flags );
	} else {
		return UITextView::onMouseLeave( position, flags );
	}
}

Uint32 UIDropDownList::onKeyDown( const KeyEvent& Event ) {
	if ( NULL != mListBox )
		mListBox->onKeyDown( Event );

	return UITextInput::onKeyDown( Event );
}

void UIDropDownList::destroyListBox() {
	if ( !SceneManager::instance()->isShootingDown() && NULL != mListBox &&
		 mListBox->getParent() != this ) {
		mListBox->setParent( this );
	}
}

bool UIDropDownList::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::PopUpToRoot:
			setPopUpToRoot( attribute.asBool() );
			break;
		case PropertyId::MaxVisibleItems:
			setMaxNumVisibleItems( attribute.asUint() );
			break;
		case PropertyId::SelectedIndex:
		case PropertyId::SelectedText:
		case PropertyId::ScrollBarStyle:
		case PropertyId::RowHeight:
		case PropertyId::VScrollMode:
		case PropertyId::HScrollMode:
			if ( NULL != mListBox )
				return mListBox->applyProperty( attribute );
			else
				return false;
		default:
			return UITextInput::applyProperty( attribute );
	}

	return true;
}

std::string UIDropDownList::getPropertyString( const PropertyDefinition* propertyDef,
											   const Uint32& propertyIndex ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::PopUpToRoot:
			return mStyleConfig.PopUpToRoot ? "true" : "false";
		case PropertyId::MaxVisibleItems:
			return String::toStr( mStyleConfig.MaxNumVisibleItems );
		case PropertyId::SelectedIndex:
		case PropertyId::SelectedText:
		case PropertyId::ScrollBarStyle:
		case PropertyId::RowHeight:
		case PropertyId::VScrollMode:
		case PropertyId::HScrollMode:
			if ( NULL != mListBox )
				return mListBox->getPropertyString( propertyDef, propertyIndex );
		default:
			return UITextInput::getPropertyString( propertyDef, propertyIndex );
	}

	return "";
}

void UIDropDownList::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	if ( NULL != mListBox )
		mListBox->loadItemsFromXmlNode( node );

	UITextInput::loadFromXmlNode( node );

	endAttributesTransaction();
}

}} // namespace EE::UI
