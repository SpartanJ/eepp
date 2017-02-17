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
		mIcon->size( Params.IconMinSize );
	}

	mIcon->visible( true );
	mIcon->enabled( false );

	icon( Params.Icon );

	UITextBox::CreateParams TxtParams = Params;
	TxtParams.setParent( this );
	TxtParams.Flags 			= HAlignGet( Params.Flags ) | VAlignGet( Params.Flags );
	TxtParams.Font				= Params.Font;
	TxtParams.FontColor 		= Params.FontColor;
	TxtParams.FontShadowColor 	= Params.FontShadowColor;

	if ( TxtParams.Flags & UI_CLIP_ENABLE )
		TxtParams.Flags &= ~UI_CLIP_ENABLE;

	mTextBox = eeNew( UITextBox, ( TxtParams ) );
	mTextBox->visible( true );
	mTextBox->enabled( false );

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
		mTextBox->size( mSize );
		mTextBox->position( 0, 0 );
	}

	mIcon->position( mIconSpace, 0 );
	mIcon->centerVertical();

	if ( NULL != mTextBox ) {
		switch ( FontHAlignGet( flags() ) ) {
			case UI_HALIGN_LEFT:
				mTextBox->position( mIcon->position().x + mIcon->size().width(), 0 );
				mTextBox->size( mSize.width() - mIcon->position().x + mIcon->size().width(), mSize.height() );
				break;
			case UI_HALIGN_CENTER:
				if ( NULL != mIcon->subTexture() ) {
					if ( mIcon->position().x + mIcon->size().width() >= mTextBox->alignOffset().x ) {
						mTextBox->position( mIcon->position().x + mIcon->size().width() + 1 - mTextBox->alignOffset().x, mTextBox->position().y );
					}
				}

				break;
		}
	}

	if ( NULL != mTextBox && 0 == mTextBox->text().size() ) {
		mIcon->center();
	}

	/** Auto Size only for height? May be set another flag to this... */
	/**
	if ( mFlags & UI_AUTO_SIZE ) {
		if ( NULL != mTextBox ) {
			Recti P = MakePadding();

			mSize.Height( mIcon->size().height()	+ P.Top		+ P.Bottom );

			if ( 0 == mTextBox->Text().size() ) {
				mSize.Width	( mIcon->size().width()		+ P.Left	+ P.Right );

				mIcon->Center();
			} else {
				mSize.Width( mIconSpace + mIcon->position(.x + mIcon->size().width() + mTextBox->size().width() );

				if ( mSize.Height() < P.Top + P.Bottom + mTextBox->GetTextHeight() )
					mSize.Height( P.Top + P.Bottom + mTextBox->GetTextHeight() );
			}
		}
	} else {
		if ( NULL != mTextBox && 0 == mTextBox->Text().size() ) {
			mIcon->Center();
		}
	}
	*/
}

void UIPushButton::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "button" );

	doAftersetTheme();
}

void UIPushButton::doAftersetTheme() {
	if ( NULL != mTextBox && NULL == mTextBox->font() && NULL != mSkinState && NULL != mSkinState->getSkin() && NULL != mSkinState->getSkin()->theme() && NULL != mSkinState->getSkin()->theme()->font() )
		mTextBox->font( mSkinState->getSkin()->theme()->font() );

	if ( mControlFlags & UI_CTRL_FLAG_FREE_USE ) {
		Recti RMargin = makePadding( true, false, false, false, true );
		mIconSpace = RMargin.Left;
	}

	if ( mFlags & UI_AUTO_SIZE ) {
		mSize.height( getSkinSize().height() );
	}

	autoPadding();

	onSizeChange();
}

void UIPushButton::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		padding( makePadding( true, false, true, false ) );
	}
}

void UIPushButton::icon( SubTexture * Icon ) {
	mIcon->subTexture( Icon );
	onSizeChange();
}

UIGfx * UIPushButton::icon() const {
	return mIcon;
}

void UIPushButton::text( const String& text ) {
	mTextBox->text( text );
	onSizeChange();
}

const String& UIPushButton::text() {
	return mTextBox->text();
}

void UIPushButton::padding( const Recti& padding ) {
	mTextBox->padding( padding );
}

const Recti& UIPushButton::padding() const {
	return mTextBox->padding();
}

void UIPushButton::iconHorizontalMargin( Int32 margin ) {
	mIconSpace = margin;
	onSizeChange();
}

const Int32& UIPushButton::iconHorizontalMargin() const {
	return mIconSpace;
}

UITextBox * UIPushButton::getTextBox() const {
	return mTextBox;
}

void UIPushButton::onAlphaChange() {
	UIControlAnim::onAlphaChange();

	mIcon->alpha( mAlpha );
	mTextBox->alpha( mAlpha );
}

void UIPushButton::onStateChange() {
	if ( mSkinState->getState() == UISkinState::StateMouseEnter ) {
		mTextBox->color( mFontOverColor );
	} else {
		mTextBox->color( mFontColor );
	}

	mTextBox->alpha( mAlpha );
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
const ColorA& UIPushButton::fontColor() const {
	return mFontColor;
}

void UIPushButton::fontColor( const ColorA& color ) {
	mFontColor = color;
	onStateChange();
}

const ColorA& UIPushButton::fontOverColor() const {
	return mFontOverColor;
}

void UIPushButton::fontOverColor( const ColorA& color ) {
	mFontOverColor = color;
	onStateChange();
}

}}
