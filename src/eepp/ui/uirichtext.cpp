#include <algorithm>
#include <cmath>
#include <eepp/core/debug.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/stylesheetparser.hpp>
#include <eepp/ui/uiborderdrawable.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uilayouter.hpp>
#include <eepp/ui/uinodedrawable.hpp>
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

struct ProcessedText {
	String::View view;
	String storage;
	bool owns{ false };

	explicit ProcessedText( String::View source ) : view( source ) {}

	bool empty() const { return view.empty(); }

	std::size_t length() const { return view.length(); }

	String::StringBaseType front() const { return view.front(); }

	String::StringBaseType back() const { return view.back(); }

	void setStorage( String&& value ) {
		storage = std::move( value );
		view = storage.view();
		owns = true;
	}

	void removePrefix( std::size_t count ) {
		if ( count == 0 || view.empty() )
			return;
		if ( owns ) {
			storage.erase( 0, count );
			view = storage.view();
		} else {
			view.remove_prefix( count );
		}
	}

	void removeSuffix( std::size_t count ) {
		if ( count == 0 || view.empty() )
			return;
		if ( owns ) {
			for ( std::size_t i = 0; i < count && !storage.empty(); ++i )
				storage.pop_back();
			view = storage.view();
		} else {
			view.remove_suffix( count );
		}
	}
};

static bool sUseCodeEditorForPreCodeBlocks = false;

static bool parseTabSizeCount( const std::string& value, Uint32& out ) {
	Uint32 parsed = 0;
	if ( !String::fromString( parsed, value ) || parsed == 0 )
		return false;
	out = parsed;
	return true;
}

UIRichText::WhiteSpaceCollapse UIRichText::toWhiteSpaceCollapse( std::string_view val ) {
	val = String::trim( val );
	if ( String::iequals( val, "preserve" ) )
		return WhiteSpaceCollapse::Preserve;
	if ( String::iequals( val, "preserve-breaks" ) )
		return WhiteSpaceCollapse::PreserveBreaks;
	if ( String::iequals( val, "preserve-spaces" ) )
		return WhiteSpaceCollapse::PreserveSpaces;
	if ( String::iequals( val, "break-spaces" ) )
		return WhiteSpaceCollapse::BreakSpaces;
	if ( String::iequals( val, "discard" ) )
		return WhiteSpaceCollapse::Discard;
	return WhiteSpaceCollapse::Collapse;
}

std::string UIRichText::fromWhiteSpaceCollapse( WhiteSpaceCollapse val ) {
	switch ( val ) {
		case WhiteSpaceCollapse::Preserve:
			return "preserve";
		case WhiteSpaceCollapse::PreserveBreaks:
			return "preserve-breaks";
		case WhiteSpaceCollapse::PreserveSpaces:
			return "preserve-spaces";
		case WhiteSpaceCollapse::BreakSpaces:
			return "break-spaces";
		case WhiteSpaceCollapse::Discard:
			return "discard";
		case WhiteSpaceCollapse::Collapse:
		default:
			return "collapse";
	}
}

std::string UIRichText::fromWhiteSpace( WhiteSpaceCollapse collapse, bool lineWrap ) {
	switch ( collapse ) {
		case WhiteSpaceCollapse::Preserve:
			return lineWrap ? "pre-wrap" : "pre";
		case WhiteSpaceCollapse::PreserveBreaks:
			return "pre-line";
		case WhiteSpaceCollapse::BreakSpaces:
			return "break-spaces";
		case WhiteSpaceCollapse::PreserveSpaces:
			return lineWrap ? "preserve-spaces" : "preserve nowrap";
		case WhiteSpaceCollapse::Discard:
			return lineWrap ? "discard" : "discard nowrap";
		case WhiteSpaceCollapse::Collapse:
		default:
			return lineWrap ? "normal" : "nowrap";
	}
}

void UIRichText::setUseCodeEditorForPreCodeBlocks( bool enabled ) {
	sUseCodeEditorForPreCodeBlocks = enabled;
}

bool UIRichText::getUseCodeEditorForPreCodeBlocks() {
	return sUseCodeEditorForPreCodeBlocks;
}

static bool hasMarkdownViewAncestor( const Node* node ) {
	for ( const Node* cur = node; cur; cur = cur->getParent() ) {
		if ( cur->isType( UI_TYPE_MARKDOWNVIEW ) )
			return true;
	}
	return false;
}

UIHTMLHtml* UIHTMLHtml::New( const std::string& tag ) {
	return eeNew( UIHTMLHtml, ( tag ) );
}

UIHTMLHtml::UIHTMLHtml( const std::string& tag ) : UIRichText( tag ) {
	enableReportSizeChangeToChildren();
	getUISceneNode()->loadHTMLBaseCSS();
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
	getUISceneNode()->loadHTMLBaseCSS();
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
		case PropertyId::MinHeight:
			mMinHeightLocal = attribute.asStyleSheetLength();
			updateDocumentMinHeight();
			return true;
		default:
			break;
	}

	return UIRichText::applyProperty( attribute );
}

void UIHTMLBody::updateLayout() {
	UIRichText::updateLayout();

	if ( mChild && mChild->isWidget() && !mSettingBodyHeight ) {
		mSettingBodyHeight = true;

		updateDocumentContentMinHeightFromChildren();

		mSettingBodyHeight = false;

		if ( getParent() && getParent()->isType( UI_TYPE_HTML_HTML ) ) {
			auto* html = getParent()->asType<UIHTMLHtml>();
			const Float bodyBottom = getPixelsPosition().y + getPixelsSize().getHeight();
			if ( bodyBottom > html->getPixelsSize().getHeight() + 0.5f ) {
				// The body has a special relationship with the root html element: after late
				// resource/style changes, body can discover a content height while html is already
				// walking its layout tree. The RichText re-entrancy guard intentionally absorbs
				// that child notification to avoid rebuilding the same inline stream many times.
				// Since browsers require the root html box to contain body, enqueue exactly the
				// html layout for a normal later pass instead of forcing a synchronous parent
				// recompute.
				html->setLayoutDirty( LayoutInvalidation::Document );
			}
		}
	}
}

void UIHTMLBody::setDocumentViewportMinHeight( const Float& height ) {
	if ( mDocumentViewportMinHeight == height )
		return;
	mDocumentViewportMinHeight = height;
	updateDocumentMinHeight();
}

Float UIHTMLBody::getLocalMinHeight() const {
	if ( !getParent() )
		return 0;

	Float parentHeight = getParent()->isUINode()
							 ? getParent()->asType<UINode>()->getPixelsSize().getHeight()
							 : getParent()->getSize().getHeight();

	return convertLengthAsDp( mMinHeightLocal, parentHeight );
}

void UIHTMLBody::setDocumentContentMinHeight( const Float& height ) {
	if ( mDocumentContentMinHeight == height )
		return;
	mDocumentContentMinHeight = height;
	updateDocumentMinHeight();
}

void UIHTMLBody::updateDocumentMinHeight() {
	const Float oldMinHeight = getCurMinSize().getHeight();
	Float minHeight =
		std::max( { getLocalMinHeight(), mDocumentViewportMinHeight, mDocumentContentMinHeight } );
	setMinHeight( minHeight );

	if ( minHeight < oldMinHeight &&
		 getPixelsSize().getHeight() > PixelDensity::dpToPx( minHeight ) )
		// Lowering min-height does not shrink the current box by itself. Reapply size through
		// min/max fitting so the body can settle at the new floor.
		setPixelsSize( { getPixelsSize().getWidth(), 0 } );
	else if ( minHeight > oldMinHeight &&
			  getPixelsSize().getHeight() < PixelDensity::dpToPx( minHeight ) )
		// Raising min-height must expand the body to the new minimum when it is currently smaller.
		setPixelsSize( { getPixelsSize().getWidth(), PixelDensity::dpToPx( minHeight ) } );
}

void UIHTMLBody::updateDocumentContentMinHeightFromChildren() {
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
				Float childH = widget->getPixelsPosition().y + widget->getPixelsSize().getHeight();
				maxH = std::max( maxH, childH );
			}
		}
		child = child->getNextNode();
	}

	setDocumentContentMinHeight( maxH > 0 ? std::trunc( PixelDensity::pxToDp( maxH ) ) : 0 );
}

Uint32 UIHTMLBody::onMessage( const NodeMessage* Msg ) {
	Uint32 ret = UIRichText::onMessage( Msg );

	if ( Msg->getMsg() == NodeMessage::LayoutAttributeChange && Msg->getSender() != this &&
		 !mSettingBodyHeight ) {
		mSettingBodyHeight = true;
		updateDocumentContentMinHeightFromChildren();
		mSettingBodyHeight = false;

		if ( getParent() && getParent()->isType( UI_TYPE_HTML_HTML ) )
			getParent()->asType<UIHTMLHtml>()->setLayoutDirty( LayoutInvalidation::Document );
	}

	return ret;
}

UIHTMLHead* UIHTMLHead::New() {
	return eeNew( UIHTMLHead, () );
}

UIHTMLHead::UIHTMLHead() : UIWidget() {
	mVisible = false;
	mEnabled = false;
}

Uint32 UIHTMLHead::getType() const {
	return UI_TYPE_HTML_HEAD;
}

bool UIHTMLHead::isType( const Uint32& type ) const {
	return UIHTMLHead::getType() == type ? true : UIWidget::isType( type );
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

UIRichText* UIRichText::NewHr() {
	auto hr = UIRichText::NewWithTag( "hr" );
	hr->setClipType( ClipType::None );
	return hr;
};

UIRichText* UIRichText::New() {
	return eeNew( UIRichText, () );
}

UIRichText* UIRichText::NewWithTag( const std::string& tag ) {
	return eeNew( UIRichText, ( tag ) );
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

	mRichText.getFontStyleConfig().FontColor = Color::Black;
	mRichText.setTabWidth( mTabSize );

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

		// Block-level flex and grid containers do not own text painting for their item children.
		// Their children are blockified and laid out/drawn as independent nodes. A container
		// can still have a stale mRichText from a previous display state or an intermediate
		// layout pass, so bail out here too; otherwise grid/flex items are painted twice: once by
		// this parent stream and once by the child UITextSpan::draw().
		//
		// Do not use isFlex()/isGrid() here: those also include inline-flex/inline-grid. Inline
		// flex/grid containers are atomic inline-level boxes in their parent formatting context
		// and must not be treated like block-level containers for this paint-ownership shortcut.
		if ( getDisplay() == CSSDisplay::Flex || getDisplay() == CSSDisplay::Grid )
			return;

		if ( mRichText.getSize().getWidth() > 0.f ) {
			Rectf contentOffset = getPixelsContentOffset();
			if ( isClipped() ) {
				clipSmartEnable( mScreenPos.x + contentOffset.Left,
								 mScreenPos.y + contentOffset.Top,
								 mSize.getWidth() - contentOffset.Left - contentOffset.Right,
								 mSize.getHeight() - contentOffset.Top - contentOffset.Bottom );
			}

			if ( isType( UI_TYPE_TEXTSPAN ) && !asType<UITextSpan>()->isInline() &&
				 asType<UITextSpan>()->getFontBackgroundColor() != Color::Transparent ) {
				Primitives p;
				p.setColor( asType<UITextSpan>()->getFontBackgroundColor() );
				p.drawRectangle( Rectf( mScreenPos.trunc(), mSize.floor() ), 0.f, Vector2f::One );
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
		case PropertyId::TextDecoration:
			setTextDecoration( attribute.asTextDecoration() );
			break;
		case PropertyId::FontStyle:
			setFontStyle( attribute.asFontStyle() );
			break;
		case PropertyId::FontWeight:
			setFontWeight( Text::stringToFontWeight( attribute.value() ) );
			break;
		case PropertyId::Color: {
			Color color = attribute.asColor();
			if ( color == Color::Transparent &&
				 attribute.getValue().find( "var(" ) != std::string::npos ) {
#ifdef EE_DEBUG
				eePRINTL( "UIRichText: unresolved var() in Color value for <%s>: '%s'",
						  getElementTag().c_str(), attribute.getValue().c_str() );
#endif
				break;
			}
			setFontColor( attribute.asColor() );
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
		case PropertyId::LineHeight:
			setLineHeightEq( attribute.value() );
			break;
		case PropertyId::TextIndent:
			setTextIndentEq( attribute.value() );
			break;
		case PropertyId::TabSize: {
			Uint32 tabSize = 0;
			if ( parseTabSizeCount( attribute.value(), tabSize ) ) {
				setTabSize( tabSize );
			} else {
				Float tabSizePx = lengthFromValue( attribute );
				const auto& style = mRichText.getFontStyleConfig();
				if ( tabSizePx > 0 && style.Font != nullptr ) {
					bool bold = ( style.Style & Text::Style::Bold ) != 0;
					bool italic = ( style.Style & Text::Style::Italic ) != 0;
					Float spaceWidth = style.Font
										   ->getGlyph( L' ', style.CharacterSize, bold, italic,
													   style.OutlineThickness )
										   .advance;
					if ( spaceWidth > 0 )
						setTabSize( eemax<Uint32>(
							1, static_cast<Uint32>( std::round( tabSizePx / spaceWidth ) ) ) );
				}
			}
			break;
		}
		case PropertyId::WhiteSpace:
			applyWhiteSpace( attribute.value() );
			break;
		case PropertyId::WhiteSpaceCollapse:
			setWhiteSpaceCollapse( toWhiteSpaceCollapse( attribute.value() ) );
			break;
		case PropertyId::TextTransform:
			setTextTransform( TextTransform::fromString( attribute.asString() ) );
			break;
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
		case PropertyId::FontWeight:
			return Text::fontWeightToString( mRichText.getFontStyleConfig().Weight );
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
		case PropertyId::LineHeight:
			return mLineHeightEq.empty() ? "normal" : mLineHeightEq;
		case PropertyId::TextIndent:
			return mTextIndentEq.empty() ? "0" : mTextIndentEq;
		case PropertyId::TabSize:
			return String::toString( getTabSize() );
		case PropertyId::WhiteSpace:
			return fromWhiteSpace( mWhiteSpaceCollapse, mLineWrap );
		case PropertyId::WhiteSpaceCollapse:
			return fromWhiteSpaceCollapse( mWhiteSpaceCollapse );
		case PropertyId::TextTransform:
			return TextTransform::toString( getTextTransform() );
		default:
			return UIHTMLWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIRichText::getPropertiesImplemented() const {
	auto props = UIHTMLWidget::getPropertiesImplemented();
	auto local = {
		PropertyId::FontFamily,		 PropertyId::FontSize,			 PropertyId::FontStyle,
		PropertyId::Color,			 PropertyId::TextShadowColor,	 PropertyId::TextShadowOffset,
		PropertyId::TextStrokeWidth, PropertyId::TextStrokeColor,	 PropertyId::TextAlign,
		PropertyId::SelectionColor,	 PropertyId::SelectionBackColor, PropertyId::TextSelection,
		PropertyId::TextDecoration,	 PropertyId::LineHeight,		 PropertyId::TextIndent,
		PropertyId::TabSize,		 PropertyId::WhiteSpace,		 PropertyId::WhiteSpaceCollapse,
		PropertyId::TextTransform };
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
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
		updateDefaultSpansStyle();
	}
	return this;
}

Uint32 UIRichText::getFontSize() const {
	return mRichText.getFontStyleConfig().CharacterSize;
}

UIRichText* UIRichText::setFontSize( const Uint32& characterSize ) {
	if ( mRichText.getFontStyleConfig().CharacterSize != characterSize ) {
		if ( characterSize == 0 )
			return this;
		mRichText.getFontStyleConfig().CharacterSize = characterSize;
		mRichText.invalidate();
		mLineHeightPxDirty = true;

		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
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

		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
		updateDefaultSpansStyle();
	}
	return this;
}

UIRichText* UIRichText::setFontStyle( const Uint32& fontStyle ) {
	if ( mRichText.getFontStyleConfig().Style != fontStyle ) {
		mRichText.getFontStyleConfig().Style = fontStyle;
		mRichText.invalidate();

		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
		updateDefaultSpansStyle();

		if ( auto* newFont = getUISceneNode()->reevaluateFontStyle(
				 mRichText.getFontStyleConfig().Font, fontStyle,
				 ( fontStyle & Text::Bold ) ? FontWeight::Bold : FontWeight::Normal ) )
			setFont( newFont );
	}
	return this;
}

FontWeight UIRichText::getFontWeight() const {
	return mRichText.getFontStyleConfig().Weight;
}

UIRichText* UIRichText::setFontWeight( const FontWeight& weight ) {
	mRichText.getFontStyleConfig().Weight = weight;

	Uint32 weightStyle = ( weight >= FontWeight::SemiBold ) ? Text::Bold : 0;
	Uint32 newStyle = ( mRichText.getFontStyleConfig().Style & ~Text::Bold ) | weightStyle;

	if ( mRichText.getFontStyleConfig().Style != newStyle ) {
		mRichText.getFontStyleConfig().Style = newStyle;
		mRichText.invalidate();

		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
		updateDefaultSpansStyle();

		if ( auto* newFont = getUISceneNode()->reevaluateFontStyle(
				 mRichText.getFontStyleConfig().Font, newStyle, weight ) )
			setFont( newFont );
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

		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
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

		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
	}
	return this;
}

UIRichText::WhiteSpaceCollapse UIRichText::getWhiteSpaceCollapse() const {
	return mWhiteSpaceCollapse;
}

void UIRichText::setWhiteSpaceCollapse( WhiteSpaceCollapse collapse ) {
	if ( mWhiteSpaceCollapse != collapse ) {
		mWhiteSpaceCollapse = collapse;
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
	}
}

bool UIRichText::getLineWrap() const {
	return mLineWrap;
}

void UIRichText::setLineWrap( bool lineWrap ) {
	if ( mLineWrap != lineWrap ) {
		mLineWrap = lineWrap;
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
	}
}

void UIRichText::applyWhiteSpace( std::string_view val ) {
	val = String::trim( val );
	if ( val.empty() ) {
		return;
	}
	if ( String::iequals( val, "normal" ) ) {
		setWhiteSpaceCollapse( WhiteSpaceCollapse::Collapse );
		setLineWrap( true );
	} else if ( String::iequals( val, "nowrap" ) ) {
		setWhiteSpaceCollapse( WhiteSpaceCollapse::Collapse );
		setLineWrap( false );
	} else if ( String::iequals( val, "pre" ) ) {
		setWhiteSpaceCollapse( WhiteSpaceCollapse::Preserve );
		setLineWrap( false );
	} else if ( String::iequals( val, "pre-wrap" ) ) {
		setWhiteSpaceCollapse( WhiteSpaceCollapse::Preserve );
		setLineWrap( true );
	} else if ( String::iequals( val, "pre-line" ) ) {
		setWhiteSpaceCollapse( WhiteSpaceCollapse::PreserveBreaks );
		setLineWrap( true );
	} else if ( String::iequals( val, "break-spaces" ) ) {
		setWhiteSpaceCollapse( WhiteSpaceCollapse::BreakSpaces );
		setLineWrap( true );
	} else if ( String::iequals( val, "preserve-spaces" ) ) {
		setWhiteSpaceCollapse( WhiteSpaceCollapse::PreserveSpaces );
		setLineWrap( true );
	} else if ( String::iequals( val, "preserve-breaks" ) ) {
		setWhiteSpaceCollapse( WhiteSpaceCollapse::PreserveBreaks );
		setLineWrap( true );
	} else if ( String::iequals( val, "discard" ) ) {
		setWhiteSpaceCollapse( WhiteSpaceCollapse::Discard );
		setLineWrap( true );
	} else {
		bool foundCollapse = false;
		bool foundWrap = false;
		WhiteSpaceCollapse collapse = mWhiteSpaceCollapse;
		bool lineWrap = mLineWrap;
		String::readBySeparator(
			val,
			[&]( std::string_view token ) {
				token = String::trim( token );
				if ( !token.empty() ) {
					if ( String::iequals( token, "wrap" ) ) {
						lineWrap = true;
						foundWrap = true;
					} else if ( String::iequals( token, "nowrap" ) ) {
						lineWrap = false;
						foundWrap = true;
					} else if ( String::iequals( token, "collapse" ) ||
								String::iequals( token, "preserve" ) ||
								String::iequals( token, "preserve-breaks" ) ||
								String::iequals( token, "preserve-spaces" ) ||
								String::iequals( token, "break-spaces" ) ||
								String::iequals( token, "discard" ) ) {
						collapse = toWhiteSpaceCollapse( token );
						foundCollapse = true;
					}
				}
			},
			' ' );
		if ( foundCollapse )
			setWhiteSpaceCollapse( collapse );
		if ( foundWrap )
			setLineWrap( lineWrap );
	}
}

UIRichText* UIRichText::setLineHeightEq( const std::string& eq ) {
	if ( mLineHeightEq != eq ) {
		mLineHeightEq = eq;
		mLineHeightPxDirty = true;
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
	}
	return this;
}

Float UIRichText::getLineHeightPx() const {
	if ( !mLineHeightPxDirty )
		return mLineHeightPxCache;
	mLineHeightPxDirty = false;
	if ( mLineHeightEq.empty() || mLineHeightEq == "normal" ) {
		mLineHeightPxCache = 0;
		return 0;
	}

	// Temporal hack until we support calc
	if ( mLineHeightEq.find( "calc(" ) != std::string::npos ||
		 mLineHeightEq.find( "var(" ) != std::string::npos ) {
		mLineHeightPxCache = 0;
		return 0;
	}

	bool isUnitless = !mLineHeightEq.empty();
	for ( char c : mLineHeightEq ) {
		if ( c != '-' && c != '+' && c != '.' && !String::isNumber( c, false ) ) {
			isUnitless = false;
			break;
		}
	}
	if ( isUnitless ) {
		Float multiplier;
		if ( String::fromString( multiplier, mLineHeightEq ) )
			mLineHeightPxCache = multiplier * getFontSize();
		else
			mLineHeightPxCache = 0;
	} else {
		mLineHeightPxCache = const_cast<UIRichText*>( this )->lengthFromValue(
			mLineHeightEq, CSS::PropertyRelativeTarget::FontSize, 0, 0 );
	}
	return mLineHeightPxCache;
}

UIRichText* UIRichText::setTextIndentEq( const std::string& eq ) {
	if ( mTextIndentEq != eq ) {
		mTextIndentEq = eq;
		mTextIndentPxDirty = true;
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
	}
	return this;
}

Float UIRichText::getTextIndentPx() const {
	if ( !mTextIndentPxDirty )
		return mTextIndentPxCache;
	mTextIndentPxDirty = false;
	if ( mTextIndentEq.empty() ) {
		mTextIndentPxCache = 0;
		return 0;
	}
	mTextIndentPxCache = const_cast<UIRichText*>( this )->lengthFromValue(
		mTextIndentEq, CSS::PropertyRelativeTarget::None, 0, 0 );
	return mTextIndentPxCache;
}

Uint32 UIRichText::getTabSize() const {
	return mTabSize;
}

UIRichText* UIRichText::setTabSize( Uint32 tabSize ) {
	tabSize = eemax<Uint32>( 1, tabSize );
	if ( mTabSize != tabSize ) {
		mTabSize = tabSize;
		mRichText.setTabWidth( mTabSize );
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
	}
	return this;
}

const TextTransform::Value& UIRichText::getTextTransform() const {
	return mTextTransform;
}

void UIRichText::setTextTransform( const TextTransform::Value& textTransform ) {
	if ( textTransform != mTextTransform ) {
		mTextTransform = textTransform;
		notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
	}
}

void UIRichText::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	UIWidget::loadFromXmlNode( node );

	for ( pugi::xml_node child = node.first_child(); child; child = child.next_sibling() ) {
		if ( child.type() == pugi::node_element ) {
			if ( mTag == "pre" && String::iequals( child.name(), "code" ) &&
				 ( sUseCodeEditorForPreCodeBlocks || hasMarkdownViewAncestor( this ) ) ) {
				// Use a UICodeEditor for markdown <pre><code> blocks, or when explicitly enabled.
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
					editor->getUIStyle()->applyInheritedProperties();
					editor->onWidgetCreated();
				}
			} else if ( String::iequals( child.name(), "style" ) ) {
				CSS::StyleSheetParser parser;
				parser.setBaseURI( getUISceneNode()->getURI() );
				std::string styleContent;
				for ( pugi::xml_node styleChild = child.first_child(); styleChild;
					  styleChild = styleChild.next_sibling() ) {
					if ( styleChild.type() == pugi::node_pcdata ||
						 styleChild.type() == pugi::node_cdata ) {
						styleContent += styleChild.value();
					}
				}

				if ( parser.loadFromString( std::string_view{ styleContent } ) ) {
					parser.getStyleSheet().setMarker( getUISceneNode()->getCurrentMarker() );
					getUISceneNode()->combineStyleSheet( parser.getStyleSheet(), false );
				}
				continue;
			} else if ( String::iequals( child.name(), "script" ) ) {
				// No plans to support it
				continue;
			} else {
				// Let parent logic load standard child widget
				UIWidget* uiwidget = UIWidgetCreator::createFromName( child.name() );

				// For not known elements it should create an HTMLUnknown element, in practice
				// an span with a the tag name used is enough
				if ( uiwidget == nullptr )
					uiwidget = UITextSpan::NewWithTag( child.name() );

				if ( uiwidget ) {
					uiwidget->setParent( this );
					uiwidget->loadFromXmlNode( child );
					uiwidget->getUIStyle()->applyInheritedProperties();

					if ( !uiwidget->loadsItsChildren() ) {
						if ( child.first_child() && !uiwidget->loadsItsChildren() ) {
							getUISceneNode()->loadNode( child.first_child(), uiwidget, 0 );
						}
					}

					uiwidget->onWidgetCreated();
				}
			}
		} else if ( child.type() == pugi::node_pcdata ) {
			UITextNode* textNode = UITextNode::New();
			textNode->setParent( this );
			textNode->setText( child.value() );
		}
	}

	endAttributesTransaction();
}

void UIRichText::onSizeChange() {
	if ( isOutOfFlow() ) {
		// Out-of-flow elements are sized/positioned by updateOutOfFlowPosition(). That routine can
		// apply a computed width/height after the normal layouter has run. Running
		// UIHTMLWidget::onSizeChange() from here would call tryUpdateLayout() on the same element
		// while updateOutOfFlowPosition() is still on the stack, which can recurse forever through
		// updateLayout() -> updateOutOfFlowPosition() -> setPixelsSize() -> onSizeChange().
		//
		// We still emit the generic UIWidget size-change notification so drawing/events stay
		// current, and absolute elements still notify ancestors below: body-like containers may
		// include absolute descendants when computing their effective content height. Fixed
		// elements are excluded from the parent notification because they are viewport-relative and
		// do not participate in normal-flow sizing.
		UIWidget::onSizeChange();
	} else {
		UIHTMLWidget::onSizeChange();
	}
	if ( getCSSPosition() != CSSPosition::Fixed )
		notifyLayoutAttrChangeParent( LayoutInvalidation::ParentChildChange );
}

void UIRichText::onPaddingChange() {
	UIHTMLWidget::onPaddingChange();
	notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
	notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
}

void UIRichText::onChildCountChange( Node* child, const bool& removed ) {
	UIHTMLWidget::onChildCountChange( child, removed );
	if ( !removed && child->isWidget() && child->isType( UI_TYPE_TEXTSPAN ) ) {
		static_cast<UITextSpan*>( child )->setInheritedStyle( mRichText.getFontStyleConfig() );
	}

	notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
	notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
}

void UIRichText::onFontChanged() {
	notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
	notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
}

void UIRichText::onFontStyleChanged() {
	notifyLayoutAttrChange( LayoutInvalidation::TextFormatting );
	notifyLayoutAttrChangeParent( LayoutInvalidation::ParentReplacedFormatting );
}

String UIRichText::UIRichText::collapseInternalWhitespace( const String& s ) {
	String out;
	out.reserve( s.size() );
	bool inSpace = false;
	for ( size_t i = 0; i < s.size(); ++i ) {
		if ( s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r' || s[i] == '\v' ) {
			if ( !inSpace ) {
				out += ' ';
				inSpace = true;
			}
		} else {
			out += s[i];
			inSpace = false;
		}
	}
	return out;
}

static bool isCSSCollapsibleWhiteSpace( const String::StringBaseType& c ) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

static bool isCSSSegmentBreak( const String::StringBaseType& c ) {
	return c == '\n' || c == '\r';
}

static bool isSingleForcedLineBreak( String::View text ) {
	return text.size() == 1 && text[0] == '\n';
}

static bool shouldCollapseAcrossTextRuns( UIRichText::WhiteSpaceCollapse collapse ) {
	return collapse == UIRichText::WhiteSpaceCollapse::Collapse ||
		   collapse == UIRichText::WhiteSpaceCollapse::PreserveBreaks;
}

static bool shouldTrimBlockBoundarySpace( UIRichText::WhiteSpaceCollapse collapse ) {
	return collapse == UIRichText::WhiteSpaceCollapse::Collapse ||
		   collapse == UIRichText::WhiteSpaceCollapse::PreserveBreaks ||
		   collapse == UIRichText::WhiteSpaceCollapse::Discard;
}

static bool trimFinalSegmentBreakBeforeEndTag( ProcessedText& text ) {
	if ( text.empty() )
		return false;

	if ( text.back() == '\n' ) {
		std::size_t count = 1;
		if ( text.length() > 1 && text.view[text.length() - 2] == '\r' )
			count++;
		text.removeSuffix( count );
		return true;
	}

	if ( text.back() == '\r' ) {
		text.removeSuffix( 1 );
		return true;
	}

	return false;
}

static bool containsCarriageReturn( String::View text ) {
	for ( auto c : text ) {
		if ( c == '\r' )
			return true;
	}
	return false;
}

static bool preserveSpacesNeedsTransform( String::View text ) {
	for ( auto c : text ) {
		if ( isCSSCollapsibleWhiteSpace( c ) && c != ' ' )
			return true;
	}
	return false;
}

static bool preserveBreaksNeedsTransform( String::View text ) {
	bool inSpace = false;
	for ( std::size_t i = 0; i < text.size(); ++i ) {
		auto c = text[i];
		if ( isCSSSegmentBreak( c ) ) {
			if ( c == '\r' )
				return true;
			inSpace = false;
		} else if ( isCSSCollapsibleWhiteSpace( c ) ) {
			if ( c != ' ' || inSpace )
				return true;
			inSpace = true;
		} else {
			inSpace = false;
		}
	}
	return false;
}

static bool collapseNeedsTransform( String::View text ) {
	bool inSpace = false;
	for ( auto c : text ) {
		if ( isCSSCollapsibleWhiteSpace( c ) ) {
			if ( c != ' ' || inSpace )
				return true;
			inSpace = true;
		} else {
			inSpace = false;
		}
	}
	return false;
}

static bool discardNeedsTransform( String::View text ) {
	for ( auto c : text ) {
		if ( isCSSCollapsibleWhiteSpace( c ) )
			return true;
	}
	return false;
}

static ProcessedText processWhiteSpaceForLayout( String::View source,
												 UIRichText::WhiteSpaceCollapse collapse,
												 bool prevIsInline, bool nextIsInline,
												 bool lastSpanEndsWithSpace,
												 bool discardSegmentBreakBeforeEndTag ) {
	ProcessedText text( source );
	if ( text.empty() )
		return text;

	if ( discardSegmentBreakBeforeEndTag )
		trimFinalSegmentBreakBeforeEndTag( text );

	if ( text.empty() )
		return text;

	String out;
	bool needsStorage = false;

	switch ( collapse ) {
		case UIRichText::WhiteSpaceCollapse::Preserve:
		case UIRichText::WhiteSpaceCollapse::BreakSpaces:
			needsStorage = containsCarriageReturn( text.view );
			if ( needsStorage ) {
				out.reserve( text.length() );
				for ( std::size_t i = 0; i < text.length(); ++i ) {
					if ( text.view[i] == '\r' ) {
						out += '\n';
						if ( i + 1 < text.length() && text.view[i + 1] == '\n' )
							i++;
					} else {
						out += text.view[i];
					}
				}
			}
			break;
		case UIRichText::WhiteSpaceCollapse::PreserveSpaces:
			needsStorage = preserveSpacesNeedsTransform( text.view );
			if ( needsStorage ) {
				out.reserve( text.length() );
				for ( std::size_t i = 0; i < text.length(); ++i ) {
					if ( isCSSSegmentBreak( text.view[i] ) ) {
						out += ' ';
						if ( text.view[i] == '\r' && i + 1 < text.length() &&
							 text.view[i + 1] == '\n' )
							i++;
					} else {
						out += isCSSCollapsibleWhiteSpace( text.view[i] ) ? ' ' : text.view[i];
					}
				}
			}
			break;
		case UIRichText::WhiteSpaceCollapse::PreserveBreaks:
			needsStorage = preserveBreaksNeedsTransform( text.view );
			if ( needsStorage ) {
				out.reserve( text.length() );
				bool inSpace = false;
				for ( std::size_t i = 0; i < text.length(); ++i ) {
					if ( isCSSSegmentBreak( text.view[i] ) ) {
						out += '\n';
						inSpace = false;
						if ( text.view[i] == '\r' && i + 1 < text.length() &&
							 text.view[i + 1] == '\n' )
							i++;
					} else if ( isCSSCollapsibleWhiteSpace( text.view[i] ) ) {
						if ( !inSpace ) {
							out += ' ';
							inSpace = true;
						}
					} else {
						out += text.view[i];
						inSpace = false;
					}
				}
			}
			break;
		case UIRichText::WhiteSpaceCollapse::Discard:
			needsStorage = discardNeedsTransform( text.view );
			if ( needsStorage ) {
				out.reserve( text.length() );
				for ( auto c : text.view ) {
					if ( !isCSSCollapsibleWhiteSpace( c ) )
						out += c;
				}
			}
			break;
		case UIRichText::WhiteSpaceCollapse::Collapse:
		default:
			needsStorage = collapseNeedsTransform( text.view );
			if ( needsStorage ) {
				out.reserve( text.length() );
				bool inSpace = false;
				for ( auto c : text.view ) {
					if ( isCSSCollapsibleWhiteSpace( c ) ) {
						if ( !inSpace ) {
							out += ' ';
							inSpace = true;
						}
					} else {
						out += c;
						inSpace = false;
					}
				}
			}
			break;
	}

	if ( needsStorage )
		text.setStorage( std::move( out ) );

	if ( shouldTrimBlockBoundarySpace( collapse ) ) {
		if ( !prevIsInline && !text.empty() && text.front() == ' ' )
			text.removePrefix( 1 );
		if ( !nextIsInline && !text.empty() && text.back() == ' ' )
			text.removeSuffix( 1 );
		if ( lastSpanEndsWithSpace && !text.empty() && text.front() == ' ' )
			text.removePrefix( 1 );
	}

	return text;
}

static void applyTextTransformValue( String& text, TextTransform::Value transform ) {
	switch ( transform ) {
		case TextTransform::LowerCase:
			text = text.toLower();
			break;
		case TextTransform::UpperCase:
			text = text.toUpper();
			break;
		case TextTransform::Capitalize:
			text = text.capitalize();
			break;
		default:
			break;
	}
}

static String materializeTransformedText( String::View text, TextTransform::Value transform ) {
	String transformed( text );
	if ( transform != TextTransform::None )
		applyTextTransformValue( transformed, transform );
	return transformed;
}

static void addProcessedTextSpan( RichText& richText, ProcessedText&& text,
								  const FontStyleConfig& style, bool inlineText,
								  RichText::InlineSource source = {} ) {
	if ( text.owns ) {
		if ( inlineText )
			richText.addInlineText( std::move( text.storage ), style, Rectf::Zero, Rectf::Zero, 0,
									{}, source );
		else
			richText.addSpan( std::move( text.storage ), style, Rectf::Zero, Rectf::Zero, 0, {},
							  source );
		return;
	}

	if ( inlineText )
		richText.addInlineText( text.view, style, Rectf::Zero, Rectf::Zero, 0, {}, source );
	else
		richText.addSpan( text.view, style, Rectf::Zero, Rectf::Zero, 0, {}, source );
}

static UIRichText::WhiteSpaceCollapse getEffectiveWhiteSpaceCollapse( Node* node,
																	  UIRichText* fallback ) {
	for ( Node* cur = node; cur; cur = cur->getParent() ) {
		if ( cur->isType( UI_TYPE_TEXTSPAN ) || cur->isType( UI_TYPE_RICHTEXT ) )
			return cur->asType<UIRichText>()->getWhiteSpaceCollapse();
	}
	return fallback ? fallback->getWhiteSpaceCollapse() : UIRichText::WhiteSpaceCollapse::Collapse;
}

static RichText::WhiteSpaceWrapMode
toRichTextWhiteSpaceWrapMode( UIRichText::WhiteSpaceCollapse collapse ) {
	switch ( collapse ) {
		case UIRichText::WhiteSpaceCollapse::BreakSpaces:
			return RichText::WhiteSpaceWrapMode::BreakSpaces;
		case UIRichText::WhiteSpaceCollapse::Preserve:
		case UIRichText::WhiteSpaceCollapse::PreserveSpaces:
			return RichText::WhiteSpaceWrapMode::Preserve;
		case UIRichText::WhiteSpaceCollapse::Collapse:
		case UIRichText::WhiteSpaceCollapse::PreserveBreaks:
		case UIRichText::WhiteSpaceCollapse::Discard:
		default:
			return RichText::WhiteSpaceWrapMode::Normal;
	}
}

static Float getAtomicInlineBoxBaseline( UIWidget* widget, const Sizef& widgetSize,
										 const Rectf& margin ) {
	Float fallbackBaseline = widgetSize.getHeight() + margin.Top + margin.Bottom;
	if ( !widget->isType( UI_TYPE_HTML_WIDGET ) )
		return fallbackBaseline;

	auto* htmlWidget = widget->asType<UIHTMLWidget>();
	auto* rt = htmlWidget->getRichTextPtr();
	if ( rt == nullptr )
		return fallbackBaseline;

	rt->updateLayout();
	const auto& lines = rt->getLines();
	for ( auto it = lines.rbegin(); it != lines.rend(); ++it ) {
		if ( !it->spans.empty() )
			return margin.Top + widget->getPixelsContentOffset().Top + it->y + it->maxAscent;
	}

	return fallbackBaseline;
}

static CSSBaselineAlignValue getWidgetBaselineAlign( UIWidget* widget ) {
	if ( widget->isType( UI_TYPE_HTML_WIDGET ) )
		return widget->asType<UIHTMLWidget>()->getBaselineAlign();
	return {};
}

static RichText::BaselineAlignValue toRichTextBaselineAlign( const CSSBaselineAlignValue& align ) {
	RichText::BaselineAlignValue out;
	out.value = align.value;
	switch ( align.type ) {
		case CSSBaselineAlignment::Sub:
			out.type = RichText::BaselineAlignment::Sub;
			break;
		case CSSBaselineAlignment::Super:
			out.type = RichText::BaselineAlignment::Super;
			break;
		case CSSBaselineAlignment::TextTop:
			out.type = RichText::BaselineAlignment::TextTop;
			break;
		case CSSBaselineAlignment::TextBottom:
			out.type = RichText::BaselineAlignment::TextBottom;
			break;
		case CSSBaselineAlignment::Middle:
			out.type = RichText::BaselineAlignment::Middle;
			break;
		case CSSBaselineAlignment::Top:
			out.type = RichText::BaselineAlignment::Top;
			break;
		case CSSBaselineAlignment::Bottom:
			out.type = RichText::BaselineAlignment::Bottom;
			break;
		case CSSBaselineAlignment::Length:
			out.type = RichText::BaselineAlignment::Length;
			break;
		case CSSBaselineAlignment::Percentage:
			out.type = RichText::BaselineAlignment::Percentage;
			break;
		case CSSBaselineAlignment::Auto:
			out.type = RichText::BaselineAlignment::Auto;
			break;
		case CSSBaselineAlignment::Baseline:
		default:
			out.type = RichText::BaselineAlignment::Baseline;
			break;
	}
	return out;
}

static RichText::InlineFloat toRichTextFloat( CSSFloat val ) {
	switch ( val ) {
		case CSSFloat::Left:
			return RichText::InlineFloat::Left;
		case CSSFloat::Right:
			return RichText::InlineFloat::Right;
		case CSSFloat::None:
		default:
			return RichText::InlineFloat::None;
	}
}

static RichText::InlineClear toRichTextClear( CSSClear val ) {
	switch ( val ) {
		case CSSClear::Left:
			return RichText::InlineClear::Left;
		case CSSClear::Right:
			return RichText::InlineClear::Right;
		case CSSClear::Both:
			return RichText::InlineClear::Both;
		case CSSClear::None:
		default:
			return RichText::InlineClear::None;
	}
}

static RichText::InlineSource toRichTextTextSource( UITextNode* node ) {
	return { RichText::InlineSourceType::TextNode, node };
}

static RichText::InlineSource toRichTextWidgetSource( UIWidget* widget ) {
	return { RichText::InlineSourceType::Widget, widget };
}

static Float getInlineBorderWidth( UIWidget* widget ) {
	if ( widget == nullptr || !widget->hasBorder() || widget->getBorder() == nullptr )
		return 0.f;

	const auto& borders = widget->getBorder()->getBorders();
	return eemax( eemax( static_cast<Float>( borders.left.width ),
						 static_cast<Float>( borders.right.width ) ),
				  eemax( static_cast<Float>( borders.top.width ),
						 static_cast<Float>( borders.bottom.width ) ) );
}

static Color getInlineBorderColor( UIWidget* widget ) {
	if ( widget == nullptr || !widget->hasBorder() || widget->getBorder() == nullptr )
		return Color::Transparent;

	const auto& borders = widget->getBorder()->getBorders();
	if ( borders.top.width > 0 )
		return borders.top.color;
	if ( borders.right.width > 0 )
		return borders.right.color;
	if ( borders.bottom.width > 0 )
		return borders.bottom.color;
	if ( borders.left.width > 0 )
		return borders.left.color;
	return Color::Transparent;
}

struct InlineBackgroundDrawable {
	Drawable* colorDrawable{ nullptr };
	Drawable* drawable{ nullptr };
	bool usesFragmentColor{ false };
};

static InlineBackgroundDrawable getInlineBackgroundDrawable( UIWidget* widget,
															 const Color& backgroundColor ) {
	if ( widget == nullptr || !( widget->getFlags() & UI_FILL_BACKGROUND ) ||
		 !widget->hasBackground() )
		return {};

	UINodeDrawable* background = widget->getBackground();
	const bool hasRoundedColorBackground =
		backgroundColor != Color::Transparent && background->getBackgroundDrawable().hasRadius();
	if ( background->hasDrawableLayers() )
		return { hasRoundedColorBackground ? &background->getBackgroundDrawable() : nullptr,
				 background, false };
	if ( background->getBackgroundColor() != Color::Transparent )
		return { nullptr, background, false };

	if ( hasRoundedColorBackground )
		return { nullptr, &background->getBackgroundDrawable(), true };

	return {};
}

static Drawable* getInlineBorderDrawable( UIWidget* widget ) {
	return widget != nullptr && widget->hasBorder() && widget->getBorder() != nullptr
			   ? widget->getBorder()
			   : nullptr;
}

void UIRichText::rebuildRichText( UILayout* container, RichText& richText, IntrinsicMode mode ) {
	UILayout::countRichTextRebuild();
	richText.clear();
	if ( container->isType( UI_TYPE_RICHTEXT ) || container->isType( UI_TYPE_TEXTSPAN ) ) {
		auto* uiRt = static_cast<UIRichText*>( container );
		richText.setLineHeight( uiRt->getLineHeightPx() );
		richText.setTextIndent( uiRt->getTextIndentPx() );
		richText.setLineWrap( uiRt->getLineWrap() );
		richText.setTabWidth( uiRt->getTabSize() );
		richText.setWhiteSpaceWrapMode(
			toRichTextWhiteSpaceWrapMode( uiRt->getWhiteSpaceCollapse() ) );
	}
	UIRichText* containerRichText =
		container->isType( UI_TYPE_RICHTEXT ) || container->isType( UI_TYPE_TEXTSPAN )
			? container->asType<UIRichText>()
			: nullptr;
	bool lastSpanEndsWithSpace = false;
	Float maxWidth = 0;
	bool isInlineBlockTextSpan =
		container->isType( UI_TYPE_TEXTSPAN ) && container->asType<UITextSpan>()->isInlineBlock();
	Node* parentNode = container->getParent();
	bool parentIsFlexOrGrid = parentNode && parentNode->isType( UI_TYPE_HTML_WIDGET ) &&
							  ( parentNode->asType<UIHTMLWidget>()->isFlex() ||
								parentNode->asType<UIHTMLWidget>()->isGrid() );
	if ( isInlineBlockTextSpan && mode == IntrinsicMode::None &&
		 container->getPixelsSize().getWidth() > 0 ) {
		maxWidth = container->getPixelsSize().getWidth() -
				   container->getPixelsContentOffset().Left -
				   container->getPixelsContentOffset().Right;
	} else if ( parentIsFlexOrGrid &&
				container->getLayoutWidthPolicy() == SizePolicy::WrapContent &&
				mode == IntrinsicMode::None ) {
		maxWidth = 0;
	} else if ( container->getLayoutWidthPolicy() == SizePolicy::WrapContent ) {
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

	auto getEffectiveTextTransform = []( Node* node ) -> TextTransform::Value {
		while ( node ) {
			if ( node->isType( UI_TYPE_RICHTEXT ) || node->isType( UI_TYPE_TEXTSPAN ) ) {
				auto tt = node->asType<UIRichText>()->getTextTransform();
				if ( tt != TextTransform::None )
					return tt;
			}
			node = node->getParent();
		}
		return TextTransform::None;
	};

	if ( container->isType( UI_TYPE_TEXTSPAN ) ) {
		UITextSpan* selfSpan = container->asType<UITextSpan>();
		bool parentIsFlex = parentNode && parentNode->isType( UI_TYPE_HTML_WIDGET ) &&
							parentNode->asType<UIHTMLWidget>()->isFlex();
		bool parentIsGrid = parentNode && parentNode->isType( UI_TYPE_HTML_WIDGET ) &&
							parentNode->asType<UIHTMLWidget>()->isGrid();
		if ( !selfSpan->getText().empty() &&
			 ( !selfSpan->isInline() || parentIsFlex || parentIsGrid ) &&
			 NULL != selfSpan->getFontStyleConfig().Font ) {
			auto collapse = getEffectiveWhiteSpaceCollapse( selfSpan, containerRichText );
			richText.setWhiteSpaceWrapMode( toRichTextWhiteSpaceWrapMode( collapse ) );
			auto selfText = processWhiteSpaceForLayout( selfSpan->getText().view(), collapse, true,
														true, lastSpanEndsWithSpace,
														selfSpan->getFirstChild() == nullptr );
			auto tt = getEffectiveTextTransform( selfSpan );
			FontStyleConfig style = selfSpan->getFontStyleConfig();
			style.BackgroundColor = Color::Transparent;
			bool endsWithSpace = shouldCollapseAcrossTextRuns( collapse ) && !selfText.empty() &&
								 selfText.back() == ' ';
			if ( tt != TextTransform::None )
				richText.addSpan( materializeTransformedText( selfText.view, tt ), style,
								  Rectf::Zero, Rectf::Zero, 0, {} );
			else
				addProcessedTextSpan( richText, std::move( selfText ), style, false );
			if ( shouldCollapseAcrossTextRuns( collapse ) )
				lastSpanEndsWithSpace = endsWithSpace;
		}
	}

	int inlineBoxDepth = 0;
	auto processNode = [&]( Node* node, auto& processNodeRef ) -> void {
		// Helper: walk up through inline ancestors to find the logical prev/next widget
		auto findLogicalPrev = []( Node* n ) -> Node* {
			while ( n ) {
				Node* sib = n->getPrevNode();
				while ( sib && sib->isTextNode() ) {
					auto* tn = sib->asType<UITextNode>();
					if ( !tn->isWhitespaceOnly() )
						return sib;
					sib = sib->getPrevNode();
				}
				if ( sib && sib->isWidget() )
					return sib;
				n = n->getParent();
				if ( !n || !n->isWidget() || !n->asType<UIWidget>()->isInlineDisplay() )
					break;
			}
			return nullptr;
		};
		auto findLogicalNext = []( Node* n ) -> Node* {
			while ( n ) {
				Node* sib = n->getNextNode();
				while ( sib && sib->isTextNode() ) {
					auto* tn = sib->asType<UITextNode>();
					if ( !tn->isWhitespaceOnly() )
						return sib;
					sib = sib->getNextNode();
				}
				if ( sib && sib->isWidget() )
					return sib;
				n = n->getParent();
				if ( !n || !n->isWidget() || !n->asType<UIWidget>()->isInlineDisplay() )
					break;
			}
			return nullptr;
		};

		if ( node->isTextNode() ) {
			UITextNode* textNode = node->asType<UITextNode>();

			Node* prev = findLogicalPrev( node );
			bool prevIsInline =
				prev && prev->isWidget() && prev->asType<UIWidget>()->isInlineDisplay();

			Node* next = findLogicalNext( node );
			bool nextIsInline =
				next && next->isWidget() && next->asType<UIWidget>()->isInlineDisplay();
			auto isFloatingOrOutOfFlow = []( Node* n ) {
				if ( n == nullptr || !n->isType( UI_TYPE_HTML_WIDGET ) )
					return false;
				auto* htmlWidget = n->asType<UIHTMLWidget>();
				return htmlWidget->getCSSFloat() != CSSFloat::None || htmlWidget->isOutOfFlow();
			};

			auto collapse =
				getEffectiveWhiteSpaceCollapse( textNode->getParent(), containerRichText );
			richText.setWhiteSpaceWrapMode( toRichTextWhiteSpaceWrapMode( collapse ) );

			if ( collapse == WhiteSpaceCollapse::Collapse && textNode->isWhitespaceOnly() &&
				 ( !prevIsInline || isFloatingOrOutOfFlow( prev ) ) &&
				 ( !nextIsInline || isFloatingOrOutOfFlow( next ) ) ) {
				textNode->setLayoutCharCount( 0 );
				return;
			}

			auto text = processWhiteSpaceForLayout(
				textNode->getText().view(), collapse, prevIsInline, nextIsInline,
				lastSpanEndsWithSpace, textNode->getNextNode() == nullptr );
			auto tt = getEffectiveTextTransform( textNode );
			String transformed;
			if ( tt != TextTransform::None ) {
				transformed = materializeTransformedText( text.view, tt );
				text.view = transformed.view();
				text.owns = false;
			}

			if ( isSingleForcedLineBreak( text.view ) ) {
				textNode->setLayoutCharCount( text.length() );
				richText.addLineBreak();
				lastSpanEndsWithSpace = false;
				return;
			}

			if ( text.empty() ) {
				textNode->setLayoutCharCount( 0 );
				return;
			}

			textNode->setLayoutCharCount( text.length() );

			if ( shouldCollapseAcrossTextRuns( collapse ) )
				lastSpanEndsWithSpace = text.back() == ' ';

			FontStyleConfig style;
			if ( node->getParent()->isType( UI_TYPE_TEXTSPAN ) ) {
				style = node->getParent()->asType<UITextSpan>()->getFontStyleConfig();
			} else if ( node->getParent()->isType( UI_TYPE_RICHTEXT ) ) {
				style = node->getParent()->asType<UIRichText>()->getRichText().getFontStyleConfig();
			} else {
				style = richText.getFontStyleConfig();
			}
			if ( inlineBoxDepth > 0 ) {
				if ( tt != TextTransform::None )
					richText.addInlineText( std::move( transformed ), style, Rectf::Zero,
											Rectf::Zero, 0, {}, toRichTextTextSource( textNode ) );
				else
					addProcessedTextSpan( richText, std::move( text ), style, true,
										  toRichTextTextSource( textNode ) );
			} else {
				if ( tt != TextTransform::None )
					richText.addSpan( std::move( transformed ), style, Rectf::Zero, Rectf::Zero, 0,
									  {}, toRichTextTextSource( textNode ) );
				else
					addProcessedTextSpan( richText, std::move( text ), style, false,
										  toRichTextTextSource( textNode ) );
			}
			return;
		}

		if ( !node->isWidget() || !node->isVisible() )
			return;

		UIWidget* widget = node->asType<UIWidget>();

		// Skip <head> - it must not participate in layout
		if ( widget->isType( UI_TYPE_HTML_HEAD ) )
			return;

		bool handled = false;

		if ( widget->isType( UI_TYPE_HTML_WIDGET ) && widget->asType<UIHTMLWidget>()->isInline() &&
			 widget->asType<UIHTMLWidget>()->getCSSFloat() == CSSFloat::None &&
			 !widget->asType<UIHTMLWidget>()->isOutOfFlow() ) {
			UITextSpan* span = widget->asType<UITextSpan>();
			span->setLayoutCharCount( 0 );
			Rectf margin = span->getLayoutPixelsMargin();
			Rectf padding = span->getPixelsPadding();
			Float borderWidth = getInlineBorderWidth( span );
			Color borderColor = getInlineBorderColor( span );
			bool hasOwnText = !span->getText().empty() && NULL != span->getFontStyleConfig().Font;

			Float spanLineHeight = span->getLineHeightPx();
			Float parentLineHeight = 0;
			Node* parent = span->getParent();
			if ( parent != nullptr && parent->isType( UI_TYPE_RICHTEXT ) )
				parentLineHeight = parent->asType<UIRichText>()->getLineHeightPx();
			else if ( parent != nullptr && parent->isType( UI_TYPE_TEXTSPAN ) )
				parentLineHeight = parent->asType<UITextSpan>()->getLineHeightPx();
			if ( spanLineHeight > 0 && std::abs( spanLineHeight - parentLineHeight ) <= 0.01f )
				spanLineHeight = 0;
			if ( spanLineHeight <= 0 && span->isInlineBlock() ) {
				auto& fontStyle = span->getFontStyleConfig();
				if ( fontStyle.Font )
					spanLineHeight =
						(Float)fontStyle.Font->getFontHeight( fontStyle.CharacterSize );
			}

			auto backgroundDrawable =
				getInlineBackgroundDrawable( span, span->getFontStyleConfig().BackgroundColor );
			richText.pushInlineBox(
				margin, padding, spanLineHeight,
				toRichTextBaselineAlign( span->getBaselineAlign() ),
				span->getFontStyleConfig().BackgroundColor, borderWidth, borderColor,
				span->getTextDecoration(), toRichTextWidgetSource( span ),
				backgroundDrawable.colorDrawable, backgroundDrawable.drawable,
				getInlineBorderDrawable( span ), backgroundDrawable.usesFragmentColor );
			inlineBoxDepth++;

			if ( hasOwnText ) {
				Node* prev = findLogicalPrev( node );
				bool prevIsInline =
					prev && prev->isWidget() && prev->asType<UIWidget>()->isInlineDisplay();

				Node* next = findLogicalNext( node );
				bool nextIsInline =
					next && next->isWidget() && next->asType<UIWidget>()->isInlineDisplay();

				auto collapse = getEffectiveWhiteSpaceCollapse( span, containerRichText );
				richText.setWhiteSpaceWrapMode( toRichTextWhiteSpaceWrapMode( collapse ) );
				auto spanText = processWhiteSpaceForLayout(
					span->getText().view(), collapse, prevIsInline, nextIsInline,
					lastSpanEndsWithSpace, span->getFirstChild() == nullptr );
				auto tt = getEffectiveTextTransform( span );
				if ( tt != TextTransform::None ) {
					String transformed = materializeTransformedText( spanText.view, tt );
					spanText.setStorage( std::move( transformed ) );
				}

				if ( !spanText.empty() ) {
					auto spanTextLength = spanText.length();
					bool endsWithSpace = shouldCollapseAcrossTextRuns( collapse ) &&
										 !spanText.empty() && spanText.back() == ' ';
					addProcessedTextSpan( richText, std::move( spanText ),
										  span->getFontStyleConfig(), true );
					span->setLayoutCharCount( spanTextLength );
					if ( shouldCollapseAcrossTextRuns( collapse ) )
						lastSpanEndsWithSpace = endsWithSpace;
				} else {
					span->setLayoutCharCount( 0 );
				}
			}

			Node* spanChild = span->getFirstChild();
			while ( spanChild != NULL ) {
				bool isOutOfFlow = spanChild->isType( UI_TYPE_HTML_WIDGET ) &&
								   spanChild->asType<UIHTMLWidget>()->isOutOfFlow();
				if ( !isOutOfFlow )
					processNodeRef( spanChild, processNodeRef );
				spanChild = spanChild->getNextNode();
			}

			if ( shouldCollapseAcrossTextRuns(
					 getEffectiveWhiteSpaceCollapse( span, containerRichText ) ) &&
				 span->isInlineBlock() )
				lastSpanEndsWithSpace = true;

			inlineBoxDepth--;
			richText.popInlineBox();

			handled = true;
		}

		if ( !handled ) {
			if ( widget->isType( UI_TYPE_BR ) ) {
				richText.addSpan(
					"\n", widget->asType<UILineBreak>()->getRichText().getFontStyleConfig() );
				lastSpanEndsWithSpace = false;
			} else {
				if ( widget->hasLayoutMarginAuto() )
					widget->updateLayoutMarginAuto();
				Rectf margin =
					widget->isType( UI_TYPE_HTML_WIDGET )
						? widget->asType<UIHTMLWidget>()->getNormalFlowLayoutPixelsMargin()
						: widget->getLayoutPixelsMargin();
				bool isBlock = widget->getLayoutWidthPolicy() == SizePolicy::MatchParent;
				if ( widget->isType( UI_TYPE_HTML_WIDGET ) ) {
					CSSDisplay display = widget->asType<UIHTMLWidget>()->getDisplay();
					if ( display == CSSDisplay::Inline || display == CSSDisplay::InlineBlock )
						isBlock = false;
					else if ( display != CSSDisplay::None )
						isBlock = true;
				}

				bool fillParent =
					isBlock && widget->getLayoutWidthPolicy() == SizePolicy::MatchParent;

				if ( mode == IntrinsicMode::None ) {
					if ( fillParent ) {
						if ( container->getLayoutWidthPolicy() != SizePolicy::WrapContent &&
							 container->getPixelsSize().getWidth() != 0 ) {
							Float maxSize =
								eemax( 0.f, container->getPixelsSize().getWidth() -
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

					if ( widget->isType( UI_TYPE_TEXTSPAN ) &&
						 widget->asType<UITextSpan>()->isInlineBlock() )
						widget->asType<UITextSpan>()->updateLayout();

					if ( widget->getLayoutWidthPolicy() == SizePolicy::Fixed &&
						 widget->getUIStyle() ) {
						const StyleSheetProperty* wprop =
							widget->getUIStyle()->getProperty( PropertyId::Width );
						if ( wprop && StyleSheetLength::isPercentage( wprop->value() ) ) {
							widget->setPixelsSize( { widget->lengthFromValue( *wprop ),
													 widget->getPixelsSize().getHeight() } );
						}
					}
					if ( widget->getLayoutHeightPolicy() == SizePolicy::Fixed &&
						 widget->getUIStyle() ) {
						const StyleSheetProperty* hprop =
							widget->getUIStyle()->getProperty( PropertyId::Height );
						if ( hprop && StyleSheetLength::isPercentage( hprop->value() ) ) {
							widget->setPixelsSize( { widget->getPixelsSize().getWidth(),
													 widget->lengthFromValue( *hprop ) } );
						}
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
				if ( fillParent && mode == IntrinsicMode::None &&
					 container->getLayoutWidthPolicy() != SizePolicy::WrapContent &&
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
				bool isFloating = floatType != CSSFloat::None;
				bool isNormalFlowBlock = isBlock && !isFloating;
				bool isBlockFormattingContext =
					isNormalFlowBlock && widget->isType( UI_TYPE_HTML_WIDGET ) &&
					widget->asType<UIHTMLWidget>()->establishesBlockFormattingContext();
				bool shrinkToFitFloat =
					isFloating && widget->getLayoutWidthPolicy() == SizePolicy::MatchParent;

				if ( isNormalFlowBlock )
					richText.addLineBreak();

				if ( shrinkToFitFloat && mode == IntrinsicMode::None ) {
					container->onAutoSizeChild( widget );
					Float availableWidth = 0.f;
					if ( container->getPixelsSize().getWidth() > 0 ) {
						availableWidth = eemax( 0.f, container->getPixelsSize().getWidth() -
														 container->getPixelsContentOffset().Left -
														 container->getPixelsContentOffset().Right -
														 margin.Left - margin.Right );
					}
					Float preferredMin = widget->getMinIntrinsicWidth();
					Float preferred = widget->getMaxIntrinsicWidth();
					Float shrinkWidth = preferred;
					if ( availableWidth > 0 )
						shrinkWidth = eemin( eemax( preferredMin, availableWidth ), preferred );
					widget->setPixelsSize( shrinkWidth, widget->getPixelsSize().getHeight() );
					size = widget->getPixelsSize();
					w = shrinkWidth;
				}

				Sizef customSize( w + margin.Left + margin.Right,
								  size.getHeight() + margin.Top + margin.Bottom );
				richText.addCustomSize(
					customSize, toRichTextFloat( floatType ), toRichTextClear( clearType ),
					getAtomicInlineBoxBaseline( widget, size, margin ),
					toRichTextBaselineAlign( getWidgetBaselineAlign( widget ) ),
					toRichTextWidgetSource( widget ), isNormalFlowBlock, isBlockFormattingContext );

				if ( widget->isType( UI_TYPE_TEXTSPAN ) &&
					 widget->asType<UITextSpan>()->isInlineBlock() &&
					 widget->asType<UIRichText>()->getRichTextPtr() )
					widget->asType<UIRichText>()->getRichTextPtr()->updateLayout();

				if ( isNormalFlowBlock )
					richText.addLineBreak();
				else if ( widget->isType( UI_TYPE_TEXTSPAN ) &&
						  widget->asType<UITextSpan>()->isInlineBlock() &&
						  widget->asType<UIRichText>()->getRichTextPtr() &&
						  widget->asType<UIRichText>()->getRichTextPtr()->getLines().size() > 1 )
					richText.addLineBreak();

				lastSpanEndsWithSpace = false;
			}
		}
	};

	if ( container->isType( UI_TYPE_HTML_WIDGET ) ) {
		auto* htmlContainer = container->asType<UIHTMLWidget>();
		// RichText is the inline-formatting stream for normal block containers. Block-level
		// flex/grid containers are different: every in-flow child becomes a flex/grid item and
		// must not be folded into the parent's text stream. Keeping the parent stream empty
		// prevents duplicate paint and keeps item geometry sourced from the layouter, not from
		// inline fragments.
		//
		// Inline-flex/inline-grid are intentionally excluded here for the same reason as in
		// UIRichText::draw(): isFlex()/isGrid() includes inline display variants, but these
		// ownership exits are only valid for block-level flex/grid containers.
		CSSDisplay display = htmlContainer->getDisplay();
		if ( display == CSSDisplay::Flex || display == CSSDisplay::Grid )
			return;
	}

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
			bool packing = isPacking();
			if ( packing )
				return 1;
			auto reasons = layoutInvalidationFromMessage( Msg );

			// PaintOnly invalidations must not trigger layout.
			if ( reasons && ( reasons & ~toLayoutInvalidationFlags(
											LayoutInvalidationReason::PaintOnly ) ) == 0 ) {
				invalidateDraw();
				return 1;
			}

			bool senderIsFixed =
				Msg->getSender()->isType( UI_TYPE_HTML_WIDGET ) &&
				static_cast<const UIHTMLWidget*>( Msg->getSender() )->getCSSPosition() ==
					CSSPosition::Fixed;

			// updateOutOfFlowPosition() may set this element's own size while the scene is already
			// updating layouts. That self size-change message has already been accounted for by
			// the out-of-flow positioning routine; responding to it by relaying into
			// tryUpdateLayout() re-enters updateOutOfFlowPosition() and can recurse until stack
			// overflow. Parent notifications for absolute elements are handled by onSizeChange(),
			// so ignoring this self-message does not hide the size change from ancestors that need
			// it.
			if ( Msg->getSender() == this && isOutOfFlow() && mUISceneNode->isUpdatingLayouts() )
				return 1;

			// A block RichText owns an inline formatting stream built from its descendants.
			// During updateLayoutTree(), BlockLayouter rebuilds that stream, measures embedded
			// widgets, and positions children. Those child size/position changes naturally emit
			// LayoutAttributeChange messages back to this RichText. Re-entering this same block
			// immediately would throw away the stream being built and rebuild it again while the
			// first pass is still positioning descendants. Documents expose this as thousands of
			// redundant rebuildRichText()/onAutoSizeChild() calls per frame.
			//
			// When mUpdatingLayoutTree is true, the current pass is already incorporating these
			// descendant measurements, so the correct action is to invalidate intrinsic width
			// caches and absorb the message. Unlike the original guard, the typed invalidation path
			// keeps the reasons for onLayoutUpdate(), so async/resource/layout changes that still
			// matter after the active pass can be replayed without re-entering the current rebuild.
			if ( !senderIsFixed && mUISceneNode->isUpdatingLayouts() && mUpdatingLayoutTree &&
				 !isOutOfFlow() ) {
				invalidateIntrinsicSize();
				mDeferredLayoutReasons |= reasons;
				return 1;
			}

			// A descendant change invalidates the inline/block formatting stream owned by this
			// RichText. Do not immediately relay that invalidation to the parent: the parent's
			// layout depends on this RichText's box, not on the raw descendant event. Relaying
			// first lets dirty-layout coalescing replace this dirty RichText with an ancestor
			// layout, and generic ancestors such as UILinearLayout measure children before the
			// recursive child update runs. Delayed image loads then leave the ancestor with the old
			// RichText height until an unrelated hover/resize causes another invalidation.
			//
			// Rebuild this RichText instead. If the rebuilt stream changes the RichText box,
			// UIRichText::onSizeChange() will notify ancestors with the actual size change; if the
			// box does not change, no ancestor layout work was necessary. Fixed-position
			// descendants are ignored below because they are viewport-relative and do not affect
			// this normal-flow box.
			if ( reasons & toLayoutInvalidationFlags( LayoutInvalidationReason::IntrinsicSize ) )
				invalidateIntrinsicSize();

			if ( !senderIsFixed )
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

void UIRichText::onLayoutUpdate() {
	// This trashes too much the layout, there must be a better way, also, all tests pass without
	// this, and also I cannot find an example where this ends up being 100% necessary.
	/* if ( mDeferredLayoutReasons ) {
		LayoutInvalidationFlags deferred = mDeferredLayoutReasons;
		mDeferredLayoutReasons = 0;

		LayoutInvalidationFlags nonPaint =
			deferred & ~toLayoutInvalidationFlags( LayoutInvalidationReason::PaintOnly );

		if ( nonPaint ) {
			const Sizef oldSize = getPixelsSize();
			const LayoutInvalidationFlags formattingReasons =
				toLayoutInvalidationFlags( LayoutInvalidationReason::IntrinsicSize ) |
				toLayoutInvalidationFlags( LayoutInvalidationReason::FormattingContext ) |
				toLayoutInvalidationFlags( LayoutInvalidationReason::Style ) |
				toLayoutInvalidationFlags( LayoutInvalidationReason::DocumentExtent ) |
				toLayoutInvalidationFlags( LayoutInvalidationReason::Viewport );
			if ( nonPaint & formattingReasons )
				tryUpdateLayout();
			if ( nonPaint & toLayoutInvalidationFlags( LayoutInvalidationReason::OutOfFlowChild ) )
				updateOutOfFlowPosition();
			if ( oldSize != getPixelsSize() ) {
				LayoutInvalidationFlags parentReasons = LayoutInvalidation::ParentChildChange;
				if ( nonPaint &
					 toLayoutInvalidationFlags( LayoutInvalidationReason::FormattingContext ) )
					parentReasons |=
						toLayoutInvalidationFlags( LayoutInvalidationReason::FormattingContext );
				notifyLayoutAttrChangeParent( parentReasons );
			}
		}
	} */
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
