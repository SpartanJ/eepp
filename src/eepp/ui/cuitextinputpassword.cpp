#include <eepp/ui/cuitextinputpassword.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/ui/cuithememanager.hpp>
#include <eepp/graphics/ctextcache.hpp>
#include <eepp/graphics/cfont.hpp>

namespace EE { namespace UI {

cUITextInputPassword::cUITextInputPassword( const cUITextInput::CreateParams& Params ) :
	cUITextInput( Params )
{
	mPassCache = eeNew( cTextCache, () );
	mPassCache->Font( Params.Font );
	mPassCache->Color( mFontColor );
	mPassCache->ShadowColor( mFontShadowColor );

	if ( NULL == Params.Font ) {
		if ( NULL != cUIThemeManager::instance()->DefaultFont() )
			mPassCache->Font( cUIThemeManager::instance()->DefaultFont() );
		else
			eePRINTL( "cUITextInputPassword::cUITextInputPassword : Created a UI TextInputPassword without a defined font." );
	}

	AutoAlign();
}

void cUITextInputPassword::Draw() {
	if ( mVisible && 0.f != mAlpha ) {
		cUIControlAnim::Draw();

		if ( mPassCache->GetTextWidth() ) {
			if ( mFlags & UI_CLIP_ENABLE ) {
				cUIManager::instance()->ClipEnable(
						mScreenPos.x + mPadding.Left,
						mScreenPos.y + mPadding.Top,
						mSize.Width() - mPadding.Left - mPadding.Right,
						mSize.Height() - mPadding.Top - mPadding.Bottom
				);
			}

			mPassCache->Flags( Flags() );
			mPassCache->Draw( (Float)mScreenPos.x + mAlignOffset.x + (Float)mPadding.Left, (Float)mScreenPos.y + mAlignOffset.y + (Float)mPadding.Top, Vector2f::One, 0.f, Blend() );

			if ( mFlags & UI_CLIP_ENABLE ) {
				cUIManager::instance()->ClipDisable();
			}
		}
	}

	DrawWaitingCursor();
}

void cUITextInputPassword::AlignFix() {
	if ( FontHAlignGet( Flags() ) == UI_HALIGN_LEFT ) {
		Uint32 NLPos	= 0;
		Uint32 LineNum	= mTextBuffer.GetCurPosLinePos( NLPos );

		String curStr( mTextBuffer.Buffer().substr( NLPos, mTextBuffer.CurPos() - NLPos ) );
		String pasStr;

		for ( size_t i = 0; i < curStr.size(); i++ )
			pasStr += '*';

		mPassCache->Font()->SetText( pasStr );

		Float tW	= mPassCache->Font()->GetTextWidth();
		Float tX	= mAlignOffset.x + tW;

		mCurPos.x	= tW;
		mCurPos.y	= (Float)LineNum * (Float)mPassCache->Font()->GetFontHeight();

		if ( !mTextBuffer.SupportNewLine() ) {
			if ( tX < 0.f )
				mAlignOffset.x = -( mAlignOffset.x + ( tW - mAlignOffset.x ) );
			else if ( tX > mSize.Width() - mPadding.Left - mPadding.Right )
				mAlignOffset.x = mSize.Width() - mPadding.Left - mPadding.Right - ( mAlignOffset.x + ( tW - mAlignOffset.x ) );
		}
	}
}

void cUITextInputPassword::AutoAlign() {
	switch ( FontHAlignGet( Flags() ) ) {
		case UI_HALIGN_CENTER:
			mAlignOffset.x = (Float)( (Int32)( mSize.x - mPassCache->GetTextWidth() ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mAlignOffset.x = ( (Float)mSize.x - (Float)mPassCache->GetTextWidth() );
			break;
		case UI_HALIGN_LEFT:
			mAlignOffset.x = 0.f;
			break;
	}

	switch ( FontVAlignGet( Flags() ) ) {
		case UI_VALIGN_CENTER:
			mAlignOffset.y = (Float)( ( (Int32)( mSize.y - mPassCache->GetTextHeight() ) ) / 2 ) - 1;
			break;
		case UI_VALIGN_BOTTOM:
			mAlignOffset.y = ( (Float)mSize.y - (Float)mPassCache->GetTextHeight() );
			break;
		case UI_VALIGN_TOP:
			mAlignOffset.y = 0.f;
			break;
	}
}

void cUITextInputPassword::UpdateText() {
	UpdatePass( mTextCache->Text() );
}

void cUITextInputPassword::UpdatePass( const String& pass ) {
	mPassCache->Text().clear();

	String newTxt;

	for ( size_t i = 0; i < pass.size(); i++ ) {
		newTxt += '*';
	}

	mPassCache->Text( newTxt );
}

void cUITextInputPassword::Text( const String& text ) {
	cUITextInput::Text( text );

	UpdatePass( text );
}

cTextCache *cUITextInputPassword::GetPassCache() const {
	return mPassCache;
}

const String& cUITextInputPassword::Text() {
	return cUITextBox::Text();
}


}}
