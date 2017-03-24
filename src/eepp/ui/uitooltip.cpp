#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/graphics/text.hpp>

namespace EE { namespace UI {

UITooltip *UITooltip::New() {
	return eeNew( UITooltip, () );
}

UITooltip::UITooltip() :
	UIControlAnim(),
	mAlignOffset( 0.f, 0.f ),
	mTooltipTime( Time::Zero ),
	mTooltipOf()
{
	setFlags( UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_PADDING | UI_AUTO_SIZE );

	mTextCache = eeNew( Text, () );

	UITheme * theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != theme ) {
		setStyleConfig( theme->getTooltipStyleConfig() );
	}

	if ( NULL == getFont() ) {
		if ( NULL != UIThemeManager::instance()->getDefaultFont() )
			setFont( UIThemeManager::instance()->getDefaultFont() );
		else
			eePRINTL( "UITooltip::UITooltip : Created a UI TextBox without a defined font." );
	}

	autoPadding();

	applyDefaultTheme();
}

UITooltip::~UITooltip() {
	eeSAFE_DELETE( mTextCache );

	if ( NULL != mTooltipOf && mTooltipOf->isWidget() ) {
		reinterpret_cast<UIWidget*>( mTooltipOf )->tooltipRemove();
	}
}

Uint32 UITooltip::getType() const {
	return UI_TYPE_TOOLTIP;
}

bool UITooltip::isType( const Uint32& type ) const {
	return UITooltip::getType() == type ? true : UIControlAnim::isType( type );
}

void UITooltip::setTheme( UITheme * Theme ) {
	setThemeControl( Theme, "tooltip" );

	autoPadding();
}

void UITooltip::autoPadding() {
	if ( ( mFlags & UI_AUTO_PADDING ) && mStyleConfig.Padding == Recti() ) {
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
	if ( mVisible && 0.f != mAlpha && mTextCache->getString().size() > 0 ) {
		UIControlAnim::draw();

		if ( mTextCache->getTextWidth() ) {
			mTextCache->setAlign( getFlags() );
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
	return mTextCache->getString();
}

void UITooltip::setText( const String& text ) {
	mTextCache->setString( text );
	autoPadding();
	onAutoSize();
	autoAlign();
	onTextChanged();
}

const ColorA& UITooltip::getFontColor() const {
	return mStyleConfig.Color;
}

void UITooltip::setFontColor( const ColorA& color ) {
	mStyleConfig.Color = color;
	mTextCache->setColor( mStyleConfig.Color );
	setAlpha( color.a );
}

const ColorA& UITooltip::getFontShadowColor() const {
	return mStyleConfig.ShadowColor;
}

void UITooltip::setFontShadowColor( const ColorA& color ) {
	mStyleConfig.ShadowColor = color;
	setAlpha( color.a );
	mTextCache->setShadowColor( mStyleConfig.ShadowColor );
}

void UITooltip::setAlpha( const Float& alpha ) {
	UIControlAnim::setAlpha( alpha );
	mStyleConfig.Color.a = (Uint8)alpha;
	mStyleConfig.ShadowColor.a = (Uint8)alpha;

	mTextCache->setColor( mStyleConfig.Color );
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
}

void UITooltip::onTextChanged() {
	sendCommonEvent( UIEvent::EventOnTextChanged );
}

void UITooltip::onFontChanged() {
	sendCommonEvent( UIEvent::EventOnFontChanged );
}

void UITooltip::setPadding( const Recti& padding ) {
	mStyleConfig.Padding = padding;
	mRealPadding = PixelDensity::dpToPxI( mStyleConfig.Padding );
}

const Recti& UITooltip::getPadding() const {
	return mStyleConfig.Padding;
}

Text * UITooltip::getTextCache() {
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

UITooltipStyleConfig UITooltip::getStyleConfig() const {
	return mStyleConfig;
}

void UITooltip::setStyleConfig(const UITooltipStyleConfig & styleConfig) {
	mStyleConfig = styleConfig;

	if ( mStyleConfig.Padding != Recti() )
		setPadding( mStyleConfig.Padding );

	setFont( mStyleConfig.Font );
	setFontColor( mStyleConfig.Color );
	setFontShadowColor( mStyleConfig.ShadowColor );
	mTextCache->setCharacterSize( mStyleConfig.CharacterSize );
	mTextCache->setStyle( mStyleConfig.Style );
	mTextCache->setOutlineThickness( mStyleConfig.OutlineThickness );
	mTextCache->setOutlineColor( mStyleConfig.OutlineColor );
}

}}
