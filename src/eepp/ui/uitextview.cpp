#include <eepp/ui/uitextview.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/textcache.hpp>
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
	mSelCurEnd( -1 )
{
	mFontStyleConfig = UIThemeManager::instance()->getDefaultFontStyleConfig();

	mTextCache = eeNew( TextCache, () );
	mTextCache->setFont( mFontStyleConfig.Font );
	mTextCache->setColor( mFontStyleConfig.FontColor );
	mTextCache->setShadowColor( mFontStyleConfig.FontShadowColor );

	autoAlign();
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

			mTextCache->setFlags( getFlags() );
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

void UITextView::setFont( Graphics::Font * font ) {
	if ( mTextCache->getFont() != font ) {
		mTextCache->setFont( font );
		autoShrink();
		onAutoSize();
		autoAlign();
		onFontChanged();
	}
}

const String& UITextView::getText() {
	if ( mFlags & UI_WORD_WRAP )
		return mString;

	return mTextCache->getText();
}

UITextView * UITextView::setText( const String& text ) {
	if ( mFlags & UI_WORD_WRAP ) {
		mString = text;
		mTextCache->setText( mString );
	} else {
		mTextCache->setText( text );
	}

	autoShrink();
	onAutoSize();
	autoAlign();
	onTextChanged();

	return this;
}

const ColorA& UITextView::getFontColor() const {
	return mFontStyleConfig.FontColor;
}

void UITextView::setFontColor( const ColorA& color ) {
	mFontStyleConfig.FontColor = color;
	mTextCache->setColor( color );

	setAlpha( color.a() );
}

const ColorA& UITextView::getFontShadowColor() const {
	return mFontStyleConfig.FontShadowColor;
}

void UITextView::setFontShadowColor( const ColorA& color ) {
	mFontStyleConfig.FontShadowColor = color;
	mTextCache->setShadowColor( mFontStyleConfig.FontShadowColor );
}

const ColorA& UITextView::getSelectionBackColor() const {
	return mFontStyleConfig.FontSelectionBackColor;
}

void UITextView::setSelectionBackColor( const ColorA& color ) {
	mFontStyleConfig.FontSelectionBackColor = color;
}

void UITextView::setAlpha( const Float& alpha ) {
	UIControlAnim::setAlpha( alpha );
	mFontStyleConfig.FontColor.Alpha = (Uint8)alpha;
	mFontStyleConfig.FontShadowColor.Alpha = (Uint8)alpha;

	mTextCache->setAlpha( mFontStyleConfig.FontColor.Alpha );
}

void UITextView::autoShrink() {
	if ( mFlags & UI_WORD_WRAP ) {
		shrinkText( mRealSize.getWidth() );
	}
}

void UITextView::shrinkText( const Uint32& MaxWidth ) {
	if ( mFlags & UI_WORD_WRAP ) {
		mTextCache->setText( mString );
	}

	mTextCache->getFont()->shrinkText( mTextCache->getText(), MaxWidth );
	mTextCache->cacheWidth();
}

void UITextView::onAutoSize() {
	if ( ( mFlags & UI_AUTO_SIZE && 0 == mSize.getHeight() ) ) {
		setInternalPixelsSize( Sizei( (int)mTextCache->getTextWidth(), (int)mTextCache->getTextHeight() ) );

		autoAlign();
	}
}

void UITextView::autoAlign() {
	switch ( fontHAlignGet( getFlags() ) ) {
		case UI_HALIGN_CENTER:
			mRealAlignOffset.x = (Float)( (Int32)( mRealSize.x - mTextCache->getTextWidth() ) / 2 );
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
			mRealAlignOffset.y = (Float)( ( (Int32)( mRealSize.y - mTextCache->getTextHeight() ) ) / 2 ) - 1;
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
	autoShrink();
	onAutoSize();
	autoAlign();

	UIControlAnim::onSizeChange();

	mTextCache->cacheWidth();
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

TextCache * UITextView::getTextCache() {
	return mTextCache;
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

		Int32 curPos = mTextCache->getFont()->findClosestCursorPosFromPoint( mTextCache->getText(), controlPos );

		if ( -1 != curPos ) {
			Int32 tSelCurInit, tSelCurEnd;

			mTextCache->getFont()->selectSubStringFromCursor( mTextCache->getText(), curPos, tSelCurInit, tSelCurEnd );

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

		Int32 curPos = mTextCache->getFont()->findClosestCursorPosFromPoint( mTextCache->getText(), controlPos );

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

void UITextView::drawSelection( TextCache * textCache ) {
	if ( selCurInit() != selCurEnd() ) {
		Int32 init		= eemin( selCurInit(), selCurEnd() );
		Int32 end		= eemax( selCurInit(), selCurEnd() );

		if ( init < 0 && end > (Int32)textCache->getText().size() ) {
			return;
		}

		Int32 lastEnd;
		Vector2i initPos, endPos;

		Primitives P;
		P.setColor( mFontStyleConfig.FontSelectionBackColor );

		do {
			initPos	= textCache->getFont()->getCursorPos( textCache->getText(), init );
			lastEnd = textCache->getText().find_first_of( '\n', init );

			if ( lastEnd < end && -1 != lastEnd ) {
				endPos	= textCache->getFont()->getCursorPos( textCache->getText(), lastEnd );
				init	= lastEnd + 1;
			} else {
				endPos	= textCache->getFont()->getCursorPos( textCache->getText(), end );
				lastEnd = end;
			}

			P.drawRectangle( Rectf( mScreenPos.x + initPos.x + mRealAlignOffset.x + mRealPadding.Left,
									  mScreenPos.y + initPos.y - textCache->getFont()->getFontHeight() + mRealAlignOffset.y + mRealPadding.Top,
									  mScreenPos.x + endPos.x + mRealAlignOffset.x + mRealPadding.Left,
									  mScreenPos.y + endPos.y + mRealAlignOffset.y + mRealPadding.Top )
			);
		} while ( end != lastEnd );
	}
}

bool UITextView::isTextSelectionEnabled() const {
	return 0 != ( mFlags & UI_TEXT_SELECTION_ENABLED );
}

TooltipStyleConfig UITextView::getFontStyleConfig() const
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
	autoAlign();
}

void UITextView::setFontStyleConfig( const TooltipStyleConfig& fontStyleConfig ) {
	mFontStyleConfig = fontStyleConfig;

	setFont( mFontStyleConfig.getFont() );
	setFontColor( mFontStyleConfig.getFontColor() );
	setFontShadowColor( mFontStyleConfig.getFontShadowColor() );
}

const Recti& UITextView::getPadding() const {
	return mPadding;
}

void UITextView::setPadding(const Recti & padding) {
	if ( padding != mPadding ) {
		mPadding = padding;
		mRealPadding = PixelDensity::dpToPxI( mPadding );
		onPaddingChange();
	}
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
		} else if ( "padding" == name ) {
			int val = ait->as_int();
			setPadding( Recti( val, val, val, val ) );
		} else if ( "paddingleft" == name ) {
			setPadding( Recti( ait->as_int(), mPadding.Top, mPadding.Right, mPadding.Bottom ) );
		} else if ( "paddingright" == name ) {
			setPadding( Recti( mPadding.Left, mPadding.Top, ait->as_int(), mPadding.Bottom ) );
		} else if ( "paddingtop" == name ) {
			setPadding( Recti( mPadding.Left, ait->as_int(), mPadding.Right, mPadding.Bottom ) );
		} else if ( "paddingbottom" == name ) {
			setPadding( Recti( mPadding.Left, mPadding.Top, mPadding.Right, ait->as_int() ) );
		}
	}
}

}}
