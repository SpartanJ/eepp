#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/tools/htmlformatter.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uilayouter.hpp>
#include <eepp/ui/uirichtext.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextnode.hpp>
#include <eepp/ui/uitextspan.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidgetcreator.hpp>

#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIHTMLHtml* UIHTMLHtml::New( const std::string& tag ) {
	return eeNew( UIHTMLHtml, ( tag ) );
}

UIHTMLHtml::UIHTMLHtml( const std::string& tag ) : UIRichText( tag ) {
	enableReportSizeChangeToChildren();
}

Uint32 UIHTMLHtml::getType() const {
	return UI_TYPE_HTML_HTML;
}

bool UIHTMLHtml::isType( const Uint32& type ) const {
	return UIHTMLHtml::getType() == type ? true : UIRichText::isType( type );
}

bool UIHTMLHtml::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Width:
		case PropertyId::Height:
			return false; // Ignore width and height set from CSS
		default:
			break;
	}

	return UIRichText::applyProperty( attribute );
}

UILineBreak* UILineBreak::New( const std::string& tag ) {
	return eeNew( UILineBreak, ( tag ) );
}

UILineBreak::UILineBreak( const std::string& tag ) : UIRichText( tag ) {}

Uint32 UILineBreak::getType() const {
	return UI_TYPE_BR;
}

bool UILineBreak::isType( const Uint32& type ) const {
	return UILineBreak::getType() == type ? true : UIHTMLWidget::isType( type );
}

UIHTMLBody* UIHTMLBody::New( const std::string& tag ) {
	return eeNew( UIHTMLBody, ( tag ) );
}

UIHTMLBody::UIHTMLBody( const std::string& tag ) : UIRichText( tag ) {
	enableReportSizeChangeToChildren();
}

Uint32 UIHTMLBody::getType() const {
	return UI_TYPE_HTML_BODY;
}

bool UIHTMLBody::isType( const Uint32& type ) const {
	return UIHTMLBody::getType() == type ? true : UIRichText::isType( type );
}

bool UIHTMLBody::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Width:
		case PropertyId::Height:
			return false; // Ignore width and height set from CSS
		case PropertyId::BackgroundColor:
		case PropertyId::BackgroundImage:
		case PropertyId::BackgroundTint:
		case PropertyId::BackgroundPositionX:
		case PropertyId::BackgroundPositionY:
		case PropertyId::BackgroundRepeat:
		case PropertyId::BackgroundSize: {
			if ( getParent() && getParent()->isType( UI_TYPE_HTML_HTML ) ) {
				UIWidget* htmlParent = getParent()->asType<UIWidget>();
				if ( htmlParent->getBackgroundColor() == Color::Transparent ||
					 mPropagatedBackground ) {
					mPropagatedBackground = true;
					htmlParent->applyProperty( attribute );
					return true;
				}
			}
			break;
		}
		default:
			break;
	}

	return UIRichText::applyProperty( attribute );
}

void UIHTMLBody::updateLayout() {
	UIRichText::updateLayout();

	if ( mChild && mChild->isWidget() ) {
		Float maxH = 0;
		Node* child = mChild;
		while ( child ) {
			if ( child->isWidget() ) {
				UIWidget* widget = child->asType<UIWidget>();
				bool isFixed = false;
				if ( widget->isType( UI_TYPE_HTML_WIDGET ) &&
					 widget->asType<UIHTMLWidget>()->getCSSPosition() == CSSPosition::Fixed ) {
					isFixed = true;
				}
				if ( !isFixed ) {
					Float childH =
						widget->getPixelsPosition().y + widget->getPixelsSize().getHeight();
					maxH = std::max( maxH, childH );
				}
			}
			child = child->getNextNode();
		}
		if ( maxH > 0 ) {
			Float dpH = PixelDensity::pxToDp( maxH );
			if ( dpH != getMinSize().getHeight() )
				setMinHeight( dpH );
		}
	}
}

UIRichText* UIRichText::NewHtml() {
	auto* html = UIHTMLHtml::New( "html" );
	html->setClipType( ClipType::None );
	return html;
}

UIRichText* UIRichText::NewBody() {
	auto* body = UIHTMLBody::New( "body" );
	body->setClipType( ClipType::None );
	return body;
}

UIRichText* UIRichText::NewBr() {
	return UILineBreak::New( "br" );
};

static void applyDefaultBlockMargins( UIWidget* widget, const std::string& tag ) {
	static const UnorderedMap<std::string, std::pair<Float, Float>> defaults = {
		{ "h1", { 0.67f, 0.67f } },			{ "h2", { 0.83f, 0.83f } },
		{ "h3", { 1.00f, 1.00f } },			{ "h4", { 1.33f, 1.33f } },
		{ "h5", { 1.67f, 1.67f } },			{ "h6", { 2.33f, 2.33f } },
		{ "p", { 1.00f, 1.00f } },			{ "pre", { 1.00f, 1.00f } },
		{ "blockquote", { 1.00f, 1.00f } }, { "hr", { 0.50f, 0.50f } },
		{ "ul", { 1.00f, 1.00f } },			{ "ol", { 1.00f, 1.00f } },
		{ "dl", { 1.00f, 1.00f } },			{ "body", { 0.67f, 0.67f } },
	};
	auto it = defaults.find( tag );
	if ( it != defaults.end() ) {
		widget->applyProperty(
			StyleSheetProperty( "margin-top", String::format( "%gem", it->second.first ) ) );
		widget->applyProperty(
			StyleSheetProperty( "margin-bottom", String::format( "%gem", it->second.second ) ) );
	}
}

UIRichText* UIRichText::NewHr() {
	auto* w = UILineBreak::New( "hr" );
	applyDefaultBlockMargins( w, "hr" );
	return w;
};

UIRichText* UIRichText::New() {
	return eeNew( UIRichText, () );
}

UIRichText* UIRichText::NewWithTag( const std::string& tag ) {
	auto* w = eeNew( UIRichText, ( tag ) );
	applyDefaultBlockMargins( w, tag );
	return w;
}

UIRichText::UIRichText( const std::string& tag ) : UIHTMLWidget( tag ) {
	mFlags |= UI_HTML_ELEMENT | UI_LOADS_ITS_CHILDREN | UI_OWNS_CHILDREN_POSITION;

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

	setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
}

Uint32 UIRichText::getType() const {
	return UI_TYPE_RICHTEXT;
}

bool UIRichText::isType( const Uint32& type ) const {
	return UIRichText::getType() == type ? true : UIHTMLWidget::isType( type );
}

const RichText& UIRichText::getRichText() {
	return mRichText;
}

void UIRichText::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UIWidget::draw();

		if ( mRichText.getSize().getWidth() > 0.f ) {
			Rectf contentOffset = getPixelsContentOffset();
			if ( isClipped() ) {
				clipSmartEnable( mScreenPos.x + contentOffset.Left,
								 mScreenPos.y + contentOffset.Top,
								 mSize.getWidth() - contentOffset.Left - contentOffset.Right,
								 mSize.getHeight() - contentOffset.Top - contentOffset.Bottom );
			}

			mRichText.draw( std::trunc( mScreenPos.x ) + (int)contentOffset.Left,
							std::trunc( mScreenPos.y ) + (int)contentOffset.Top, Vector2f::One, 0.f,
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
			Font* font = getUISceneNode()->getFontFromNamesList( attribute.value() );
			if ( NULL != font && font->loaded() ) {
				setFont( font );
			}
			break;
		}
		case PropertyId::FontSize:
			setFontSize( lengthFromValue( attribute ) );
			break;
		case PropertyId::TextDecoration:
			setTextDecoration( attribute.asTextDecoration() );
			break;
		case PropertyId::FontStyle:
			setFontStyle( attribute.asFontStyle() );
			break;
		case PropertyId::Color:
			setFontColor( attribute.asColor() );
			break;
		case PropertyId::BackgroundColor:
			setBackgroundColor( attribute.asColor() );
			setFontBackgroundColor( attribute.asColor() );
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
		case PropertyId::SelectionColor:
			setSelectionColor( attribute.asColor() );
			break;
		case PropertyId::SelectionBackColor:
			setSelectionBackColor( attribute.asColor() );
			break;
		case PropertyId::TextSelection:
			setTextSelectionEnabled( attribute.asBool() );
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
		case PropertyId::DataLanguage: {
			if ( mTag == "pre" && mChild && mChild->isType( UI_TYPE_CODEEDITOR ) ) {
				mChild->asType<UICodeEditor>()->applyProperty( attribute );
			} else
				mDataProperties["data-language"] = attribute;
			break;
		}
		default:
			return UIHTMLWidget::applyProperty( attribute );
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
			return String::fromFloat( PixelDensity::pxToDp( getFontSize() ), "dp" );
		case PropertyId::FontStyle:
			return Text::styleFlagToString( getFontStyle() );
		case PropertyId::TextDecoration:
			return Text::styleFlagToString( getTextDecoration() );
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
		case PropertyId::SelectionColor:
			return getSelectionColor().toHexString();
		case PropertyId::SelectionBackColor:
			return getSelectionBackColor().toHexString();
		case PropertyId::TextSelection:
			return isTextSelectionEnabled() ? "true" : "false";
		case PropertyId::TextAlign:
			return getTextAlign() == TEXT_ALIGN_CENTER
					   ? "center"
					   : ( getTextAlign() == TEXT_ALIGN_RIGHT ? "right" : "left" );
		default:
			return UIHTMLWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIRichText::getPropertiesImplemented() const {
	auto props = UIHTMLWidget::getPropertiesImplemented();
	auto local = { PropertyId::FontFamily,		 PropertyId::FontSize,
				   PropertyId::FontStyle,		 PropertyId::Color,
				   PropertyId::BackgroundColor,	 PropertyId::TextShadowColor,
				   PropertyId::TextShadowOffset, PropertyId::TextStrokeWidth,
				   PropertyId::TextStrokeColor,	 PropertyId::TextAlign,
				   PropertyId::SelectionColor,	 PropertyId::SelectionBackColor,
				   PropertyId::TextSelection,	 PropertyId::TextDecoration };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

Font* UIRichText::getFont() const {
	return mRichText.getFontStyleConfig().Font;
}

UIRichText* UIRichText::setFont( Font* font ) {
	if ( NULL != font && mRichText.getFontStyleConfig().Font != font ) {
		mRichText.getFontStyleConfig().Font = font;
		mRichText.invalidate();
		notifyLayoutAttrChange();
		notifyLayoutAttrChangeParent();
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

		notifyLayoutAttrChange();
		notifyLayoutAttrChangeParent();
		updateDefaultSpansStyle();
	}
	return this;
}

const Uint32& UIRichText::getFontStyle() const {
	return mRichText.getFontStyleConfig().Style;
}

Uint32 UIRichText::getTextDecoration() const {
	Uint32 flags = mRichText.getFontStyleConfig().Style;
	flags &= ~( Text::Style::Bold | Text::Style::Italic | Text::Style::Shadow );
	return flags;
}

UIRichText* UIRichText::setTextDecoration( const Uint32& textDecoration ) {
	if ( mRichText.getFontStyleConfig().Style != textDecoration ) {
		mRichText.getFontStyleConfig().Style &= ~( Text::Underlined | Text::StrikeThrough );
		mRichText.getFontStyleConfig().Style |= textDecoration;
		mRichText.invalidate();

		notifyLayoutAttrChange();
		notifyLayoutAttrChangeParent();
		updateDefaultSpansStyle();
	}
	return this;
}

UIRichText* UIRichText::setFontStyle( const Uint32& fontStyle ) {
	if ( mRichText.getFontStyleConfig().Style != fontStyle ) {
		mRichText.getFontStyleConfig().Style = fontStyle;
		mRichText.invalidate();

		notifyLayoutAttrChange();
		notifyLayoutAttrChangeParent();
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

const Color& UIRichText::getFontBackgroundColor() const {
	return mRichText.getFontStyleConfig().BackgroundColor;
}

UIRichText* UIRichText::setFontBackgroundColor( const Color& color ) {
	if ( mRichText.getFontStyleConfig().BackgroundColor != color ) {
		mRichText.getFontStyleConfig().BackgroundColor = color;
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
			mRichText.getFontStyleConfig().Style |= Text::Shadow;
		else
			mRichText.getFontStyleConfig().Style &= ~Text::Shadow;
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

		notifyLayoutAttrChange();
		notifyLayoutAttrChangeParent();
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

		notifyLayoutAttrChange();
		notifyLayoutAttrChangeParent();
	}
	return this;
}

void UIRichText::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	UIWidget::loadFromXmlNode( node );

	for ( pugi::xml_node child = node.first_child(); child; child = child.next_sibling() ) {
		if ( child.type() == pugi::node_element ) {
			if ( mTag == "pre" && String::iequals( child.name(), "code" ) ) {
				// Use a UICodeEditor for <pre><code>
				UICodeEditor* editor = UICodeEditor::NewWithTag( "code" );
				if ( editor ) {
					editor->setParent( this );
					editor->setDynamicTheming( true );
					editor->loadFromXmlNode( child );
					editor->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
					editor->setLineWrapMode( LineWrapMode::Word );
					editor->setLineWrapType( LineWrapType::Viewport );
					editor->disableEditorFeatures( false );
					editor->setCursorVisible( false );
					editor->setAllowSelectingTextFromGutter( false );
					editor->setDisableCursorBlinkingAfterAMinuteOfInactivity( false );
					editor->setCursorBlinkTime( Time::Zero );
					editor->setShowLineNumber( false );
					editor->setShowFoldingRegion( false );
					editor->setLocked( true );

					auto langIt = mDataProperties.find( "data-language" );
					if ( langIt != mDataProperties.end() ) {
						editor->applyProperty( langIt->second );
					}
				}
			} else if ( String::iequals( child.name(), "style" ) ) {
				getUISceneNode()->loadNode( child, this, 0 );
			} else if ( String::iequals( child.name(), "script" ) ) {
				// No plans to support it
				continue;
			} else {
				// Let parent logic load standard child widget
				UIWidget* uiwidget = UIWidgetCreator::createFromName( child.name() );
				if ( uiwidget ) {
					uiwidget->setParent( this );
					uiwidget->loadFromXmlNode( child );

					if ( !uiwidget->loadsItsChildren() ) {
						if ( child.first_child() && !uiwidget->loadsItsChildren() ) {
							getUISceneNode()->loadNode( child.first_child(), uiwidget, 0 );
						}
					}

					uiwidget->onWidgetCreated();
				}
			}
		} else if ( child.type() == pugi::node_pcdata ) {
			String text = Tools::HTMLFormatter::collapseXmlWhitespace( child.value(), child );
			if ( !text.empty() ) {
				UITextNode* span = UITextNode::New();
				span->setParent( this );
				span->setText( text );
			}
		}
	}

	endAttributesTransaction();
}

void UIRichText::onSizeChange() {
	UIHTMLWidget::onSizeChange();
	notifyLayoutAttrChange();
	notifyLayoutAttrChangeParent();
}

void UIRichText::onPaddingChange() {
	UIHTMLWidget::onPaddingChange();
	notifyLayoutAttrChange();
	notifyLayoutAttrChangeParent();
}

void UIRichText::onChildCountChange( Node* child, const bool& removed ) {
	UIHTMLWidget::onChildCountChange( child, removed );
	if ( !removed && child->isWidget() && child->isType( UI_TYPE_TEXTSPAN ) ) {
		static_cast<UITextSpan*>( child )->setInheritedStyle( mRichText.getFontStyleConfig() );
	}

	notifyLayoutAttrChange();
	notifyLayoutAttrChangeParent();
}

void UIRichText::onFontChanged() {
	notifyLayoutAttrChange();
	notifyLayoutAttrChangeParent();
}

void UIRichText::onFontStyleChanged() {
	notifyLayoutAttrChange();
	notifyLayoutAttrChangeParent();
}

void UIRichText::rebuildRichText( UILayout* container, RichText& richText, IntrinsicMode mode ) {
	richText.clear();
	Float maxWidth = 0;
	if ( container->getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
		maxWidth = container->getMatchParentWidth() - container->getPixelsContentOffset().Left -
				   container->getPixelsContentOffset().Right;
	} else {
		maxWidth = container->getPixelsSize().getWidth() -
				   container->getPixelsContentOffset().Left -
				   container->getPixelsContentOffset().Right;
	}

	if ( maxWidth < 0 )
		maxWidth = 0;

	Float mw = 0.f;
	if ( !container->getMaxWidthEq().empty() ) {
		mw = container->getMaxSizePx().getWidth() - container->getPixelsContentOffset().Left -
			 container->getPixelsContentOffset().Right;
		if ( mw < 0 )
			mw = 0.f;
	}

	if ( mode == IntrinsicMode::None ) {
		if ( !container->getMaxWidthEq().empty() && ( maxWidth == 0 || mw < maxWidth ) ) {
			richText.setMaxWidth( mw );
		} else {
			richText.setMaxWidth( maxWidth );
		}
	} else {
		richText.setMaxWidth( 0.f ); // Let it grow unbounded to query text bounds later
	}

	if ( container->isType( UI_TYPE_TEXTSPAN ) ) {
		UITextSpan* selfSpan = container->asType<UITextSpan>();
		if ( !selfSpan->getText().empty() && !selfSpan->isMergeable() &&
			 NULL != selfSpan->getFontStyleConfig().Font ) {
			richText.addSpan( selfSpan->getText(), selfSpan->getFontStyleConfig() );
		}
	}

	auto processNode = [&]( Node* node, auto& processNodeRef ) -> void {
		if ( node->isTextNode() ) {
			UITextNode* textNode = static_cast<UITextNode*>( node );
			if ( !textNode->getText().empty() ) {
				FontStyleConfig style;
				if ( node->getParent()->isType( UI_TYPE_TEXTSPAN ) ) {
					style = node->getParent()->asType<UITextSpan>()->getFontStyleConfig();
				} else if ( node->getParent()->isType( UI_TYPE_RICHTEXT ) ) {
					style =
						node->getParent()->asType<UIRichText>()->getRichText().getFontStyleConfig();
				} else {
					style = richText.getFontStyleConfig();
				}
				richText.addSpan( textNode->getText(), style );
			}
			return;
		}

		if ( !node->isWidget() || !node->isVisible() )
			return;

		UIWidget* widget = node->asType<UIWidget>();

		if ( widget->isType( UI_TYPE_HTML_WIDGET ) &&
			 widget->asType<UIHTMLWidget>()->isMergeable() ) {
			UITextSpan* span = widget->asType<UITextSpan>();
			Rectf margin = span->getLayoutPixelsMargin();
			Rectf padding = span->getPixelsPadding();
			bool hasOwnText = !span->getText().empty() && NULL != span->getFontStyleConfig().Font;

			if ( hasOwnText ) {
				richText.addSpan( span->getText(), span->getFontStyleConfig(), margin, padding );
			} else if ( margin.Left > 0 || margin.Top > 0 || padding.Left > 0 || padding.Top > 0 ) {
				Rectf leftOnly( margin.Left, margin.Top, 0, 0 );
				Rectf padLeftOnly( padding.Left, padding.Top, 0, 0 );
				richText.addSpan( "", span->getFontStyleConfig(), leftOnly, padLeftOnly );
			}

			Node* spanChild = span->getFirstChild();
			while ( spanChild != NULL ) {
				bool isOutOfFlow = spanChild->isType( UI_TYPE_HTML_WIDGET ) &&
								   spanChild->asType<UIHTMLWidget>()->isOutOfFlow();
				if ( !isOutOfFlow )
					processNodeRef( spanChild, processNodeRef );
				spanChild = spanChild->getNextNode();
			}

			if ( !hasOwnText && ( margin.Right > 0 || margin.Bottom > 0 || padding.Right > 0 ||
								  padding.Bottom > 0 ) ) {
				Rectf rightOnly( 0, 0, margin.Right, margin.Bottom );
				Rectf padRightOnly( 0, 0, padding.Right, padding.Bottom );
				richText.addSpan( "", span->getFontStyleConfig(), rightOnly, padRightOnly );
			}
		} else if ( widget->isType( UI_TYPE_BR ) ) {
			richText.addSpan( "\n",
							  widget->asType<UILineBreak>()->getRichText().getFontStyleConfig() );
		} else {
			Rectf margin = widget->getLayoutPixelsMargin();
			bool isBlock = widget->getLayoutWidthPolicy() == SizePolicy::MatchParent;
			if ( widget->isType( UI_TYPE_HTML_WIDGET ) ) {
				CSSDisplay display = widget->asType<UIHTMLWidget>()->getDisplay();
				if ( display == CSSDisplay::Inline || display == CSSDisplay::InlineBlock )
					isBlock = false;
				else if ( display == CSSDisplay::ListItem )
					isBlock = true;
			}

			if ( mode == IntrinsicMode::None ) {
				if ( isBlock ) {
					if ( container->getPixelsSize().getWidth() != 0 ) {
						Float maxSize = eemax( 0.f, container->getPixelsSize().getWidth() -
														container->getPixelsContentOffset().Left -
														container->getPixelsContentOffset().Right -
														margin.Left - margin.Right );
						widget->setPixelsSize( eemax( 0.f, maxSize ),
											   widget->getPixelsSize().getHeight() );
					} else {
						container->onAutoSizeChild( widget );
					}
				} else if ( widget->getLayoutWidthPolicy() == SizePolicy::WrapContent ||
							widget->getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
					container->onAutoSizeChild( widget );
				}
			}

			Sizef size;
			if ( mode == IntrinsicMode::Min ) {
				size = Sizef( widget->getMinIntrinsicWidth(), 0 );
			} else if ( mode == IntrinsicMode::Max ) {
				size = Sizef( widget->getMaxIntrinsicWidth(), 0 );
			} else {
				size = widget->getPixelsSize();
			}

			Float w = size.getWidth();
			if ( isBlock && mode == IntrinsicMode::None &&
				 container->getPixelsSize().getWidth() != 0 ) {
				w = eemax( 0.f, container->getPixelsSize().getWidth() -
									container->getPixelsContentOffset().Left -
									container->getPixelsContentOffset().Right - margin.Left -
									margin.Right );
			}

			CSSFloat floatType = CSSFloat::None;
			CSSClear clearType = CSSClear::None;
			if ( widget->isType( UI_TYPE_HTML_WIDGET ) ) {
				floatType = widget->asType<UIHTMLWidget>()->getCSSFloat();
				clearType = widget->asType<UIHTMLWidget>()->getCSSClear();
			}

			richText.addCustomSize( Sizef( w + margin.Left + margin.Right,
										   size.getHeight() + margin.Top + margin.Bottom ),
									isBlock, floatType, clearType );
		}
	};

	Node* child = container->getFirstChild();
	while ( NULL != child ) {
		bool isOutOfFlow =
			child->isType( UI_TYPE_HTML_WIDGET ) && child->asType<UIHTMLWidget>()->isOutOfFlow();
		if ( !isOutOfFlow )
			processNode( child, processNode );
		child = child->getNextNode();
	}
}

void UIRichText::rebuildRichText( RichText& richText, IntrinsicMode mode ) {
	rebuildRichText( this, richText, mode );
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

Float UIRichText::getMinIntrinsicWidth() const {
	if ( mWidthPolicy == SizePolicy::Fixed ) {
		return getPropertyWidth();
	}

	UILayouter* layouter = const_cast<UIRichText*>( this )->getLayouter();
	if ( mIntrinsicWidthsDirty && layouter ) {
		layouter->computeIntrinsicWidths();
		mMinIntrinsicWidth = layouter->getMinIntrinsicWidth();
		mMaxIntrinsicWidth = layouter->getMaxIntrinsicWidth();
	} else if ( mIntrinsicWidthsDirty ) {
		RichText richText( mRichText );
		UIRichText::rebuildRichText( const_cast<UIRichText*>( this ), richText,
									 IntrinsicMode::Min );
		mMinIntrinsicWidth = richText.getMinIntrinsicWidth() + getPixelsContentOffset().Left +
							 getPixelsContentOffset().Right;
		UIRichText::rebuildRichText( const_cast<UIRichText*>( this ), richText,
									 IntrinsicMode::Max );
		mMaxIntrinsicWidth = richText.getMaxIntrinsicWidth() + getPixelsContentOffset().Left +
							 getPixelsContentOffset().Right;
		mIntrinsicWidthsDirty = false;
	}

	Float minWidth = mMinIntrinsicWidth;
	if ( !mMinWidthEq.empty() )
		minWidth = eemax( minWidth, getMinSizePx().getWidth() );
	if ( !mMaxWidthEq.empty() )
		minWidth = eemin( minWidth, getMaxSizePx().getWidth() );
	return minWidth;
}

Float UIRichText::getMaxIntrinsicWidth() const {
	if ( mWidthPolicy == SizePolicy::Fixed ) {
		return getPropertyWidth();
	}

	Float maxW = 0;
	if ( const_cast<UIRichText*>( this )->getLayouter() ) {
		maxW = const_cast<UIRichText*>( this )->getLayouter()->getMaxIntrinsicWidth();
	} else {
		if ( mIntrinsicWidthsDirty ) {
			RichText richText( mRichText );
			const_cast<UIRichText*>( this )->rebuildRichText( richText, IntrinsicMode::Min );
			mMinIntrinsicWidth = richText.getMinIntrinsicWidth() + getPixelsContentOffset().Left +
								 getPixelsContentOffset().Right;
			const_cast<UIRichText*>( this )->rebuildRichText( richText, IntrinsicMode::Max );
			mMaxIntrinsicWidth = richText.getMaxIntrinsicWidth() + getPixelsContentOffset().Left +
								 getPixelsContentOffset().Right;
			mIntrinsicWidthsDirty = false;
		}
		maxW = mMaxIntrinsicWidth;
	}

	Float maxWidth = maxW;
	if ( !mMinWidthEq.empty() )
		maxWidth = eemax( maxWidth, getMinSizePx().getWidth() );
	if ( !mMaxWidthEq.empty() )
		maxWidth = eemin( maxWidth, getMaxSizePx().getWidth() );
	return maxWidth;
}

Uint32 UIRichText::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::LayoutAttributeChange: {
			if ( Msg->getSender() != this && !mPacking ) {
				invalidateIntrinsicSize();
				notifyLayoutAttrChangeParent();
			}
			tryUpdateLayout();
			return 1;
		}
		case NodeMessage::MouseDown: {
			if ( Msg->getSender()->isType( UI_TYPE_TEXTSPAN ) )
				return onMouseDown( getEventDispatcher()->getMousePos(), Msg->getFlags() );
		}
		case NodeMessage::MouseUp: {
			if ( Msg->getSender()->isType( UI_TYPE_TEXTSPAN ) ) {
				onMouseUp( getEventDispatcher()->getMousePos(), Msg->getFlags() );
				return 0;
			}
		}
		case NodeMessage::MouseClick: {
			if ( Msg->getSender()->isType( UI_TYPE_TEXTSPAN ) )
				return onMouseClick( getEventDispatcher()->getMousePos(), Msg->getFlags() );
		}
		case NodeMessage::MouseDoubleClick: {
			if ( Msg->getSender()->isType( UI_TYPE_TEXTSPAN ) )
				return onMouseDoubleClick( getEventDispatcher()->getMousePos(), Msg->getFlags() );
		}
	}

	return 0;
}

bool UIRichText::isTextSelectionEnabled() const {
	return 0 != ( mFlags & UI_TEXT_SELECTION_ENABLED );
}

void UIRichText::setTextSelectionEnabled( bool active ) {
	if ( active ) {
		mFlags |= UI_TEXT_SELECTION_ENABLED;
	} else {
		mFlags &= ~UI_TEXT_SELECTION_ENABLED;
	}
}

const Color& UIRichText::getSelectionBackColor() const {
	return mRichText.getSelectionBackColor();
}

void UIRichText::setSelectionBackColor( const Color& color ) {
	mRichText.setSelectionBackColor( color );
	invalidateDraw();
}

const Color& UIRichText::getSelectionColor() const {
	return mRichText.getSelectionColor();
}

void UIRichText::setSelectionColor( const Color& color ) {
	mRichText.setSelectionColor( color );
	invalidateDraw();
}

std::pair<Int64, Int64> UIRichText::getTextSelectionRange() const {
	return { mSelCurInit, mSelCurEnd };
}

void UIRichText::setTextSelectionRange( TextSelectionRange range ) {
	selCurInit( std::clamp( range.start, (Int64)0, mRichText.getCharacterCount() ) );
	selCurEnd( std::clamp( range.end, (Int64)0, mRichText.getCharacterCount() ) );
	onSelectionChange();
}

String UIRichText::getSelectionString() const {
	return mRichText.getSelectionString();
}

Uint32 UIRichText::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	if ( NULL != getEventDispatcher() && isTextSelectionEnabled() && ( flags & EE_BUTTON_LMASK ) &&
		 ( getEventDispatcher()->getMouseDownNode() == this ||
		   inParentTreeOf( getEventDispatcher()->getMouseDownNode() ) ) ) {
		Vector2f nodePos( Vector2f( position.x, position.y ) );
		worldToNode( nodePos );
		nodePos = PixelDensity::dpToPx( nodePos ) - Vector2f( mPaddingPx.Left, mPaddingPx.Top );
		nodePos.x = eemax( 0.f, nodePos.x );
		nodePos.y = eemax( 0.f, nodePos.y );

		Int64 curPos = mRichText.findCharacterFromPos( nodePos.asInt() );

		if ( -1 != curPos ) {
			if ( !mSelecting ) {
				selCurInit( curPos );
				selCurEnd( curPos );
			} else {
				selCurInit( curPos );
			}

			onSelectionChange();
		}

		mSelecting = true;
	}

	return UIHTMLWidget::onMouseDown( position, flags );
}

Uint32 UIRichText::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	if ( isTextSelectionEnabled() && ( flags & EE_BUTTON_LMASK ) ) {
		mSelecting = false;
	}

	return UIHTMLWidget::onMouseClick( position, flags );
}

Uint32 UIRichText::onMouseDoubleClick( const Vector2i& position, const Uint32& flags ) {
	return UIHTMLWidget::onMouseDoubleClick( position, flags );
}

Uint32 UIRichText::onFocusLoss() {
	UIHTMLWidget::onFocusLoss();

	selCurEnd( selCurInit() );
	onSelectionChange();

	return 1;
}

void UIRichText::onSelectionChange() {
	mRichText.setSelection( { mSelCurInit, mSelCurEnd } );
	invalidateDraw();
}

void UIRichText::selCurInit( const Int64& init ) {
	if ( mSelCurInit != init ) {
		mSelCurInit = init;
		invalidateDraw();
	}
}

void UIRichText::selCurEnd( const Int64& end ) {
	if ( mSelCurEnd != end ) {
		mSelCurEnd = end;
		invalidateDraw();
	}
}

}} // namespace EE::UI
