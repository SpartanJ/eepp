#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/window/clipboard.hpp>
#include <pugixml/pugixml.hpp>
#include <eepp/graphics/fontmanager.hpp>

namespace EE { namespace UI {

UITextView * UITextView::New() {
	return eeNew( UITextView, () );
}

UITextView::UITextView() :
	UIWidget(),
	mRealAlignOffset( 0.f, 0.f ),
	mSelCurInit( -1 ),
	mSelCurEnd( -1 ),
	mLastSelCurInit( -1 ),
	mLastSelCurEnd( -1 ),
	mSelecting( false )
{
	mFontStyleConfig = UIThemeManager::instance()->getDefaultFontStyleConfig();

	mTextCache = eeNew( Text, () );
	mTextCache->setFont( mFontStyleConfig.Font );
	mTextCache->setCharacterSize( mFontStyleConfig.CharacterSize );
	mTextCache->setStyle( mFontStyleConfig.Style );
	mTextCache->setFillColor( mFontStyleConfig.FontColor );
	mTextCache->setShadowColor( mFontStyleConfig.ShadowColor );
	mTextCache->setOutlineThickness( mFontStyleConfig.OutlineThickness );
	mTextCache->setOutlineColor( mFontStyleConfig.OutlineColor );

	alignFix();
}

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

		drawSelection( mTextCache );

		if ( mTextCache->getTextWidth() ) {
			if ( mFlags & UI_CLIP_ENABLE ) {
				UIManager::instance()->clipSmartEnable(
						this,
						mScreenPos.x + mRealPadding.Left,
						mScreenPos.y + mRealPadding.Top,
						mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
						mSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom
				);
			}

			mTextCache->setAlign( getFlags() );
			mTextCache->draw( (Float)mScreenPosi.x + (int)mRealAlignOffset.x + (int)mRealPadding.Left, (Float)mScreenPosi.y + (int)mRealAlignOffset.y + (int)mRealPadding.Top, Vector2f::One, 0.f, getBlendMode() );

			if ( mFlags & UI_CLIP_ENABLE ) {
				UIManager::instance()->clipSmartDisable( this );
			}
		}
	}
}

Graphics::Font * UITextView::getFont() const {
	return mTextCache->getFont();
}

UITextView * UITextView::setFont( Graphics::Font * font ) {
	if ( mTextCache->getFont() != font ) {
		mTextCache->setFont( font );
		recalculate();
		onFontChanged();
		notifyLayoutAttrChange();
	}

	return this;
}

Uint32 UITextView::getCharacterSize() {
	return mTextCache->getCharacterSize();
}

UITextView *UITextView::setCharacterSize( const Uint32 & characterSize ) {
	if ( mTextCache->getCharacterSize() != characterSize ) {
		mFontStyleConfig.CharacterSize = characterSize;
		mTextCache->setCharacterSize( characterSize );
		recalculate();
		notifyLayoutAttrChange();
		invalidateDraw();
	}

	return this;
}

const Uint32 &UITextView::getFontStyle() const {
	return mFontStyleConfig.Style;
}

const Float &UITextView::getOutlineThickness() const {
	return mFontStyleConfig.OutlineThickness;
}

UITextView * UITextView::setOutlineThickness( const Float & outlineThickness ) {
	if ( mFontStyleConfig.OutlineThickness != outlineThickness ) {
		mTextCache->setOutlineThickness( outlineThickness );
		mFontStyleConfig.OutlineThickness = outlineThickness;
		recalculate();
		notifyLayoutAttrChange();
		invalidateDraw();
	}

	return this;
}

const Color &UITextView::getOutlineColor() const {
	return mFontStyleConfig.OutlineColor;
}

UITextView * UITextView::setOutlineColor(const Color & outlineColor) {
	if ( mFontStyleConfig.OutlineColor != outlineColor ) {
		mTextCache->setOutlineColor( outlineColor );
		mFontStyleConfig.OutlineColor = outlineColor;
		invalidateDraw();
	}

	return this;
}

UITextView * UITextView::setFontStyle(const Uint32 & fontStyle) {
	if ( mFontStyleConfig.Style != fontStyle ) {
		mTextCache->setStyle( fontStyle );
		mFontStyleConfig.Style = fontStyle;
		recalculate();
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

UITextView * UITextView::setText( const String& text ) {
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

UITextView * UITextView::setFontColor( const Color& color ) {
	if ( mFontStyleConfig.FontColor != color ) {
		mFontStyleConfig.FontColor = color;
		mTextCache->setFillColor( color );

		setAlpha( color.a );
	}

	return this;
}

const Color& UITextView::getFontShadowColor() const {
	return mFontStyleConfig.ShadowColor;
}

UITextView * UITextView::setFontShadowColor( const Color& color ) {
	if ( mFontStyleConfig.ShadowColor != color ) {
		mFontStyleConfig.ShadowColor = color;
		mTextCache->setShadowColor( mFontStyleConfig.ShadowColor );
		invalidateDraw();
	}

	return this;
}

const Color& UITextView::getSelectionBackColor() const {
	return mFontStyleConfig.FontSelectionBackColor;
}

UITextView * UITextView::setSelectionBackColor( const Color& color ) {
	if ( mFontStyleConfig.FontSelectionBackColor != color ) {
		mFontStyleConfig.FontSelectionBackColor = color;
		invalidateDraw();
	}

	return this;
}

void UITextView::setAlpha( const Float& alpha ) {
	if ( mAlpha != alpha ) {
		UINode::setAlpha( alpha );
		mFontStyleConfig.FontColor.a = (Uint8)alpha;
		mFontStyleConfig.ShadowColor.a = (Uint8)alpha;

		mTextCache->setAlpha( mFontStyleConfig.FontColor.a );
	}
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
	if ( ( mFlags & UI_AUTO_SIZE && 0 == mDpSize.getWidth() ) ) {
		setInternalPixelsSize( Sizef( mTextCache->getTextWidth(), mTextCache->getTextHeight() ) );
	}

	if ( mLayoutWidthRules == WRAP_CONTENT ) {
		setInternalPixelsWidth( (int)mTextCache->getTextWidth() );
	}

	if ( mLayoutHeightRules == WRAP_CONTENT ) {
		setInternalPixelsHeight( (int)mTextCache->getTextHeight() );
	}
}

void UITextView::alignFix() {
	switch ( fontHAlignGet( getFlags() ) ) {
		case UI_HALIGN_CENTER:
			mRealAlignOffset.x = (Float)( (Int32)( mSize.x / 2 - mTextCache->getTextWidth() / 2 ) );
			break;
		case UI_HALIGN_RIGHT:
			mRealAlignOffset.x = ( (Float)mSize.x - (Float)mTextCache->getTextWidth() );
			break;
		case UI_HALIGN_LEFT:
			mRealAlignOffset.x = 0.f;
			break;
	}

	switch ( fontVAlignGet( getFlags() ) ) {
		case UI_VALIGN_CENTER:
			mRealAlignOffset.y = (Float)( (Int32)( mSize.y / 2 - mTextCache->getTextHeight() / 2 ) ) - 1;
			break;
		case UI_VALIGN_BOTTOM:
			mRealAlignOffset.y = ( (Float)mSize.y - (Float)mTextCache->getTextHeight() );
			break;
		case UI_VALIGN_TOP:
			mRealAlignOffset.y = 0.f;
			break;
	}

	mAlignOffset = PixelDensity::pxToDp( mRealAlignOffset );
}

Uint32 UITextView::onFocusLoss() {
	UIWidget::onFocusLoss();

	selCurInit( -1 );
	selCurEnd( -1 );
	return 1;
}

void UITextView::onSizeChange() {
	recalculate();
	UINode::onSizeChange();
}

void UITextView::onTextChanged() {
	sendCommonEvent( UIEvent::OnTextChanged );
	invalidateDraw();
}

void UITextView::onFontChanged() {
	sendCommonEvent( UIEvent::OnFontChanged );
	invalidateDraw();
}

void UITextView::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	if ( NULL == mTextCache->getFont() && NULL != Theme->getFontStyleConfig().getFont() ) {
		mTextCache->setFont( Theme->getFontStyleConfig().getFont() );
	}
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

const Vector2f& UITextView::getAlignOffset() const {
	return mAlignOffset;
}

Uint32 UITextView::onMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		Vector2f controlPos( Vector2f( Pos.x, Pos.y ) );
		worldToNode( controlPos );
		controlPos = PixelDensity::dpToPx( controlPos );

		Int32 curPos = mTextCache->findCharacterFromPos( Vector2i( controlPos.x, controlPos.y ) );

		if ( -1 != curPos ) {
			Int32 tSelCurInit, tSelCurEnd;

			mTextCache->findWordFromCharacterIndex( curPos, tSelCurInit, tSelCurEnd );

			selCurInit( tSelCurInit );
			selCurEnd( tSelCurEnd );

			mSelecting = false;
		}
	}

	return UIWidget::onMouseDoubleClick( Pos, Flags );
}

Uint32 UITextView::onMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		if ( selCurInit() == selCurEnd() ) {
			selCurInit( -1 );
			selCurEnd( -1 );
		}

		mSelecting = false;
	}

	return UIWidget::onMouseClick( Pos, Flags );
}

Uint32 UITextView::onMouseDown( const Vector2i& Pos, const Uint32 Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) && UIManager::instance()->getDownControl() == this ) {
		Vector2f controlPos( Vector2f( Pos.x, Pos.y ) );
		worldToNode( controlPos );
		controlPos = PixelDensity::dpToPx( controlPos ) - mRealAlignOffset;

		Int32 curPos = mTextCache->findCharacterFromPos( Vector2i( controlPos.x, controlPos.y ) );

		if ( -1 != curPos ) {
			if ( -1 == selCurInit() || !mSelecting ) {
				selCurInit( curPos );
				selCurEnd( curPos );
			} else {
				selCurEnd( curPos );
			}
		}

		mSelecting = true;
	}

	return UIWidget::onMouseDown( Pos, Flags );
}

void UITextView::drawSelection( Text * textCache ) {
	if ( selCurInit() != selCurEnd() ) {
		Int32 init		= eemin( selCurInit(), selCurEnd() );
		Int32 end		= eemax( selCurInit(), selCurEnd() );

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
				initPos	= textCache->findCharacterPos( init );
				lastEnd = textCache->getString().find_first_of( '\n', init );

				if ( lastEnd < end && -1 != lastEnd ) {
					endPos	= textCache->findCharacterPos( lastEnd );
					init	= lastEnd + 1;
				} else {
					endPos	= textCache->findCharacterPos( end );
					lastEnd = end;
				}

				mSelPosCache.push_back( SelPosCache( initPos, endPos ) );
			} while ( end != lastEnd );
		}

		if ( mSelPosCache.size() ) {
			Primitives P;
			P.setColor( mFontStyleConfig.FontSelectionBackColor );
			Float vspace = textCache->getFont()->getLineSpacing( textCache->getCharacterSizePx() );

			for ( size_t i = 0; i < mSelPosCache.size(); i++ ) {
				initPos = mSelPosCache[i].initPos;
				endPos = mSelPosCache[i].endPos;

				P.drawRectangle( Rectf( mScreenPos.x + initPos.x + mRealAlignOffset.x + mRealPadding.Left,
										  mScreenPos.y + initPos.y + mRealAlignOffset.y + mRealPadding.Top,
										  mScreenPos.x + endPos.x + mRealAlignOffset.x + mRealPadding.Left,
										  mScreenPos.y + endPos.y + vspace + mRealAlignOffset.y + mRealPadding.Top )
				);
			}
		}
	}
}

bool UITextView::isTextSelectionEnabled() const {
	return 0 != ( mFlags & UI_TEXT_SELECTION_ENABLED );
}

UITooltipStyleConfig UITextView::getFontStyleConfig() const {
	return mFontStyleConfig;
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

void UITextView::recalculate() {
	autoShrink();
	onAutoSize();
	alignFix();
	resetSelCache();
}

void UITextView::resetSelCache() {
	mLastSelCurInit = mLastSelCurEnd = -1;
	invalidateDraw();
}

void UITextView::setFontStyleConfig( const UITooltipStyleConfig& fontStyleConfig ) {
	mFontStyleConfig = fontStyleConfig;

	setFont( mFontStyleConfig.getFont() );
	setFontColor( mFontStyleConfig.getFontColor() );
	setCharacterSize( mFontStyleConfig.getFontCharacterSize() );
	setFontShadowColor( mFontStyleConfig.getFontShadowColor() );
	setFontStyle( mFontStyleConfig.getFontStyle() );
	setOutlineThickness( mFontStyleConfig.getOutlineThickness() );
	setOutlineColor( mFontStyleConfig.getOutlineColor() );
}

const Rectf& UITextView::getPadding() const {
	return mPadding;
}

UITextView * UITextView::setPadding(const Rectf& padding) {
	if ( padding != mPadding ) {
		mPadding = padding;
		mRealPadding = PixelDensity::dpToPx( mPadding );
		onPaddingChange();
		notifyLayoutAttrChange();
	}

	return this;
}

void UITextView::onPaddingChange() {
	invalidateDraw();
}

void UITextView::loadFromXmlNode(const pugi::xml_node & node) {
	beginPropertiesTransaction();

	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "text" == name ) {
			setText( UIManager::instance()->getTranslatorString( ait->as_string() ) );
		} else if ( "textcolor" == name ) {
			setFontColor( Color::fromString( ait->as_string() ) );
		} else if ( "textshadowcolor" == name ) {
			setFontShadowColor( Color::fromString( ait->as_string() ) );
		} else if ( "textovercolor" == name ) {
			mFontStyleConfig.FontOverColor = Color::fromString( ait->as_string() );
		} else if ( "textselectedcolor" == name ) {
			mFontStyleConfig.FontSelectedColor = Color::fromString( ait->as_string() );
		} else if ( "textselectionbackcolor" == name ) {
			setSelectionBackColor( Color::fromString( ait->as_string() ) );
		} else if ( "fontfamily" == name || "fontname" == name ) {
			Font * font = FontManager::instance()->getByName( ait->as_string() );

			if ( NULL != font )
				setFont( font );
		} else if ( "textsize" == name || "fontsize" == name || "charactersize" == name ) {
			setCharacterSize( PixelDensity::toDpFromStringI( ait->as_string() ) );
		} else if ( "textstyle" == name || "fontstyle" == name ) {
			std::string valStr = ait->as_string();
			String::toLowerInPlace( valStr );
			std::vector<std::string> strings = String::split( valStr, '|' );
			Uint32 flags = Text::Regular;

			if ( strings.size() ) {
				for ( std::size_t i = 0; i < strings.size(); i++ ) {
					std::string cur = strings[i];
					String::toLowerInPlace( cur );

					if ( "underlined" == cur || "underline" == cur )
						flags |= Text::Underlined;
					else if ( "bold" == cur )
						flags |= Text::Bold;
					else if ( "italic" == cur )
						flags |= Text::Italic;
					else if ( "strikethrough" == cur )
						flags |= Text::StrikeThrough;
					else if ( "shadowed" == cur || "shadow" == cur )
						flags |= Text::Shadow;
					else if ( "wordwrap" == cur ) {
						mFlags |= UI_WORD_WRAP;
						autoShrink();
					}
				}

				setFontStyle( flags );
			}
		} else if ( "fontoutlinethickness" == name ) {
			setOutlineThickness( PixelDensity::toDpFromString( ait->as_string() ) );
		} else if ( "fontoutlinecolor" == name ) {
			setOutlineColor( Color::fromString( ait->as_string() ) );
		} else if ( "padding" == name ) {
			int val = PixelDensity::toDpFromStringI( ait->as_string() );
			setPadding( Rectf( val, val, val, val ) );
		} else if ( "paddingleft" == name ) {
			setPadding( Rectf( PixelDensity::toDpFromString( ait->as_string() ), mPadding.Top, mPadding.Right, mPadding.Bottom ) );
		} else if ( "paddingright" == name ) {
			setPadding( Rectf( mPadding.Left, mPadding.Top, PixelDensity::toDpFromString( ait->as_string() ), mPadding.Bottom ) );
		} else if ( "paddingtop" == name ) {
			setPadding( Rectf( mPadding.Left, PixelDensity::toDpFromString( ait->as_string() ), mPadding.Right, mPadding.Bottom ) );
		} else if ( "paddingbottom" == name ) {
			setPadding( Rectf( mPadding.Left, mPadding.Top, mPadding.Right, PixelDensity::toDpFromString( ait->as_string() ) ) );
		}
	}

	endPropertiesTransaction();
}

}}
