#include <eepp/graphics/font.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uitextedit.hpp>
#include <eepp/ui/uithememanager.hpp>

namespace EE { namespace UI {

UITextEdit* UITextEdit::New() {
	return eeNew( UITextEdit, ( "textedit" ) );
}

UITextEdit* UITextEdit::NewWithTag( const std::string& tag ) {
	return eeNew( UITextEdit, ( tag ) );
}

UITextEdit::UITextEdit( const std::string& tag ) : UICodeEditor( tag, true, true ) {
	setFlags( UI_AUTO_PADDING );
	setClipType( ClipType::ContentBox );
	mFont = NULL;
	mHorizontalScrollBarEnabled = true;
	mKerningEnabled = true;

	UITheme* theme = getUISceneNode()->getUIThemeManager()->getDefaultTheme();

	if ( NULL != theme && NULL != theme->getDefaultFont() ) {
		setFont( theme->getDefaultFont() );
	}

	if ( NULL != theme ) {
		setFontSize( theme->getDefaultFontSize() );
	} else {
		setFontSize( getUISceneNode()->getUIThemeManager()->getDefaultFontSize() );
	}

	if ( NULL == getFont() ) {
		if ( NULL != getUISceneNode()->getUIThemeManager()->getDefaultFont() ) {
			setFont( getUISceneNode()->getUIThemeManager()->getDefaultFont() );
		} else {
			Log::error( "UITextEdit::UITextEdit : Created a without a defined font." );
		}
	}

	mHintCache.setTextHints( getWidgetTextDrawHints() );
	setHintFont( theme && theme->getDefaultFont() ? theme->getDefaultFont() : getFont() );
	setHintFontSize( theme ? theme->getDefaultFontSize()
						   : getUISceneNode()->getUIThemeManager()->getDefaultFontSize() );

	disableEditorFeatures();
	applyDefaultTheme();
}

UITextEdit::~UITextEdit() {}

Uint32 UITextEdit::getType() const {
	return UI_TYPE_TEXTEDIT;
}

bool UITextEdit::isType( const Uint32& type ) const {
	return UITextEdit::getType() == type ? true : UICodeEditor::isType( type );
}

void UITextEdit::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "textedit" );

	onThemeLoaded();
}

String UITextEdit::getText() const {
	return mDoc->getText();
}

void UITextEdit::setText( const String& text ) {
	mDoc->reset();
	mDoc->textInput( text );
	if ( !hasFocus() )
		mCursorVisible = false;
	invalidateLongestLineWidth();
}

void UITextEdit::setWordWrap( bool enabled ) {
	if ( enabled )
		setFlags( UI_WORD_WRAP );
	else
		unsetFlags( UI_WORD_WRAP );

	setLineWrapKeepIndentation( false );
	setLineWrapType( LineWrapType::Viewport );
	setLineWrapMode( enabled ? LineWrapMode::Word : LineWrapMode::NoWrap );
}

const String& UITextEdit::getHint() const {
	return mHintCache.getString();
}

UITextEdit* UITextEdit::setHint( const String& hint ) {
	if ( hint != mHintCache.getString() ) {
		mHintCache.setString( hint );
		invalidateDraw();
	}
	return this;
}

const Color& UITextEdit::getHintColor() const {
	return mHintStyleConfig.getFontColor();
}

UITextEdit* UITextEdit::setHintColor( const Color& hintColor ) {
	if ( hintColor != mHintStyleConfig.getFontColor() ) {
		mHintCache.setFillColor( hintColor );
		mHintStyleConfig.FontColor = hintColor;
		invalidateDraw();
	}
	return this;
}

const Color& UITextEdit::getHintShadowColor() const {
	return mHintStyleConfig.getFontShadowColor();
}

UITextEdit* UITextEdit::setHintShadowColor( const Color& shadowColor ) {
	if ( shadowColor != mHintStyleConfig.getFontShadowColor() ) {
		mHintCache.setShadowColor( shadowColor );
		mHintStyleConfig.ShadowColor = shadowColor;
		invalidateDraw();
	}
	return this;
}

const Vector2f& UITextEdit::getHintShadowOffset() const {
	return mHintStyleConfig.getFontShadowOffset();
}

UITextEdit* UITextEdit::setHintShadowOffset( const Vector2f& shadowOffset ) {
	if ( shadowOffset != mHintStyleConfig.getFontShadowOffset() ) {
		mHintCache.setShadowOffset( shadowOffset );
		mHintStyleConfig.ShadowOffset = shadowOffset;
		invalidateDraw();
	}
	return this;
}

Font* UITextEdit::getHintFont() const {
	return mHintStyleConfig.getFont();
}

UITextEdit* UITextEdit::setHintFont( Font* font ) {
	if ( font != mHintStyleConfig.getFont() ) {
		mHintCache.setFont( font );
		mHintStyleConfig.Font = font;
		invalidateDraw();
	}
	return this;
}

Uint32 UITextEdit::getHintFontSize() const {
	return mHintCache.getCharacterSize();
}

UITextEdit* UITextEdit::setHintFontSize( const Uint32& characterSize ) {
	if ( characterSize != mHintCache.getCharacterSize() ) {
		mHintCache.setFontSize( characterSize );
		mHintStyleConfig.CharacterSize = characterSize;
		invalidateDraw();
	}
	return this;
}

const Uint32& UITextEdit::getHintFontStyle() const {
	return mHintStyleConfig.Style;
}

UITextEdit* UITextEdit::setHintFontStyle( const Uint32& fontStyle ) {
	if ( fontStyle != mHintStyleConfig.Style ) {
		mHintCache.setStyle( fontStyle );
		mHintStyleConfig.Style = fontStyle;
		invalidateDraw();
	}
	return this;
}

const Float& UITextEdit::getHintOutlineThickness() const {
	return mHintStyleConfig.OutlineThickness;
}

UITextEdit* UITextEdit::setHintOutlineThickness( const Float& outlineThickness ) {
	if ( outlineThickness != mHintStyleConfig.OutlineThickness ) {
		mHintCache.setOutlineThickness( outlineThickness );
		mHintStyleConfig.OutlineThickness = outlineThickness;
		invalidateDraw();
	}
	return this;
}

const Color& UITextEdit::getHintOutlineColor() const {
	return mHintStyleConfig.OutlineColor;
}

UITextEdit* UITextEdit::setHintOutlineColor( const Color& outlineColor ) {
	if ( outlineColor != mHintStyleConfig.OutlineColor ) {
		mHintStyleConfig.OutlineColor = outlineColor;
		Color color( outlineColor.r, outlineColor.g, outlineColor.b,
					 outlineColor.a * mAlpha / 255.f );
		mHintCache.setOutlineColor( color );
		invalidateDraw();
	}
	return this;
}

void UITextEdit::setHintDisplay( HintDisplay display ) {
	if ( display != mHintDisplay ) {
		mHintDisplay = display;
		invalidateDraw();
	}
}

HintDisplay UITextEdit::getHintDisplay() const {
	return mHintDisplay;
}

bool UITextEdit::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
		case PropertyId::Value:
			setText( attribute.value() );
			break;
		case PropertyId::Wordwrap:
			setWordWrap( attribute.asBool() );
			break;
		case PropertyId::Hint:
			setHint( getTranslatorString( attribute.value() ) );
			break;
		case PropertyId::HintColor:
			setHintColor( attribute.asColor() );
			break;
		case PropertyId::HintShadowColor:
			setHintShadowColor( attribute.asColor() );
			break;
		case PropertyId::HintShadowOffset:
			setHintShadowOffset( attribute.asVector2f() );
			break;
		case PropertyId::HintFontSize:
			setHintFontSize( lengthFromValue( attribute ) );
			break;
		case PropertyId::HintFontFamily:
			setHintFont(
				getUISceneNode()
					? getUISceneNode()->getResourceScope()->findFont( attribute.value() ).get()
					: nullptr );
			break;
		case PropertyId::HintFontStyle:
			setHintFontStyle( attribute.asFontStyle() );
			break;
		case PropertyId::HintStrokeWidth:
			setHintOutlineThickness( PixelDensity::dpToPx( attribute.asDpDimension() ) );
			break;
		case PropertyId::HintStrokeColor:
			setHintOutlineColor( attribute.asColor() );
			break;
		case PropertyId::HintDisplay:
			setHintDisplay( String::toLower( attribute.asString() ) == "focus"
								? HintDisplay::Focus
								: HintDisplay::Always );
			break;
		default:
			return UICodeEditor::applyProperty( attribute );
	}

	return true;
}

std::string UITextEdit::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Hint:
			return getHint().toUtf8();
		case PropertyId::HintColor:
			return getHintColor().toHexString();
		case PropertyId::HintShadowColor:
			return getHintShadowColor().toHexString();
		case PropertyId::HintShadowOffset:
			return String::fromFloat( getHintShadowOffset().x ) + " " +
				   String::fromFloat( getHintShadowOffset().y );
		case PropertyId::HintFontSize:
			return String::format( "%ddp", getHintFontSize() );
		case PropertyId::HintFontFamily:
			return getHintFont() ? getUISceneNode()->getFontFamilyName( getHintFont() ) : "";
		case PropertyId::HintFontStyle:
			return Text::styleFlagToString( getHintFontStyle() );
		case PropertyId::HintStrokeWidth:
			return String::fromFloat( PixelDensity::dpToPx( getHintOutlineThickness() ), "px" );
		case PropertyId::HintStrokeColor:
			return getHintOutlineColor().toHexString();
		case PropertyId::HintDisplay:
			return mHintDisplay == HintDisplay::Always ? "always" : "focus";
		default:
			return UICodeEditor::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UITextEdit::getPropertiesImplemented() const {
	auto props = UICodeEditor::getPropertiesImplemented();
	auto local = { PropertyId::Hint,
				   PropertyId::HintColor,
				   PropertyId::HintShadowColor,
				   PropertyId::HintShadowOffset,
				   PropertyId::HintFontSize,
				   PropertyId::HintFontFamily,
				   PropertyId::HintFontStyle,
				   PropertyId::HintStrokeWidth,
				   PropertyId::HintStrokeColor,
				   PropertyId::HintDisplay };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

void UITextEdit::drawLineText( const Int64& line, Vector2f position, const Float& fontSize,
							   const Float& lineHeight,
							   const DocumentViewLineRange& visibleLineRange ) {
	UICodeEditor::drawLineText( line, position, fontSize, lineHeight, visibleLineRange );
	if ( line == 0 && mDoc->isEmpty() && !mHintCache.getString().empty() &&
		 ( mHintDisplay == HintDisplay::Always || hasFocus() ) ) {
		mHintCache.draw( std::trunc( position.x ), std::trunc( position.y ), Vector2f::One, 0.f,
						 getBlendMode() );
	}
}

void UITextEdit::drawCursor( const Vector2f& startScroll, const Float& lineHeight,
							 const TextPosition& cursor ) {
	if ( mCursorVisible && !mLocked && isTextSelectionEnabled() ) {
		auto offset = getTextPositionOffset( cursor, {}, false, false,
											 Text::LigatureCaretMode::ClosestGlyph );
		Vector2f cursorPos( startScroll.x + offset.x, startScroll.y + offset.y );
		Primitives primitives;
		primitives.setColor( Color( mFontStyleConfig.FontColor ).blendAlpha( mAlpha ) );
		primitives.drawRectangle(
			Rectf( cursorPos, Sizef( PixelDensity::dpToPx( 1 ), lineHeight ) ) );
	}
}

void UITextEdit::onTextHintsChanged() {
	UICodeEditor::onTextHintsChanged();
	mHintCache.setTextHints( getWidgetTextDrawHints() );
}

}} // namespace EE::UI
