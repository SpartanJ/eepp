#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIRichText* UIRichText::New() {
	return eeNew( UIRichText, () );
}

UIRichText* UIRichText::NewWithTag( const std::string& tag ) {
	return eeNew( UIRichText, ( tag ) );
}

UIRichText::UIRichText( const std::string& tag ) : UILayout( tag ) {
	mFlags |= UI_LOADS_ITS_CHILDREN;

	UITheme* theme = getUISceneNode()->getUIThemeManager()->getDefaultTheme();

	if ( NULL != theme && NULL != theme->getDefaultFont() ) {
		mRichText.getFontStyleConfig().Font = theme->getDefaultFont();
	} else if ( NULL != getUISceneNode()->getUIThemeManager()->getDefaultFont() ) {
		mRichText.getFontStyleConfig().Font =
			getUISceneNode()->getUIThemeManager()->getDefaultFont();
	}

	if ( NULL != theme ) {
		mRichText.getFontStyleConfig().CharacterSize = theme->getDefaultFontSize();
	} else {
		mRichText.getFontStyleConfig().CharacterSize =
			getUISceneNode()->getUIThemeManager()->getDefaultFontSize();
	}

	setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
}

Uint32 UIRichText::getType() const {
	return UI_TYPE_RICHTEXT;
}

bool UIRichText::isType( const Uint32& type ) const {
	return UIRichText::getType() == type ? true : UILayout::isType( type );
}

const Graphics::RichText& UIRichText::getRichText() {
	return mRichText;
}

void UIRichText::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UIWidget::draw();

		if ( mRichText.getSize().getWidth() > 0.f ) {
			if ( isClipped() ) {
				clipSmartEnable( mScreenPos.x + mPaddingPx.Left, mScreenPos.y + mPaddingPx.Top,
								 mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
								 mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
			}

			mRichText.draw( std::trunc( mScreenPos.x ) + (int)mPaddingPx.Left,
							std::trunc( mScreenPos.y ) + (int)mPaddingPx.Top, Vector2f::One, 0.f,
							getBlendMode() );

			if ( isClipped() )
				clipSmartDisable();
		}
	}
}

bool UIRichText::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
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
		case PropertyId::Color:
			setFontColor( attribute.asColor() );
			break;
		case PropertyId::TextShadowColor:
			setFontShadowColor( attribute.asColor() );
			break;
		case PropertyId::TextShadowOffset:
			setFontShadowOffset( attribute.asVector2f() );
			break;
		case PropertyId::TextStrokeWidth:
			setOutlineThickness( lengthFromValue( attribute ) );
			break;
		case PropertyId::TextStrokeColor:
			setOutlineColor( attribute.asColor() );
			break;
		case PropertyId::TextAlign: {
			std::string align = String::toLower( attribute.value() );
			if ( align == "center" )
				setTextAlign( TEXT_ALIGN_CENTER );
			else if ( align == "left" )
				setTextAlign( TEXT_ALIGN_LEFT );
			else if ( align == "right" )
				setTextAlign( TEXT_ALIGN_RIGHT );
			break;
		}
		default:
			return UILayout::applyProperty( attribute );
	}

	return true;
}

std::string UIRichText::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
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
		case PropertyId::TextAlign:
			return getTextAlign() == TEXT_ALIGN_CENTER
					   ? "center"
					   : ( getTextAlign() == TEXT_ALIGN_RIGHT ? "right" : "left" );
		default:
			return UILayout::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIRichText::getPropertiesImplemented() const {
	auto props = UILayout::getPropertiesImplemented();
	auto local = {
		PropertyId::FontFamily,		 PropertyId::FontSize,		  PropertyId::FontStyle,
		PropertyId::Color,			 PropertyId::TextShadowColor, PropertyId::TextShadowOffset,
		PropertyId::TextStrokeWidth, PropertyId::TextStrokeColor, PropertyId::TextAlign };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

Graphics::Font* UIRichText::getFont() const {
	return mRichText.getFontStyleConfig().Font;
}

UIRichText* UIRichText::setFont( Graphics::Font* font ) {
	if ( NULL != font && mRichText.getFontStyleConfig().Font != font ) {
		mRichText.getFontStyleConfig().Font = font;
		mRichText.invalidate();
		setLayoutDirty();
		updateDefaultSpansStyle();
	}
	return this;
}

Uint32 UIRichText::getFontSize() const {
	return mRichText.getFontStyleConfig().CharacterSize;
}

UIRichText* UIRichText::setFontSize( const Uint32& characterSize ) {
	if ( mRichText.getFontStyleConfig().CharacterSize != characterSize ) {
		mRichText.getFontStyleConfig().CharacterSize = characterSize;
		mRichText.invalidate();
		setLayoutDirty();
		updateDefaultSpansStyle();
	}
	return this;
}

const Uint32& UIRichText::getFontStyle() const {
	return mRichText.getFontStyleConfig().Style;
}

UIRichText* UIRichText::setFontStyle( const Uint32& fontStyle ) {
	if ( mRichText.getFontStyleConfig().Style != fontStyle ) {
		mRichText.getFontStyleConfig().Style = fontStyle;
		mRichText.invalidate();
		setLayoutDirty();
		updateDefaultSpansStyle();
	}
	return this;
}

const Color& UIRichText::getFontColor() const {
	return mRichText.getFontStyleConfig().FontColor;
}

UIRichText* UIRichText::setFontColor( const Color& color ) {
	if ( mRichText.getFontStyleConfig().FontColor != color ) {
		mRichText.getFontStyleConfig().FontColor = color;
		mRichText.invalidate();
		updateDefaultSpansStyle();
	}
	return this;
}

const Color& UIRichText::getFontShadowColor() const {
	return mRichText.getFontStyleConfig().ShadowColor;
}

UIRichText* UIRichText::setFontShadowColor( const Color& color ) {
	if ( mRichText.getFontStyleConfig().ShadowColor != color ) {
		mRichText.getFontStyleConfig().ShadowColor = color;
		if ( mRichText.getFontStyleConfig().ShadowColor != Color::Transparent )
			mRichText.getFontStyleConfig().Style |= Graphics::Text::Shadow;
		else
			mRichText.getFontStyleConfig().Style &= ~Graphics::Text::Shadow;
		mRichText.invalidate();
		updateDefaultSpansStyle();
	}
	return this;
}

const Vector2f& UIRichText::getFontShadowOffset() const {
	return mRichText.getFontStyleConfig().ShadowOffset;
}

UIRichText* UIRichText::setFontShadowOffset( const Vector2f& offset ) {
	if ( mRichText.getFontStyleConfig().ShadowOffset != offset ) {
		mRichText.getFontStyleConfig().ShadowOffset = offset;
		mRichText.invalidate();
		updateDefaultSpansStyle();
	}
	return this;
}

const Float& UIRichText::getOutlineThickness() const {
	return mRichText.getFontStyleConfig().OutlineThickness;
}

UIRichText* UIRichText::setOutlineThickness( const Float& outlineThickness ) {
	if ( mRichText.getFontStyleConfig().OutlineThickness != outlineThickness ) {
		mRichText.getFontStyleConfig().OutlineThickness = outlineThickness;
		mRichText.invalidate();
		setLayoutDirty();
		updateDefaultSpansStyle();
	}
	return this;
}

const Color& UIRichText::getOutlineColor() const {
	return mRichText.getFontStyleConfig().OutlineColor;
}

UIRichText* UIRichText::setOutlineColor( const Color& outlineColor ) {
	if ( mRichText.getFontStyleConfig().OutlineColor != outlineColor ) {
		mRichText.getFontStyleConfig().OutlineColor = outlineColor;
		mRichText.invalidate();
		updateDefaultSpansStyle();
	}
	return this;
}

Uint32 UIRichText::getTextAlign() const {
	return mRichText.getAlign();
}

UIRichText* UIRichText::setTextAlign( const Uint32& align ) {
	if ( mRichText.getAlign() != align ) {
		mRichText.setAlign( align );
		setLayoutDirty();
	}
	return this;
}

void UIRichText::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	UIWidget::loadFromXmlNode( node );

	auto collapseXmlWhitespace = []( const String& text ) -> String {
		String res;
		res.reserve( text.size() );
		bool inSpace = false;
		for ( size_t i = 0; i < text.size(); ++i ) {
			if ( text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r' ||
				 text[i] == '\v' ) {
				if ( !inSpace ) {
					res += ' ';
					inSpace = true;
				}
			} else {
				res += text[i];
				inSpace = false;
			}
		}
		return res;
	};

	for ( pugi::xml_node child = node.first_child(); child; child = child.next_sibling() ) {
		if ( child.type() == pugi::node_element ) {
			if ( String::iequals( child.name(), "span" ) ||
				 String::iequals( child.name(), "textspan" ) ) {
				UITextSpan* span = UITextSpan::New();
				span->setParent( this );
				span->loadFromXmlNode( child );
			} else {
				// Let parent logic load standard child widget
				UIWidget* widget = UIWidgetCreator::createFromName( child.name() );
				if ( widget ) {
					widget->setParent( this );
					widget->loadFromXmlNode( child );
				}
			}
		} else if ( child.type() == pugi::node_pcdata ) {
			String text = collapseXmlWhitespace( getTranslatorString( child.value() ) );
			if ( !text.empty() ) {
				UITextSpan* span = UITextSpan::New();
				span->setParent( this );
				span->setInheritedStyle( mRichText.getFontStyleConfig() );
				span->setText( text );
			}
		}
	}

	endAttributesTransaction();
	setLayoutDirty();
}

void UIRichText::onSizeChange() {
	UILayout::onSizeChange();
	setLayoutDirty(); // Re-wrap if size changes
}

void UIRichText::onPaddingChange() {
	UILayout::onPaddingChange();
	setLayoutDirty();
}

void UIRichText::onChildCountChange( Node* child, const bool& removed ) {
	UILayout::onChildCountChange( child, removed );
	if ( !removed && child->isWidget() && child->isType( UI_TYPE_TEXTSPAN ) ) {
		static_cast<UITextSpan*>( child )->setInheritedStyle( mRichText.getFontStyleConfig() );
	}
	setLayoutDirty();
}

void UIRichText::onFontChanged() {
	setLayoutDirty();
}

void UIRichText::onFontStyleChanged() {
	setLayoutDirty();
}

void UIRichText::onAlphaChange() {
	UILayout::onAlphaChange();
}

void UIRichText::rebuildRichText() {
	mRichText.clear();

	// Calculate maximum layout width for the RichText block
	Float maxWidth = mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right;
	if ( maxWidth < 0 )
		maxWidth = 0;
	if ( mWidthPolicy == SizePolicy::WrapContent ) {
		mRichText.setMaxWidth( 0.f ); // Let it grow unbounded to query text bounds later
	} else {
		mRichText.setMaxWidth( maxWidth );
	}

	Node* child = mChild;
	while ( NULL != child ) {
		if ( child->isWidget() ) {
			UIWidget* widget = static_cast<UIWidget*>( child );
			if ( widget->isType( UI_TYPE_TEXTSPAN ) ) {
				UITextSpan* span = static_cast<UITextSpan*>( widget );
				mRichText.addSpan( span->getText(), span->getFontStyleConfig() );
			} else {
				mRichText.addCustomSize( widget->getPixelsSize() );
			}
		}
		child = child->getNextNode();
	}
}

void UIRichText::positionChildren() {
	const auto& lines = mRichText.getLines();

	Node* child = mChild;

	size_t currentLine = 0;
	size_t currentSpan = 0;

	// Helper to find the next RenderSpan of type CustomSize
	auto getNextCustomSpan = [&]() -> const Graphics::RichText::RenderSpan* {
		while ( currentLine < lines.size() ) {
			const auto& line = lines[currentLine];
			while ( currentSpan < line.spans.size() ) {
				const auto& span = line.spans[currentSpan];
				currentSpan++;
				if ( span.block.type == Graphics::RichText::BlockType::CustomSize ) {
					return &span;
				}
			}
			currentSpan = 0;
			currentLine++;
		}
		return nullptr;
	};

	while ( NULL != child ) {
		if ( child->isWidget() ) {
			UIWidget* widget = static_cast<UIWidget*>( child );
			if ( !widget->isType( UI_TYPE_TEXTSPAN ) ) {
				const auto* span = getNextCustomSpan();
				if ( span ) {
					size_t lineIdx = currentSpan > 0 ? currentLine : currentLine - 1;
					Float lineY = lines[lineIdx].y;

					widget->setPixelsPosition( mPaddingPx.Left + span->position.x,
											   mPaddingPx.Top + lineY + span->position.y );
				}
			}
		}
		child = child->getNextNode();
	}
}

void UIRichText::updateDefaultSpansStyle() {
	Node* child = mChild;
	while ( NULL != child ) {
		if ( child->isWidget() && child->isType( UI_TYPE_TEXTSPAN ) ) {
			static_cast<UITextSpan*>( child )->setInheritedStyle( mRichText.getFontStyleConfig() );
		}
		child = child->getNextNode();
	}
}

void UIRichText::onLayoutUpdate() {
	rebuildRichText();

	mRichText.getSize(); // Forces an updateLayout internally

	positionChildren();

	// Resize logic
	if ( mWidthPolicy == SizePolicy::WrapContent ) {
		setInternalPixelsWidth( mRichText.getSize().getWidth() + mPaddingPx.Left +
								mPaddingPx.Right );
	}
	if ( mHeightPolicy == SizePolicy::WrapContent ) {
		setInternalPixelsHeight( mRichText.getSize().getHeight() + mPaddingPx.Top +
								 mPaddingPx.Bottom );
	}

	UILayout::onLayoutUpdate();
}

}} // namespace EE::UI
