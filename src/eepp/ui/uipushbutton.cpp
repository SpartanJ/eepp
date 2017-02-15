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
	GfxParams.Parent( this );
	GfxParams.SubTexture = Params.Icon;

	if ( Params.IconMinSize.x != 0 && Params.IconMinSize.y != 0 ) {
		GfxParams.Flags = UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	} else {
		GfxParams.Flags = UI_AUTO_SIZE | UI_VALIGN_CENTER | UI_HALIGN_CENTER;
	}

	mIcon = eeNew( UIGfx, ( GfxParams ) );

	if ( Params.IconMinSize.x != 0 && Params.IconMinSize.y != 0 ) {
		mIcon->Size( Params.IconMinSize );
	}

	mIcon->Visible( true );
	mIcon->Enabled( false );

	Icon( Params.Icon );

	UITextBox::CreateParams TxtParams = Params;
	TxtParams.Parent( this );
	TxtParams.Flags 			= HAlignGet( Params.Flags ) | VAlignGet( Params.Flags );
	TxtParams.Font				= Params.Font;
	TxtParams.FontColor 		= Params.FontColor;
	TxtParams.FontShadowColor 	= Params.FontShadowColor;

	if ( TxtParams.Flags & UI_CLIP_ENABLE )
		TxtParams.Flags &= ~UI_CLIP_ENABLE;

	mTextBox = eeNew( UITextBox, ( TxtParams ) );
	mTextBox->Visible( true );
	mTextBox->Enabled( false );

	if ( Params.IconAutoMargin )
		mControlFlags |= UI_CTRL_FLAG_FREE_USE;

	OnSizeChange();

	ApplyDefaultTheme();
}

UIPushButton::~UIPushButton() {
}

Uint32 UIPushButton::Type() const {
	return UI_TYPE_PUSHBUTTON;
}

bool UIPushButton::IsType( const Uint32& type ) const {
	return UIPushButton::Type() == type ? true : UIComplexControl::IsType( type );
}

void UIPushButton::OnSizeChange() {
	if ( NULL != mTextBox ) {
		mTextBox->Size( mSize );
		mTextBox->Pos( 0, 0 );
	}

	mIcon->Pos( mIconSpace, 0 );
	mIcon->CenterVertical();

	if ( NULL != mTextBox ) {
		switch ( FontHAlignGet( Flags() ) ) {
			case UI_HALIGN_LEFT:
				mTextBox->Pos( mIcon->Pos().x + mIcon->Size().width(), 0 );
				mTextBox->Size( mSize.width() - mIcon->Pos().x + mIcon->Size().width(), mSize.height() );
				break;
			case UI_HALIGN_CENTER:
				if ( NULL != mIcon->SubTexture() ) {
					if ( mIcon->Pos().x + mIcon->Size().width() >= mTextBox->AlignOffset().x ) {
						mTextBox->Pos( mIcon->Pos().x + mIcon->Size().width() + 1 - mTextBox->AlignOffset().x, mTextBox->Pos().y );
					}
				}

				break;
		}
	}

	if ( NULL != mTextBox && 0 == mTextBox->Text().size() ) {
		mIcon->Center();
	}

	/** Auto Size only for height? May be set another flag to this... */
	/**
	if ( mFlags & UI_AUTO_SIZE ) {
		if ( NULL != mTextBox ) {
			Recti P = MakePadding();

			mSize.Height( mIcon->Size().height()	+ P.Top		+ P.Bottom );

			if ( 0 == mTextBox->Text().size() ) {
				mSize.Width	( mIcon->Size().width()		+ P.Left	+ P.Right );

				mIcon->Center();
			} else {
				mSize.Width( mIconSpace + mIcon->Pos().x + mIcon->Size().width() + mTextBox->Size().width() );

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

void UIPushButton::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "button" );

	DoAfterSetTheme();
}

void UIPushButton::DoAfterSetTheme() {
	if ( NULL != mTextBox && NULL == mTextBox->Font() && NULL != mSkinState && NULL != mSkinState->GetSkin() && NULL != mSkinState->GetSkin()->Theme() && NULL != mSkinState->GetSkin()->Theme()->Font() )
		mTextBox->Font( mSkinState->GetSkin()->Theme()->Font() );

	if ( mControlFlags & UI_CTRL_FLAG_FREE_USE ) {
		Recti RMargin = MakePadding( true, false, false, false, true );
		mIconSpace = RMargin.Left;
	}

	if ( mFlags & UI_AUTO_SIZE ) {
		mSize.height( GetSkinSize().height() );
	}

	AutoPadding();

	OnSizeChange();
}

void UIPushButton::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		Padding( MakePadding( true, false, true, false ) );
	}
}

void UIPushButton::Icon( SubTexture * Icon ) {
	mIcon->SubTexture( Icon );
	OnSizeChange();
}

UIGfx * UIPushButton::Icon() const {
	return mIcon;
}

void UIPushButton::Text( const String& text ) {
	mTextBox->Text( text );
	OnSizeChange();
}

const String& UIPushButton::Text() {
	return mTextBox->Text();
}

void UIPushButton::Padding( const Recti& padding ) {
	mTextBox->Padding( padding );
}

const Recti& UIPushButton::Padding() const {
	return mTextBox->Padding();
}

void UIPushButton::IconHorizontalMargin( Int32 margin ) {
	mIconSpace = margin;
	OnSizeChange();
}

const Int32& UIPushButton::IconHorizontalMargin() const {
	return mIconSpace;
}

UITextBox * UIPushButton::TextBox() const {
	return mTextBox;
}

void UIPushButton::OnAlphaChange() {
	UIControlAnim::OnAlphaChange();

	mIcon->Alpha( mAlpha );
	mTextBox->Alpha( mAlpha );
}

void UIPushButton::OnStateChange() {
	if ( mSkinState->GetState() == UISkinState::StateMouseEnter ) {
		mTextBox->Color( mFontOverColor );
	} else {
		mTextBox->Color( mFontColor );
	}

	mTextBox->Alpha( mAlpha );
}

Uint32 UIPushButton::OnKeyDown( const UIEventKey& Event ) {
	if ( Event.KeyCode() == KEY_RETURN ) {
		UIMessage Msg( this, UIMessage::MsgClick, EE_BUTTON_LMASK );
		MessagePost( &Msg );
		OnMouseClick( Vector2i(0,0), EE_BUTTON_LMASK );

		SetSkinState( UISkinState::StateMouseDown );
	}

	return UIComplexControl::OnKeyDown( Event );
}

Uint32 UIPushButton::OnKeyUp( const UIEventKey& Event ) {
	if ( Event.KeyCode() == KEY_RETURN ) {
		SetPrevSkinState();
	}

	return UIComplexControl::OnKeyUp( Event );
}
const ColorA& UIPushButton::FontColor() const {
	return mFontColor;
}

void UIPushButton::FontColor( const ColorA& color ) {
	mFontColor = color;
	OnStateChange();
}

const ColorA& UIPushButton::FontOverColor() const {
	return mFontOverColor;
}

void UIPushButton::FontOverColor( const ColorA& color ) {
	mFontOverColor = color;
	OnStateChange();
}

}}
