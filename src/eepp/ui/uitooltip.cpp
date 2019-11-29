#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/graphics/fontmanager.hpp>

namespace EE { namespace UI {

UITooltip * UITooltip::New() {
	return eeNew( UITooltip, () );
}

UITooltip::UITooltip() :
	UIWidget( "tooltip" ),
	mAlignOffset( 0.f, 0.f ),
	mTooltipTime( Time::Zero ),
	mTooltipOf()
{
	setFlags( UI_CONTROL_DEFAULT_FLAGS_CENTERED | UI_AUTO_PADDING | UI_AUTO_SIZE );

	mTextCache = Text::New();

	UITheme * theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != theme ) {
		mStyleConfig.Font = theme->getDefaultFont();
	}

	if ( NULL == getFont() ) {
		if ( NULL != UIThemeManager::instance()->getDefaultFont() )
			setFont( UIThemeManager::instance()->getDefaultFont() );
		else
			eePRINTL( "UITooltip::UITooltip : Created a without a defined font." );
	}

	autoPadding();

	applyDefaultTheme();
}

UITooltip::~UITooltip() {
	eeSAFE_DELETE( mTextCache );

	if ( NULL != mTooltipOf && mTooltipOf->isWidget() ) {
		mTooltipOf->asType<UIWidget>()->tooltipRemove();
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
	onThemeLoaded();
}

void UITooltip::autoPadding() {
	if ( ( mFlags & UI_AUTO_PADDING ) && mPadding == Rectf() ) {
		setPadding( makePadding( true, true, true, true ) );
	}
}

void UITooltip::show() {
	if ( !isVisible() || 0 == mAlpha ) {
		toFront();

		setVisible( true );

		if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
			runAction( Actions::Sequence::New( Actions::Fade::New( 255.f == mAlpha ? 0.f : mAlpha, 255.f, UIThemeManager::instance()->getControlsFadeOutTime() ),
											   Actions::Spawn::New( Actions::Enable::New(), Actions::Visible::New( true ) ) ) );
		}
	}
}

void UITooltip::hide() {
	if ( isVisible() ) {
		if ( UIThemeManager::instance()->getDefaultEffectsEnabled() ) {
			runAction( Actions::Sequence::New( Actions::FadeOut::New( UIThemeManager::instance()->getControlsFadeOutTime() ), Actions::Spawn::New( Actions::Disable::New(), Actions::Visible::New( false ) ) ) );
		} else {
			setVisible( false );
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
	if ( NULL != font && mTextCache->getFont() != font ) {
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
	if ( mStyleConfig.FontColor != color ) {
		mStyleConfig.FontColor = color;
		onAlphaChange();
		invalidateDraw();
	}
}

const Color& UITooltip::getFontShadowColor() const {
	return mStyleConfig.ShadowColor;
}

void UITooltip::setFontShadowColor( const Color& color ) {
	if ( mStyleConfig.ShadowColor != color ) {
		mStyleConfig.ShadowColor = color;
		onAlphaChange();
		invalidateDraw();
	}
}

void UITooltip::onAutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		setInternalPixelsSize( Sizef(
			(int)mTextCache->getTextWidth() + mRealPadding.Left + mRealPadding.Right,
			(int)mTextCache->getTextHeight() + mRealPadding.Top + mRealPadding.Bottom
		) );

		autoAlign();
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
	sendCommonEvent( Event::OnTextChanged );
	invalidateDraw();
}

void UITooltip::onFontChanged() {
	sendCommonEvent( Event::OnFontChanged );
	invalidateDraw();
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

const UIFontStyleConfig& UITooltip::getFontStyleConfig() const {
	return mStyleConfig;
}

Uint32 UITooltip::getCharacterSize() const {
	return mTextCache->getCharacterSize();
}

UITooltip * UITooltip::setCharacterSize( const Uint32 & characterSize ) {
	if ( mTextCache->getCharacterSize() != characterSize ) {
		mStyleConfig.CharacterSize = characterSize;
		mTextCache->setCharacterSize( characterSize );
		onAutoSize();
		autoAlign();
		invalidateDraw();
	}

	return this;
}

UITooltip * UITooltip::setFontStyle(const Uint32 & fontStyle) {
	if ( mStyleConfig.Style != fontStyle ) {
		mTextCache->setStyle( fontStyle );
		mStyleConfig.Style = fontStyle;
		onAutoSize();
		autoAlign();
		invalidateDraw();
	}

	return this;
}

const Uint32 &UITooltip::getFontStyle() const {
	return mStyleConfig.Style;
}

const Float &UITooltip::getOutlineThickness() const {
	return mStyleConfig.OutlineThickness;
}

UITooltip * UITooltip::setOutlineThickness( const Float & outlineThickness ) {
	if ( mStyleConfig.OutlineThickness != outlineThickness ) {
		mTextCache->setOutlineThickness( outlineThickness );
		mStyleConfig.OutlineThickness = outlineThickness;
		onAutoSize();
		autoAlign();
		invalidateDraw();
	}

	return this;
}

const Color &UITooltip::getOutlineColor() const {
	return mStyleConfig.OutlineColor;
}

UITooltip * UITooltip::setOutlineColor(const Color & outlineColor) {
	if ( mStyleConfig.OutlineColor != outlineColor ) {
		mTextCache->setOutlineColor( outlineColor );
		onAlphaChange();
		invalidateDraw();
	}

	return this;
}

void UITooltip::setFontStyleConfig(const UIFontStyleConfig & styleConfig) {
	mStyleConfig = styleConfig;
	setFont( mStyleConfig.Font );
	setFontColor( mStyleConfig.FontColor );
	setFontShadowColor( mStyleConfig.ShadowColor );
	mTextCache->setCharacterSize( mStyleConfig.CharacterSize );
	mTextCache->setStyle( mStyleConfig.Style );
	mTextCache->setOutlineThickness( mStyleConfig.OutlineThickness );
	mTextCache->setOutlineColor( mStyleConfig.OutlineColor );
}

bool UITooltip::applyProperty( const StyleSheetProperty& attribute, const Uint32& state ) {
	const std::string& name = attribute.getName();

	if ( "color" == name ) {
		setFontColor( attribute.asColor() );
	} else if ( "shadow-color" == name  || "textshadowcolor" == name ) {
		setFontShadowColor( attribute.asColor() );
	} else if ( "font-family" == name || "font-name" == name || "fontfamily" == name || "fontname" == name ) {
		setFont( FontManager::instance()->getByName( attribute.asString() ) );
	} else if ( "font-size" == name || "textsize" == name || "fontsize" == name ) {
		setCharacterSize( attribute.asDpDimensionI() );
	} else if ( "font-style" == name || "textstyle" == name || "fontstyle" == name ) {
		setFontStyle( attribute.asFontStyle() );
	} else if ( "text-stroke-width" == name || "fontoutlinethickness" == name ) {
		setOutlineThickness( attribute.asDpDimension() );
	} else if ( "text-stroke-color" == name || "fontoutlinecolor" == name ) {
		setOutlineColor( attribute.asColor() );
	} else {
		return UIWidget::applyProperty( attribute, state );
	}

	return true;
}

void UITooltip::onAlphaChange() {
	Color color( mStyleConfig.FontColor );
	color.a = mStyleConfig.FontColor.a * getAlpha() / 255.f;

	Color shadowColor( mStyleConfig.ShadowColor );
	shadowColor.a = mStyleConfig.ShadowColor.a * getAlpha() / 255.f;

	Color outlineColor( mStyleConfig.OutlineColor );
	outlineColor.a = mStyleConfig.OutlineColor.a * getAlpha() / 255.f;

	mTextCache->setFillColor( color );
	mTextCache->setShadowColor( shadowColor );
	mTextCache->setOutlineColor( outlineColor );
}

}}
