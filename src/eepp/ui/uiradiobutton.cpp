#include <eepp/graphics/text.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiradiobutton.hpp>

namespace EE { namespace UI {

UIRadioButton* UIRadioButton::New() {
	return eeNew( UIRadioButton, () );
}

UIRadioButton::UIRadioButton() :
	UITextView( "radiobutton" ),
	mActiveButton( NULL ),
	mInactiveButton( NULL ),
	mActive( false ),
	mTextSeparation( 4 ) {
	auto cb = [this]( const Event* ) { onAutoSize(); };

	mActiveButton = UIWidget::NewWithTag( "radiobutton::active" );
	mActiveButton->setVisible( false );
	mActiveButton->setEnabled( true );
	mActiveButton->setParent( this );
	mActiveButton->setPosition( 0, 0 );
	mActiveButton->setSize( 8, 8 );
	mActiveButton->addEventListener( Event::OnSizeChange, cb );
	mActiveButton->unsetTabFocusable();

	mInactiveButton = UIWidget::NewWithTag( "radiobutton::inactive" );
	mInactiveButton->setVisible( true );
	mInactiveButton->setEnabled( true );
	mInactiveButton->setParent( this );
	mInactiveButton->setPosition( 0, 0 );
	mInactiveButton->setSize( 9, 8 );
	mInactiveButton->addEventListener( Event::OnSizeChange, cb );
	mInactiveButton->unsetTabFocusable();

	onPaddingChange();

	applyDefaultTheme();
}

UIRadioButton::~UIRadioButton() {}

Uint32 UIRadioButton::getType() const {
	return UI_TYPE_RADIOBUTTON;
}

bool UIRadioButton::isType( const Uint32& type ) const {
	return UIRadioButton::getType() == type ? true : UITextView::isType( type );
}

void UIRadioButton::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );
	setThemeSkin( Theme, "radiobutton" );

	mActiveButton->setThemeSkin( Theme, "radiobutton_active" );
	mInactiveButton->setThemeSkin( Theme, "radiobutton_inactive" );

	onThemeLoaded();
}

void UIRadioButton::onThemeLoaded() {
	UISkin* tSkin = mActiveButton->getSkin();

	if ( tSkin )
		mActiveButton->setSize( tSkin->getSize() );

	tSkin = mInactiveButton->getSkin();

	if ( NULL != tSkin )
		mInactiveButton->setSize( tSkin->getSize() );

	setMinSize( mActiveButton->getSkinSize() );

	onPaddingChange();

	UIWidget::onThemeLoaded();
}

void UIRadioButton::onAutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		if ( getSize().getWidth() == 0 ) {
			setInternalPixelsWidth(
				(int)mTextCache->getTextWidth() + mActiveButton->getPixelsSize().getWidth() +
				PixelDensity::dpToPx( mTextSeparation ) + mPaddingPx.Left + mPaddingPx.Right );
		}

		if ( getSize().getHeight() == 0 ) {
			setInternalHeight( mActiveButton->getSize().getHeight() + mPaddingPx.Top +
							   mPaddingPx.Bottom );
		}
	}

	if ( mWidthPolicy == SizePolicy::WrapContent ) {
		setInternalPixelsWidth( (int)mTextCache->getTextWidth() + mPaddingPx.Left +
								mPaddingPx.Right + mActiveButton->getPixelsSize().getWidth() +
								PixelDensity::dpToPx( mTextSeparation ) );
	}

	if ( mHeightPolicy == SizePolicy::WrapContent ) {
		setInternalPixelsHeight( (int)mTextCache->getTextHeight() + mPaddingPx.Top +
								 mPaddingPx.Bottom );
	}

	alignFix();
}

void UIRadioButton::onSizeChange() {
	UITextView::onSizeChange();

	alignFix();
}

Uint32 UIRadioButton::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseUp: {
			if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
				switchState();
			}

			if ( NULL != getEventDispatcher() ) {
				if ( Msg->getSender() == mActiveButton || Msg->getSender() == mInactiveButton ) {
					sendMouseEvent( Event::MouseClick, getEventDispatcher()->getMousePos(),
									getEventDispatcher()->getPressTrigger() );
				}
			}

			return 1;
		}
	}

	return 0;
}

void UIRadioButton::switchState() {
	setActive( !mActive );
}

void UIRadioButton::setActive( const bool& active ) {
	if ( !active ) {
		if ( checkActives() ) {
			mActiveButton->setVisible( false );
			mInactiveButton->setVisible( true );

			mActive = false;

			onValueChange();
		}
	} else {
		mActiveButton->setVisible( true );
		mInactiveButton->setVisible( false );

		mActive = true;

		onValueChange();
	}

	if ( active && NULL != mParentNode ) {
		Node* tChild = mParentNode->getFirstChild();

		while ( NULL != tChild ) {
			if ( tChild->isType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					UIRadioButton* tRB = tChild->asType<UIRadioButton>();

					if ( tRB->isActive() )
						tRB->setActive( false );
				}
			}

			tChild = tChild->getNextNode();
		}
	}
}

bool UIRadioButton::checkActives() {
	if ( NULL != mParentNode ) {
		Node* tChild = mParentNode->getFirstChild();

		while ( NULL != tChild ) {
			if ( tChild->isType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					UIRadioButton* tRB = tChild->asType<UIRadioButton>();

					if ( tRB->isActive() )
						return true;
				}
			}

			tChild = tChild->getNextNode();
		}
	}

	return false;
}

void UIRadioButton::autoActivate() {
	eeASSERT( NULL != mParentNode );

	if ( NULL != mParentNode ) {
		Node* tChild = mParentNode->getFirstChild();

		while ( NULL != tChild ) {
			if ( tChild->isType( UI_TYPE_RADIOBUTTON ) ) {
				if ( tChild != this ) {
					UIRadioButton* tRB = tChild->asType<UIRadioButton>();

					if ( tRB->isActive() ) {
						return;
					}
				}
			}

			tChild = tChild->getNextNode();
		}
	}

	setActive( true );
}

const bool& UIRadioButton::isActive() const {
	return mActive;
}

void UIRadioButton::onPaddingChange() {
	mActiveButton->setPosition( mPadding.Left, mActiveButton->getPosition().y );
	mInactiveButton->setPosition( mPadding.Left, mInactiveButton->getPosition().y );
	alignFix();
	UITextView::onPaddingChange();
}

void UIRadioButton::alignFix() {
	UITextView::alignFix();

	mActiveButton->centerVertical();
	mInactiveButton->centerVertical();

	switch ( Font::getHorizontalAlign( getFlags() ) ) {
		case UI_HALIGN_CENTER:
			mRealAlignOffset.x = (Float)( (Int32)( ( mSize.x - mPaddingPx.Left - mPaddingPx.Right -
													 mTextCache->getTextWidth() -
													 mActiveButton->getPixelsSize().getWidth() +
													 PixelDensity::dpToPx( mTextSeparation ) ) /
												   2.f ) ) +
								 mActiveButton->getPixelsSize().getWidth() +
								 PixelDensity::dpToPx( mTextSeparation );
			break;
		case UI_HALIGN_RIGHT:
			mRealAlignOffset.x = ( (Float)mSize.x - mPaddingPx.Left - mPaddingPx.Right -
								   (Float)mTextCache->getTextWidth() );
			break;
		case UI_HALIGN_LEFT:
			mRealAlignOffset.x =
				mActiveButton->getPixelsSize().getWidth() + PixelDensity::dpToPx( mTextSeparation );
			break;
	}
}

UIWidget* UIRadioButton::getActiveButton() const {
	return mActiveButton;
}

UIWidget* UIRadioButton::getInactiveButton() const {
	return mInactiveButton;
}

Int32 UIRadioButton::getTextSeparation() const {
	return mTextSeparation;
}

void UIRadioButton::setTextSeparation( const Int32& textSeparation ) {
	mTextSeparation = textSeparation;

	setPadding( getPadding() );
}

std::string UIRadioButton::getPropertyString( const PropertyDefinition* propertyDef,
											  const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Selected:
		case PropertyId::Value:
			return isActive() ? "true" : "false";
			break;
		default:
			return UITextView::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIRadioButton::getPropertiesImplemented() const {
	auto props = UITextView::getPropertiesImplemented();
	auto local = { PropertyId::Selected };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

bool UIRadioButton::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Selected:
		case PropertyId::Value:
			setActive( attribute.asBool() );
			break;
		default:
			return UITextView::applyProperty( attribute );
	}

	return true;
}

Uint32 UIRadioButton::onKeyDown( const KeyEvent& Event ) {
	if ( Event.getKeyCode() == KEY_SPACE ) {
		if ( Sys::getTicks() - mLastTick > 250 ) {
			mLastTick = Sys::getTicks();

			setActive( true );
		}
	}

	return UITextView::onKeyDown( Event );
}

void UIRadioButton::onAlphaChange() {
	UITextView::onAlphaChange();

	mActiveButton->setAlpha( mAlpha );
	mInactiveButton->setAlpha( mAlpha );
}

}} // namespace EE::UI
