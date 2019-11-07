#include <eepp/ui/uitextinputpassword.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/graphics/font.hpp>

namespace EE { namespace UI {

UITextInputPassword * UITextInputPassword::New() {
	return eeNew( UITextInputPassword, () );
}

UITextInputPassword::UITextInputPassword() :
	UITextInput( "textinputpassword" )
{
	mPassCache = Text::New();

	updateFontStyleConfig();

	alignFix();
}

UITextInputPassword::~UITextInputPassword() {
	eeSAFE_DELETE( mPassCache );
}

void UITextInputPassword::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UINode::draw();

		drawSelection( mPassCache );

		if ( mPassCache->getTextWidth() ) {
			if ( isClipped() ) {
				clipSmartEnable(
						mScreenPos.x + mRealPadding.Left,
						mScreenPos.y + mRealPadding.Top,
						mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
						mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom
				);
			}

			mPassCache->setAlign( getFlags() );
			mPassCache->draw( (Float)mScreenPosi.x + (int)mRealAlignOffset.x + (int)mRealPadding.Left, (Float)mScreenPosi.y + (int)mRealAlignOffset.y + (int)mRealPadding.Top, Vector2f::One, 0.f, getBlendMode() );

			if ( isClipped() ) {
				clipSmartDisable();
			}
		}
	}

	drawWaitingCursor();
}

void UITextInputPassword::alignFix() {
	switch ( fontHAlignGet( getFlags() ) ) {
		case UI_HALIGN_CENTER:
			mRealAlignOffset.x = (Float)( (Int32)( mSize.x - mPassCache->getTextWidth() ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mRealAlignOffset.x = ( (Float)mSize.x - (Float)mPassCache->getTextWidth() );
			break;
		case UI_HALIGN_LEFT:
			mRealAlignOffset.x = 0.f;
			break;
	}

	switch ( fontVAlignGet( getFlags() ) ) {
		case UI_VALIGN_CENTER:
			mRealAlignOffset.y = (Float)( ( (Int32)( mSize.y - mPassCache->getTextHeight() ) ) / 2 ) - 1;
			break;
		case UI_VALIGN_BOTTOM:
			mRealAlignOffset.y = ( (Float)mSize.y - (Float)mPassCache->getTextHeight() );
			break;
		case UI_VALIGN_TOP:
			mRealAlignOffset.y = 0.f;
			break;
	}

	if ( fontHAlignGet( getFlags() ) == UI_HALIGN_LEFT ) {
		Uint32 NLPos	= 0;
		Uint32 LineNum	= mTextBuffer.getCurPosLinePos( NLPos );

		String curStr( mTextBuffer.getBuffer().substr( NLPos, mTextBuffer.getCursorPosition() - NLPos ) );
		String pasStr;

		for ( size_t i = 0; i < curStr.size(); i++ )
			pasStr += '*';

		mPassCache->setString( pasStr );

		Float tW	= mPassCache->getTextWidth();
		Float tX	= mRealAlignOffset.x + tW;

		mCurPos.x	= tW;
		mCurPos.y	= (Float)LineNum * (Float)mPassCache->getFont()->getLineSpacing( mPassCache->getCharacterSizePx() );

		if ( !mTextBuffer.setSupportNewLine() ) {
			if ( tX < 0.f )
				mRealAlignOffset.x = -( mRealAlignOffset.x + ( tW - mRealAlignOffset.x ) );
			else if ( tX > mSize.getWidth() - mRealPadding.Left - mRealPadding.Right )
				mRealAlignOffset.x = mSize.getWidth() - mRealPadding.Left - mRealPadding.Right - ( mRealAlignOffset.x + ( tW - mRealAlignOffset.x ) );
		}
	}
}

void UITextInputPassword::updateText() {
	updatePass( mTextCache->getString() );
}

void UITextInputPassword::updatePass( const String& pass ) {
	String newTxt;

	for ( size_t i = 0; i < pass.size(); i++ )
		newTxt += '*';

	mPassCache->setString( newTxt );
}

UITextView * UITextInputPassword::setText( const String& text ) {
	UITextInput::setText( text );

	updatePass( text );

	return this;
}

Text *UITextInputPassword::getPassCache() const {
	return mPassCache;
}

void UITextInputPassword::updateFontStyleConfig() {
	mPassCache->setCharacterSize( mFontStyleConfig.CharacterSize );
	mPassCache->setFont( mFontStyleConfig.getFont() );
	mPassCache->setFillColor( mFontStyleConfig.getFontColor() );
	mPassCache->setShadowColor( mFontStyleConfig.getFontShadowColor() );
}

void UITextInputPassword::onStateChange() {
	updateFontStyleConfig();
	UITextInput::onStateChange();
}

const String& UITextInputPassword::getText() {
	return UITextView::getText();
}


}}
