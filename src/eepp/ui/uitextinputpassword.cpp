#include <eepp/ui/uitextinputpassword.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/textcache.hpp>
#include <eepp/graphics/font.hpp>

namespace EE { namespace UI {

UITextInputPassword::UITextInputPassword( const UITextInput::CreateParams& Params ) :
	UITextInput( Params )
{
	mPassCache = eeNew( TextCache, () );
	mPassCache->font( Params.Font );
	mPassCache->color( mFontColor );
	mPassCache->shadowColor( mFontShadowColor );

	if ( NULL == Params.Font ) {
		if ( NULL != UIThemeManager::instance()->defaultFont() )
			mPassCache->font( UIThemeManager::instance()->defaultFont() );
		else
			eePRINTL( "UITextInputPassword::UITextInputPassword : Created a UI TextInputPassword without a defined font." );
	}

	autoAlign();
}

void UITextInputPassword::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UIControlAnim::draw();

		if ( mPassCache->getTextWidth() ) {
			if ( mFlags & UI_CLIP_ENABLE ) {
				UIManager::instance()->clipEnable(
						mScreenPos.x + mPadding.Left,
						mScreenPos.y + mPadding.Top,
						mSize.width() - mPadding.Left - mPadding.Right,
						mSize.height() - mPadding.Top - mPadding.Bottom
				);
			}

			mPassCache->flags( flags() );
			mPassCache->draw( (Float)mScreenPos.x + mAlignOffset.x + (Float)mPadding.Left, (Float)mScreenPos.y + mAlignOffset.y + (Float)mPadding.Top, Vector2f::One, 0.f, blend() );

			if ( mFlags & UI_CLIP_ENABLE ) {
				UIManager::instance()->clipDisable();
			}
		}
	}

	drawWaitingCursor();
}

void UITextInputPassword::alignFix() {
	if ( FontHAlignGet( flags() ) == UI_HALIGN_LEFT ) {
		Uint32 NLPos	= 0;
		Uint32 LineNum	= mTextBuffer.getCurPosLinePos( NLPos );

		String curStr( mTextBuffer.getBuffer().substr( NLPos, mTextBuffer.getCursorPos() - NLPos ) );
		String pasStr;

		for ( size_t i = 0; i < curStr.size(); i++ )
			pasStr += '*';

		mPassCache->font()->setText( pasStr );

		Float tW	= mPassCache->font()->getTextWidth();
		Float tX	= mAlignOffset.x + tW;

		mCurPos.x	= tW;
		mCurPos.y	= (Float)LineNum * (Float)mPassCache->font()->getFontHeight();

		if ( !mTextBuffer.setSupportNewLine() ) {
			if ( tX < 0.f )
				mAlignOffset.x = -( mAlignOffset.x + ( tW - mAlignOffset.x ) );
			else if ( tX > mSize.width() - mPadding.Left - mPadding.Right )
				mAlignOffset.x = mSize.width() - mPadding.Left - mPadding.Right - ( mAlignOffset.x + ( tW - mAlignOffset.x ) );
		}
	}
}

void UITextInputPassword::autoAlign() {
	switch ( FontHAlignGet( flags() ) ) {
		case UI_HALIGN_CENTER:
			mAlignOffset.x = (Float)( (Int32)( mSize.x - mPassCache->getTextWidth() ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mAlignOffset.x = ( (Float)mSize.x - (Float)mPassCache->getTextWidth() );
			break;
		case UI_HALIGN_LEFT:
			mAlignOffset.x = 0.f;
			break;
	}

	switch ( FontVAlignGet( flags() ) ) {
		case UI_VALIGN_CENTER:
			mAlignOffset.y = (Float)( ( (Int32)( mSize.y - mPassCache->getTextHeight() ) ) / 2 ) - 1;
			break;
		case UI_VALIGN_BOTTOM:
			mAlignOffset.y = ( (Float)mSize.y - (Float)mPassCache->getTextHeight() );
			break;
		case UI_VALIGN_TOP:
			mAlignOffset.y = 0.f;
			break;
	}
}

void UITextInputPassword::updateText() {
	updatePass( mTextCache->text() );
}

void UITextInputPassword::updatePass( const String& pass ) {
	mPassCache->text().clear();

	String newTxt;

	for ( size_t i = 0; i < pass.size(); i++ ) {
		newTxt += '*';
	}

	mPassCache->text( newTxt );
}

void UITextInputPassword::text( const String& text ) {
	UITextInput::text( text );

	updatePass( text );
}

TextCache *UITextInputPassword::getPassCache() const {
	return mPassCache;
}

const String& UITextInputPassword::text() {
	return UITextBox::text();
}


}}
