#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI {

UITooltip* UITooltip::New() {
	return eeNew( UITooltip, () );
}

Vector2f UITooltip::getTooltipPosition( UITooltip* toolip, const Vector2f& requestedPosition ) {
	UISceneNode* uiSceneNode = toolip->getUISceneNode();

	if ( NULL == uiSceneNode )
		return Vector2f::Zero;

	UIThemeManager* themeManager = toolip->getUISceneNode()->getUIThemeManager();
	if ( NULL == themeManager )
		return Vector2f::Zero;

	Vector2f pos = requestedPosition;
	pos -= uiSceneNode->getScreenPos(); // TODO: Fix UISceneNode inside UISceneNode position
	pos.x += themeManager->getCursorSize().x / 2;
	pos.y += themeManager->getCursorSize().y;

	if ( pos.x + toolip->getPixelsSize().getWidth() > uiSceneNode->getPixelsSize().getWidth() ) {
		pos.x = requestedPosition.x - toolip->getPixelsSize().getWidth() - 1;
	}

	if ( pos.y + toolip->getPixelsSize().getHeight() > uiSceneNode->getPixelsSize().getHeight() ) {
		pos.y = requestedPosition.y - toolip->getPixelsSize().getHeight() - 1;
	}

	if ( pos.x < 0 &&
		 toolip->getPixelsSize().getWidth() < uiSceneNode->getPixelsSize().getWidth() ) {
		auto distLeft = std::abs( std::max( 0.f, pos.x ) - requestedPosition.x );
		auto distRight = std::abs( uiSceneNode->getPixelsSize().getWidth() - requestedPosition.x );
		if ( distLeft < distRight ) {
			pos.x = 0;
		} else {
			pos.x = uiSceneNode->getPixelsSize().getWidth() - toolip->getPixelsSize().getWidth();
		}
	}

	if ( pos.y < 0 &&
		 toolip->getPixelsSize().getHeight() < uiSceneNode->getPixelsSize().getHeight() ) {
		pos.y = 0;
	}

	pos.x = eeclamp( pos.x, 0.f,
					 eemax( 0.f, uiSceneNode->getPixelsSize().getWidth() -
									 toolip->getPixelsSize().getWidth() ) );

	return pos;
}

UITooltip::UITooltip() :
	UIWidget( "tooltip" ), mAlignOffset( 0.f, 0.f ), mTooltipTime( Time::Zero ), mTooltipOf() {
	setFlags( UI_NODE_DEFAULT_FLAGS_CENTERED | UI_AUTO_PADDING | UI_AUTO_SIZE );

	mTextCache = Text::New();
	mEnabled = false;

	UITheme* theme = getUISceneNode()->getUIThemeManager()->getDefaultTheme();

	if ( NULL != theme && NULL != theme->getDefaultFont() ) {
		setFont( theme->getDefaultFont() );
	}

	if ( NULL == getFont() ) {
		if ( NULL != getUISceneNode()->getUIThemeManager()->getDefaultFont() ) {
			setFont( getUISceneNode()->getUIThemeManager()->getDefaultFont() );
		} else {
			Log::error( "UITextView::UITextView : Created a without a defined font." );
		}
	}

	if ( NULL != theme ) {
		setFontSize( theme->getDefaultFontSize() );
	} else {
		setFontSize( getUISceneNode()->getUIThemeManager()->getDefaultFontSize() );
	}

	autoPadding();

	applyDefaultTheme();

	getUISceneNode()->invalidateStyle( this );
	getUISceneNode()->invalidateStyleState( this );
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

void UITooltip::setTheme( UITheme* Theme ) {
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
		setVisible( true );

		if ( getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
			runAction( Actions::Sequence::New(
				Actions::Fade::New(
					255.f == mAlpha ? 0.f : mAlpha, 255.f,
					getUISceneNode()->getUIThemeManager()->getWidgetsFadeOutTime() ),
				Actions::Visible::New( true ) ) );
		}
	}

	toFront();
}

void UITooltip::hide() {
	if ( isVisible() ) {
		if ( getUISceneNode()->getUIThemeManager()->getDefaultEffectsEnabled() ) {
			runAction( Actions::Sequence::New(
				Actions::FadeOut::New(
					getUISceneNode()->getUIThemeManager()->getWidgetsFadeOutTime() ),
				Actions::Visible::New( false ) ) );
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
			mTextCache->draw( (Float)mScreenPosi.x + (int)mAlignOffset.x,
							  (Float)mScreenPosi.y + (int)mAlignOffset.y, Vector2f::One, 0.f,
							  getBlendMode() );
		}
	}
}

Graphics::Font* UITooltip::getFont() const {
	return mTextCache->getFont();
}

void UITooltip::setFont( Graphics::Font* font ) {
	if ( NULL != font && mTextCache->getFont() != font ) {
		mTextCache->setFont( font );
		mStyleConfig.Font = font;
		autoPadding();
		onAutoSize();
		autoAlign();
		onFontChanged();
	}
}

const String& UITooltip::getText() {
	if ( mFlags & UI_WORD_WRAP )
		return mStringBuffer;

	return mTextCache->getString();
}

void UITooltip::setText( const String& text ) {
	mStringBuffer = text;
	mTextCache->setString( text );
	autoPadding();
	autoWrap();
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
		mTextCache->setColor( color );
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
		if ( mStyleConfig.ShadowColor != Color::Transparent )
			mStyleConfig.Style |= Text::Shadow;
		else
			mStyleConfig.Style &= ~Text::Shadow;
		mTextCache->setShadowColor( color );
		onAlphaChange();
		invalidateDraw();
	}
}

const Vector2f& UITooltip::getFontShadowOffset() const {
	return mStyleConfig.ShadowOffset;
}

void UITooltip::notifyTextChangedFromTextCache() {
	autoPadding();
	onAutoSize();
	autoAlign();
	invalidateDraw();
}

void UITooltip::setFontShadowOffset( const Vector2f& offset ) {
	if ( mStyleConfig.ShadowOffset != offset ) {
		mStyleConfig.ShadowOffset = offset;
		mTextCache->setShadowOffset( offset );
		invalidateDraw();
	}
}

void UITooltip::onAutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		setInternalPixelsSize(
			Sizef( (int)mTextCache->getTextWidth() + mPaddingPx.Left + mPaddingPx.Right,
				   (int)mTextCache->getTextHeight() + mPaddingPx.Top + mPaddingPx.Bottom ) );

		autoAlign();
	}
}

void UITooltip::autoAlign() {
	Uint32 Width = mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right;
	Uint32 Height = mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom;

	switch ( Font::getHorizontalAlign( getFlags() ) ) {
		case UI_HALIGN_CENTER:
			mAlignOffset.x =
				mPaddingPx.Left + (Float)( (Int32)( Width - mTextCache->getTextWidth() ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mAlignOffset.x =
				( (Float)Width - (Float)mTextCache->getTextWidth() ) - mPaddingPx.Right;
			break;
		case UI_HALIGN_LEFT:
			mAlignOffset.x = mPaddingPx.Left;
			break;
	}

	switch ( Font::getVerticalAlign( getFlags() ) ) {
		case UI_VALIGN_CENTER:
			mAlignOffset.y =
				mPaddingPx.Top + (Float)( ( (Int32)( Height - mTextCache->getTextHeight() ) ) / 2 );
			break;
		case UI_VALIGN_BOTTOM:
			mAlignOffset.y =
				( (Float)Height - (Float)mTextCache->getTextHeight() ) - mPaddingPx.Bottom;
			break;
		case UI_VALIGN_TOP:
			mAlignOffset.y = mPaddingPx.Top;
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

const Text* UITooltip::getTextCache() const {
	return mTextCache;
}

Text* UITooltip::getTextCache() {
	return mTextCache;
}

Float UITooltip::getTextWidth() {
	return mTextCache->getTextWidth();
}

Float UITooltip::getTextHeight() {
	return mTextCache->getTextHeight();
}

Uint32 UITooltip::getNumLines() {
	return mTextCache->getNumLines();
}

Vector2f UITooltip::getAlignOffset() {
	return PixelDensity::pxToDp( mAlignOffset );
}

UINode* UITooltip::getTooltipOf() const {
	return mTooltipOf;
}

void UITooltip::setTooltipOf( UINode* tooltipOf ) {
	mTooltipOf = tooltipOf;
}

const UIFontStyleConfig& UITooltip::getFontStyleConfig() const {
	return mStyleConfig;
}

Uint32 UITooltip::getCharacterSize() const {
	return mTextCache->getCharacterSize();
}

UITooltip* UITooltip::setFontSize( const Uint32& characterSize ) {
	if ( mTextCache->getCharacterSize() != characterSize ) {
		mStyleConfig.CharacterSize = characterSize;
		mTextCache->setFontSize( characterSize );
		onAutoSize();
		autoAlign();
		invalidateDraw();
	}

	return this;
}

UITooltip* UITooltip::setFontStyle( const Uint32& fontStyle ) {
	if ( mStyleConfig.Style != fontStyle ) {
		mTextCache->setStyle( fontStyle );
		mStyleConfig.Style = fontStyle;
		onAutoSize();
		autoAlign();
		invalidateDraw();
	}

	return this;
}

const Uint32& UITooltip::getFontStyle() const {
	return mStyleConfig.Style;
}

const Float& UITooltip::getOutlineThickness() const {
	return mStyleConfig.OutlineThickness;
}

UITooltip* UITooltip::setOutlineThickness( const Float& outlineThickness ) {
	if ( mStyleConfig.OutlineThickness != outlineThickness ) {
		mTextCache->setOutlineThickness( outlineThickness );
		mStyleConfig.OutlineThickness = outlineThickness;
		onAutoSize();
		autoAlign();
		invalidateDraw();
	}

	return this;
}

const Color& UITooltip::getOutlineColor() const {
	return mStyleConfig.OutlineColor;
}

UITooltip* UITooltip::setOutlineColor( const Color& outlineColor ) {
	if ( mStyleConfig.OutlineColor != outlineColor ) {
		mStyleConfig.OutlineColor = outlineColor;
		mTextCache->setOutlineColor( outlineColor );
		onAlphaChange();
		invalidateDraw();
	}

	return this;
}

void UITooltip::setFontStyleConfig( const UIFontStyleConfig& styleConfig ) {
	mStyleConfig = styleConfig;
	setFont( mStyleConfig.Font );
	setFontColor( mStyleConfig.FontColor );
	setFontShadowColor( mStyleConfig.ShadowColor );
	mTextCache->setFontSize( mStyleConfig.CharacterSize );
	mTextCache->setStyle( mStyleConfig.Style );
	mTextCache->setOutlineThickness( mStyleConfig.OutlineThickness );
	mTextCache->setOutlineColor( mStyleConfig.OutlineColor );
}

std::string UITooltip::getPropertyString( const PropertyDefinition* propertyDef,
										  const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::TextTransform:
			return TextTransform::toString( getTextTransform() );
		case PropertyId::Color:
			return getFontColor().toHexString();
		case PropertyId::TextShadowColor:
			return getFontShadowColor().toHexString();
		case PropertyId::TextShadowOffset:
			return String::fromFloat( getFontShadowOffset().x ) + " " +
				   String::fromFloat( getFontShadowOffset().y );
		case PropertyId::FontFamily:
			return NULL != getFont() ? getFont()->getName() : "";
		case PropertyId::FontSize:
			return String::format( "%dpx", getCharacterSize() );
		case PropertyId::FontStyle:
			return Text::styleFlagToString( getFontStyle() );
		case PropertyId::TextStrokeWidth:
			return String::fromFloat( PixelDensity::dpToPx( getOutlineThickness() ), "px" );
		case PropertyId::TextStrokeColor:
			return getOutlineColor().toHexString();
		case PropertyId::Wordwrap:
			return mFlags & UI_WORD_WRAP ? "true" : "false";
		case PropertyId::TextAlign:
			return Font::getHorizontalAlign( getFlags() ) == UI_HALIGN_CENTER
					   ? "center"
					   : ( Font::getHorizontalAlign( getFlags() ) == UI_HALIGN_RIGHT ? "right"
																					 : "left" );
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UITooltip::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = {
		PropertyId::TextTransform,	  PropertyId::Color,		   PropertyId::TextShadowColor,
		PropertyId::TextShadowOffset, PropertyId::FontFamily,	   PropertyId::FontSize,
		PropertyId::FontStyle,		  PropertyId::TextStrokeWidth, PropertyId::TextStrokeColor,
		PropertyId::TextAlign,		  PropertyId::Wordwrap };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

const String& UITooltip::getStringBuffer() const {
	return mStringBuffer;
}

void UITooltip::setStringBuffer( const String& stringBuffer ) {
	mStringBuffer = stringBuffer;
}

void UITooltip::resetTextToStringBuffer() {
	setText( mStringBuffer );
}

bool UITooltip::dontAutoHideOnMouseMove() const {
	return mDontAutoHideOnMouseMove;
}

void UITooltip::setDontAutoHideOnMouseMove( bool dontAutoHideOnMouseMove ) {
	mDontAutoHideOnMouseMove = dontAutoHideOnMouseMove;
}

void UITooltip::transformText() {
	mTextCache->transformText( mTextTransform );
}

const TextTransform::Value& UITooltip::getTextTransform() const {
	return mTextTransform;
}

void UITooltip::setTextTransform( const TextTransform::Value& textTransform ) {
	if ( textTransform != mTextTransform ) {
		mTextTransform = textTransform;
		transformText();
	}
}

Vector2f UITooltip::getTooltipPosition( const Vector2f& requestedPosition ) {
	return UITooltip::getTooltipPosition( this, requestedPosition );
}

bool UITooltip::getUsingCustomStyling() const {
	return mUsingCustomStyling;
}

void UITooltip::setUsingCustomStyling( bool usingCustomStyling ) {
	mUsingCustomStyling = usingCustomStyling;
}

bool UITooltip::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::TextTransform:
			if ( !mUsingCustomStyling )
				setTextTransform( TextTransform::fromString( attribute.asString() ) );
		case PropertyId::Color:
			if ( !mUsingCustomStyling )
				setFontColor( attribute.asColor() );
			break;
		case PropertyId::TextShadowColor: {
			if ( !mUsingCustomStyling )
				setFontShadowColor( attribute.asColor() );
			break;
		}
		case PropertyId::TextShadowOffset:
			if ( !mUsingCustomStyling )
				setFontShadowOffset( attribute.asVector2f() );
			break;
		case PropertyId::FontFamily: {
			Font* font = FontManager::instance()->getByName( attribute.value() );

			if ( !mUsingCustomStyling && NULL != font && font->loaded() )
				setFont( font );
			break;
		}
		case PropertyId::FontSize:
			if ( !mUsingCustomStyling ) {
				Uint32 flags = attribute.asFontStyle();

				if ( flags & UI_WORD_WRAP ) {
					mFlags |= UI_WORD_WRAP;
					flags &= ~UI_WORD_WRAP;
					autoWrap();
				}

				setFontStyle( flags );
			}
			break;
		case PropertyId::Wordwrap:
			if ( attribute.asBool() )
				mFlags |= UI_WORD_WRAP;
			else
				mFlags &= ~UI_WORD_WRAP;
			autoWrap();
			break;
		case PropertyId::FontStyle:
			if ( !mUsingCustomStyling )
				setFontStyle( attribute.asFontStyle() );
			break;
		case PropertyId::TextStrokeWidth:
			if ( !mUsingCustomStyling )
				setOutlineThickness( PixelDensity::dpToPx( attribute.asDpDimension() ) );
			break;
		case PropertyId::TextStrokeColor:
			if ( !mUsingCustomStyling )
				setOutlineColor( attribute.asColor() );
			break;
		case PropertyId::TextAlign: {
			std::string align = String::toLower( attribute.value() );
			if ( align == "center" )
				setFlags( UI_HALIGN_CENTER );
			else if ( align == "left" )
				setFlags( UI_HALIGN_LEFT );
			else if ( align == "right" )
				setFlags( UI_HALIGN_RIGHT );
			break;
		}
		default:
			return UIWidget::applyProperty( attribute );
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

void UITooltip::autoWrap() {
	if ( mFlags & UI_WORD_WRAP && !mMaxWidthEq.empty() ) {
		Float length =
			lengthFromValue( mMaxWidthEq, CSS::PropertyRelativeTarget::ContainingBlockWidth );
		wrapText( length );
	}
}

void UITooltip::wrapText( const Uint32& maxWidth ) {
	if ( mFlags & UI_WORD_WRAP ) {
		mTextCache->setString( mStringBuffer );
	}

	mTextCache->wrapText( maxWidth );
	invalidateDraw();
}

void UITooltip::setWordWrap( bool set ) {
	if ( set != isWordWrap() ) {
		if ( set )
			mFlags |= UI_WORD_WRAP;
		else
			mFlags &= ~UI_WORD_WRAP;
		autoWrap();
	}
}

bool UITooltip::isWordWrap() const {
	return mFlags & UI_WORD_WRAP;
}

}} // namespace EE::UI
