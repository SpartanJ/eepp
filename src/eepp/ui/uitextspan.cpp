#include <eepp/core/debug.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiborderdrawable.hpp>
#include <eepp/ui/uicheckbox.hpp>
#include <eepp/ui/uihtmlinput.hpp>
#include <eepp/ui/uiradiobutton.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UITextSpan* UITextSpan::New() {
	return eeNew( UITextSpan, () );
}

UITextSpan* UITextSpan::NewWithTag( const std::string& tag ) {
	return eeNew( UITextSpan, ( tag ) );
}

UITextSpan::UITextSpan( const std::string& tag ) : UIRichText( tag ) {
	mDisplay = CSSDisplay::Inline;
	setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
	mFlags &= ~UI_OWNS_CHILDREN_POSITION;
	mFlags |= UI_VALIGN_CENTER | UI_HALIGN_LEFT | UI_LOADS_ITS_CHILDREN;

	if ( NULL == mRichText.getFontStyleConfig().Font ) {
		UISceneNode* sceneNode =
			getUISceneNode() ? getUISceneNode() : SceneManager::instance()->getUISceneNode();
		UITheme* theme = sceneNode ? sceneNode->getUIThemeManager()->getDefaultTheme() : nullptr;

		if ( NULL != theme && NULL != theme->getDefaultFont() ) {
			mRichText.getFontStyleConfig().Font = theme->getDefaultFont();
		} else if ( sceneNode && NULL != sceneNode->getUIThemeManager()->getDefaultFont() ) {
			mRichText.getFontStyleConfig().Font = sceneNode->getUIThemeManager()->getDefaultFont();
		}

		if ( NULL != theme ) {
			mRichText.getFontStyleConfig().CharacterSize = theme->getDefaultFontSize();
		} else if ( sceneNode ) {
			mRichText.getFontStyleConfig().CharacterSize =
				sceneNode->getUIThemeManager()->getDefaultFontSize();
		}
	}
}

UITextSpan::~UITextSpan() {}

Uint32 UITextSpan::getType() const {
	return UI_TYPE_TEXTSPAN;
}

bool UITextSpan::isType( const Uint32& type ) const {
	return UITextSpan::getType() == type ? true : UIRichText::isType( type );
}

bool UITextSpan::isInline() const {
	return mDisplay == CSSDisplay::Inline && getCSSFloat() == CSSFloat::None && !isOutOfFlow();
}

bool UITextSpan::isInlineBlock() const {
	return mDisplay == CSSDisplay::InlineBlock && !isOutOfFlow();
}

void UITextSpan::onDisplayChange() {
	bool nowInline = isInline();
	if ( nowInline && getFontBackgroundColor() == Color::Transparent ) {
		const Color& widgetBg = getBackgroundColor();
		if ( widgetBg != Color::Transparent ) {
			setFontBackgroundColor( widgetBg );
			setBackgroundColor( Color::Transparent );
		}
	} else if ( !nowInline && getBackgroundColor() == Color::Transparent ) {
		const Color& fontBg = getFontBackgroundColor();
		if ( fontBg != Color::Transparent ) {
			setBackgroundColor( fontBg );
			setFontBackgroundColor( Color::Transparent );
		}
	}
	if ( !nowInline )
		mHitBoxes.clear();
	UIHTMLWidget::onDisplayChange();
}

void UITextSpan::draw() {
	bool parentIsBlockFlexOrGrid = false;
	if ( getParent() && getParent()->isType( UI_TYPE_HTML_WIDGET ) ) {
		CSSDisplay parentDisplay = getParent()->asType<UIHTMLWidget>()->getDisplay();
		parentIsBlockFlexOrGrid =
			parentDisplay == CSSDisplay::Flex || parentDisplay == CSSDisplay::Grid;
	}

	// When a UITextSpan is an item in a block-level flex/grid container, it is laid out
	// independently by the container (blockification per CSS Flexbox §4 / CSS Grid §6). In that
	// case the parent container must not render its text via rebuildRichText(), so the span must
	// draw itself. This must stay in sync with UIRichText::draw() and
	// UIRichText::rebuildRichText(): block-level parent flex/grid containers skip their own text
	// stream, and item spans are the single paint owner for their text.
	//
	// Do not use UIHTMLWidget::isFlex()/isGrid() for the parent test. Those include inline-flex
	// and inline-grid, which are atomic inline-level boxes and do not use this self-paint ownership
	// rule.
	if ( !isInline() || parentIsBlockFlexOrGrid ) {
		UIRichText::draw();
	}
}

bool UITextSpan::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
			setText( getTranslatorString( attribute.value() ) );
			break;
		case PropertyId::Color: {
			Color color = attribute.asColor();
			if ( color == Color::Transparent &&
				 attribute.getValue().find( "var(" ) != std::string::npos ) {
#ifdef EE_DEBUG
				eePRINTL( "UITextSpan: unresolved var() in Color value for <%s>: '%s'",
						  getElementTag().c_str(), attribute.getValue().c_str() );
#endif
				break;
			}
			setFontColor( color );
			break;
		}
		case PropertyId::BackgroundColor:
			if ( isInline() ) {
				setBackgroundColor( Color::Transparent );
				setFontBackgroundColor( attribute.asColor() );
			} else {
				setFontBackgroundColor( Color::Transparent );
				setBackgroundColor( attribute.asColor() );
			}
			break;
		case PropertyId::TextShadowColor:
			setFontShadowColor( attribute.asColor() );
			break;
		case PropertyId::TextShadowOffset:
			setFontShadowOffset( attribute.asVector2f() );
			break;
		case PropertyId::FontFamily: {
			Font* font =
				getUISceneNode()->getFontFromNamesList( attribute.value(), getFontStyle() );
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
		case PropertyId::FontWeight:
			setFontWeight( Text::stringToFontWeight( attribute.value() ) );
			break;
		case PropertyId::TextStrokeWidth:
			setOutlineThickness( lengthFromValue( attribute ) );
			break;
		case PropertyId::TextStrokeColor:
			setOutlineColor( attribute.asColor() );
			break;
		case PropertyId::TextDecoration:
			setTextDecoration( attribute.asTextDecoration() );
			break;
		case PropertyId::TextTransform:
			setTextTransform( TextTransform::fromString( attribute.asString() ) );
			break;
		default:
			return UIRichText::applyProperty( attribute );
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
			return String::fromFloat( PixelDensity::pxToDp( getFontSize() ), "dp" );
		case PropertyId::FontStyle:
			return Text::styleFlagToString( getFontStyle() );
		case PropertyId::FontWeight:
			return Text::fontWeightToString( mRichText.getFontStyleConfig().Weight );
		case PropertyId::Color:
			return getFontColor().toHexString();
		case PropertyId::BackgroundColor:
			return getFontBackgroundColor().toHexString();
		case PropertyId::TextShadowColor:
			return getFontShadowColor().toHexString();
		case PropertyId::TextShadowOffset:
			return String::fromFloat( getFontShadowOffset().x ) + " " +
				   String::fromFloat( getFontShadowOffset().y );
		case PropertyId::TextStrokeWidth:
			return String::fromFloat( PixelDensity::dpToPx( getOutlineThickness() ), "px" );
		case PropertyId::TextStrokeColor:
			return getOutlineColor().toHexString();
		case PropertyId::TextDecoration:
			return Text::styleFlagToString( getTextDecoration() );
		case PropertyId::TextTransform:
			return TextTransform::toString( getTextTransform() );
		default:
			return UIRichText::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UITextSpan::getPropertiesImplemented() const {
	auto props = UIRichText::getPropertiesImplemented();
	auto local = { PropertyId::Text,
				   PropertyId::FontFamily,
				   PropertyId::FontSize,
				   PropertyId::FontStyle,
				   PropertyId::Color,
				   PropertyId::BackgroundColor,
				   PropertyId::TextShadowColor,
				   PropertyId::TextShadowOffset,
				   PropertyId::TextStrokeWidth,
				   PropertyId::TextStrokeColor,
				   PropertyId::TextDecoration,
				   PropertyId::TextTransform };
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
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		invalidateDraw();
	}
	return this;
}

const FontStyleConfig& UITextSpan::getFontStyleConfig() const {
	return mRichText.getFontStyleConfig();
}

void UITextSpan::setFontStyleConfig( const UIFontStyleConfig& fontStyleConfig ) {
	mRichText.getFontStyleConfig() = fontStyleConfig;
	mStyleState = StyleStateAll;
	mRichText.invalidate();
	onFontChanged();
	onFontStyleChanged();
	notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
}

Font* UITextSpan::getFont() const {
	return mRichText.getFontStyleConfig().getFont();
}

UITextSpan* UITextSpan::setFont( Font* font ) {
	if ( mRichText.getFontStyleConfig().Font != font || !( mStyleState & StyleStateFont ) ) {
		mRichText.getFontStyleConfig().Font = font;
		mStyleState |= StyleStateFont;
		mRichText.invalidate();
		onFontChanged();
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
	}
	return this;
}

Uint32 UITextSpan::getFontSize() const {
	return mRichText.getFontStyleConfig().getFontCharacterSize();
}

UITextSpan* UITextSpan::setFontSize( const Uint32& characterSize ) {
	if ( mRichText.getFontStyleConfig().CharacterSize != characterSize ||
		 !( mStyleState & StyleStateFontSize ) ) {
		if ( characterSize == 0 )
			return this;
		mRichText.getFontStyleConfig().CharacterSize = characterSize;
		mStyleState |= StyleStateFontSize;
		mRichText.invalidate();
		onFontStyleChanged();
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		invalidateDraw();
	}
	return this;
}

const Uint32& UITextSpan::getFontStyle() const {
	return mRichText.getFontStyleConfig().getFontStyle();
}

UITextSpan* UITextSpan::setFontStyle( const Uint32& fontStyle ) {
	if ( mRichText.getFontStyleConfig().Style != fontStyle ) {
		mRichText.getFontStyleConfig().Style = fontStyle;
		mStyleState |= StyleStateFontStyle;
		mRichText.invalidate();
		onFontStyleChanged();
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		invalidateDraw();

		if ( auto* newFont = getUISceneNode()->reevaluateFontStyle(
				 mRichText.getFontStyleConfig().getFont(), fontStyle,
				 ( fontStyle & Text::Bold ) ? FontWeight::Bold : FontWeight::Normal ) )
			setFont( newFont );
	}
	return this;
}

FontWeight UITextSpan::getFontWeight() const {
	return mRichText.getFontStyleConfig().Weight;
}

UITextSpan* UITextSpan::setFontWeight( const FontWeight& weight ) {
	mRichText.getFontStyleConfig().Weight = weight;

	Uint32 weightStyle = ( weight >= FontWeight::SemiBold ) ? Text::Bold : 0;
	Uint32 oldStyle = mRichText.getFontStyleConfig().Style;
	Uint32 newStyle = ( oldStyle & ~Text::Bold ) | weightStyle;

	if ( oldStyle != newStyle ) {
		mRichText.getFontStyleConfig().Style = newStyle;
		mStyleState |= StyleStateFontStyle;
		mRichText.invalidate();
		onFontStyleChanged();
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		invalidateDraw();

		if ( auto* newFont = getUISceneNode()->reevaluateFontStyle(
				 mRichText.getFontStyleConfig().getFont(), newStyle, weight ) )
			setFont( newFont );
	} else if ( weight != FontWeight::Normal ) {
		invalidateDraw();
	}

	return this;
}

Uint32 UITextSpan::getTextDecoration() const {
	Uint32 flags = mRichText.getFontStyleConfig().Style;
	flags &= ~( Text::Style::Bold | Text::Style::Italic | Text::Style::Shadow );
	return flags;
}

UITextSpan* UITextSpan::setTextDecoration( const Uint32& textDecoration ) {
	if ( mRichText.getFontStyleConfig().Style != textDecoration ) {
		mRichText.getFontStyleConfig().Style &= ~( Text::Underlined | Text::StrikeThrough );
		mRichText.getFontStyleConfig().Style |= textDecoration;
		mStyleState |= StyleStateFontStyle;
		mRichText.invalidate();
		onFontStyleChanged();
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		invalidateDraw();
	}
	return this;
}

const Float& UITextSpan::getOutlineThickness() const {
	return mRichText.getFontStyleConfig().getOutlineThickness();
}

UITextSpan* UITextSpan::setOutlineThickness( const Float& outlineThickness ) {
	if ( mRichText.getFontStyleConfig().OutlineThickness != outlineThickness ||
		 !( mStyleState & StyleStateOutlineThickness ) ) {
		mRichText.getFontStyleConfig().OutlineThickness = outlineThickness;
		mStyleState |= StyleStateOutlineThickness;
		mRichText.invalidate();
		onFontStyleChanged();
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		invalidateDraw();
	}
	return this;
}

const Color& UITextSpan::getOutlineColor() const {
	return mRichText.getFontStyleConfig().getOutlineColor();
}

UITextSpan* UITextSpan::setOutlineColor( const Color& outlineColor ) {
	if ( mRichText.getFontStyleConfig().OutlineColor != outlineColor ||
		 !( mStyleState & StyleStateOutlineColor ) ) {
		mRichText.getFontStyleConfig().OutlineColor = outlineColor;
		mStyleState |= StyleStateOutlineColor;
		mRichText.invalidate();
		onFontStyleChanged();
		invalidateDraw();
	}
	return this;
}

const Color& UITextSpan::getFontColor() const {
	return mRichText.getFontStyleConfig().getFontColor();
}

UITextSpan* UITextSpan::setFontColor( const Color& color ) {
	if ( mRichText.getFontStyleConfig().FontColor != color ||
		 !( mStyleState & StyleStateFontColor ) ) {
		mRichText.getFontStyleConfig().FontColor = color;
		mStyleState |= StyleStateFontColor;
		mRichText.invalidate();
		onFontStyleChanged();
		invalidateDraw();
	}
	return this;
}

const Color& UITextSpan::getFontBackgroundColor() const {
	return mRichText.getFontStyleConfig().getBackgroundColor();
}

UITextSpan* UITextSpan::setFontBackgroundColor( const Color& color ) {
	if ( mRichText.getFontStyleConfig().BackgroundColor != color ) {
		mRichText.getFontStyleConfig().BackgroundColor = color;
		mStyleState |= StyleStateFontBackgroundColor;
		mRichText.invalidate();
		onFontStyleChanged();
		invalidateDraw();
	}
	return this;
}

const Color& UITextSpan::getFontShadowColor() const {
	return mRichText.getFontStyleConfig().getFontShadowColor();
}

UITextSpan* UITextSpan::setFontShadowColor( const Color& color ) {
	if ( mRichText.getFontStyleConfig().ShadowColor != color ) {
		mRichText.getFontStyleConfig().ShadowColor = color;
		if ( color != Color::Transparent )
			mRichText.getFontStyleConfig().Style |= Text::Shadow;
		else
			mRichText.getFontStyleConfig().Style &= ~Text::Shadow;
		mStyleState |= StyleStateFontShadowColor;
		mRichText.invalidate();
		onFontStyleChanged();
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		invalidateDraw();
	}
	return this;
}

const Vector2f& UITextSpan::getFontShadowOffset() const {
	return mRichText.getFontStyleConfig().getFontShadowOffset();
}

UITextSpan* UITextSpan::setFontShadowOffset( const Vector2f& offset ) {
	if ( mRichText.getFontStyleConfig().ShadowOffset != offset ) {
		mRichText.getFontStyleConfig().ShadowOffset = offset;
		mStyleState |= StyleStateFontShadowOffset;
		mRichText.invalidate();
		onFontStyleChanged();
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		invalidateDraw();
	}
	return this;
}

void UITextSpan::onFontChanged() {
	sendCommonEvent( Event::OnFontChanged );
	notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
}

void UITextSpan::onFontStyleChanged() {
	sendCommonEvent( Event::OnFontStyleChanged );
	notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
}

void UITextSpan::onTextChanged() {
	sendCommonEvent( Event::OnTextChanged );
	sendCommonEvent( Event::OnValueChange );
	notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
}

Uint32 UITextSpan::onMessage( const NodeMessage* Msg ) {
	if ( !isInline() )
		return UIRichText::onMessage( Msg );

	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
			return 1;
		}
	}
	return UIHTMLWidget::onMessage( Msg );
}

void UITextSpan::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	UIHTMLWidget::loadFromXmlNode( node );

	bool hasElements = false;
	for ( pugi::xml_node child = node.first_child(); child; child = child.next_sibling() ) {
		if ( child.type() == pugi::node_element ) {
			hasElements = true;
			break;
		}
	}

	if ( hasElements ) {
		for ( pugi::xml_node child = node.first_child(); child; child = child.next_sibling() ) {
			if ( child.type() == pugi::node_element ) {
				UIWidget* widget = UIWidgetCreator::createFromName( child.name() );

				if ( widget == nullptr )
					widget = UITextSpan::NewWithTag( child.name() );

				if ( widget ) {
					widget->setParent( this );
					widget->loadFromXmlNode( child );
				}
			} else if ( child.type() == pugi::node_pcdata ) {
				UITextNode* textNode = UITextNode::New();
				textNode->setParent( this );
				textNode->setText( child.value() );
			}
		}
	} else {
		for ( pugi::xml_node child = node.first_child(); child; child = child.next_sibling() ) {
			if ( child.type() == pugi::node_pcdata ) {
				mText += child.value();
			}
		}
	}

	endAttributesTransaction();
}

void UITextSpan::setInheritedStyle( const FontStyleConfig& fontStyleConfig ) {
	bool fontChanged = false;
	bool fontStyleChanged = false;

	if ( !hasFont() && mRichText.getFontStyleConfig().Font != fontStyleConfig.Font ) {
		mRichText.getFontStyleConfig().Font = fontStyleConfig.Font;
		fontChanged = true;
	}

	if ( !hasFontSize() &&
		 mRichText.getFontStyleConfig().CharacterSize != fontStyleConfig.CharacterSize ) {
		mRichText.getFontStyleConfig().CharacterSize = fontStyleConfig.CharacterSize;
		fontStyleChanged = true;
	}

	if ( !hasFontStyle() && mRichText.getFontStyleConfig().Style != fontStyleConfig.Style ) {
		mRichText.getFontStyleConfig().Style = fontStyleConfig.Style;
		fontStyleChanged = true;
	}

	if ( !hasFontColor() &&
		 mRichText.getFontStyleConfig().FontColor != fontStyleConfig.FontColor ) {
		mRichText.getFontStyleConfig().FontColor = fontStyleConfig.FontColor;
		fontStyleChanged = true;
	}

	if ( !hasOutlineThickness() &&
		 mRichText.getFontStyleConfig().OutlineThickness != fontStyleConfig.OutlineThickness ) {
		mRichText.getFontStyleConfig().OutlineThickness = fontStyleConfig.OutlineThickness;
		fontStyleChanged = true;
	}

	if ( !hasOutlineColor() &&
		 mRichText.getFontStyleConfig().OutlineColor != fontStyleConfig.OutlineColor ) {
		mRichText.getFontStyleConfig().OutlineColor = fontStyleConfig.OutlineColor;
		fontStyleChanged = true;
	}

	if ( !hasFontShadowColor() &&
		 mRichText.getFontStyleConfig().ShadowColor != fontStyleConfig.ShadowColor ) {
		mRichText.getFontStyleConfig().ShadowColor = fontStyleConfig.ShadowColor;
		fontStyleChanged = true;
	}

	if ( !hasFontShadowOffset() &&
		 mRichText.getFontStyleConfig().ShadowOffset != fontStyleConfig.ShadowOffset ) {
		mRichText.getFontStyleConfig().ShadowOffset = fontStyleConfig.ShadowOffset;
		fontStyleChanged = true;
	}

	if ( !hasFontBackgroundColor() &&
		 mRichText.getFontStyleConfig().BackgroundColor != fontStyleConfig.BackgroundColor ) {
		mRichText.getFontStyleConfig().BackgroundColor = fontStyleConfig.BackgroundColor;
		fontStyleChanged = true;
	}

	if ( fontChanged || fontStyleChanged )
		mRichText.invalidate();

	if ( fontChanged )
		onFontChanged();

	if ( fontStyleChanged )
		onFontStyleChanged();

	if ( fontChanged || fontStyleChanged )
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );

	Node* child = mChild;
	while ( NULL != child ) {
		if ( child->isWidget() && child->isType( UI_TYPE_TEXTSPAN ) ) {
			static_cast<UITextSpan*>( child )->setInheritedStyle( mRichText.getFontStyleConfig() );
		}
		child = child->getNextNode();
	}
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

bool UITextSpan::hasFontBackgroundColor() const {
	return 0 != ( mStyleState & StyleStateFontBackgroundColor );
}

bool UITextSpan::hasTextTransform() const {
	return 0 != ( mStyleState & StyleStateTextTransform );
}

SpanHitBoxes& UITextSpan::getHitBoxes() {
	return mHitBoxes;
}

const SpanHitBoxes& UITextSpan::getHitBoxes() const {
	return mHitBoxes;
}

void UITextSpan::setHitBoxes( SpanHitBoxes&& hitBoxes ) {
	mHitBoxes = std::move( hitBoxes );
}

Node* UITextSpan::overFind( const Vector2f& point ) {
	Node* pOver = NULL;
	if ( ( mNodeFlags & NODE_FLAG_OVER_FIND_ALLOWED ) && mEnabled && mVisible ) {
		updateWorldPolygon();
		if ( mWorldBounds.contains( point ) && mPoly.pointInside( point ) ) {
			bool hit = false;
			if ( !mHitBoxes.empty() ) {
				Vector2f localPoint = convertToNodeSpace( point );
				for ( const auto& rect : mHitBoxes ) {
					if ( rect.contains( localPoint ) ) {
						hit = true;
						break;
					}
				}
			} else {
				hit = true;
			}

			if ( hit ) {
				writeNodeFlag( NODE_FLAG_MOUSEOVER_ME_OR_CHILD, 1 );
				mSceneNode->addMouseOverNode( this );

				Node* child = mChildLast;

				while ( NULL != child ) {
					Node* childOver = child->overFind( point );

					if ( NULL != childOver ) {
						pOver = childOver;
						break;
					}

					child = child->getPrevNode();
				}

				if ( NULL == pOver )
					pOver = this;
			}
		}
	}

	return pOver;
}

UIAnchorSpan* UIAnchorSpan::New() {
	return eeNew( UIAnchorSpan, () );
}

UIAnchorSpan::UIAnchorSpan( const std::string& tag ) : UITextSpan( tag ) {
	mPseudoClasses |= StyleSheetSelectorRule::PseudoClasses::Link;
	mState |= UIState::StateFlagLink;
}

Uint32 UIAnchorSpan::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseClick: {
			if ( !mHref.empty() && ( Msg->getFlags() & EE_BUTTON_LMASK ) )
				getUISceneNode()->openURL( mHref );
			return 1;
		}
	}
	return UITextSpan::onMessage( Msg );
}

bool UIAnchorSpan::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Target: {
			mTarget = attribute.value();
			break;
		}
		case PropertyId::Href:
			setHref( attribute.asString() );
			break;
		default:
			UITextSpan::applyProperty( attribute );
			break;
	}

	return true;
}

void UIAnchorSpan::setHref( const std::string& href ) {
	if ( href != mHref ) {
		mHref = href;
	}
}

const std::string& UIAnchorSpan::getHref() const {
	return mHref;
}

Uint32 UIAnchorSpan::onKeyDown( const KeyEvent& event ) {
	if ( event.getKeyCode() == KEY_KP_ENTER || event.getKeyCode() == KEY_RETURN ) {
		if ( !mHref.empty() ) {
			getUISceneNode()->openURL( mHref );
			return 1;
		}
	}

	return UITextSpan::onKeyDown( event );
}

std::string UIAnchorSpan::getPropertyString( const PropertyDefinition* propertyDef,
											 const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Target:
			return mTarget;
		case PropertyId::Href:
			return mHref;
		default:
			return UITextSpan::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIAnchorSpan::getPropertiesImplemented() const {
	auto props = UITextSpan::getPropertiesImplemented();
	auto local = { PropertyId::Href, PropertyId::Target };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

UILabelSpan* UILabelSpan::New() {
	return eeNew( UILabelSpan, () );
}

UILabelSpan::UILabelSpan( const std::string& tag ) : UITextSpan( tag ) {
	mPseudoClasses |= StyleSheetSelectorRule::PseudoClasses::Link;
	mState |= UIState::StateFlagLink;
}

Uint32 UILabelSpan::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseClick: {
			if ( !mFor.empty() && ( Msg->getFlags() & EE_BUTTON_LMASK ) )
				activateTarget();
			return 1;
		}
	}
	return UITextSpan::onMessage( Msg );
}

Uint32 UILabelSpan::onKeyDown( const KeyEvent& event ) {
	if ( event.getKeyCode() == KEY_KP_ENTER || event.getKeyCode() == KEY_RETURN ||
		 event.getKeyCode() == KEY_SPACE ) {
		if ( !mFor.empty() ) {
			activateTarget();
			return 1;
		}
	}

	return UITextSpan::onKeyDown( event );
}

bool UILabelSpan::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::For:
			setFor( attribute.value() );
			break;
		default:
			UITextSpan::applyProperty( attribute );
			break;
	}

	return true;
}

void UILabelSpan::setFor( const std::string& forAttr ) {
	if ( forAttr != mFor ) {
		mFor = forAttr;
	}
}

const std::string& UILabelSpan::getFor() const {
	return mFor;
}

std::string UILabelSpan::getPropertyString( const PropertyDefinition* propertyDef,
											const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::For:
			return mFor;
		default:
			return UITextSpan::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UILabelSpan::getPropertiesImplemented() const {
	auto props = UITextSpan::getPropertiesImplemented();
	auto local = { PropertyId::For };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

void UILabelSpan::activateTarget() {
	UISceneNode* sceneNode = getUISceneNode();
	if ( !sceneNode )
		return;

	Node* target = sceneNode->find( mFor );
	if ( !target )
		return;

	if ( !target->isWidget() )
		return;

	UIWidget* widget = static_cast<UIWidget*>( target );

	if ( widget->isType( UI_TYPE_RADIOBUTTON ) ) {
		static_cast<UIRadioButton*>( widget )->setActive( true );
	} else if ( widget->isType( UI_TYPE_CHECKBOX ) ) {
		UICheckBox* cb = static_cast<UICheckBox*>( widget );
		cb->setChecked( !cb->isChecked() );
	} else if ( widget->isType( UI_TYPE_HTML_INPUT ) ) {
		UIWidget* child = static_cast<UIHTMLInput*>( widget )->getChildWidget();
		if ( child ) {
			if ( child->isType( UI_TYPE_RADIOBUTTON ) ) {
				static_cast<UIRadioButton*>( child )->setActive( true );
			} else if ( child->isType( UI_TYPE_CHECKBOX ) ) {
				UICheckBox* cb = static_cast<UICheckBox*>( child );
				cb->setChecked( !cb->isChecked() );
			}
		}
	}
}

}} // namespace EE::UI
