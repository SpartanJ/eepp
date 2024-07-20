#include <eepp/graphics/font.hpp>
#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/scene/actions/actions.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/engine.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

using namespace std::literals;

namespace EE { namespace UI {

UITextView* UITextView::New() {
	return eeNew( UITextView, () );
}

UITextView* UITextView::NewWithTag( const std::string& tag ) {
	return eeNew( UITextView, ( tag ) );
}

UITextView::UITextView( const std::string& tag ) :
	UIWidget( tag ),
	mRealAlignOffset( 0.f, 0.f ),
	mSelCurInit( 0 ),
	mSelCurEnd( 0 ),
	mLastSelCurInit( 0 ),
	mLastSelCurEnd( 0 ),
	mSelecting( false ) {
	mTextCache = Text::New();

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

	applyDefaultTheme();
}

UITextView::UITextView() : UITextView( "textview" ) {}

UITextView::~UITextView() {
	eeSAFE_DELETE( mTextCache );
}

Uint32 UITextView::getType() const {
	return UI_TYPE_TEXTVIEW;
}

bool UITextView::isType( const Uint32& type ) const {
	return UITextView::getType() == type ? true : UIWidget::isType( type );
}

void UITextView::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UINode::draw();

		if ( mTextCache->getTextWidth() <= 0.f )
			return;

		drawSelection( mTextCache );

		if ( isClipped() ) {
			clipSmartEnable( mScreenPos.x + mPaddingPx.Left, mScreenPos.y + mPaddingPx.Top,
							 mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right,
							 mSize.getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );
		}

		mTextCache->setAlign( Font::getHorizontalAlign( getFlags() ) );
		mTextCache->draw( (Float)mScreenPosi.x + (int)mRealAlignOffset.x + (int)mPaddingPx.Left,
						  (Float)mScreenPosi.y + (int)mRealAlignOffset.y + (int)mPaddingPx.Top,
						  Vector2f::One, 0.f, getBlendMode() );

		if ( isClipped() )
			clipSmartDisable();
	}
}

Graphics::Font* UITextView::getFont() const {
	return mTextCache->getFont();
}

UITextView* UITextView::setFont( Graphics::Font* font ) {
	if ( NULL != font && mTextCache->getFont() != font ) {
		mTextCache->setFont( font );
		mFontStyleConfig.Font = font;
		recalculate();
		onFontChanged();
		notifyLayoutAttrChange();
		invalidateDraw();
	}

	return this;
}

Uint32 UITextView::getFontSize() const {
	return mTextCache->getCharacterSize();
}

UITextView* UITextView::setFontSize( const Uint32& characterSize ) {
	if ( mTextCache->getCharacterSize() != characterSize ) {
		mFontStyleConfig.CharacterSize = characterSize;
		mTextCache->setFontSize( characterSize );
		recalculate();
		onFontStyleChanged();
		notifyLayoutAttrChange();
		invalidateDraw();
	}

	return this;
}

const Uint32& UITextView::getFontStyle() const {
	return mFontStyleConfig.Style;
}

const Float& UITextView::getOutlineThickness() const {
	return mFontStyleConfig.OutlineThickness;
}

UITextView* UITextView::setOutlineThickness( const Float& outlineThickness ) {
	if ( mFontStyleConfig.OutlineThickness != outlineThickness ) {
		mTextCache->setOutlineThickness( outlineThickness );
		mFontStyleConfig.OutlineThickness = outlineThickness;
		recalculate();
		onFontStyleChanged();
		notifyLayoutAttrChange();
		invalidateDraw();
	}

	return this;
}

const Color& UITextView::getOutlineColor() const {
	return mFontStyleConfig.OutlineColor;
}

UITextView* UITextView::setOutlineColor( const Color& outlineColor ) {
	if ( mFontStyleConfig.OutlineColor != outlineColor ) {
		mFontStyleConfig.OutlineColor = outlineColor;
		Color newColor( outlineColor.r, outlineColor.g, outlineColor.b,
						outlineColor.a * mAlpha / 255.f );
		mTextCache->setOutlineColor( newColor );
		onFontStyleChanged();
		invalidateDraw();
	}

	return this;
}

UITextView* UITextView::setFontStyle( const Uint32& fontStyle ) {
	if ( mFontStyleConfig.Style != fontStyle ) {
		mTextCache->setStyle( fontStyle );
		mFontStyleConfig.Style = fontStyle;
		recalculate();
		onFontStyleChanged();
		notifyLayoutAttrChange();
		invalidateDraw();
	}

	return this;
}

const String& UITextView::getText() const {
	if ( mFlags & UI_WORD_WRAP )
		return mString;

	return mTextCache->getString();
}

UITextView* UITextView::setText( const String& text ) {
	if ( mString != text ) {
		mString = text;
		mTextCache->setString( mString );

		recalculate();
		onTextChanged();
		notifyLayoutAttrChange();
	}

	return this;
}

const Color& UITextView::getFontColor() const {
	return mFontStyleConfig.FontColor;
}

UITextView* UITextView::setFontColor( const Color& color ) {
	if ( mFontStyleConfig.FontColor != color ) {
		mFontStyleConfig.FontColor = color;
		Color newColor( color.r, color.g, color.b, color.a * mAlpha / 255.f );
		mTextCache->setFillColor( newColor );
		invalidateDraw();
	}

	return this;
}

UITextView* UITextView::setFontFillColor( const Color& color, Uint32 from, Uint32 to ) {
	Color newColor( color.r, color.g, color.b, color.a * mAlpha / 255.f );
	mTextCache->setFillColor( newColor, from, to );
	invalidateDraw();
	return this;
}

const Text* UITextView::getTextCache() const {
	return mTextCache;
}

Text* UITextView::getTextCache() {
	return mTextCache;
}

const Vector2f& UITextView::getRealAlignOffset() const {
	return mRealAlignOffset;
}

const TextTransform::Value& UITextView::getTextTransform() const {
	return mTextTransform;
}

void UITextView::transformText() {
	mTextCache->setString( mString );
	mTextCache->transformText( mTextTransform );
}

void UITextView::setTextTransform( const TextTransform::Value& textTransform ) {
	if ( textTransform != mTextTransform ) {
		mTextTransform = textTransform;
		transformText();
		recalculate();
	}
}

const Color& UITextView::getFontShadowColor() const {
	return mFontStyleConfig.ShadowColor;
}

UITextView* UITextView::setFontShadowColor( const Color& color ) {
	if ( mFontStyleConfig.ShadowColor != color ) {
		mFontStyleConfig.ShadowColor = color;
		if ( mFontStyleConfig.ShadowColor != Color::Transparent )
			mFontStyleConfig.Style |= Text::Shadow;
		else
			mFontStyleConfig.Style &= ~Text::Shadow;
		Color newColor( color.r, color.g, color.b, color.a * mAlpha / 255.f );
		mTextCache->setShadowColor( newColor );
		mTextCache->setStyle( mFontStyleConfig.Style );
		onFontStyleChanged();
		recalculate();
		invalidateDraw();
	}

	return this;
}

const Vector2f& UITextView::getFontShadowOffset() const {
	return mFontStyleConfig.ShadowOffset;
}

UITextView* UITextView::setFontShadowOffset( const Vector2f& offset ) {
	if ( mFontStyleConfig.ShadowOffset != offset ) {
		mFontStyleConfig.ShadowOffset = offset;
		mTextCache->setShadowOffset( offset );
		onFontStyleChanged();
		invalidateDraw();
	}

	return this;
}

const Color& UITextView::getSelectionBackColor() const {
	return mFontStyleConfig.FontSelectionBackColor;
}

UITextView* UITextView::setSelectionBackColor( const Color& color ) {
	if ( mFontStyleConfig.FontSelectionBackColor != color ) {
		mFontStyleConfig.FontSelectionBackColor = color;
		onFontStyleChanged();
		invalidateDraw();
	}

	return this;
}

void UITextView::autoWrap() {
	if ( mFlags & UI_WORD_WRAP ) {
		wrapText( mSize.getWidth() );
	}
}

void UITextView::wrapText( const Uint32& maxWidth ) {
	if ( mFlags & UI_WORD_WRAP ) {
		mTextCache->setString( mString );
	}

	mTextCache->wrapText( maxWidth );
	invalidateDraw();
}

void UITextView::onAutoSize() {
	bool sizeChanged = false;

	if ( ( mFlags & UI_AUTO_SIZE && 0 == getSize().getWidth() ) ) {
		setInternalPixelsSize( Sizef( getTextWidth(), mTextCache->getTextHeight() ) );
		sizeChanged = true;
	}

	if ( mWidthPolicy == SizePolicy::WrapContent ) {
		Float totW = (int)getTextWidth() + mPaddingPx.Left + mPaddingPx.Right;

		if ( !getMaxWidthEq().empty() ) {
			Float oldW = totW;
			totW = eemin( totW, getMaxSizePx().getWidth() );
			if ( oldW != totW )
				setClipType( ClipType::ContentBox );
		}
		if ( mSize.x != totW ) {
			setInternalPixelsWidth( totW );
			sizeChanged = true;
		}
	}

	if ( mHeightPolicy == SizePolicy::WrapContent ) {
		Float totH = (int)mTextCache->getTextHeight() + mPaddingPx.Top + mPaddingPx.Bottom;
		if ( !getMaxHeightEq().empty() ) {
			Float oldH = totH;
			totH = eemin( totH, getMaxSizePx().getHeight() );
			if ( oldH != totH )
				setClipType( ClipType::ContentBox );
		}
		if ( mSize.y != totH ) {
			setInternalPixelsHeight( totH );
			sizeChanged = true;
		}
	}

	if ( sizeChanged ) {
		updateTextOverflow();
		notifyLayoutAttrChange();
	}
}

void UITextView::alignFix() {
	switch ( Font::getHorizontalAlign( getFlags() ) ) {
		case UI_HALIGN_CENTER:
			mRealAlignOffset.x =
				(Float)( (Int32)( ( mSize.x - mPaddingPx.Left - mPaddingPx.Right ) * 0.5f -
								  getTextWidth() * 0.5f ) );
			break;
		case UI_HALIGN_RIGHT:
			mRealAlignOffset.x =
				( (Float)mSize.x - mPaddingPx.Left - mPaddingPx.Right - (Float)getTextWidth() );
			break;
		case UI_HALIGN_LEFT:
			mRealAlignOffset.x = 0;
			break;
	}

	switch ( Font::getVerticalAlign( getFlags() ) ) {
		case UI_VALIGN_CENTER: {
			Float height = mSize.y - mPaddingPx.Top - mPaddingPx.Bottom;
			Float fontSize = mTextCache->getCharacterSize();
			Float lineSpacing = mTextCache->getFont()->getLineSpacing( fontSize );
			Float center = eefloor( ( lineSpacing - fontSize ) * 0.5f );
			Float textHeight = mTextCache->getTextHeight();
			mRealAlignOffset.y = eefloor( ( height - textHeight + center ) * 0.5f );
			break;
		}
		case UI_VALIGN_BOTTOM:
			mRealAlignOffset.y = ( (Float)mSize.y - mPaddingPx.Top - mPaddingPx.Bottom -
								   (Float)mTextCache->getTextHeight() );
			break;
		case UI_VALIGN_TOP:
			mRealAlignOffset.y = 0;
			break;
	}
}

Uint32 UITextView::onFocusLoss() {
	UIWidget::onFocusLoss();

	selCurEnd( selCurInit() );
	onSelectionChange();

	return 1;
}

void UITextView::onSizeChange() {
	recalculate();
	UINode::onSizeChange();
}

void UITextView::onTextChanged() {
	sendCommonEvent( Event::OnTextChanged );
	sendCommonEvent( Event::OnValueChange );
	invalidateDraw();
}

void UITextView::onFontChanged() {
	sendCommonEvent( Event::OnFontChanged );
	invalidateDraw();
}

void UITextView::onFontStyleChanged() {
	sendCommonEvent( Event::OnFontStyleChanged );
	invalidateDraw();
}

void UITextView::onAlphaChange() {
	Color color( getFontColor() );
	Color newColor( color.r, color.g, color.b, color.a * mAlpha / 255.f );
	mTextCache->setFillColor( newColor );

	color = getFontShadowColor();
	newColor = Color( color.r, color.g, color.b, color.a * mAlpha / 255.f );
	mTextCache->setShadowColor( newColor );

	color = getOutlineColor();
	newColor = Color( color.r, color.g, color.b, color.a * mAlpha / 255.f );
	mTextCache->setOutlineColor( newColor );

	invalidateDraw();
}

void UITextView::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	onThemeLoaded();
}

Float UITextView::getTextWidth() {
	return hasTextOverflow() ? Text::getTextWidth( mString, mFontStyleConfig )
							 : mTextCache->getTextWidth();
}

Float UITextView::getTextHeight() {
	return mTextCache->getTextHeight();
}

Uint32 UITextView::getNumLines() {
	return mTextCache->getNumLines();
}

Vector2f UITextView::getAlignOffset() const {
	return PixelDensity::pxToDp( mRealAlignOffset );
}

Uint32 UITextView::onMouseDoubleClick( const Vector2i& Pos, const Uint32& Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		Vector2f nodePos( Vector2f( Pos.x, Pos.y ) );
		worldToNode( nodePos );
		nodePos = PixelDensity::dpToPx( nodePos ) - mRealAlignOffset -
				  Vector2f( mPaddingPx.Left, mPaddingPx.Top );
		nodePos.x = eemax( 0.f, nodePos.x );
		nodePos.y = eemax( 0.f, nodePos.y );

		Int32 curPos = mTextCache->findCharacterFromPos( nodePos.asInt() );

		if ( -1 != curPos ) {
			Int32 tSelCurInit, tSelCurEnd;

			getVisibleTextCache()->findWordFromCharacterIndex( curPos, tSelCurInit, tSelCurEnd );

			selCurInit( tSelCurEnd );
			selCurEnd( tSelCurInit );
			onSelectionChange();

			mSelecting = false;
		}
	}

	return UIWidget::onMouseDoubleClick( Pos, Flags );
}

Uint32 UITextView::onMouseClick( const Vector2i& Pos, const Uint32& Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		mSelecting = false;
	}

	return UIWidget::onMouseClick( Pos, Flags );
}

Uint32 UITextView::onMouseDown( const Vector2i& Pos, const Uint32& Flags ) {
	if ( NULL != getEventDispatcher() && isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) &&
		 getEventDispatcher()->getMouseDownNode() == this ) {
		Vector2f nodePos( Vector2f( Pos.x, Pos.y ) );
		worldToNode( nodePos );
		nodePos = PixelDensity::dpToPx( nodePos ) - mRealAlignOffset -
				  Vector2f( mPaddingPx.Left, mPaddingPx.Top );
		nodePos.x = eemax( 0.f, nodePos.x );
		nodePos.y = eemax( 0.f, nodePos.y );

		Int32 curPos = getVisibleTextCache()->findCharacterFromPos( nodePos.asInt() );

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

	return UIWidget::onMouseDown( Pos, Flags );
}

void UITextView::drawSelection( Text* textCache ) {
	if ( selCurInit() != selCurEnd() ) {
		Int32 init = eemin( selCurInit(), selCurEnd() );
		Int32 end = eemax( selCurInit(), selCurEnd() );

		if ( init < 0 && end > (Int32)textCache->getString().size() ) {
			return;
		}

		Int32 lastEnd;
		Vector2f initPos, endPos;

		if ( mLastSelCurInit != selCurInit() || mLastSelCurEnd != selCurEnd() ) {
			mSelPosCache.clear();
			mLastSelCurInit = selCurInit();
			mLastSelCurEnd = selCurEnd();

			do {
				initPos = textCache->findCharacterPos( init );
				lastEnd = textCache->getString().find_first_of( '\n', init );

				if ( lastEnd < end && -1 != lastEnd ) {
					endPos = textCache->findCharacterPos( lastEnd );
					init = lastEnd + 1;
				} else {
					endPos = textCache->findCharacterPos( end );
					lastEnd = end;
				}

				mSelPosCache.push_back( SelPosCache( initPos, endPos ) );
			} while ( end != lastEnd );
		}

		if ( !mSelPosCache.empty() ) {
			Primitives P;
			P.setColor( mFontStyleConfig.FontSelectionBackColor );
			Float vspace = textCache->getFont()->getLineSpacing( mTextCache->getCharacterSize() );
			Float height = mSize.y - mPaddingPx.Top - mPaddingPx.Bottom;
			Float offsetY = eefloor( ( height - mTextCache->getTextHeight() ) * 0.5f );

			for ( size_t i = 0; i < mSelPosCache.size(); i++ ) {
				initPos = mSelPosCache[i].initPos;
				endPos = mSelPosCache[i].endPos;

				P.drawRectangle(
					Rectf( mScreenPos.x + initPos.x + mRealAlignOffset.x + mPaddingPx.Left,
						   mScreenPos.y + initPos.y + offsetY + mPaddingPx.Top,
						   mScreenPos.x + endPos.x + mRealAlignOffset.x + mPaddingPx.Left,
						   mScreenPos.y + endPos.y + offsetY + mPaddingPx.Top + vspace ) );
			}
		}
	}
}

bool UITextView::isTextSelectionEnabled() const {
	return 0 != ( mFlags & UI_TEXT_SELECTION_ENABLED );
}

void UITextView::setTextSelection( const bool& active ) {
	if ( active ) {
		mFlags |= UI_TEXT_SELECTION_ENABLED;
	} else {
		mFlags &= ~UI_TEXT_SELECTION_ENABLED;
	}
}

const UIFontStyleConfig& UITextView::getFontStyleConfig() const {
	return mFontStyleConfig;
}

void UITextView::setFontStyleConfig( const UIFontStyleConfig& fontStyleConfig ) {
	mFontStyleConfig = fontStyleConfig;
	setFont( fontStyleConfig.getFont() );
	setFontSize( fontStyleConfig.getFontCharacterSize() );
	setFontColor( fontStyleConfig.getFontColor() );
	setFontShadowColor( fontStyleConfig.getFontShadowColor() );
	setOutlineThickness( fontStyleConfig.getOutlineThickness() );
	setOutlineColor( fontStyleConfig.getOutlineColor() );
	setFontStyle( fontStyleConfig.getFontStyle() );
	onFontStyleChanged();
}

void UITextView::selCurInit( const Int32& init ) {
	if ( mSelCurInit != init ) {
		mSelCurInit = init;
		invalidateDraw();
	}
}

void UITextView::selCurEnd( const Int32& end ) {
	if ( mSelCurEnd != end ) {
		mSelCurEnd = end;
		invalidateDraw();
	}
}

Int32 UITextView::selCurInit() {
	return mSelCurInit;
}

Int32 UITextView::selCurEnd() {
	return mSelCurEnd;
}

void UITextView::onAlignChange() {
	UIWidget::onAlignChange();
	alignFix();
}

void UITextView::onSelectionChange() {
	mTextCache->invalidateColors();

	if ( selCurInit() != selCurEnd() ) {
		Color color( mFontStyleConfig.getFontSelectedColor() );
		color.a = mFontStyleConfig.getFontSelectedColor().a * mAlpha / 255.f;
		mTextCache->setFillColor( color, eemin<Int32>( selCurInit(), selCurEnd() ),
								  eemax<Int32>( selCurInit(), selCurEnd() ) - 1 );
	} else {
		Color color( mFontStyleConfig.getFontColor() );
		color.a = mFontStyleConfig.getFontColor().a * mAlpha / 255.f;
		mTextCache->setFillColor( color );
	}

	invalidateDraw();
}

Text* UITextView::getVisibleTextCache() const {
	return mTextCache;
}

void UITextView::recalculate() {
	updateTextOverflow();
	autoWrap();
	onAutoSize();
	alignFix();
	resetSelCache();
}

void UITextView::resetSelCache() {
	mLastSelCurInit = mLastSelCurEnd = 0;
	onSelectionChange();
}

bool UITextView::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
			setText( getTranslatorString( attribute.value() ) );
			break;
		case PropertyId::TextTransform:
			if ( !mUsingCustomStyling )
				setTextTransform( TextTransform::fromString( attribute.asString() ) );
			break;
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
		case PropertyId::SelectionColor:
			if ( !mUsingCustomStyling )
				mFontStyleConfig.FontSelectedColor = attribute.asColor();
			break;
		case PropertyId::SelectionBackColor:
			if ( !mUsingCustomStyling )
				setSelectionBackColor( attribute.asColor() );
			break;
		case PropertyId::FontFamily: {
			Font* font = FontManager::instance()->getByName( attribute.value() );

			if ( !mUsingCustomStyling && NULL != font && font->loaded() ) {
				setFont( font );
			}
			break;
		}
		case PropertyId::FontSize:
			if ( !mUsingCustomStyling )
				setFontSize( lengthFromValue( attribute ) );
			break;
		case PropertyId::FontStyle: {
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
		}
		case PropertyId::Wordwrap:
			if ( attribute.asBool() )
				mFlags |= UI_WORD_WRAP;
			else
				mFlags &= ~UI_WORD_WRAP;
			autoWrap();
			break;
		case PropertyId::TextStrokeWidth:
			if ( !mUsingCustomStyling )
				setOutlineThickness( lengthFromValue( attribute ) );
			break;
		case PropertyId::TextStrokeColor:
			if ( !mUsingCustomStyling )
				setOutlineColor( attribute.asColor() );
			break;
		case PropertyId::TextSelection:
			if ( !mUsingCustomStyling )
				setTextSelection( attribute.asBool() );
			break;
		case PropertyId::TextAlign: {
			std::string align = String::toLower( attribute.value() );
			if ( align == "center" )
				setTextAlign( UI_HALIGN_CENTER );
			else if ( align == "left" )
				setTextAlign( UI_HALIGN_LEFT );
			else if ( align == "right" )
				setTextAlign( UI_HALIGN_RIGHT );
			break;
		}
		case PropertyId::TextOverflow:
			setTextOverflow( attribute.value() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

std::string UITextView::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Text:
			return getText().toUtf8();
		case PropertyId::TextTransform:
			return TextTransform::toString( getTextTransform() );
		case PropertyId::Color:
			return getFontColor().toHexString();
		case PropertyId::TextShadowColor:
			return getFontShadowColor().toHexString();
		case PropertyId::TextShadowOffset:
			return String::fromFloat( getFontShadowOffset().x ) + " " +
				   String::fromFloat( getFontShadowOffset().y );
		case PropertyId::SelectionColor:
			return mFontStyleConfig.FontSelectedColor.toHexString();
		case PropertyId::SelectionBackColor:
			return getSelectionBackColor().toHexString();
		case PropertyId::FontFamily:
			return NULL != getFont() ? getFont()->getName() : "";
		case PropertyId::FontSize:
			return String::format( "%dpx", getFontSize() );
		case PropertyId::FontStyle:
			return Text::styleFlagToString( getFontStyle() );
		case PropertyId::TextStrokeWidth:
			return String::fromFloat( PixelDensity::dpToPx( getOutlineThickness() ), "px" );
		case PropertyId::TextStrokeColor:
			return getOutlineColor().toHexString();
		case PropertyId::Wordwrap:
			return mFlags & UI_WORD_WRAP ? "true" : "false";
		case PropertyId::TextSelection:
			return isTextSelectionEnabled() ? "true" : "false";
		case PropertyId::TextAlign:
			return Font::getHorizontalAlign( getFlags() ) == UI_HALIGN_CENTER
					   ? "center"
					   : ( Font::getHorizontalAlign( getFlags() ) == UI_HALIGN_RIGHT ? "right"
																					 : "left" );
		case PropertyId::TextOverflow:
			return mTextOverflow;
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UITextView::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::Text,
				   PropertyId::TextTransform,
				   PropertyId::Color,
				   PropertyId::TextShadowColor,
				   PropertyId::TextShadowOffset,
				   PropertyId::SelectionColor,
				   PropertyId::SelectionBackColor,
				   PropertyId::FontFamily,
				   PropertyId::FontSize,
				   PropertyId::FontStyle,
				   PropertyId::Wordwrap,
				   PropertyId::TextStrokeWidth,
				   PropertyId::TextStrokeColor,
				   PropertyId::TextSelection,
				   PropertyId::TextAlign,
				   PropertyId::TextOverflow };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

void UITextView::setTextAlign( const Uint32& align ) {
	mFlags &= ~( UI_HALIGN_CENTER | UI_HALIGN_RIGHT );
	mFlags |= align;
	onAlignChange();
}

void UITextView::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	UIWidget::loadFromXmlNode( node );

	if ( !node.text().empty() ) {
		setText( getTranslatorString( node.text().as_string() ) );
	}

	endAttributesTransaction();
}

void UITextView::updateTextOverflow() {
	if ( hasTextOverflow() ) {
		mTextOverflowWidth = Text::getTextWidth( mTextOverflow, mFontStyleConfig );

		Float maxWidth = mSize.getWidth() - mPaddingPx.Left - mPaddingPx.Right;

		std::size_t charPos =
			Text::findLastCharPosWithinLength( mString, maxWidth, mFontStyleConfig );

		if ( charPos != mString.size() ) {
			maxWidth -= mTextOverflowWidth;
			charPos = Text::findLastCharPosWithinLength( mString, maxWidth, mFontStyleConfig );
			mTextCache->setString( mString.view().substr( 0, charPos ) + mTextOverflow );
		} else {
			if ( mFlags & UI_WORD_WRAP ) {
				autoWrap();
			} else if ( mString != mTextCache->getString() ) {
				mTextCache->setString( mString );
			}
		}
	} else {
		mTextOverflowWidth = 0.f;

		if ( mFlags & UI_WORD_WRAP ) {
			autoWrap();
		} else if ( mString != mTextCache->getString() ) {
			mTextCache->setString( mString );
		}
	}
}

UITextView* UITextView::setTextOverflow( const std::string_view& textOverflow ) {
	if ( textOverflow == mTextOverflow ||
		 ( mTextOverflow == u8"…" && textOverflow == "ellipsis"sv ) )
		return this;

	if ( "ellipsis"sv == textOverflow ) {
		mTextOverflow = u8"…"; // U+2026
	} else {
		mTextOverflow = textOverflow;
	}

	updateTextOverflow();

	return this;
}

const std::string& UITextView::getTextOverflow() const {
	return mTextOverflow;
}

bool UITextView::hasTextOverflow() const {
	return !mTextOverflow.empty() && mTextOverflow != "clip"sv;
}

bool UITextView::getUsingCustomStyling() const {
	return mUsingCustomStyling;
}

void UITextView::setUsingCustomStyling( bool usingCustomStyling ) {
	mUsingCustomStyling = usingCustomStyling;
}

void UITextView::setWordWrap( bool set ) {
	if ( set != isWordWrap() ) {
		if ( set )
			mFlags |= UI_WORD_WRAP;
		else
			mFlags &= ~UI_WORD_WRAP;
		autoWrap();
	}
}

bool UITextView::isWordWrap() const {
	return mFlags & UI_WORD_WRAP;
}

UIAnchor* UIAnchor::New() {
	return eeNew( UIAnchor, () );
}

UIAnchor::UIAnchor() : UITextView( "anchor" ) {
	onClick(
		[this]( const MouseEvent* ) {
			if ( !mHref.empty() )
				Engine::instance()->openURI( mHref );
		},
		EE_BUTTON_LEFT );
}

bool UIAnchor::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Href:
			setHref( attribute.asString() );
			break;
		default:
			UITextView::applyProperty( attribute );
			break;
	}

	return true;
}

void UIAnchor::setHref( const std::string& href ) {
	if ( href != mHref ) {
		mHref = href;
	}
}

const std::string& UIAnchor::getHref() const {
	return mHref;
}

Uint32 UIAnchor::onKeyDown( const KeyEvent& event ) {
	if ( event.getKeyCode() == KEY_KP_ENTER || event.getKeyCode() == KEY_RETURN ) {
		if ( !mHref.empty() ) {
			Engine::instance()->openURI( mHref );
			return 1;
		}
	}

	return 0;
}

std::string UIAnchor::getPropertyString( const PropertyDefinition* propertyDef,
										 const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Href:
			return mHref;
		default:
			return UITextView::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UIAnchor::getPropertiesImplemented() const {
	auto props = UITextView::getPropertiesImplemented();
	auto local = { PropertyId::Href };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

}} // namespace EE::UI
