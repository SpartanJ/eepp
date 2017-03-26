#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/helper/pugixml/pugixml.hpp>
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
	mLastSelCurEnd( -1 )
{
	mFontStyleConfig = UIThemeManager::instance()->getDefaultFontStyleConfig();

	mTextCache = eeNew( Text, () );
	mTextCache->setFont( mFontStyleConfig.Font );
	mTextCache->setCharacterSize( mFontStyleConfig.CharacterSize );
	mTextCache->setStyle( mFontStyleConfig.Style );
	mTextCache->setFillColor( mFontStyleConfig.Color );
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
		UIControlAnim::draw();

		drawSelection( mTextCache );

		if ( mTextCache->getTextWidth() ) {
			if ( mFlags & UI_CLIP_ENABLE ) {
				UIManager::instance()->clipSmartEnable(
						this,
						mScreenPos.x + mRealPadding.Left,
						mScreenPos.y + mRealPadding.Top,
						mRealSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
						mRealSize.getHeight() - mRealPadding.Top - mRealPadding.Bottom
				);
			}

			mTextCache->setAlign( getFlags() );
			mTextCache->draw( (Float)mScreenPos.x + mRealAlignOffset.x + (Float)mRealPadding.Left, (Float)mScreenPos.y + mRealAlignOffset.y + (Float)mRealPadding.Top, Vector2f::One, 0.f, getBlendMode() );

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
	}

	return this;
}

Uint32 UITextView::getCharacterSize() {
	return mTextCache->getCharacterSize();
}

UITextView *UITextView::setCharacterSize( const Uint32 & characterSize ) {
	if ( mTextCache->getCharacterSize() != characterSize ) {
		mTextCache->setCharacterSize( characterSize );
		recalculate();
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
	}

	return this;
}

const ColorA &UITextView::getOutlineColor() const {
	return mFontStyleConfig.OutlineColor;
}

UITextView * UITextView::setOutlineColor(const ColorA & outlineColor) {
	if ( mFontStyleConfig.OutlineColor != outlineColor ) {
		mTextCache->setOutlineColor( outlineColor );
		mFontStyleConfig.OutlineColor = outlineColor;
	}

	return this;
}

UITextView * UITextView::setFontStyle(const Uint32 & fontStyle) {
	if ( mFontStyleConfig.Style != fontStyle ) {
		mTextCache->setStyle( fontStyle );
		mFontStyleConfig.Style = fontStyle;
		recalculate();
	}

	return this;
}

const String& UITextView::getText() {
	if ( mFlags & UI_WORD_WRAP )
		return mString;

	return mTextCache->getString();
}

UITextView * UITextView::setText( const String& text ) {
	if ( mFlags & UI_WORD_WRAP ) {
		mString = text;
		mTextCache->setString( mString );
	} else {
		mTextCache->setString( text );
	}

	recalculate();
	onTextChanged();

	return this;
}

const ColorA& UITextView::getFontColor() const {
	return mFontStyleConfig.Color;
}

UITextView * UITextView::setFontColor( const ColorA& color ) {
	mFontStyleConfig.Color = color;
	mTextCache->setFillColor( color );

	setAlpha( color.a );

	return this;
}

const ColorA& UITextView::getFontShadowColor() const {
	return mFontStyleConfig.ShadowColor;
}

UITextView * UITextView::setFontShadowColor( const ColorA& color ) {
	mFontStyleConfig.ShadowColor = color;
	mTextCache->setShadowColor( mFontStyleConfig.ShadowColor );

	return this;
}

const ColorA& UITextView::getSelectionBackColor() const {
	return mFontStyleConfig.FontSelectionBackColor;
}

UITextView * UITextView::setSelectionBackColor( const ColorA& color ) {
	mFontStyleConfig.FontSelectionBackColor = color;
	return this;
}

void UITextView::setAlpha( const Float& alpha ) {
	UIControlAnim::setAlpha( alpha );
	mFontStyleConfig.Color.a = (Uint8)alpha;
	mFontStyleConfig.ShadowColor.a = (Uint8)alpha;

	mTextCache->setAlpha( mFontStyleConfig.Color.a );
}

void UITextView::autoShrink() {
	if ( mFlags & UI_WORD_WRAP ) {
		shrinkText( mRealSize.getWidth() );
	}
}

void UITextView::shrinkText( const Uint32& MaxWidth ) {
	if ( mFlags & UI_WORD_WRAP ) {
		mTextCache->setString( mString );
	}

	mTextCache->shrinkText( MaxWidth );
}

void UITextView::onAutoSize() {
	if ( ( mFlags & UI_AUTO_SIZE && 0 == mSize.getHeight() ) ) {
		setInternalPixelsSize( Sizei( (int)mTextCache->getTextWidth(), (int)mTextCache->getTextHeight() ) );

		alignFix();
	}
}

void UITextView::alignFix() {
	switch ( fontHAlignGet( getFlags() ) ) {
		case UI_HALIGN_CENTER:
			mRealAlignOffset.x = (Float)( (Int32)( mRealSize.x / 2 - mTextCache->getTextWidth() / 2 ) );
			break;
		case UI_HALIGN_RIGHT:
			mRealAlignOffset.x = ( (Float)mRealSize.x - (Float)mTextCache->getTextWidth() );
			break;
		case UI_HALIGN_LEFT:
			mRealAlignOffset.x = 0.f;
			break;
	}

	switch ( fontVAlignGet( getFlags() ) ) {
		case UI_VALIGN_CENTER:
			mRealAlignOffset.y = (Float)( (Int32)( mRealSize.y / 2 - mTextCache->getTextHeight() / 2 ) ) - 1;
			break;
		case UI_VALIGN_BOTTOM:
			mRealAlignOffset.y = ( (Float)mRealSize.y - (Float)mTextCache->getTextHeight() );
			break;
		case UI_VALIGN_TOP:
			mRealAlignOffset.y = 0.f;
			break;
	}

	mAlignOffset = PixelDensity::pxToDpI( mRealAlignOffset );
}

Uint32 UITextView::onFocusLoss() {
	selCurInit( -1 );
	selCurEnd( -1 );

	return 1;
}

void UITextView::onSizeChange() {
	recalculate();

	UIControlAnim::onSizeChange();
}

void UITextView::onTextChanged() {
	sendCommonEvent( UIEvent::EventOnTextChanged );
}

void UITextView::onFontChanged() {
	sendCommonEvent( UIEvent::EventOnFontChanged );
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

const Vector2i& UITextView::getAlignOffset() const {
	return mAlignOffset;
}

Uint32 UITextView::onMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		Vector2i controlPos( Pos );
		worldToControl( controlPos );
		controlPos = PixelDensity::dpToPxI( controlPos );

		Int32 curPos = mTextCache->findCharacterFromPos( controlPos );

		if ( -1 != curPos ) {
			Int32 tSelCurInit, tSelCurEnd;

			mTextCache->findWordFromCharacterIndex( curPos, tSelCurInit, tSelCurEnd );

			selCurInit( tSelCurInit );
			selCurEnd( tSelCurEnd );

			mControlFlags &= ~UI_CTRL_FLAG_SELECTING;
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

		mControlFlags &= ~UI_CTRL_FLAG_SELECTING;
	}

	return 1;
}

Uint32 UITextView::onMouseDown( const Vector2i& Pos, const Uint32 Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) && UIManager::instance()->getDownControl() == this ) {
		Vector2i controlPos( Pos );
		worldToControl( controlPos );
		controlPos = PixelDensity::dpToPxI( controlPos ) - Vector2i( (Int32)mRealAlignOffset.x, (Int32)mRealAlignOffset.y );

		Int32 curPos = mTextCache->findCharacterFromPos( controlPos );

		if ( -1 != curPos ) {
			if ( -1 == selCurInit() || !( mControlFlags & UI_CTRL_FLAG_SELECTING ) ) {
				selCurInit( curPos );
				selCurEnd( curPos );
			} else {
				selCurEnd( curPos );
			}
		}

		mControlFlags |= UI_CTRL_FLAG_SELECTING;
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

UITooltipStyleConfig UITextView::getFontStyleConfig() const
{
	return mFontStyleConfig;
}

void UITextView::selCurInit( const Int32& init ) {
	mSelCurInit = init;
}

void UITextView::selCurEnd( const Int32& end ) {
	mSelCurEnd = end;
}

Int32 UITextView::selCurInit() {
	return mSelCurInit;
}

Int32 UITextView::selCurEnd() {
	return mSelCurEnd;
}

void UITextView::onAlignChange() {
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

const Recti& UITextView::getPadding() const {
	return mPadding;
}

UITextView * UITextView::setPadding(const Recti & padding) {
	if ( padding != mPadding ) {
		mPadding = padding;
		mRealPadding = PixelDensity::dpToPxI( mPadding );
		onPaddingChange();
	}

	return this;
}

void UITextView::onPaddingChange() {
}

void UITextView::loadFromXmlNode(const pugi::xml_node & node) {
	UIWidget::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "text" == name ) {
			setText( ait->as_string() );
		} else if ( "textcolor" == name ) {
			setFontColor( ColorA::fromString( ait->as_string() ) );
		} else if ( "textshadowcolor" == name ) {
			setFontShadowColor( ColorA::fromString( ait->as_string() ) );
		} else if ( "textovercolor" == name ) {
			mFontStyleConfig.FontOverColor = ColorA::fromString( ait->as_string() );
		} else if ( "textselectedcolor" == name ) {
			mFontStyleConfig.FontSelectedColor = ColorA::fromString( ait->as_string() );
		} else if ( "textselectionbackcolor" == name ) {
			setSelectionBackColor( ColorA::fromString( ait->as_string() ) );
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
				}

				setFontStyle( flags );
			}
		} else if ( "fontoutlinethickness" == name ) {
			setOutlineThickness( PixelDensity::toDpFromString( ait->as_string() ) );
		} else if ( "fontoutlinecolor" == name ) {
			setOutlineColor( ColorA::fromString( ait->as_string() ) );
		} else if ( "padding" == name ) {
			int val = PixelDensity::toDpFromStringI( ait->as_string() );
			setPadding( Recti( val, val, val, val ) );
		} else if ( "paddingleft" == name ) {
			setPadding( Recti( PixelDensity::toDpFromStringI( ait->as_string() ), mPadding.Top, mPadding.Right, mPadding.Bottom ) );
		} else if ( "paddingright" == name ) {
			setPadding( Recti( mPadding.Left, mPadding.Top, PixelDensity::toDpFromStringI( ait->as_string() ), mPadding.Bottom ) );
		} else if ( "paddingtop" == name ) {
			setPadding( Recti( mPadding.Left, PixelDensity::toDpFromStringI( ait->as_string() ), mPadding.Right, mPadding.Bottom ) );
		} else if ( "paddingbottom" == name ) {
			setPadding( Recti( mPadding.Left, mPadding.Top, mPadding.Right, PixelDensity::toDpFromStringI( ait->as_string() ) ) );
		}
	}
}

}}
