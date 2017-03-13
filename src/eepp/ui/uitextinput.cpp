#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/textcache.hpp>
#include <eepp/helper/pugixml/pugixml.hpp>

namespace EE { namespace UI {

UITextInput * UITextInput::New() {
	return eeNew( UITextInput, () );
}

UITextInput::UITextInput() :
	UITextView(),
	mCursorPos(0),
	mAllowEditing( true ),
	mShowingWait( true )
{
	setFlags( UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE | UI_TEXT_SELECTION_ENABLED );

	mTextBuffer.start();
	mTextBuffer.setActive( false );
	mTextBuffer.setFreeEditing( true );
	mTextBuffer.setTextSelectionEnabled( isTextSelectionEnabled() );
	mTextBuffer.setReturnCallback( cb::Make0( this, &UITextInput::privOnPressEnter ) );

	applyDefaultTheme();
}

UITextInput::~UITextInput() {
}

Uint32 UITextInput::getType() const {
	return UI_TYPE_TEXTINPUT;
}

bool UITextInput::isType( const Uint32& type ) const {
	return UITextInput::getType() == type ? true : UITextView::isType( type );
}

void UITextInput::update() {
	if ( isMouseOverMeOrChilds() ) {
		UIManager::instance()->setCursor( EE_CURSOR_IBEAM );
	}

	UITextView::update();

	if ( mTextBuffer.changedSinceLastUpdate() ) {
		Vector2f offSet = mRealAlignOffset;

		UITextView::setText( mTextBuffer.getBuffer() );

		updateText();

		mRealAlignOffset = offSet;

		resetWaitCursor();

		alignFix();

		mCursorPos = mTextBuffer.getCursorPos();

		mTextBuffer.setChangedSinceLastUpdate( false );

		return;
	}

	if ( mCursorPos != mTextBuffer.getCursorPos() ) {
		alignFix();
		mCursorPos = mTextBuffer.getCursorPos();
		mWaitCursorTime = 0.f;
		mShowingWait = true;
		onCursorPosChange();
	}
}

void UITextInput::onCursorPosChange() {
	sendCommonEvent( UIEvent::EventOnCursorPosChange );
}

void UITextInput::drawWaitingCursor() {
	if ( mVisible && mTextBuffer.isActive() && mTextBuffer.isFreeEditingEnabled() ) {
		mWaitCursorTime += getElapsed().asMilliseconds();

		if ( mShowingWait ) {
			bool disableSmooth = mShowingWait && GLi->isLineSmooth();

			if ( disableSmooth )
				GLi->lineSmooth( false );

			Primitives P;
			P.setColor( mFontStyleConfig.FontColor );

			Float CurPosX = mScreenPos.x + mRealAlignOffset.x + mCurPos.x + PixelDensity::dpToPxI( 1.f ) + mRealPadding.Left;
			Float CurPosY = mScreenPos.y + mRealAlignOffset.y + mCurPos.y + mRealPadding.Top;

			if ( CurPosX > (Float)mScreenPos.x + (Float)mRealSize.x )
				CurPosX = (Float)mScreenPos.x + (Float)mRealSize.x;

			P.drawLine( Line2f( Vector2f( CurPosX, CurPosY ), Vector2f( CurPosX, CurPosY + mTextCache->getFont()->getLineSpacing( mTextCache->getCharacterSizePx() ) ) ) );

			if ( disableSmooth )
				GLi->lineSmooth( true );
		}

		if ( mWaitCursorTime >= 500.f ) {
			mShowingWait = !mShowingWait;
			mWaitCursorTime = 0.f;
		}
	}
}

void UITextInput::draw() {
	UITextView::draw();

	drawWaitingCursor();
}

Uint32 UITextInput::onFocus() {
	UIControlAnim::onFocus();

	if ( mAllowEditing ) {
		mTextBuffer.setActive( true );

		resetWaitCursor();
	}

	return 1;
}

Uint32 UITextInput::onFocusLoss() {
	mTextBuffer.setActive( false );
	return UITextView::onFocusLoss();
}

Uint32 UITextInput::onPressEnter() {
	sendCommonEvent( UIEvent::EventOnPressEnter );
	return 0;
}

void UITextInput::privOnPressEnter() {
	onPressEnter();
}

void UITextInput::pushIgnoredChar( const Uint32& ch ) {
	mTextBuffer.pushIgnoredChar( ch );
}

void UITextInput::resetWaitCursor() {
	mShowingWait = true;
	mWaitCursorTime = 0.f;
}

void UITextInput::alignFix() {
	if ( fontHAlignGet( getFlags() ) == UI_HALIGN_LEFT ) {
		Uint32 NLPos	= 0;
		Uint32 LineNum	= mTextBuffer.getCurPosLinePos( NLPos );

		TextCache textCache( mTextCache->getFont(), mTextCache->getCharacterSize() );

		textCache.setText( mTextBuffer.getBuffer().substr( NLPos, mTextBuffer.getCursorPos() - NLPos ) );

		Float tW	= textCache.getTextWidth();
		Float tX	= mRealAlignOffset.x + tW;

		mCurPos.x	= tW;
		mCurPos.y	= (Float)LineNum * (Float)mTextCache->getFont()->getLineSpacing( mTextCache->getCharacterSizePx() );

		if ( !mTextBuffer.setSupportNewLine() ) {
			if ( tX < 0.f )
				mRealAlignOffset.x = -( mRealAlignOffset.x + ( tW - mRealAlignOffset.x ) );
			else if ( tX > mRealSize.getWidth() - mRealPadding.Left - mRealPadding.Right )
				mRealAlignOffset.x = mRealSize.getWidth() - mRealPadding.Left - mRealPadding.Right - ( mRealAlignOffset.x + ( tW - mRealAlignOffset.x ) );
		}
	}
}

void UITextInput::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	setThemeControl( Theme, "textinput" );

	onThemeLoaded();
}

void UITextInput::onThemeLoaded() {
	UITextView::onThemeLoaded();

	mMinControlSize.y = eemax( mMinControlSize.y, getSkinSize().getHeight() );

	autoPadding();
	onAutoSize();
}

void UITextInput::onAutoSize() {
	if ( ( mFlags & UI_AUTO_SIZE ) && 0 == mSize.getHeight() ) {
		setSize( mSize.x, getSkinSize().getHeight() );
	}
}

void UITextInput::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		setPadding( makePadding( true, true, false, false ) );
	}
}

InputTextBuffer * UITextInput::getInputTextBuffer() {
	return &mTextBuffer;
}

UITextInput * UITextInput::setAllowEditing( const bool& allow ) {
	mAllowEditing = allow;

	if ( !mAllowEditing && mTextBuffer.isActive() )
		mTextBuffer.setActive( false );

	return this;
}

const bool& UITextInput::getAllowEditing() const {
	return mAllowEditing;
}

UITextView * UITextInput::setText( const String& text ) {
	UITextView::setText( text );

	mTextBuffer.setBuffer( text );

	mTextBuffer.cursorToEnd();

	return this;
}

const String& UITextInput::getText() {
	return UITextView::getText();
}

void UITextInput::shrinkText( const Uint32& MaxWidth ) {
	mTextCache->setText( mTextBuffer.getBuffer() );

	UITextView::shrinkText( MaxWidth );

	mTextBuffer.setBuffer( mTextCache->getText() );

	alignFix();
}

void UITextInput::updateText() {
}

Uint32 UITextInput::onMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK ) {
		Vector2i controlPos( Pos );
		worldToControl( controlPos );
		controlPos = PixelDensity::dpToPxI( controlPos ) - Vector2i( (Int32)mRealAlignOffset.x, (Int32)mRealAlignOffset.y );

		Int32 curPos = mTextCache->getFont()->findClosestCursorPosFromPoint( mTextCache->getText(), mTextCache->getCharacterSizePx(), mTextCache->getStyle() & TextCache::Bold, mTextCache->getOutlineThickness(), controlPos );

		if ( -1 != curPos ) {
			mTextBuffer.setCursorPos( curPos );
			resetWaitCursor();
		}
	}

	return UITextView::onMouseClick( Pos, Flags );
}

Uint32 UITextInput::onMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
	UITextView::onMouseDoubleClick( Pos, Flags );

	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) && selCurEnd() != -1 ) {
		mTextBuffer.setCursorPos( selCurEnd() );
		resetWaitCursor();
	}

	return 1;
}

Uint32 UITextInput::onMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	UIControl::onMouseExit( Pos, Flags );

	UIManager::instance()->setCursor( EE_CURSOR_ARROW );

	return 1;
}

void UITextInput::selCurInit( const Int32& init ) {
	mTextBuffer.selCurInit( init );
}

void UITextInput::selCurEnd( const Int32& end ) {
	mTextBuffer.selCurEnd( end );

	if ( mTextBuffer.selCurEnd() != mTextBuffer.selCurInit() ) {
		mTextBuffer.setCursorPos( end );
	}
}

Int32 UITextInput::selCurInit() {
	return mTextBuffer.selCurInit();
}

Int32 UITextInput::selCurEnd() {
	return mTextBuffer.selCurEnd();
}

UITextInput * UITextInput::setMaxLength( Uint32 maxLength ) {
	mTextBuffer.setMaxLength( maxLength );
	return this;
}

Uint32 UITextInput::getMaxLength() {
	return mTextBuffer.getMaxLength();
}

UITextInput * UITextInput::setFreeEditing( bool support ) {
	mTextBuffer.setFreeEditing( support );
	return this;
}

bool UITextInput::isFreeEditingEnabled() {
	return mTextBuffer.isFreeEditingEnabled();
}

void UITextInput::loadFromXmlNode(const pugi::xml_node & node) {
	UITextView::loadFromXmlNode( node );

	for (pugi::xml_attribute_iterator ait = node.attributes_begin(); ait != node.attributes_end(); ++ait) {
		std::string name = ait->name();
		String::toLowerInPlace( name );

		if ( "text" == name ) {
			setText( ait->as_string() );
		} else if ( "allowediting" == name ) {
			setAllowEditing( ait->as_bool() );
		} else if ( "maxlength" == name ) {
			setMaxLength( ait->as_uint() );
		} else if ( "freeediting" == name ) {
			setFreeEditing( ait->as_bool() );
		}
	}
}

}}
