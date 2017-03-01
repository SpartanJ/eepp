#include <eepp/ui/uitextbox.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/textcache.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/window/clipboard.hpp>

namespace EE { namespace UI {

UITextBox *UITextBox::New() {
	return eeNew( UITextBox, () );
}

UITextBox::UITextBox( const UITextBox::CreateParams& Params ) :
	UIComplexControl( Params ),
	mFontStyleConfig( Params.fontStyleConfig ),
	mRealAlignOffset( 0.f, 0.f ),
	mSelCurInit( -1 ),
	mSelCurEnd( -1 )
{
	mTextCache = eeNew( TextCache, () );
	mTextCache->setFont( mFontStyleConfig.font );
	mTextCache->setColor( mFontStyleConfig.fontColor );
	mTextCache->setShadowColor( mFontStyleConfig.fontShadowColor );

	autoAlign();
}

UITextBox::UITextBox() :
	UIComplexControl(),
	mRealAlignOffset( 0.f, 0.f ),
	mSelCurInit( -1 ),
	mSelCurEnd( -1 )
{
	mFontStyleConfig = UIThemeManager::instance()->getDefaultFontStyleConfig();

	mTextCache = eeNew( TextCache, () );
	mTextCache->setFont( mFontStyleConfig.font );
	mTextCache->setColor( mFontStyleConfig.fontColor );
	mTextCache->setShadowColor( mFontStyleConfig.fontShadowColor );

	autoAlign();
}

UITextBox::~UITextBox() {
	eeSAFE_DELETE( mTextCache );
}

Uint32 UITextBox::getType() const {
	return UI_TYPE_TEXTBOX;
}

bool UITextBox::isType( const Uint32& type ) const {
	return UITextBox::getType() == type ? true : UIComplexControl::isType( type );
}

void UITextBox::draw() {
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

Graphics::Font * UITextBox::getFont() const {
	return mTextCache->getFont();
}

void UITextBox::setFont( Graphics::Font * font ) {
	if ( mTextCache->getFont() != font ) {
		mTextCache->setFont( font );
		autoShrink();
		autoSize();
		autoAlign();
		onFontChanged();
	}
}

const String& UITextBox::getText() {
	if ( mFlags & UI_WORD_WRAP )
		return mString;

	return mTextCache->getText();
}

void UITextBox::setText( const String& text ) {
	if ( mFlags & UI_WORD_WRAP ) {
		mString = text;
		mTextCache->setText( mString );
	} else {
		mTextCache->setText( text );
	}

	if ( mSize == Sizei::Zero ) {
		setFlags( UI_AUTO_SIZE );
	}

	autoShrink();
	autoSize();
	autoAlign();
	onTextChanged();
}

const ColorA& UITextBox::getFontColor() const {
	return mFontStyleConfig.fontColor;
}

void UITextBox::setFontColor( const ColorA& color ) {
	mFontStyleConfig.fontColor = color;
	mTextCache->setColor( color );

	setAlpha( color.a() );
}

const ColorA& UITextBox::getFontShadowColor() const {
	return mFontStyleConfig.fontShadowColor;
}

void UITextBox::setFontShadowColor( const ColorA& color ) {
	mFontStyleConfig.fontShadowColor = color;
	mTextCache->setShadowColor( mFontStyleConfig.fontShadowColor );
}

const ColorA& UITextBox::getSelectionBackColor() const {
	return mFontStyleConfig.fontSelectionBackColor;
}

void UITextBox::setSelectionBackColor( const ColorA& color ) {
	mFontStyleConfig.fontSelectionBackColor = color;
}

void UITextBox::setAlpha( const Float& alpha ) {
	UIControlAnim::setAlpha( alpha );
	mFontStyleConfig.fontColor.Alpha = (Uint8)alpha;
	mFontStyleConfig.fontShadowColor.Alpha = (Uint8)alpha;

	mTextCache->setAlpha( mFontStyleConfig.fontColor.Alpha );
}

void UITextBox::autoShrink() {
	if ( mFlags & UI_WORD_WRAP ) {
		shrinkText( mRealSize.getWidth() );
	}
}

void UITextBox::shrinkText( const Uint32& MaxWidth ) {
	if ( mFlags & UI_WORD_WRAP ) {
		mTextCache->setText( mString );
	}

	mTextCache->getFont()->shrinkText( mTextCache->getText(), MaxWidth );
	mTextCache->cacheWidth();
}

void UITextBox::autoSize() {
	if ( ( mFlags & UI_AUTO_SIZE ) ) {
		setInternalPixelsSize( Sizei( (int)mTextCache->getTextWidth(), (int)mTextCache->getTextHeight() ) );
	}
}

void UITextBox::autoAlign() {
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

Uint32 UITextBox::onFocusLoss() {
	selCurInit( -1 );
	selCurEnd( -1 );

	return 1;
}

void UITextBox::onSizeChange() {
	autoShrink();
	autoSize();
	autoAlign();

	UIControlAnim::onSizeChange();

	mTextCache->cacheWidth();
}

void UITextBox::onTextChanged() {
	sendCommonEvent( UIEvent::EventOnTextChanged );
}

void UITextBox::onFontChanged() {
	sendCommonEvent( UIEvent::EventOnFontChanged );
}

void UITextBox::setPadding( const Recti& padding ) {
	mPadding = padding;
	mRealPadding = PixelDensity::dpToPxI( padding );
}

const Recti& UITextBox::getPadding() const {
	return mPadding;
}

void UITextBox::setTheme( UITheme * Theme ) {
	UIControlAnim::setTheme( Theme );

	if ( NULL == mTextCache->getFont() && NULL != Theme->getFontStyleConfig().getFont() ) {
		mTextCache->setFont( Theme->getFontStyleConfig().getFont() );
	}
}

TextCache * UITextBox::getTextCache() {
	return mTextCache;
}

Float UITextBox::getTextWidth() {
	return mTextCache->getTextWidth();
}

Float UITextBox::getTextHeight() {
	return mTextCache->getTextHeight();
}

const int& UITextBox::getNumLines() const {
	return mTextCache->getNumLines();
}

const Vector2i& UITextBox::getAlignOffset() const {
	return mAlignOffset;
}

Uint32 UITextBox::onMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
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

	return UIComplexControl::onMouseDoubleClick( Pos, Flags );
}

Uint32 UITextBox::onMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		if ( selCurInit() == selCurEnd() ) {
			selCurInit( -1 );
			selCurEnd( -1 );
		}

		mControlFlags &= ~UI_CTRL_FLAG_SELECTING;
	}

	return 1;
}

Uint32 UITextBox::onMouseDown( const Vector2i& Pos, const Uint32 Flags ) {
	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
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

	return UIComplexControl::onMouseDown( Pos, Flags );
}

void UITextBox::drawSelection( TextCache * textCache ) {
	if ( selCurInit() != selCurEnd() ) {
		Int32 init		= eemin( selCurInit(), selCurEnd() );
		Int32 end		= eemax( selCurInit(), selCurEnd() );

		if ( init < 0 && end > (Int32)textCache->getText().size() ) {
			return;
		}

		Int32 lastEnd;
		Vector2i initPos, endPos;

		Primitives P;
		P.setColor( mFontStyleConfig.fontSelectionBackColor );

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

bool UITextBox::isTextSelectionEnabled() const {
	return 0 != ( mFlags & UI_TEXT_SELECTION_ENABLED );
}

FontStyleConfig UITextBox::getFontStyleConfig() const
{
	return mFontStyleConfig;
}

void UITextBox::selCurInit( const Int32& init ) {
	mSelCurInit = init;
}

void UITextBox::selCurEnd( const Int32& end ) {
	mSelCurEnd = end;
}

Int32 UITextBox::selCurInit() {
	return mSelCurInit;
}

Int32 UITextBox::selCurEnd() {
	return mSelCurEnd;
}

void UITextBox::onAlignChange() {
	autoAlign();
}

void UITextBox::setFontStyleConfig( const FontStyleConfig& fontStyleConfig ) {
	mFontStyleConfig = fontStyleConfig;

	setFont( mFontStyleConfig.getFont() );
	setFontColor( mFontStyleConfig.getFontColor() );
	setFontShadowColor( mFontStyleConfig.getFontShadowColor() );
}

}}
