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
	mTextBuffer.active( false );
	mTextBuffer.supportFreeEditing( Params.SupportFreeEditing );
	mTextBuffer.textSelectionEnabled( isTextSelectionEnabled() );
	mTextBuffer.maxLength( Params.MaxLength );
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

		UITextBox::text( mTextBuffer.buffer() );

		updateText();

		mAlignOffset = offSet;

		resetWaitCursor();

		alignFix();

		mCursorPos = mTextBuffer.curPos();

		mTextBuffer.changedSinceLastUpdate( false );

		return;
	}

	if ( mCursorPos != mTextBuffer.curPos() ) {
		alignFix();
		mCursorPos = mTextBuffer.curPos();
		onCursorPosChange();
	}
}

void UITextInput::onCursorPosChange() {
	sendCommonEvent( UIEvent::EventOnCursorPosChange );
}

void UITextInput::drawWaitingCursor() {
	if ( mVisible && mTextBuffer.active() && mTextBuffer.supportFreeEditing() ) {
		mWaitCursorTime += UIManager::instance()->elapsed().asMilliseconds();

		if ( mShowingWait ) {
			bool disableSmooth = mShowingWait && GLi->isLineSmooth();

			if ( disableSmooth )
				GLi->lineSmooth( false );

			Primitives P;
			P.setColor( mFontColor );

			Float CurPosX = mScreenPos.x + mAlignOffset.x + mCurPos.x + 1 + mPadding.Left;
			Float CurPosY = mScreenPos.y + mAlignOffset.y + mCurPos.y		+ mPadding.Top;

			if ( CurPosX > (Float)mScreenPos.x + (Float)mSize.x )
				CurPosX = (Float)mScreenPos.x + (Float)mSize.x;

			P.drawLine( Line2f( Vector2f( CurPosX, CurPosY ), Vector2f( CurPosX, CurPosY + mTextCache->font()->getFontHeight() ) ) );

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
		mTextBuffer.active( true );

		resetWaitCursor();
	}

	return 1;
}

Uint32 UITextInput::onFocusLoss() {
	mTextBuffer.active( false );
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
	if ( FontHAlignGet( flags() ) == UI_HALIGN_LEFT ) {
		Uint32 NLPos	= 0;
		Uint32 LineNum	= mTextBuffer.getCurPosLinePos( NLPos );

		mTextCache->font()->setText( mTextBuffer.buffer().substr( NLPos, mTextBuffer.curPos() - NLPos ) );

		Float tW	= mTextCache->font()->getTextWidth();
		Float tX	= mAlignOffset.x + tW;

		mCurPos.x	= tW;
		mCurPos.y	= (Float)LineNum * (Float)mTextCache->font()->getFontHeight();

		if ( !mTextBuffer.supportNewLine() ) {
			if ( tX < 0.f )
				mAlignOffset.x = -( mAlignOffset.x + ( tW - mAlignOffset.x ) );
			else if ( tX > mSize.width() - mPadding.Left - mPadding.Right )
				mAlignOffset.x = mSize.width() - mPadding.Left - mPadding.Right - ( mAlignOffset.x + ( tW - mAlignOffset.x ) );
		}
	}
}

void UITextInput::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "textinput" );

	autoPadding();
	autoSize();
}

void UITextInput::autoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		size( mSize.x, getSkinSize().height() );
	}
}

void UITextInput::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = makePadding( true, true, false, false );
	}
}

InputTextBuffer * UITextInput::getInputTextBuffer() {
	return &mTextBuffer;
}

void UITextInput::allowEditing( const bool& allow ) {
	mAllowEditing = allow;

	if ( !mAllowEditing && mTextBuffer.active() )
		mTextBuffer.active( false );
}

const bool& UITextInput::allowEditing() const {
	return mAllowEditing;
}

void UITextInput::text( const String& text ) {
	UITextBox::text( text );

	mTextBuffer.buffer( text );

	mTextBuffer.cursorToEnd();
}

const String& UITextInput::text() {
	return UITextBox::text();
}

void UITextInput::shrinkText( const Uint32& MaxWidth ) {
	mTextCache->text( mTextBuffer.buffer() );

	UITextBox::shrinkText( MaxWidth );

	mTextBuffer.buffer( mTextCache->text() );

	alignFix();
}

void UITextInput::updateText() {
}

Uint32 UITextInput::onMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK ) {
		Vector2i controlPos( Pos );
		worldToControl( controlPos );

		Int32 curPos = mTextCache->font()->findClosestCursorPosFromPoint( mTextCache->text(), controlPos );

		if ( -1 != curPos ) {
			mTextBuffer.curPos( curPos );
			resetWaitCursor();
		}
	}

	return UITextBox::onMouseClick( Pos, Flags );
}

Uint32 UITextInput::onMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
	UITextBox::onMouseDoubleClick( Pos, Flags );

	if ( isTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) && selCurEnd() != -1 ) {
		mTextBuffer.curPos( selCurEnd() );
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
		mTextBuffer.curPos( end );
	}
}

Int32 UITextInput::selCurInit() {
	return mTextBuffer.selCurInit();
}

Int32 UITextInput::selCurEnd() {
	return mTextBuffer.selCurEnd();
}

}}
