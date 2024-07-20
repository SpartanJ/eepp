#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/uitextinputpassword.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UITextInputPassword* UITextInputPassword::New() {
	return eeNew( UITextInputPassword, () );
}

UITextInputPassword::UITextInputPassword() :
	UITextInput( "textinputpassword" ), mBulletCharacter( "●" ) {
	mPassCache = Text::New();

	updateFontStyleConfig();
}

UITextInputPassword::~UITextInputPassword() {
	eeSAFE_DELETE( mPassCache );
}

void UITextInputPassword::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UINode::draw();

		if ( mPassCache->getTextWidth() ) {
			drawSelection( mPassCache );

			if ( isClipped() ) {
				clipSmartEnable( mScreenPos.x + mPaddingPx.Left, mScreenPos.y + mPaddingPx.Top,
								 mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
								 mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
			}

			mPassCache->setAlign( getFlags() );
			mPassCache->draw( (Float)mScreenPosi.x + (int)mRealAlignOffset.x + (int)mPaddingPx.Left,
							  (Float)mScreenPosi.y + (int)mRealAlignOffset.y + (int)mPaddingPx.Top,
							  Vector2f::One, 0.f, getBlendMode() );

			if ( isClipped() ) {
				clipSmartDisable();
			}
		} else if ( !mHintCache->getString().empty() ) {
			if ( isClipped() ) {
				clipSmartEnable( mScreenPos.x + mPaddingPx.Left, mScreenPos.y + mPaddingPx.Top,
								 mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
								 mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
			}

			mHintCache->draw( (Float)mScreenPosi.x + (int)mHintAlignOffset.x + (int)mPaddingPx.Left,
							  (Float)mScreenPosi.y + (int)mHintAlignOffset.y + (int)mPaddingPx.Top,
							  Vector2f::One, 0.f, getBlendMode() );

			if ( isClipped() ) {
				clipSmartDisable();
			}
		}
	}

	drawWaitingCursor();
}

Text* UITextInputPassword::getVisibleTextCache() const {
	return mPassCache;
}

void UITextInputPassword::updateText() {
	updatePass( mTextCache->getString() );
}

void UITextInputPassword::updatePass( const String& pass ) {
	if ( mBulletCharacter.size() == 1 && mPassCache->getFont()->getType() == FontType::TTF &&
		 !static_cast<FontTrueType*>( mPassCache->getFont() )
			  ->hasGlyph( mBulletCharacter.front() ) ) {
		mBulletCharacter = "•";
	}

	String newTxt;
	newTxt.reserve( pass.size() );
	for ( size_t i = 0; i < pass.size(); i++ )
		newTxt += mBulletCharacter;

	mPassCache->setString( newTxt );
}

UITextView* UITextInputPassword::setText( const String& text ) {
	UITextInput::setText( text );

	updatePass( text );

	return this;
}

Text* UITextInputPassword::getPassCache() const {
	return mPassCache;
}

const String& UITextInputPassword::getBulletCharacter() const {
	return mBulletCharacter;
}

void UITextInputPassword::setBulletCharacter( const String& bulletCharacter ) {
	mBulletCharacter = bulletCharacter;
}

void UITextInputPassword::updateFontStyleConfig() {
	mPassCache->setFontSize( mFontStyleConfig.CharacterSize );
	mPassCache->setFont( mFontStyleConfig.getFont() );
	mPassCache->setFillColor( mFontStyleConfig.getFontColor() );
	mPassCache->setShadowColor( mFontStyleConfig.getFontShadowColor() );
	mPassCache->setOutlineColor( mFontStyleConfig.getOutlineColor() );
	mPassCache->setOutlineThickness( mFontStyleConfig.getOutlineThickness() );
}

void UITextInputPassword::onStateChange() {
	updateFontStyleConfig();
	UITextInput::onStateChange();
}

void UITextInputPassword::onFontChanged() {
	updateFontStyleConfig();
	UITextInput::onFontChanged();
}

void UITextInputPassword::onFontStyleChanged() {
	updateFontStyleConfig();
	UITextInput::onFontStyleChanged();
}

const String& UITextInputPassword::getText() const {
	return UITextView::getText();
}

}} // namespace EE::UI
