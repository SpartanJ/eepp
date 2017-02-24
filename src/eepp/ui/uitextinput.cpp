#include <eepp/ui/uitextinput.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UITextInput::UITextInput( const UITextInput::CreateParams& Params ) :
	UITextBox( Params ),
	mCursorPos(0),
	mAllowEditing( true ),
	mShowingWait( true )
{
	mTextBuffer.start();
	mTextBuffer.setActive( false );
	mTextBuffer.setFreeEditing( Params.SupportFreeEditing );
	mTextBuffer.setTextSelectionEnabled( isTextSelectionEnabled() );
	mTextBuffer.maxLength( Params.MaxLength );
	mTextBuffer.setReturnCallback( cb::Make0( this, &UITextInput::privOnPressEnter ) );

	applyDefaultTheme();
}

UITextInput::UITextInput() :
	UITextBox(),
	mCursorPos(0),
	mAllowEditing( true ),
	mShowingWait( true )
{
	setFlags( UI_CLIP_ENABLE | UI_AUTO_PADDING | UI_AUTO_SIZE );

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
	return UITextInput::getType() == type ? true : UITextBox::isType( type );
}

void UITextInput::update() {
	if ( isMouseOverMeOrChilds() ) {
		UIManager::instance()->setCursor( EE_CURSOR_IBEAM );
	}

	UITextBox::update();

	if ( mTextBuffer.changedSinceLastUpdate() ) {
		Vector2f offSet = mAlignOffset;

		UITextBox::setText( mTextBuffer.getBuffer() );

		updateText();

		mAlignOffset = offSet;

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
			P.setColor( mFontColor );

			Float CurPosX = mScreenPos.x + mAlignOffset.x + mCurPos.x + 1 + mRealPadding.Left;
			Float CurPosY = mScreenPos.y + mAlignOffset.y + mCurPos.y + mRealPadding.Top;

			if ( CurPosX > (Float)mScreenPos.x + (Float)mRealSize.x )
				CurPosX = (Float)mScreenPos.x + (Float)mRealSize.x;

			P.drawLine( Line2f( Vector2f( CurPosX, CurPosY ), Vector2f( CurPosX, CurPosY + mTextCache->getFont()->getFontHeight() ) ) );

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
	UITextBox::draw();

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
	return UITextBox::onFocusLoss();
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

		mTextCache->getFont()->setText( mTextBuffer.getBuffer().substr( NLPos, mTextBuffer.getCursorPos() - NLPos ) );

		Float tW	= mTextCache->getFont()->getTextWidth();
		Float tX	= mAlignOffset.x + tW;

		mCurPos.x	= tW;
		mCurPos.y	= (Float)LineNum * (Float)mTextCache->getFont()->getFontHeight();

		if ( !mTextBuffer.setSupportNewLine() ) {
			if ( tX < 0.f )
				mAlignOffset.x = -( mAlignOffset.x + ( tW - mAlignOffset.x ) );
			else if ( tX > mRealSize.getWidth() - mRealPadding.Left - mRealPadding.Right )
				mAlignOffset.x = mRealSize.getWidth() - mRealPadding.Left - mRealPadding.Right - ( mAlignOffset.x + ( tW - mAlignOffset.x ) );
		}
	}
}

void UITextInput::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "textinput" );

	autoPadding();
	autoSize();
}

void UITextInput::autoSize() {
	if ( ( mFlags & UI_AUTO_SIZE ) || 0 == mSize.getHeight() ) {
		setPixelsSize( mRealSize.x, getSkinSize().getHeight() );
	}
}

void UITextInput::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		setPixelsPadding( makePadding( true, true, false, false ) );
	}
}

InputTextBuffer * UITextInput::getInputTextBuffer() {
	return &mTextBuffer;
}

void UITextInput::setAllowEditing( const bool& allow ) {
	mAllowEditing = allow;

	if ( !mAllowEditing && mTextBuffer.isActive() )
		mTextBuffer.setActive( false );
}

const bool& UITextInput::getAllowEditing() const {
	return mAllowEditing;
}

void UITextInput::setText( const String& text ) {
	UITextBox::setText( text );

	mTextBuffer.setBuffer( text );

	mTextBuffer.cursorToEnd();
}

const String& UITextInput::getText() {
	return UITextBox::getText();
}

void UITextInput::shrinkText( const Uint32& MaxWidth ) {
	mTextCache->setText( mTextBuffer.getBuffer() );

	UITextBox::shrinkText( MaxWidth );

	mTextBuffer.setBuffer( mTextCache->getText() );

	alignFix();
}

void UITextInput::updateText() {
}

Uint32 UITextInput::onMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK ) {
		Vector2i controlPos( Pos );
		worldToControl( controlPos );
		controlPos = dpToPxI( controlPos );

		Int32 curPos = mTextCache->getFont()->findClosestCursorPosFromPoint( mTextCache->getText(), controlPos );

		if ( -1 != curPos ) {
			mTextBuffer.setCursorPos( curPos );
			resetWaitCursor();
		}
	}

	return UITextBox::onMouseClick( Pos, Flags );
}

Uint32 UITextInput::onMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
	UITextBox::onMouseDoubleClick( Pos, Flags );

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

}}
