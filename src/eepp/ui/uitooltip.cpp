#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/graphics/text.hpp>

namespace EE { namespace UI {

UITooltip *UITooltip::New() {
	return eeNew( UITooltip, () );
}

UITooltip::UITooltip() :
	UINode(),
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
	return UITooltip::getType() == type ? true : UINode::isType( type );
}

void UITooltip::setTheme( UITheme * Theme ) {
	setThemeSkin( Theme, "tooltip" );

	autoPadding();
}

void UITooltip::autoPadding() {
	if ( ( mFlags & UI_AUTO_PADDING ) && mStyleConfig.Padding == Rectf() ) {
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
		UINode::draw();

		if ( mTextCache->getTextWidth() ) {
			mTextCache->setAlign( getFlags() );
			mTextCache->draw( (Float)mScreenPosi.x + (int)mAlignOffset.x, (Float)mScreenPosi.y + (int)mAlignOffset.y, Vector2f::One, 0.f, getBlendMode() );
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

const Color& UITooltip::getFontColor() const {
	return mStyleConfig.FontColor;
}

void UITooltip::setFontColor( const Color& color ) {
	mStyleConfig.FontColor = color;
	mTextCache->setFillColor( mStyleConfig.FontColor );
	setAlpha( color.a );
}

const Color& UITooltip::getFontShadowColor() const {
	return mStyleConfig.ShadowColor;
}

void UITooltip::setFontShadowColor( const Color& color ) {
	mStyleConfig.ShadowColor = color;
	setAlpha( color.a );
	mTextCache->setShadowColor( mStyleConfig.ShadowColor );
}

void UITooltip::setAlpha( const Float& alpha ) {
	UINode::setAlpha( alpha );
	mStyleConfig.FontColor.a = (Uint8)alpha;
	mStyleConfig.ShadowColor.a = (Uint8)alpha;

	mTextCache->setFillColor( mStyleConfig.FontColor );
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
	Uint32 Width	= mSize.getWidth()	- mRealPadding.Left - mRealPadding.Right;
	Uint32 Height	= mSize.getHeight()	- mRealPadding.Top	- mRealPadding.Bottom;

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

	UINode::onSizeChange();
}

void UITooltip::onTextChanged() {
	sendCommonEvent( UIEvent::OnTextChanged );
}

void UITooltip::onFontChanged() {
	sendCommonEvent( UIEvent::OnFontChanged );
}

void UITooltip::setPadding( const Rectf& padding ) {
	mStyleConfig.Padding = padding;
	mRealPadding = PixelDensity::dpToPx( padding );
}

const Rectf& UITooltip::getPadding() const {
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

UINode * UITooltip::getTooltipOf() const {
	return mTooltipOf;
}

void UITooltip::setTooltipOf(UINode * tooltipOf) {
	mTooltipOf = tooltipOf;
}

UITooltipStyleConfig UITooltip::getStyleConfig() const {
	return mStyleConfig;
}

void UITooltip::setStyleConfig(const UITooltipStyleConfig & styleConfig) {
	mStyleConfig = styleConfig;

	if ( mStyleConfig.Padding != Rectf() )
		setPadding( mStyleConfig.Padding );

	setFont( mStyleConfig.Font );
	setFontColor( mStyleConfig.FontColor );
	setFontShadowColor( mStyleConfig.ShadowColor );
	mTextCache->setCharacterSize( mStyleConfig.CharacterSize );
	mTextCache->setStyle( mStyleConfig.Style );
	mTextCache->setOutlineThickness( mStyleConfig.OutlineThickness );
	mTextCache->setOutlineColor( mStyleConfig.OutlineColor );
}

}}
