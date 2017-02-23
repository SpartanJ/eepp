#include <eepp/ui/uipushbutton.hpp>

namespace EE { namespace UI {

UIPushButton::UIPushButton( const UIPushButton::CreateParams& Params ) :
	UIComplexControl( Params ),
	mFontColor( Params.FontColor ),
	mFontOverColor( Params.FontOverColor ),
	mIcon( NULL ),
	mTextBox( NULL ),
	mIconSpace( Params.IconHorizontalMargin )
{
	UIGfx::CreateParams GfxParams;
	GfxParams.setParent( this );
	GfxParams.SubTexture = Params.Icon;

	if ( Params.IconMinSize.x != 0 && Params.IconMinSize.y != 0 ) {
		GfxParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	} else {
		GfxParams.Flags = UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	}

	mIcon = eeNew( UIGfx, ( GfxParams ) );

	if ( Params.IconMinSize.x != 0 && Params.IconMinSize.y != 0 ) {
		mIcon->setSize( Params.IconMinSize );
	}

	mIcon->setVisible( true );
	mIcon->setEnabled( false );

	setIcon( Params.Icon );

	UITextBox::CreateParams TxtParams = Params;
	TxtParams.setParent( this );
	TxtParams.Flags 			= HAlignGet( Params.Flags ) | VAlignGet( Params.Flags );
	TxtParams.Font				= Params.Font;
	TxtParams.FontColor 		= Params.FontColor;
	TxtParams.FontShadowColor 	= Params.FontShadowColor;

	if ( TxtParams.Flags & UI_CLIP_ENABLE )
		TxtParams.Flags &= ~UI_CLIP_ENABLE;

	mTextBox = eeNew( UITextBox, ( TxtParams ) );
	mTextBox->setVisible( true );
	mTextBox->setEnabled( false );

	if ( Params.IconAutoMargin )
		mControlFlags |= UI_CTRL_FLAG_FREE_USE;

	onSizeChange();

	applyDefaultTheme();
}

UIPushButton::~UIPushButton() {
}

Uint32 UIPushButton::getType() const {
	return UI_TYPE_PUSHBUTTON;
}

bool UIPushButton::isType( const Uint32& type ) const {
	return UIPushButton::getType() == type ? true : UIComplexControl::isType( type );
}

void UIPushButton::onSizeChange() {
	if ( NULL != mTextBox ) {
		mTextBox->setSize( mSize );
		mTextBox->setPosition( 0, 0 );
	}

	mIcon->setPosition( mIconSpace, 0 );
	mIcon->centerVertical();

	if ( NULL != mTextBox ) {
		switch ( fontHAlignGet( getFlags() ) ) {
			case UI_HALIGN_LEFT:
				mTextBox->setPosition( mIcon->getPosition().x + mIcon->getSize().getWidth(), 0 );
				mTextBox->setSize( mSize.getWidth() - mIcon->getPosition().x + mIcon->getSize().getWidth(), mSize.getHeight() );
				break;
			case UI_HALIGN_CENTER:
				if ( NULL != mIcon->getSubTexture() ) {
					if ( mIcon->getPosition().x + mIcon->getSize().getWidth() >= mTextBox->alignOffset().x ) {
						mTextBox->setPosition( mIcon->getPosition().x + mIcon->getSize().getWidth() + 1 - mTextBox->alignOffset().x, mTextBox->getPosition().y );
					}
				}

				break;
		}
	}

	if ( NULL != mTextBox && 0 == mTextBox->getText().size() ) {
		mIcon->center();
	}

	/** Auto Size only for height? May be set another flag to this... */
	/**
	if ( mFlags & UI_AUTO_SIZE ) {
		if ( NULL != mTextBox ) {
			Recti P = makePadding();

			setInternalHeight( mIcon->getSize().getHeight()	+ P.Top		+ P.Bottom );

			if ( 0 == mTextBox->getText().size() ) {
				setInternalWidth( mIcon->getSize().getWidth()		+ P.Left	+ P.Right );

				mIcon->center();
			} else {
				setInternalWidth( mIconSpace + mIcon->setPosition(.x + mIcon->getSize().getWidth() + mTextBox->getSize().getWidth() );

				if ( mSize.getHeight() < P.Top + P.Bottom + mTextBox->getTextHeight() )
					setInternalHeight( P.Top + P.Bottom + mTextBox->getTextHeight() );
			}
		}
	} else {
		if ( NULL != mTextBox && 0 == mTextBox->getText().size() ) {
			mIcon->center();
		}
	}
	*/
}

void UIPushButton::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "button" );

	doAftersetTheme();
}

void UIPushButton::doAftersetTheme() {
	if ( NULL != mTextBox && NULL == mTextBox->getFont() && NULL != mSkinState && NULL != mSkinState->getSkin() && NULL != mSkinState->getSkin()->getTheme() && NULL != mSkinState->getSkin()->getTheme()->getFont() )
		mTextBox->setFont( mSkinState->getSkin()->getTheme()->getFont() );

	if ( mControlFlags & UI_CTRL_FLAG_FREE_USE ) {
		Recti RMargin = makePadding( true, false, false, false, true );
		mIconSpace = RMargin.Left;
	}

	if ( ( mFlags & UI_AUTO_SIZE ) && NULL != getSkin() ) {
		setInternalPixelsHeight( getSkin()->getSize().getHeight() );
	}

	autoPadding();

	onSizeChange();
}

void UIPushButton::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mTextBox->setPadding( makePadding( true, false, true, false ) );
	}
}

void UIPushButton::setIcon( SubTexture * Icon ) {
	mIcon->setSubTexture( Icon );
	onSizeChange();
}

UIGfx * UIPushButton::getIcon() const {
	return mIcon;
}

void UIPushButton::setText( const String& text ) {
	mTextBox->setText( text );
	onSizeChange();
}

const String& UIPushButton::getText() {
	return mTextBox->getText();
}

void UIPushButton::setPadding( const Recti& padding ) {
	mTextBox->setPadding( padding );
}

const Recti& UIPushButton::getPadding() const {
	return mTextBox->getPadding();
}

void UIPushButton::setIconHorizontalMargin( Int32 margin ) {
	mIconSpace = margin;
	onSizeChange();
}

const Int32& UIPushButton::getIconHorizontalMargin() const {
	return mIconSpace;
}

UITextBox * UIPushButton::getTextBox() const {
	return mTextBox;
}

void UIPushButton::onAlphaChange() {
	UIControlAnim::onAlphaChange();

	mIcon->setAlpha( mAlpha );
	mTextBox->setAlpha( mAlpha );
}

void UIPushButton::onStateChange() {
	if ( mSkinState->getState() == UISkinState::StateMouseEnter ) {
		mTextBox->setFontColor( mFontOverColor );
	} else {
		mTextBox->setFontColor( mFontColor );
	}

	mTextBox->setAlpha( mAlpha );
}

Uint32 UIPushButton::onKeyDown( const UIEventKey& Event ) {
	if ( Event.getKeyCode() == KEY_RETURN ) {
		UIMessage Msg( this, UIMessage::MsgClick, EE_BUTTON_LMASK );
		messagePost( &Msg );
		onMouseClick( Vector2i(0,0), EE_BUTTON_LMASK );

		setSkinState( UISkinState::StateMouseDown );
	}

	return UIComplexControl::onKeyDown( Event );
}

Uint32 UIPushButton::onKeyUp( const UIEventKey& Event ) {
	if ( Event.getKeyCode() == KEY_RETURN ) {
		setPrevSkinState();
	}

	return UIComplexControl::onKeyUp( Event );
}
const ColorA& UIPushButton::getFontColor() const {
	return mFontColor;
}

void UIPushButton::setFontColor( const ColorA& color ) {
	mFontColor = color;
	onStateChange();
}

const ColorA& UIPushButton::getFontOverColor() const {
	return mFontOverColor;
}

void UIPushButton::setFontOverColor( const ColorA& color ) {
	mFontOverColor = color;
	onStateChange();
}

}}
