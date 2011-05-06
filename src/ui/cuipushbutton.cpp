#include "cuipushbutton.hpp"

namespace EE { namespace UI {

cUIPushButton::cUIPushButton( const cUIPushButton::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mFontColor( Params.FontColor ),
	mFontOverColor( Params.FontOverColor ),
	mIcon( NULL ),
	mTextBox( NULL ),
	mIconSpace( Params.IconHorizontalMargin )
{
	mType |= UI_TYPE_GET(UI_TYPE_PUSHBUTTON);

	cUIGfx::CreateParams GfxParams;
	GfxParams.Parent( this );
	GfxParams.Shape = Params.Icon;
	GfxParams.Flags = UI_AUTO_SIZE;
	mIcon = eeNew( cUIGfx, ( GfxParams ) );
	mIcon->Visible( true );
	mIcon->Enabled( false );

	Icon( Params.Icon );

	cUITextBox::CreateParams TxtParams = Params;
	TxtParams.Parent( this );
	TxtParams.Flags 			= Params.Flags;
	TxtParams.Font				= Params.Font;
	TxtParams.FontColor 		= Params.FontColor;
	TxtParams.FontShadowColor 	= Params.FontShadowColor;

	if ( TxtParams.Flags & UI_CLIP_ENABLE )
		TxtParams.Flags &= ~UI_CLIP_ENABLE;

	mTextBox = eeNew( cUITextBox, ( TxtParams ) );
	mTextBox->Visible( true );
	mTextBox->Enabled( false );

	if ( Params.IconAutoMargin )
		mControlFlags |= UI_CTRL_FLAG_FREE_USE;

	OnSizeChange();

	ApplyDefaultTheme();
}

void cUIPushButton::OnSizeChange() {
	if ( NULL != mTextBox ) {
		mTextBox->Size( mSize.Width(), mSize.Height() );
		mTextBox->Pos( 0, 0 );
	}

	mIcon->Pos( mIconSpace, 0 );
	mIcon->CenterVertical();

	if ( NULL != mTextBox ) {
		switch ( FontHAlignGet( Flags() ) ) {
			case UI_HALIGN_LEFT:
				mTextBox->Pos( mIcon->Pos().x + mIcon->Size().Width(), 0 );
				mTextBox->Size( mSize.Width() - mIcon->Pos().x + mIcon->Size().Width(), mSize.Height() );
				break;
			case UI_HALIGN_CENTER:
				if ( NULL != mIcon->Shape() ) {
					if ( mIcon->Pos().x + mIcon->Size().Width() >= mTextBox->AlignOffset().x ) {
						mTextBox->Pos( mIcon->Pos().x + mIcon->Size().Width() + 1 - mTextBox->AlignOffset().x, mTextBox->Pos().y );
					}
				}

				break;
		}
	}

	if ( mFlags & UI_AUTO_SIZE ) {
		if ( NULL != mTextBox ) {
			eeRecti P = MakePadding();

			mSize.Height( mIcon->Size().Height()	+ P.Top		+ P.Bottom );

			if ( 0 == mTextBox->Text().size() ) {
				mSize.Width	( mIcon->Size().Width()		+ P.Left	+ P.Right );

				mIcon->Center();
			} else {
				mSize.Width( mIconSpace + mIcon->Pos().x + mIcon->Size().Width() + mTextBox->Size().Width() );

				if ( mSize.Height() < P.Top + P.Bottom + mTextBox->GetTextHeight() )
					mSize.Height( P.Top + P.Bottom + mTextBox->GetTextHeight() );
			}
		}
	} else {
		if ( NULL != mTextBox && 0 == mTextBox->Text().size() ) {
			mIcon->Center();
		}
	}
}

cUIPushButton::~cUIPushButton() {
}

void cUIPushButton::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "button" );
	DoAfterSetTheme();
}

void cUIPushButton::DoAfterSetTheme() {
	if ( NULL != mTextBox && NULL == mTextBox->Font() && NULL != mSkinState && NULL != mSkinState->GetSkin() && NULL != mSkinState->GetSkin()->Theme() && NULL != mSkinState->GetSkin()->Theme()->Font() )
		mTextBox->Font( mSkinState->GetSkin()->Theme()->Font() );

	if ( mControlFlags & UI_CTRL_FLAG_FREE_USE ) {
		eeRecti RMargin = MakePadding( true, false, false, false, true );
		mIconSpace = RMargin.Left;
	}

	AutoPadding();

	OnSizeChange();
}

void cUIPushButton::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		Padding( MakePadding( true, false, true, false ) );
	}
}

void cUIPushButton::Icon( cShape * Icon ) {
	mIcon->Shape( Icon );
	OnSizeChange();
}

cUIGfx * cUIPushButton::Icon() const {
	return mIcon;
}

void cUIPushButton::Text( const String& text ) {
	mTextBox->Text( text );
	OnSizeChange();
}

const String& cUIPushButton::Text() {
	return mTextBox->Text();
}

void cUIPushButton::Padding( const eeRecti& padding ) {
	mTextBox->Padding( padding );
}

const eeRecti& cUIPushButton::Padding() const {
	return mTextBox->Padding();
}

void cUIPushButton::IconHorizontalMargin( Int32 margin ) {
	mIconSpace = margin;
	OnSizeChange();
}

const Int32& cUIPushButton::IconHorizontalMargin() const {
	return mIconSpace;
}

cUITextBox * cUIPushButton::TextBox() const {
	return mTextBox;
}

void cUIPushButton::OnAlphaChange() {
	cUIControlAnim::OnAlphaChange();

	mIcon->Alpha( mAlpha );
	mTextBox->Alpha( mAlpha );
}

void cUIPushButton::OnStateChange() {
	if ( mSkinState->GetState() == cUISkinState::StateMouseEnter ) {
		mTextBox->Color( mFontOverColor );
	} else {
		mTextBox->Color( mFontColor );
	}

	mTextBox->Alpha( mAlpha );
}

Uint32 cUIPushButton::OnKeyDown( const cUIEventKey& Event ) {
	if ( Event.KeyCode() == KEY_RETURN ) {
		cUIMessage Msg( this, cUIMessage::MsgClick, EE_BUTTON_LMASK );
		MessagePost( &Msg );
		OnMouseClick( eeVector2i(0,0), EE_BUTTON_LMASK );

		SetSkinState( cUISkinState::StateMouseDown );
	}

	return cUIComplexControl::OnKeyDown( Event );
}

Uint32 cUIPushButton::OnKeyUp( const cUIEventKey& Event ) {
	if ( Event.KeyCode() == KEY_RETURN ) {
		SetPrevSkinState();
	}

	return cUIComplexControl::OnKeyUp( Event );
}

}}
