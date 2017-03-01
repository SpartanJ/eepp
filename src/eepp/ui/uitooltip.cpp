#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UITooltip::UITooltip() :
	UIControlAnim(),
	mAlignOffset( 0.f, 0.f ),
	mPadding(),
	mTooltipTime( Time::Zero ),
	mTooltipOf()
{
	setFlags( UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_PADDING | UI_AUTO_SIZE );

	mTextCache = eeNew( TextCache, () );

	mFontStyleConfig = UIThemeManager::instance()->getDefaultFontStyleConfig();

	UITheme * Theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != Theme ) {
		if ( Theme->getTooltipPadding() != Recti() )
			setPadding( Theme->getTooltipPadding() );

		setFont( mFontStyleConfig.Font );
		setFontColor( Theme->getTooltipFontColor() );
		setFontShadowColor( mFontStyleConfig.FontShadowColor );
	}

	if ( NULL == getFont() ) {
		if ( NULL != UIThemeManager::instance()->getDefaultFont() )
			setFont( UIThemeManager::instance()->getDefaultFont() );
	} else {
		eePRINTL( "UITooltip::UITooltip : Created a UI TextBox without a defined font." );
	}

	autoPadding();

	applyDefaultTheme();
}

UITooltip::~UITooltip() {
	eeSAFE_DELETE( mTextCache );

	if ( NULL != mTooltipOf && mTooltipOf->isComplex() ) {
		reinterpret_cast<UIComplexControl*>( mTooltipOf )->tooltipRemove();
	}
}

Uint32 UITooltip::getType() const {
	return UI_TYPE_TOOLTIP;
}

bool UITooltip::isType( const Uint32& type ) const {
	return UITooltip::getType() == type ? true : UIControlAnim::isType( type );
}

void UITooltip::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "tooltip" );

	autoPadding();
}

void UITooltip::autoPadding() {
	if ( ( mFlags & UI_AUTO_PADDING ) && mPadding == Recti() ) {
		setPadding( makePadding( true, true, true, true ) );
	}
}

void UITooltip::show() {
	if ( !isVisible() || 0 == mAlpha ) {
		toFront();

		setVisible( true );
		setEnabled( true );

		if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
			startAlphaAnim( 255.f == mAlpha ? 0.f : mAlpha, 255.f, UIThemeManager::instance()->getControlsFadeInTime() );
		}
	}
}

void UITooltip::hide() {
	if ( isVisible() ) {
		if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
			disableFadeOut( UIThemeManager::instance()->getControlsFadeOutTime() );
		} else {
			setVisible( false );
			setEnabled( false );
		}
	}
}

void UITooltip::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UIControlAnim::draw();

		if ( mTextCache->getTextWidth() ) {
			mTextCache->setFlags( getFlags() );
			mTextCache->draw( (Float)mScreenPos.x + mAlignOffset.x, (Float)mScreenPos.y + mAlignOffset.y, Vector2f::One, 0.f, getBlendMode() );
		}
	}
}

Graphics::Font * UITooltip::getFont() const {
	return mTextCache->getFont();
}

void UITooltip::setFont( Graphics::Font * font ) {
	if ( mTextCache->getFont() != font ) {
		mTextCache->setFont( font );
		autoPadding();
		onAutoSize();
		autoAlign();
		onFontChanged();
	}
}

const String& UITooltip::getText() {
	return mTextCache->getText();
}

void UITooltip::setText( const String& text ) {
	mTextCache->setText( text );
	autoPadding();
	onAutoSize();
	autoAlign();
	onTextChanged();
}

const ColorA& UITooltip::getFontColor() const {
	return mFontStyleConfig.FontColor;
}

void UITooltip::setFontColor( const ColorA& color ) {
	mFontStyleConfig.FontColor = color;
	mTextCache->setColor( mFontStyleConfig.FontColor );
	setAlpha( color.a() );
}

const ColorA& UITooltip::getFontShadowColor() const {
	return mFontStyleConfig.FontShadowColor;
}

void UITooltip::setFontShadowColor( const ColorA& color ) {
	mFontStyleConfig.FontShadowColor = color;
	setAlpha( color.a() );
	mTextCache->setShadowColor( mFontStyleConfig.FontShadowColor );
}

void UITooltip::setAlpha( const Float& alpha ) {
	UIControlAnim::setAlpha( alpha );
	mFontStyleConfig.FontColor.Alpha = (Uint8)alpha;
	mFontStyleConfig.FontShadowColor.Alpha = (Uint8)alpha;

	mTextCache->setColor( mFontStyleConfig.FontColor );
}

void UITooltip::onAutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		setPixelsSize(
			(int)mTextCache->getTextWidth() + mRealPadding.Left + mRealPadding.Right,
			(int)mTextCache->getTextHeight() + mRealPadding.Top + mRealPadding.Bottom
		);
	}
}

void UITooltip::autoAlign() {
	Uint32 Width	= mRealSize.getWidth()	- mRealPadding.Left - mRealPadding.Right;
	Uint32 Height	= mRealSize.getHeight()	- mRealPadding.Top	- mRealPadding.Bottom;

	switch ( fontHAlignGet( getFlags() ) ) {
		case UI_HALIGN_CENTER:
			mAlignOffset.x = mRealPadding.Left + (Float)( (Int32)( Width - mTextCache->getTextWidth() ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mAlignOffset.x = ( (Float)Width - (Float)mTextCache->getTextWidth() ) - mRealPadding.Right;
			break;
		case UI_HALIGN_LEFT:
			mAlignOffset.x = mRealPadding.Left;
			break;
	}

	switch ( fontVAlignGet( getFlags() ) ) {
		case UI_VALIGN_CENTER:
			mAlignOffset.y = mRealPadding.Top + (Float)( ( (Int32)( Height - mTextCache->getTextHeight() ) ) / 2 );
			break;
		case UI_VALIGN_BOTTOM:
			mAlignOffset.y = ( (Float)Height - (Float)mTextCache->getTextHeight() ) - mRealPadding.Bottom;
			break;
		case UI_VALIGN_TOP:
			mAlignOffset.y = mRealPadding.Top;
			break;
	}
}

void UITooltip::onSizeChange() {
	autoPadding();
	onAutoSize();
	autoAlign();

	UIControlAnim::onSizeChange();

	mTextCache->cacheWidth();
}

void UITooltip::onTextChanged() {
	sendCommonEvent( UIEvent::EventOnTextChanged );
}

void UITooltip::onFontChanged() {
	sendCommonEvent( UIEvent::EventOnFontChanged );
}

void UITooltip::setPadding( const Recti& padding ) {
	mPadding = padding;
	mRealPadding = PixelDensity::dpToPxI( mPadding );

}

const Recti& UITooltip::getPadding() const {
	return mPadding;
}

TextCache * UITooltip::getTextCache() {
	return mTextCache;
}

Float UITooltip::getTextWidth() {
	return mTextCache->getTextWidth();
}

Float UITooltip::getTextHeight() {
	return mTextCache->getTextHeight();
}

const int& UITooltip::getNumLines() const {
	return mTextCache->getNumLines();
}

Vector2f UITooltip::getAlignOffset() {
	return PixelDensity::pxToDp( mAlignOffset );
}

void UITooltip::setTooltipTime( const Time& Time ) {
	mTooltipTime = Time;
}

void UITooltip::addTooltipTime( const Time& Time ) {
	mTooltipTime += Time;
}

const Time& UITooltip::getTooltipTime() const {
	return mTooltipTime;
}

UIControl * UITooltip::getTooltipOf() const {
	return mTooltipOf;
}

void UITooltip::setTooltipOf(UIControl * tooltipOf) {
	mTooltipOf = tooltipOf;
}

FontStyleConfig UITooltip::getFontStyleConfig() const {
	return mFontStyleConfig;
}

void UITooltip::setFontStyleConfig(const FontStyleConfig & fontStyleConfig) {
	mFontStyleConfig = fontStyleConfig;

	setFont( mFontStyleConfig.Font );
	setFontColor( mFontStyleConfig.FontColor );
	setFontShadowColor( mFontStyleConfig.FontShadowColor );
}

}}
