#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uithememanager.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UITextSpan* UITextSpan::New() {
	return eeNew( UITextSpan, () );
}

UITextSpan* UITextSpan::NewWithTag( const std::string& tag ) {
	return eeNew( UITextSpan, ( tag ) );
}

UITextSpan::UITextSpan( const std::string& tag ) : UIWidget( tag ) {
	mFlags |= UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_LOADS_ITS_CHILDREN;

	UITheme* theme = getUISceneNode()->getUIThemeManager()->getDefaultTheme();

	if ( NULL != theme && NULL != theme->getDefaultFont() ) {
		mFontStyleConfig.Font = theme->getDefaultFont();
	} else if ( NULL != getUISceneNode()->getUIThemeManager()->getDefaultFont() ) {
		mFontStyleConfig.Font = getUISceneNode()->getUIThemeManager()->getDefaultFont();
	}

	if ( NULL != theme ) {
		mFontStyleConfig.CharacterSize = theme->getDefaultFontSize();
	} else {
		mFontStyleConfig.CharacterSize =
			getUISceneNode()->getUIThemeManager()->getDefaultFontSize();
	}
}

UITextSpan::~UITextSpan() {}

Uint32 UITextSpan::getType() const {
	return UI_TYPE_TEXTSPAN;
}

bool UITextSpan::isType( const Uint32& type ) const {
	return UITextSpan::getType() == type ? true : UIWidget::isType( type );
}

void UITextSpan::draw() {
	// Skip native generic rendering because it will be drawn by UIRichText
}

bool UITextSpan::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
			setText( getTranslatorString( attribute.value() ) );
			break;
		case PropertyId::Color:
			setFontColor( attribute.asColor() );
			break;
		case PropertyId::TextShadowColor:
			setFontShadowColor( attribute.asColor() );
			break;
		case PropertyId::TextShadowOffset:
			setFontShadowOffset( attribute.asVector2f() );
			break;
		case PropertyId::FontFamily: {
			Graphics::Font* font =
				Graphics::FontManager::instance()->getByName( attribute.value() );
			if ( NULL != font && font->loaded() ) {
				setFont( font );
			}
			break;
		}
		case PropertyId::FontSize:
			setFontSize( lengthFromValue( attribute ) );
			break;
		case PropertyId::FontStyle:
			setFontStyle( attribute.asFontStyle() );
			break;
		case PropertyId::TextStrokeWidth:
			setOutlineThickness( lengthFromValue( attribute ) );
			break;
		case PropertyId::TextStrokeColor:
			setOutlineColor( attribute.asColor() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

std::string UITextSpan::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Text:
			return getText().toUtf8();
		case PropertyId::FontFamily:
			return NULL != getFont() ? getFont()->getName() : "";
		case PropertyId::FontSize:
			return String::format( "%dpx", getFontSize() );
		case PropertyId::FontStyle:
			return Graphics::Text::styleFlagToString( getFontStyle() );
		case PropertyId::Color:
			return getFontColor().toHexString();
		case PropertyId::TextShadowColor:
			return getFontShadowColor().toHexString();
		case PropertyId::TextShadowOffset:
			return String::fromFloat( getFontShadowOffset().x ) + " " +
				   String::fromFloat( getFontShadowOffset().y );
		case PropertyId::TextStrokeWidth:
			return String::fromFloat( PixelDensity::dpToPx( getOutlineThickness() ), "px" );
		case PropertyId::TextStrokeColor:
			return getOutlineColor().toHexString();
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UITextSpan::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::Text,
				   PropertyId::FontFamily,
				   PropertyId::FontSize,
				   PropertyId::FontStyle,
				   PropertyId::Color,
				   PropertyId::TextShadowColor,
				   PropertyId::TextShadowOffset,
				   PropertyId::TextStrokeWidth,
				   PropertyId::TextStrokeColor };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

const String& UITextSpan::getText() const {
	return mText;
}

UITextSpan* UITextSpan::setText( const String& text ) {
	if ( mText != text ) {
		mText = text;
		onTextChanged();
		notifyLayoutAttrChange();
	}
	return this;
}

const UIFontStyleConfig& UITextSpan::getFontStyleConfig() const {
	return mFontStyleConfig;
}

void UITextSpan::setFontStyleConfig( const UIFontStyleConfig& fontStyleConfig ) {
	mFontStyleConfig = fontStyleConfig;
	mStyleState = StyleStateAll;
	onFontStyleChanged();
	onFontChanged();
	notifyLayoutAttrChange();
}

Graphics::Font* UITextSpan::getFont() const {
	return mFontStyleConfig.getFont();
}

UITextSpan* UITextSpan::setFont( Graphics::Font* font ) {
	if ( mFontStyleConfig.Font != font ) {
		mFontStyleConfig.Font = font;
		mStyleState |= StyleStateFont;
		onFontChanged();
		notifyLayoutAttrChange();
	}
	return this;
}

Uint32 UITextSpan::getFontSize() const {
	return mFontStyleConfig.getFontCharacterSize();
}

UITextSpan* UITextSpan::setFontSize( const Uint32& characterSize ) {
	if ( mFontStyleConfig.CharacterSize != characterSize ) {
		mFontStyleConfig.CharacterSize = characterSize;
		mStyleState |= StyleStateFontSize;
		onFontStyleChanged();
		notifyLayoutAttrChange();
	}
	return this;
}

const Uint32& UITextSpan::getFontStyle() const {
	return mFontStyleConfig.getFontStyle();
}

UITextSpan* UITextSpan::setFontStyle( const Uint32& fontStyle ) {
	if ( mFontStyleConfig.Style != fontStyle ) {
		mFontStyleConfig.Style = fontStyle;
		mStyleState |= StyleStateFontStyle;
		onFontStyleChanged();
		notifyLayoutAttrChange();
	}
	return this;
}

const Float& UITextSpan::getOutlineThickness() const {
	return mFontStyleConfig.getOutlineThickness();
}

UITextSpan* UITextSpan::setOutlineThickness( const Float& outlineThickness ) {
	if ( mFontStyleConfig.OutlineThickness != outlineThickness ) {
		mFontStyleConfig.OutlineThickness = outlineThickness;
		mStyleState |= StyleStateOutlineThickness;
		onFontStyleChanged();
		notifyLayoutAttrChange();
	}
	return this;
}

const Color& UITextSpan::getOutlineColor() const {
	return mFontStyleConfig.getOutlineColor();
}

UITextSpan* UITextSpan::setOutlineColor( const Color& outlineColor ) {
	if ( mFontStyleConfig.OutlineColor != outlineColor ) {
		mFontStyleConfig.OutlineColor = outlineColor;
		mStyleState |= StyleStateOutlineColor;
		onFontStyleChanged();
	}
	return this;
}

const Color& UITextSpan::getFontColor() const {
	return mFontStyleConfig.getFontColor();
}

UITextSpan* UITextSpan::setFontColor( const Color& color ) {
	if ( mFontStyleConfig.FontColor != color ) {
		mFontStyleConfig.FontColor = color;
		mStyleState |= StyleStateFontColor;
		onFontStyleChanged();
	}
	return this;
}

const Color& UITextSpan::getFontShadowColor() const {
	return mFontStyleConfig.getFontShadowColor();
}

UITextSpan* UITextSpan::setFontShadowColor( const Color& color ) {
	if ( mFontStyleConfig.ShadowColor != color ) {
		mFontStyleConfig.ShadowColor = color;
		if ( color != Color::Transparent )
			mFontStyleConfig.Style |= Graphics::Text::Shadow;
		else
			mFontStyleConfig.Style &= ~Graphics::Text::Shadow;
		mStyleState |= StyleStateFontShadowColor;
		onFontStyleChanged();
		notifyLayoutAttrChange();
	}
	return this;
}

const Vector2f& UITextSpan::getFontShadowOffset() const {
	return mFontStyleConfig.getFontShadowOffset();
}

UITextSpan* UITextSpan::setFontShadowOffset( const Vector2f& offset ) {
	if ( mFontStyleConfig.ShadowOffset != offset ) {
		mFontStyleConfig.ShadowOffset = offset;
		mStyleState |= StyleStateFontShadowOffset;
		onFontStyleChanged();
		notifyLayoutAttrChange();
	}
	return this;
}

void UITextSpan::onAlphaChange() {
	UIWidget::onAlphaChange();
	notifyLayoutAttrChange();
}

void UITextSpan::onFontChanged() {
	sendCommonEvent( Event::OnFontChanged );
}

void UITextSpan::onFontStyleChanged() {
	sendCommonEvent( Event::OnFontStyleChanged );
}

void UITextSpan::onTextChanged() {
	sendCommonEvent( Event::OnTextChanged );
	sendCommonEvent( Event::OnValueChange );
}

void UITextSpan::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	UIWidget::loadFromXmlNode( node );

	for ( pugi::xml_node child = node.first_child(); child; child = child.next_sibling() ) {
		if ( child.type() == pugi::node_pcdata )
			mText += getTranslatorString( child.value() );
	}

	endAttributesTransaction();
}

void UITextSpan::setInheritedStyle( const UIFontStyleConfig& fontStyleConfig ) {
	bool fontChanged = false;
	bool fontStyleChanged = false;

	if ( !hasFont() && mFontStyleConfig.Font != fontStyleConfig.Font ) {
		mFontStyleConfig.Font = fontStyleConfig.Font;
		fontChanged = true;
	}

	if ( !hasFontSize() && mFontStyleConfig.CharacterSize != fontStyleConfig.CharacterSize ) {
		mFontStyleConfig.CharacterSize = fontStyleConfig.CharacterSize;
		fontStyleChanged = true;
	}

	if ( !hasFontStyle() && mFontStyleConfig.Style != fontStyleConfig.Style ) {
		mFontStyleConfig.Style = fontStyleConfig.Style;
		fontStyleChanged = true;
	}

	if ( !hasFontColor() && mFontStyleConfig.FontColor != fontStyleConfig.FontColor ) {
		mFontStyleConfig.FontColor = fontStyleConfig.FontColor;
		fontStyleChanged = true;
	}

	if ( !hasOutlineThickness() &&
		 mFontStyleConfig.OutlineThickness != fontStyleConfig.OutlineThickness ) {
		mFontStyleConfig.OutlineThickness = fontStyleConfig.OutlineThickness;
		fontStyleChanged = true;
	}

	if ( !hasOutlineColor() && mFontStyleConfig.OutlineColor != fontStyleConfig.OutlineColor ) {
		mFontStyleConfig.OutlineColor = fontStyleConfig.OutlineColor;
		fontStyleChanged = true;
	}

	if ( !hasFontShadowColor() && mFontStyleConfig.ShadowColor != fontStyleConfig.ShadowColor ) {
		mFontStyleConfig.ShadowColor = fontStyleConfig.ShadowColor;
		fontStyleChanged = true;
	}

	if ( !hasFontShadowOffset() && mFontStyleConfig.ShadowOffset != fontStyleConfig.ShadowOffset ) {
		mFontStyleConfig.ShadowOffset = fontStyleConfig.ShadowOffset;
		fontStyleChanged = true;
	}

	if ( fontChanged )
		onFontChanged();

	if ( fontStyleChanged )
		onFontStyleChanged();

	if ( fontChanged || fontStyleChanged )
		notifyLayoutAttrChange();
}

bool UITextSpan::hasFont() const {
	return 0 != ( mStyleState & StyleStateFont );
}

bool UITextSpan::hasFontSize() const {
	return 0 != ( mStyleState & StyleStateFontSize );
}

bool UITextSpan::hasFontStyle() const {
	return 0 != ( mStyleState & StyleStateFontStyle );
}

bool UITextSpan::hasFontColor() const {
	return 0 != ( mStyleState & StyleStateFontColor );
}

bool UITextSpan::hasOutlineThickness() const {
	return 0 != ( mStyleState & StyleStateOutlineThickness );
}

bool UITextSpan::hasOutlineColor() const {
	return 0 != ( mStyleState & StyleStateOutlineColor );
}

bool UITextSpan::hasFontShadowColor() const {
	return 0 != ( mStyleState & StyleStateFontShadowColor );
}

bool UITextSpan::hasFontShadowOffset() const {
	return 0 != ( mStyleState & StyleStateFontShadowOffset );
}

}} // namespace EE::UI
