#include <eepp/ui/uitextinputpassword.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/textcache.hpp>
#include <eepp/graphics/font.hpp>

namespace EE { namespace UI {

UITextInputPassword::UITextInputPassword() :
	UITextInput()
{
	mPassCache = eeNew( TextCache, () );
	setFontStyleConfig( mFontStyleConfig );

	autoAlign();
}

UITextInputPassword::~UITextInputPassword() {
	eeSAFE_DELETE( mPassCache );
}

void UITextInputPassword::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UIControlAnim::draw();

		drawSelection( mPassCache );

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
			mPassCache->draw( (Float)mScreenPos.x + mRealAlignOffset.x + (Float)mRealPadding.Left, (Float)mScreenPos.y + mRealAlignOffset.y + (Float)mRealPadding.Top, Vector2f::One, 0.f, getBlendMode() );

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
		Float tX	= mRealAlignOffset.x + tW;

		mCurPos.x	= tW;
		mCurPos.y	= (Float)LineNum * (Float)mPassCache->getFont()->getFontHeight();

		if ( !mTextBuffer.setSupportNewLine() ) {
			if ( tX < 0.f )
				mRealAlignOffset.x = -( mRealAlignOffset.x + ( tW - mRealAlignOffset.x ) );
			else if ( tX > mRealSize.getWidth() - mRealPadding.Left - mRealPadding.Right )
				mRealAlignOffset.x = mRealSize.getWidth() - mRealPadding.Left - mRealPadding.Right - ( mRealAlignOffset.x + ( tW - mRealAlignOffset.x ) );
		}
	}
}

void UITextInputPassword::autoAlign() {
	switch ( fontHAlignGet( getFlags() ) ) {
		case UI_HALIGN_CENTER:
			mRealAlignOffset.x = (Float)( (Int32)( mRealSize.x - mPassCache->getTextWidth() ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mRealAlignOffset.x = ( (Float)mRealSize.x - (Float)mPassCache->getTextWidth() );
			break;
		case UI_HALIGN_LEFT:
			mRealAlignOffset.x = 0.f;
			break;
	}

	switch ( fontVAlignGet( getFlags() ) ) {
		case UI_VALIGN_CENTER:
			mRealAlignOffset.y = (Float)( ( (Int32)( mRealSize.y - mPassCache->getTextHeight() ) ) / 2 ) - 1;
			break;
		case UI_VALIGN_BOTTOM:
			mRealAlignOffset.y = ( (Float)mRealSize.y - (Float)mPassCache->getTextHeight() );
			break;
		case UI_VALIGN_TOP:
			mRealAlignOffset.y = 0.f;
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

void UITextInputPassword::setFontStyleConfig(const FontStyleConfig & fontStyleConfig) {
	UITextInput::setFontStyleConfig( fontStyleConfig );

	mPassCache->setFont( mFontStyleConfig.getFont() );
	mPassCache->setColor( mFontStyleConfig.getFontColor() );
	mPassCache->setShadowColor( mFontStyleConfig.getFontShadowColor() );
}

const String& UITextInputPassword::getText() {
	return UITextBox::getText();
}


}}
