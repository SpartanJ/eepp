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
	mPassCache->setFont( Params.Font );
	mPassCache->setColor( mFontColor );
	mPassCache->setShadowColor( mFontShadowColor );

	if ( NULL == Params.Font ) {
		if ( NULL != UIThemeManager::instance()->getDefaultFont() )
			mPassCache->setFont( UIThemeManager::instance()->getDefaultFont() );
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
				UIManager::instance()->clipSmartEnable(
						this,
						mScreenPos.x + mRealPadding.Left,
						mScreenPos.y + mRealPadding.Top,
						mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
						mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom
				);
			}

			mPassCache->setFlags( getFlags() );
			mPassCache->draw( (Float)mScreenPos.x + mAlignOffset.x + (Float)mRealPadding.Left, (Float)mScreenPos.y + mAlignOffset.y + (Float)mRealPadding.Top, Vector2f::One, 0.f, getBlendMode() );

			if ( mFlags & UI_CLIP_ENABLE ) {
				UIManager::instance()->clipSmartDisable( this );
			}
		}
	}

	drawWaitingCursor();
}

void UITextInputPassword::alignFix() {
	if ( fontHAlignGet( getFlags() ) == UI_HALIGN_LEFT ) {
		Uint32 NLPos	= 0;
		Uint32 LineNum	= mTextBuffer.getCurPosLinePos( NLPos );

		String curStr( mTextBuffer.getBuffer().substr( NLPos, mTextBuffer.getCursorPos() - NLPos ) );
		String pasStr;

		for ( size_t i = 0; i < curStr.size(); i++ )
			pasStr += '*';

		mPassCache->getFont()->setText( pasStr );

		Float tW	= mPassCache->getFont()->getTextWidth();
		Float tX	= mAlignOffset.x + tW;

		mCurPos.x	= tW;
		mCurPos.y	= (Float)LineNum * (Float)mPassCache->getFont()->getFontHeight();

		if ( !mTextBuffer.setSupportNewLine() ) {
			if ( tX < 0.f )
				mAlignOffset.x = -( mAlignOffset.x + ( tW - mAlignOffset.x ) );
			else if ( tX > mRealSize.getWidth() - mRealPadding.Left - mRealPadding.Right )
				mAlignOffset.x = mRealSize.getWidth() - mRealPadding.Left - mRealPadding.Right - ( mAlignOffset.x + ( tW - mAlignOffset.x ) );
		}
	}
}

void UITextInputPassword::autoAlign() {
	switch ( fontHAlignGet( getFlags() ) ) {
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

	switch ( fontVAlignGet( getFlags() ) ) {
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
	updatePass( mTextCache->getText() );
}

void UITextInputPassword::updatePass( const String& pass ) {
	mPassCache->getText().clear();

	String newTxt;

	for ( size_t i = 0; i < pass.size(); i++ ) {
		newTxt += '*';
	}

	mPassCache->setText( newTxt );
}

void UITextInputPassword::setText( const String& text ) {
	UITextInput::setText( text );

	updatePass( text );
}

TextCache *UITextInputPassword::getPassCache() const {
	return mPassCache;
}

const String& UITextInputPassword::getText() {
	return UITextBox::getText();
}


}}
