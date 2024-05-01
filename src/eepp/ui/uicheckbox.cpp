#include <eepp/graphics/text.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uicheckbox.hpp>

namespace EE { namespace UI {

UICheckBox* UICheckBox::New() {
	return eeNew( UICheckBox, () );
}

UICheckBox* UICheckBox::NewWithTag( const std::string& tag ) {
	return eeNew( UICheckBox, ( tag ) );
}

UICheckBox::UICheckBox() : UICheckBox( "checkbox" ) {}

UICheckBox::UICheckBox( const std::string& tag ) :
	UITextView( tag ), mChecked( false ), mTextSeparation( 4 ) {
	auto cb = [this]( const Event* ) { onAutoSize(); };

	mActiveButton = UIWidget::NewWithTag( tag + "::active" );
	mActiveButton->setVisible( false );
	mActiveButton->setEnabled( true );
	mActiveButton->setParent( this );
	mActiveButton->setPosition( 0, 0 );
	mActiveButton->setSize( 8, 8 );
	mActiveButton->addEventListener( Event::OnSizeChange, cb );
	mActiveButton->unsetTabFocusable();

	mInactiveButton = UIWidget::NewWithTag( tag + "::inactive" );
	mInactiveButton->setVisible( true );
	mInactiveButton->setEnabled( true );
	mInactiveButton->setParent( this );
	mInactiveButton->setPosition( 0, 0 );
	mInactiveButton->setSize( 8, 8 );
	mInactiveButton->addEventListener( Event::OnSizeChange, cb );
	mInactiveButton->unsetTabFocusable();

	onPaddingChange();

	applyDefaultTheme();
}

UICheckBox::~UICheckBox() {}

Uint32 UICheckBox::getType() const {
	return UI_TYPE_CHECKBOX;
}

bool UICheckBox::isType( const Uint32& type ) const {
	return UICheckBox::getType() == type ? true : UITextView::isType( type );
}

void UICheckBox::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "checkbox" );

	mActiveButton->setThemeSkin( Theme, "checkbox_active" );
	mInactiveButton->setThemeSkin( Theme, "checkbox_inactive" );

	onThemeLoaded();
}

void UICheckBox::onThemeLoaded() {
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

void UICheckBox::onAutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		if ( getSize().getWidth() == 0 ) {
			setInternalPixelsWidth( (int)mTextCache->getTextWidth() +
									mActiveButton->getPixelsSize().getWidth() + mTextSeparation +
									mPaddingPx.Left + mPaddingPx.Right );
		}

		if ( getSize().getHeight() == 0 ) {
			setInternalHeight( mActiveButton->getSize().getHeight() + mPadding.Top +
							   mPadding.Bottom );
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

void UICheckBox::onSizeChange() {
	alignFix();
	UITextView::onSizeChange();
}

void UICheckBox::onTextChanged() {
	sendCommonEvent( Event::OnTextChanged );
	invalidateDraw();
}

Uint32 UICheckBox::onMessage( const NodeMessage* msg ) {
	switch ( msg->getMsg() ) {
		case NodeMessage::MouseClick: {
			if ( msg->getFlags() & EE_BUTTON_LMASK ) {
				if ( CheckMode::TextAndButton == mCheckMode ||
					 ( msg->getSender() == mActiveButton || msg->getSender() == mInactiveButton ) )
					switchState();
			}

			if ( NULL != getEventDispatcher() &&
				 ( msg->getSender() == mActiveButton || msg->getSender() == mInactiveButton ) ) {
				sendMouseEvent( Event::MouseClick, getEventDispatcher()->getMousePos(),
								getEventDispatcher()->getPressTrigger() );
			}

			return 1;
		}
	}

	return 0;
}

void UICheckBox::switchState() {
	setChecked( !mChecked );
}

UICheckBox* UICheckBox::setChecked( const bool& checked ) {
	if ( !checked ) {
		mActiveButton->setVisible( false );
		mInactiveButton->setVisible( true );

		mChecked = false;
		unsetFlags( UI_CHECKED );
		popState( UIState::StateChecked );
	} else {
		mActiveButton->setVisible( true );
		mInactiveButton->setVisible( false );

		mChecked = true;
		setFlags( UI_CHECKED );
		pushState( UIState::StateChecked );
	}

	alignFix();

	onValueChange();

	return this;
}

const bool& UICheckBox::isChecked() const {
	return mChecked;
}

void UICheckBox::onPaddingChange() {
	mActiveButton->setPosition( mPadding.Left, mActiveButton->getPosition().y );
	mInactiveButton->setPosition( mPadding.Left, mInactiveButton->getPosition().y );
	alignFix();
	UITextView::onPaddingChange();
}

void UICheckBox::alignFix() {
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

UIWidget* UICheckBox::getCheckedButton() const {
	return mActiveButton;
}

UIWidget* UICheckBox::getInactiveButton() const {
	return mInactiveButton;
}

UIWidget* UICheckBox::getCurrentButton() const {
	return mChecked ? mActiveButton : mInactiveButton;
}

Int32 UICheckBox::getTextSeparation() const {
	return mTextSeparation;
}

void UICheckBox::setTextSeparation( const Int32& textSeparation ) {
	mTextSeparation = textSeparation;

	setPadding( getPadding() );
}

std::string UICheckBox::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Selected:
		case PropertyId::Checked:
		case PropertyId::Value:
			return isChecked() ? "true" : "false";
		case PropertyId::CheckMode:
			return mCheckMode == CheckMode::TextAndButton ? "element" : "button";
		default:
			return UITextView::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UICheckBox::getPropertiesImplemented() const {
	auto props = UITextView::getPropertiesImplemented();
	props.push_back( PropertyId::Checked );
	props.push_back( PropertyId::CheckMode );
	return props;
}

bool UICheckBox::getCheckMode() const {
	return mCheckMode;
}

void UICheckBox::setCheckMode( UICheckBox::CheckMode mode ) {
	mCheckMode = mode;
}

bool UICheckBox::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Selected:
		case PropertyId::Checked:
		case PropertyId::Value:
			setChecked( attribute.asBool() );
			break;
		case PropertyId::CheckMode:
			setCheckMode( String::toLower( attribute.value() ) == "button"
							  ? CheckMode::Button
							  : CheckMode::TextAndButton );
			break;
		case PropertyId::Tooltip:
			if ( mActiveButton )
				mActiveButton->applyProperty( attribute );
		default:
			return UITextView::applyProperty( attribute );
	}

	return true;
}

Uint32 UICheckBox::onKeyDown( const KeyEvent& Event ) {
	UITextView::onKeyDown( Event );

	if ( Event.getKeyCode() == KEY_SPACE ) {
		if ( Sys::getTicks() - mLastTick > 250 ) {
			mLastTick = Sys::getTicks();

			setChecked( !mChecked );
		}
	}

	return 1;
}

void UICheckBox::onAlphaChange() {
	UITextView::onAlphaChange();

	mActiveButton->setAlpha( mAlpha );
	mInactiveButton->setAlpha( mAlpha );
}

}} // namespace EE::UI
