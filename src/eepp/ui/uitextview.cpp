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
	mSelCurInit( -1 ),
	mSelCurEnd( -1 ),
	mLastSelCurInit( -1 ),
	mLastSelCurEnd( -1 ),
	mFontLineCenter( 0 ),
	mSelecting( false ) {
	mTextCache = Text::New();

	UITheme* theme = getUISceneNode()->getUIThemeManager()->getDefaultTheme();

	if ( NULL != theme && NULL != theme->getDefaultFont() ) {
		setFont( theme->getDefaultFont() );
	}

	if ( NULL != theme ) {
		setFontSize( theme->getDefaultFontSize() );
	} else {
		mTextCache->setFontSize( getUISceneNode()->getUIThemeManager()->getDefaultFontSize() );
	}

	if ( NULL == getFont() ) {
		if ( NULL != getUISceneNode()->getUIThemeManager()->getDefaultFont() ) {
			setFont( getUISceneNode()->getUIThemeManager()->getDefaultFont() );
		} else {
			eePRINTL( "UITextView::UITextView : Created a without a defined font." );
		}
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

		if ( mTextCache->getTextWidth() ) {
			drawSelection( mTextCache );

			if ( isClipped() ) {
				clipSmartEnable( mScreenPos.x + mRealPadding.Left, mScreenPos.y + mRealPadding.Top,
								 mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
								 mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom );
			}

			mTextCache->setAlign( getFlags() );
			mTextCache->draw( (Float)mScreenPosi.x + (int)mRealAlignOffset.x +
								  (int)mRealPadding.Left,
							  mFontLineCenter + (Float)mScreenPosi.y + (int)mRealAlignOffset.y +
								  (int)mRealPadding.Top,
							  Vector2f::One, 0.f, getBlendMode() );

			if ( isClipped() ) {
				clipSmartDisable();
			}
		}
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

Uint32 UITextView::getCharacterSize() const {
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

const String& UITextView::getText() {
	if ( mFlags & UI_WORD_WRAP )
		return mString;

	return mTextCache->getString();
}

UITextView* UITextView::setText( const String& text ) {
	if ( ( mFlags & UI_WORD_WRAP ) ) {
		if ( mString != text ) {
			mString = text;
			mTextCache->setString( mString );

			recalculate();
			onTextChanged();
			notifyLayoutAttrChange();
		}
	} else if ( mTextCache->getString() != text ) {
		mTextCache->setString( text );

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
	}

	return this;
}

const Color& UITextView::getFontShadowColor() const {
	return mFontStyleConfig.ShadowColor;
}

UITextView* UITextView::setFontShadowColor( const Color& color ) {
	if ( mFontStyleConfig.ShadowColor != color ) {
		mFontStyleConfig.ShadowColor = color;
		Color newColor( color.r, color.g, color.b, color.a * mAlpha / 255.f );
		mTextCache->setShadowColor( newColor );
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

void UITextView::autoShrink() {
	if ( mFlags & UI_WORD_WRAP ) {
		shrinkText( mSize.getWidth() );
	}
}

void UITextView::shrinkText( const Uint32& MaxWidth ) {
	if ( mFlags & UI_WORD_WRAP ) {
		mTextCache->setString( mString );
	}

	mTextCache->shrinkText( MaxWidth );
	invalidateDraw();
}

void UITextView::onAutoSize() {
	if ( ( mFlags & UI_AUTO_SIZE && 0 == getSize().getWidth() ) ) {
		setInternalPixelsSize( Sizef( mTextCache->getTextWidth(), mTextCache->getTextHeight() ) );
	}

	if ( mLayoutWidthRule == LayoutSizeRule::WrapContent ) {
		setInternalPixelsWidth( (int)mTextCache->getTextWidth() + mRealPadding.Left +
								mRealPadding.Right );
	}

	if ( mLayoutHeightRule == LayoutSizeRule::WrapContent ) {
		setInternalPixelsHeight( (int)mTextCache->getTextHeight() + mRealPadding.Top +
								 mRealPadding.Bottom );
	}
}

void UITextView::alignFix() {
	switch ( Font::getHorizontalAlign( getFlags() ) ) {
		case UI_HALIGN_CENTER:
			mRealAlignOffset.x =
				( Float )( ( Int32 )( ( mSize.x - mRealPadding.Left - mRealPadding.Right ) / 2 -
									  mTextCache->getTextWidth() / 2 ) );
			break;
		case UI_HALIGN_RIGHT:
			mRealAlignOffset.x = ( (Float)mSize.x - mRealPadding.Left - mRealPadding.Right -
								   (Float)mTextCache->getTextWidth() );
			break;
		case UI_HALIGN_LEFT:
			mRealAlignOffset.x = 0;
			break;
	}

	switch ( Font::getVerticalAlign( getFlags() ) ) {
		case UI_VALIGN_CENTER:
			mRealAlignOffset.y =
				( Float )( ( Int32 )( ( mSize.y - mRealPadding.Top - mRealPadding.Bottom ) / 2 -
									  mTextCache->getTextHeight() / 2 ) ) -
				1;
			break;
		case UI_VALIGN_BOTTOM:
			mRealAlignOffset.y = ( (Float)mSize.y - mRealPadding.Top - mRealPadding.Bottom -
								   (Float)mTextCache->getTextHeight() );
			break;
		case UI_VALIGN_TOP:
			mRealAlignOffset.y = 0;
			break;
	}
}

Uint32 UITextView::onFocusLoss() {
	UIWidget::onFocusLoss();

	selCurInit( -1 );
	selCurEnd( -1 );
	onSelectionChange();

	return 1;
}

void UITextView::onSizeChange() {
	recalculate();
	UINode::onSizeChange();
}

void UITextView::onTextChanged() {
	sendCommonEvent( Event::OnTextChanged );
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
	return mTextCache->getTextWidth();
}

Float UITextView::getTextHeight() {
	return mTextCache->getTextHeight();
}

const int& UITextView::getNumLines() const {
	return mTextCache->getNumLines();
}

Vector2f UITextView::getAlignOffset() const {
	return PixelDensity::pxToDp( mRealAlignOffset );
}

Uint32 UITextView::onMouseDoubleClick( const Vector2i& Pos, const Uint32& Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		Vector2f controlPos( Vector2f( Pos.x, Pos.y ) );
		worldToNode( controlPos );
		controlPos = PixelDensity::dpToPx( controlPos ) - mRealAlignOffset -
					 Vector2f( mRealPadding.Left, mRealPadding.Top );
		controlPos.x = eemax( 0.f, controlPos.x );
		controlPos.y = eemax( 0.f, controlPos.y );

		Int32 curPos = mTextCache->findCharacterFromPos( controlPos.asInt() );

		if ( -1 != curPos ) {
			Int32 tSelCurInit, tSelCurEnd;

			mTextCache->findWordFromCharacterIndex( curPos, tSelCurInit, tSelCurEnd );

			selCurInit( tSelCurInit );
			selCurEnd( tSelCurEnd );
			onSelectionChange();

			mSelecting = false;
		}
	}

	return UIWidget::onMouseDoubleClick( Pos, Flags );
}

Uint32 UITextView::onMouseClick( const Vector2i& Pos, const Uint32& Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		if ( selCurInit() == selCurEnd() ) {
			selCurInit( -1 );
			selCurEnd( -1 );
			onSelectionChange();
		}

		mSelecting = false;
	}

	return UIWidget::onMouseClick( Pos, Flags );
}

Uint32 UITextView::onMouseDown( const Vector2i& Pos, const Uint32& Flags ) {
	if ( NULL != getEventDispatcher() && isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) &&
		 getEventDispatcher()->getDownControl() == this ) {
		Vector2f controlPos( Vector2f( Pos.x, Pos.y ) );
		worldToNode( controlPos );
		controlPos = PixelDensity::dpToPx( controlPos ) - mRealAlignOffset -
					 Vector2f( mRealPadding.Left, mRealPadding.Top );
		controlPos.x = eemax( 0.f, controlPos.x );
		controlPos.y = eemax( 0.f, controlPos.y );

		Int32 curPos = mTextCache->findCharacterFromPos( controlPos.asInt() );

		if ( -1 != curPos ) {
			if ( -1 == selCurInit() || !mSelecting ) {
				selCurInit( curPos );
				selCurEnd( curPos );
			} else {
				selCurEnd( curPos );
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
			Float vspace = textCache->getFont()->getFontHeight( textCache->getCharacterSizePx() );

			for ( size_t i = 0; i < mSelPosCache.size(); i++ ) {
				initPos = mSelPosCache[i].initPos;
				endPos = mSelPosCache[i].endPos;

				P.drawRectangle( Rectf(
					mScreenPos.x + initPos.x + mRealAlignOffset.x + mRealPadding.Left,
					mScreenPos.y + initPos.y + mRealAlignOffset.y + mRealPadding.Top,
					mScreenPos.x + endPos.x + mRealAlignOffset.x + mRealPadding.Left,
					mScreenPos.y + endPos.y + vspace + mRealAlignOffset.y + mRealPadding.Top ) );
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

const Int32& UITextView::getFontLineCenter() {
	return mFontLineCenter;
}

void UITextView::recalculate() {
	int fontHeight = mTextCache->getCharacterSizePx();
	mFontLineCenter = eefloor(
		( Float )( ( mTextCache->getFont()->getLineSpacing( fontHeight ) - fontHeight ) / 2 ) );

	autoShrink();
	onAutoSize();
	alignFix();
	resetSelCache();
}

void UITextView::resetSelCache() {
	mLastSelCurInit = mLastSelCurEnd = -1;
	invalidateDraw();
	onSelectionChange();
}

bool UITextView::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Text:
			if ( NULL != mUISceneNode )
				setText( mUISceneNode->getTranslatorString( attribute.asString() ) );
			break;
		case PropertyId::Color:
			setFontColor( attribute.asColor() );
			break;
		case PropertyId::ShadowColor:
			setFontShadowColor( attribute.asColor() );
			break;
		case PropertyId::SelectionColor:
			mFontStyleConfig.FontSelectedColor = attribute.asColor();
			break;
		case PropertyId::SelectionBackColor:
			setSelectionBackColor( attribute.asColor() );
			break;
		case PropertyId::FontFamily: {
			Font* font = FontManager::instance()->getByName( attribute.asString() );

			if ( NULL != font && font->loaded() ) {
				setFont( font );
			}
			break;
		}
		case PropertyId::FontSize:
			setFontSize( attribute.asDpDimensionI() );
			break;
		case PropertyId::FontStyle: {
			Uint32 flags = attribute.asFontStyle();

			if ( flags & UI_WORD_WRAP ) {
				mFlags |= UI_WORD_WRAP;
				flags &= ~UI_WORD_WRAP;
				autoShrink();
			}

			setFontStyle( flags );
			break;
		}
		case PropertyId::Wordwrap:
			if ( attribute.asBool() )
				mFlags |= UI_WORD_WRAP;
			else
				mFlags &= ~UI_WORD_WRAP;
			autoShrink();
			break;
		case PropertyId::TextStrokeWidth:
			setOutlineThickness( lengthFromValue( attribute ) );
			break;
		case PropertyId::TextStrokeColor:
			setOutlineColor( attribute.asColor() );
			break;
		case PropertyId::TextSelection:
			setTextSelection( attribute.asBool() );
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

std::string UITextView::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::Text:
			return getText().toUtf8();
		case PropertyId::Color:
			return getFontColor().toHexString();
		case PropertyId::ShadowColor:
			return getFontShadowColor().toHexString();
		case PropertyId::SelectionColor:
			return mFontStyleConfig.FontSelectedColor.toHexString();
		case PropertyId::SelectionBackColor:
			return getSelectionBackColor().toHexString();
		case PropertyId::FontFamily:
			return NULL != getFont() ? getFont()->getName() : "";
		case PropertyId::FontSize:
			return String::format( "%ddp", getCharacterSize() );
		case PropertyId::FontStyle:
			return Text::styleFlagToString( getFontStyle() );
		case PropertyId::TextStrokeWidth:
			return String::toStr( PixelDensity::dpToPx( getOutlineThickness() ) );
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
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

}} // namespace EE::UI
