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
	mTextBuffer.Start();
	mTextBuffer.Active( false );
	mTextBuffer.SupportFreeEditing( Params.SupportFreeEditing );
	mTextBuffer.TextSelectionEnabled( IsTextSelectionEnabled() );
	mTextBuffer.MaxLength( Params.MaxLength );
	mTextBuffer.SetReturnCallback( cb::Make0( this, &UITextInput::PrivOnPressEnter ) );

	ApplyDefaultTheme();
}

UITextInput::~UITextInput() {
}

Uint32 UITextInput::Type() const {
	return UI_TYPE_TEXTINPUT;
}

bool UITextInput::IsType( const Uint32& type ) const {
	return UITextInput::Type() == type ? true : UITextBox::IsType( type );
}

void UITextInput::Update() {
	if ( IsMouseOverMeOrChilds() ) {
		UIManager::instance()->SetCursor( EE_CURSOR_IBEAM );
	}

	UITextBox::Update();

	if ( mTextBuffer.ChangedSinceLastUpdate() ) {
		Vector2f offSet = mAlignOffset;

		UITextBox::Text( mTextBuffer.Buffer() );

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

void UITextInput::OnCursorPosChange() {
	SendCommonEvent( UIEvent::EventOnCursorPosChange );
}

void UITextInput::DrawWaitingCursor() {
	if ( mVisible && mTextBuffer.Active() && mTextBuffer.SupportFreeEditing() ) {
		mWaitCursorTime += UIManager::instance()->Elapsed().AsMilliseconds();

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

void UITextInput::Draw() {
	UITextBox::Draw();

	DrawWaitingCursor();
}

Uint32 UITextInput::OnFocus() {
	UIControlAnim::OnFocus();

	if ( mAllowEditing ) {
		mTextBuffer.Active( true );

		ResetWaitCursor();
	}

	return 1;
}

Uint32 UITextInput::OnFocusLoss() {
	mTextBuffer.Active( false );
	return UITextBox::OnFocusLoss();
}

Uint32 UITextInput::OnPressEnter() {
	SendCommonEvent( UIEvent::EventOnPressEnter );
	return 0;
}

void UITextInput::PrivOnPressEnter() {
	OnPressEnter();
}

void UITextInput::PushIgnoredChar( const Uint32& ch ) {
	mTextBuffer.PushIgnoredChar( ch );
}

void UITextInput::ResetWaitCursor() {
	mShowingWait = true;
	mWaitCursorTime = 0.f;
}

void UITextInput::AlignFix() {
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

void UITextInput::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "textinput" );

	AutoPadding();
	AutoSize();
}

void UITextInput::AutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		Size( mSize.x, GetSkinSize().Height() );
	}
}

void UITextInput::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = MakePadding( true, true, false, false );
	}
}

InputTextBuffer * UITextInput::GetInputTextBuffer() {
	return &mTextBuffer;
}

void UITextInput::AllowEditing( const bool& allow ) {
	mAllowEditing = allow;

	if ( !mAllowEditing && mTextBuffer.Active() )
		mTextBuffer.Active( false );
}

const bool& UITextInput::AllowEditing() const {
	return mAllowEditing;
}

void UITextInput::Text( const String& text ) {
	UITextBox::Text( text );

	mTextBuffer.Buffer( text );

	mTextBuffer.CursorToEnd();
}

const String& UITextInput::Text() {
	return UITextBox::Text();
}

void UITextInput::ShrinkText( const Uint32& MaxWidth ) {
	mTextCache->Text( mTextBuffer.Buffer() );

	UITextBox::ShrinkText( MaxWidth );

	mTextBuffer.Buffer( mTextCache->Text() );

	AlignFix();
}

void UITextInput::UpdateText() {
}

Uint32 UITextInput::OnMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( Flags & EE_BUTTON_LMASK ) {
		Vector2i controlPos( Pos );
		WorldToControl( controlPos );

		Int32 curPos = mTextCache->Font()->FindClosestCursorPosFromPoint( mTextCache->Text(), controlPos );

		if ( -1 != curPos ) {
			mTextBuffer.CurPos( curPos );
			ResetWaitCursor();
		}
	}

	return UITextBox::OnMouseClick( Pos, Flags );
}

Uint32 UITextInput::OnMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
	UITextBox::OnMouseDoubleClick( Pos, Flags );

	if ( IsTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) && SelCurEnd() != -1 ) {
		mTextBuffer.CurPos( SelCurEnd() );
		ResetWaitCursor();
	}

	return 1;
}

Uint32 UITextInput::OnMouseExit( const Vector2i& Pos, const Uint32 Flags ) {
	UIControl::OnMouseExit( Pos, Flags );

	UIManager::instance()->SetCursor( EE_CURSOR_ARROW );

	return 1;
}

void UITextInput::SelCurInit( const Int32& init ) {
	mTextBuffer.SelCurInit( init );
}

void UITextInput::SelCurEnd( const Int32& end ) {
	mTextBuffer.SelCurEnd( end );

	if ( mTextBuffer.SelCurEnd() != mTextBuffer.SelCurInit() ) {
		mTextBuffer.CurPos( end );
	}
}

Int32 UITextInput::SelCurInit() {
	return mTextBuffer.SelCurInit();
}

Int32 UITextInput::SelCurEnd() {
	return mTextBuffer.SelCurEnd();
}

}}
