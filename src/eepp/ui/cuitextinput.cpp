#include <eepp/ui/cuitextinput.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

cUITextInput::cUITextInput( const cUITextInput::CreateParams& Params ) :
	cUITextBox( Params ),
	mCursorPos(0),
	mAllowEditing( true ),
	mShowingWait( true )
{
	mTextBuffer.Start();
	mTextBuffer.Active( false );
	mTextBuffer.SupportFreeEditing( Params.SupportFreeEditing );
	mTextBuffer.TextSelectionEnabled( IsTextSelectionEnabled() );
	mTextBuffer.MaxLength( Params.MaxLength );
	mTextBuffer.SetReturnCallback( cb::Make0( this, &cUITextInput::PrivOnPressEnter ) );

	ApplyDefaultTheme();
}

cUITextInput::~cUITextInput() {
}

Uint32 cUITextInput::Type() const {
	return UI_TYPE_TEXTINPUT;
}

bool cUITextInput::IsType( const Uint32& type ) const {
	return cUITextInput::Type() == type ? true : cUITextBox::IsType( type );
}

void cUITextInput::Update() {
	if ( IsMouseOverMeOrChilds() ) {
		cUIManager::instance()->SetCursor( EE_CURSOR_IBEAM );
	}

	cUITextBox::Update();

	if ( mTextBuffer.ChangedSinceLastUpdate() ) {
		Vector2f offSet = mAlignOffset;

		cUITextBox::Text( mTextBuffer.Buffer() );

		UpdateText();

		mAlignOffset = offSet;

		ResetWaitCursor();

		AlignFix();

		mCursorPos = mTextBuffer.CurPos();

		mTextBuffer.ChangedSinceLastUpdate( false );

		return;
	}

	if ( mCursorPos != mTextBuffer.CurPos() ) {
		AlignFix();
		mCursorPos = mTextBuffer.CurPos();
		OnCursorPosChange();
	}
}

void cUITextInput::OnCursorPosChange() {
	SendCommonEvent( cUIEvent::EventOnCursorPosChange );
}

void cUITextInput::DrawWaitingCursor() {
	if ( mVisible && mTextBuffer.Active() && mTextBuffer.SupportFreeEditing() ) {
		mWaitCursorTime += cUIManager::instance()->Elapsed().AsMilliseconds();

		if ( mShowingWait ) {
			bool disableSmooth = mShowingWait && GLi->IsLineSmooth();

			if ( disableSmooth )
				GLi->LineSmooth( false );

			Primitives P;
			P.SetColor( mFontColor );

			Float CurPosX = mScreenPos.x + mAlignOffset.x + mCurPos.x + 1 + mPadding.Left;
			Float CurPosY = mScreenPos.y + mAlignOffset.y + mCurPos.y		+ mPadding.Top;

			if ( CurPosX > (Float)mScreenPos.x + (Float)mSize.x )
				CurPosX = (Float)mScreenPos.x + (Float)mSize.x;

			P.DrawLine( Line2f( Vector2f( CurPosX, CurPosY ), Vector2f( CurPosX, CurPosY + mTextCache->Font()->GetFontHeight() ) ) );

			if ( disableSmooth )
				GLi->LineSmooth( true );
		}

		if ( mWaitCursorTime >= 500.f ) {
			mShowingWait = !mShowingWait;
			mWaitCursorTime = 0.f;
		}
	}
}

void cUITextInput::Draw() {
	cUITextBox::Draw();

	DrawWaitingCursor();
}

Uint32 cUITextInput::OnFocus() {
	cUIControlAnim::OnFocus();

	if ( mAllowEditing ) {
		mTextBuffer.Active( true );

		ResetWaitCursor();
	}

	return 1;
}

Uint32 cUITextInput::OnFocusLoss() {
	mTextBuffer.Active( false );
	return cUITextBox::OnFocusLoss();
}

Uint32 cUITextInput::OnPressEnter() {
	SendCommonEvent( cUIEvent::EventOnPressEnter );
	return 0;
}

void cUITextInput::PrivOnPressEnter() {
	OnPressEnter();
}

void cUITextInput::PushIgnoredChar( const Uint32& ch ) {
	mTextBuffer.PushIgnoredChar( ch );
}

void cUITextInput::ResetWaitCursor() {
	mShowingWait = true;
	mWaitCursorTime = 0.f;
}

void cUITextInput::AlignFix() {
	if ( FontHAlignGet( Flags() ) == UI_HALIGN_LEFT ) {
		Uint32 NLPos	= 0;
		Uint32 LineNum	= mTextBuffer.GetCurPosLinePos( NLPos );

		mTextCache->Font()->SetText( mTextBuffer.Buffer().substr( NLPos, mTextBuffer.CurPos() - NLPos ) );

		Float tW	= mTextCache->Font()->GetTextWidth();
		Float tX	= mAlignOffset.x + tW;

		mCurPos.x	= tW;
		mCurPos.y	= (Float)LineNum * (Float)mTextCache->Font()->GetFontHeight();

		if ( !mTextBuffer.SupportNewLine() ) {
			if ( tX < 0.f )
				mAlignOffset.x = -( mAlignOffset.x + ( tW - mAlignOffset.x ) );
			else if ( tX > mSize.Width() - mPadding.Left - mPadding.Right )
				mAlignOffset.x = mSize.Width() - mPadding.Left - mPadding.Right - ( mAlignOffset.x + ( tW - mAlignOffset.x ) );
		}
	}
}

void cUITextInput::SetTheme( cUITheme * Theme ) {
	cUIControl::SetThemeControl( Theme, "textinput" );

	AutoPadding();
	AutoSize();
}

void cUITextInput::AutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		Size( mSize.x, GetSkinSize().Height() );
	}
}

void cUITextInput::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = MakePadding( true, true, false, false );
	}
}

InputTextBuffer * cUITextInput::GetInputTextBuffer() {
	return &mTextBuffer;
}

void cUITextInput::AllowEditing( const bool& allow ) {
	mAllowEditing = allow;

	if ( !mAllowEditing && mTextBuffer.Active() )
		mTextBuffer.Active( false );
}

const bool& cUITextInput::AllowEditing() const {
	return mAllowEditing;
}

void cUITextInput::Text( const String& text ) {
	cUITextBox::Text( text );

	mTextBuffer.Buffer( text );

	mTextBuffer.CursorToEnd();
}

const String& cUITextInput::Text() {
	return cUITextBox::Text();
}

void cUITextInput::ShrinkText( const Uint32& MaxWidth ) {
	mTextCache->Text( mTextBuffer.Buffer() );

	cUITextBox::ShrinkText( MaxWidth );

	mTextBuffer.Buffer( mTextCache->Text() );

	AlignFix();
}

void cUITextInput::UpdateText() {
}

Uint32 cUITextInput::OnMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK ) {
		Vector2i controlPos( Pos );
		WorldToControl( controlPos );

		Int32 curPos = mTextCache->Font()->FindClosestCursorPosFromPoint( mTextCache->Text(), controlPos );

		if ( -1 != curPos ) {
			mTextBuffer.CurPos( curPos );
			ResetWaitCursor();
		}
	}

	return cUITextBox::OnMouseClick( Pos, Flags );
}

Uint32 cUITextInput::OnMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
	cUITextBox::OnMouseDoubleClick( Pos, Flags );

	if ( IsTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) && SelCurEnd() != -1 ) {
		mTextBuffer.CurPos( SelCurEnd() );
		ResetWaitCursor();
	}

	return 1;
}

Uint32 cUITextInput::OnMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	cUIControl::OnMouseExit( Pos, Flags );

	cUIManager::instance()->SetCursor( EE_CURSOR_ARROW );

	return 1;
}

void cUITextInput::SelCurInit( const Int32& init ) {
	mTextBuffer.SelCurInit( init );
}

void cUITextInput::SelCurEnd( const Int32& end ) {
	mTextBuffer.SelCurEnd( end );

	if ( mTextBuffer.SelCurEnd() != mTextBuffer.SelCurInit() ) {
		mTextBuffer.CurPos( end );
	}
}

Int32 cUITextInput::SelCurInit() {
	return mTextBuffer.SelCurInit();
}

Int32 cUITextInput::SelCurEnd() {
	return mTextBuffer.SelCurEnd();
}

}}
