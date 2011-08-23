#include "cuitextedit.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUITextEdit::cUITextEdit( cUITextEdit::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mTextInput( NULL ),
	mHScrollBar( NULL ),
	mVScrollBar( NULL ),
	mHScrollBarMode( Params.HScrollBar ),
	mVScrollBarMode( Params.VScrollBar ),
	mSkipValueChange( false )
{
	Uint32 extraFlags = 0;

	if ( mFlags & UI_ANCHOR_LEFT )
		extraFlags |= UI_ANCHOR_LEFT;

	if ( mFlags & UI_ANCHOR_RIGHT )
		extraFlags |= UI_ANCHOR_RIGHT;

	if ( mFlags & UI_ANCHOR_TOP )
		extraFlags |= UI_ANCHOR_TOP;

	if ( mFlags & UI_ANCHOR_BOTTOM )
		extraFlags |= UI_ANCHOR_BOTTOM;

	cUITextInput::CreateParams TIParams;
	TIParams.Parent( this );
	TIParams.Size				= mSize;
	TIParams.Flags				= UI_VALIGN_TOP | UI_HALIGN_LEFT | extraFlags;
	TIParams.MaxLenght			= 1024 * 1024 * 10;
	TIParams.Font				= Params.Font;
	TIParams.FontColor			= Params.FontColor;
	TIParams.FontShadowColor	= Params.FontShadowColor;

	if ( Params.WordWrap && !( mFlags & UI_AUTO_SHRINK_TEXT ) )
		mFlags |= UI_AUTO_SHRINK_TEXT;

	mTextInput	= eeNew( cUITextInput, ( TIParams ) );
	mTextInput->GetInputTextBuffer()->SupportNewLine( true );
	mTextInput->Visible( true );
	mTextInput->Enabled( true );
	mTextInput->AddEventListener( cUIEvent::EventOnSizeChange		, cb::Make1( this, &cUITextEdit::OnInputSizeChange ) );
	mTextInput->AddEventListener( cUIEvent::EventOnTextChanged		, cb::Make1( this, &cUITextEdit::OnInputSizeChange ) );
	mTextInput->AddEventListener( cUIEvent::EventOnPressEnter		, cb::Make1( this, &cUITextEdit::OnInputSizeChange ) );
	mTextInput->AddEventListener( cUIEvent::EventOnCursorPosChange	, cb::Make1( this, &cUITextEdit::OnCursorPosChange ) );

	cUIScrollBar::CreateParams ScrollBarP;
	ScrollBarP.Parent( this );
	ScrollBarP.PosSet( mSize.Width() - 15, 0 );
	ScrollBarP.Size					= eeSize( 15, mSize.Height() );
	ScrollBarP.Flags				= UI_AUTO_SIZE;
	ScrollBarP.VerticalScrollBar	= true;
	mVScrollBar = eeNew( cUIScrollBar, ( ScrollBarP ) );
	mVScrollBar->Value( 1 );

	ScrollBarP.PosSet( 0, mSize.Height() - 15 );
	ScrollBarP.Size					= eeSize( mSize.Width() - mVScrollBar->Size().Width(), 15 );
	ScrollBarP.VerticalScrollBar	= false;
	mHScrollBar = eeNew( cUIScrollBar, ( ScrollBarP ) );

	mVScrollBar->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cUITextEdit::OnVScrollValueChange ) );
	mHScrollBar->AddEventListener( cUIEvent::EventOnValueChange, cb::Make1( this, &cUITextEdit::OnHScrollValueChange ) );

	AutoPadding();

	OnSizeChange();

	ApplyDefaultTheme();

	mTextInput->Size( mSize - eeSize( mPadding.Left + mPadding.Right, mPadding.Top + mPadding.Bottom ) );
}

cUITextEdit::~cUITextEdit() {
}

Uint32 cUITextEdit::Type() const {
	return UI_TYPE_TEXTEDIT;
}

bool cUITextEdit::IsType( const Uint32& type ) const {
	return cUITextEdit::Type() == type ? true : cUIComplexControl::IsType( type );
}

void cUITextEdit::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "textedit" );

	mTextInput->ForceThemeSkin( Theme, "textedit_box" );

	AutoPadding();

	OnSizeChange();
}

void cUITextEdit::OnSizeChange() {
	mHScrollBar->Pos( 0, mSize.Height() - mHScrollBar->Size().Height() );
	mVScrollBar->Pos( mSize.Width() - mVScrollBar->Size().Width(), 0 );

	mHScrollBar->Size( mSize.Width(), mHScrollBar->Size().Height() );
	mVScrollBar->Size( mVScrollBar->Size().Width(), mSize.Height() );

	mTextInput->Pos( mPadding.Left, mPadding.Top );

	ScrollbarsSet();

	FixScroll();
}

void cUITextEdit::OnParentSizeChange( const eeVector2i& SizeChange ) {
	cUIComplexControl::OnParentSizeChange( SizeChange );

	OnInputSizeChange( NULL );
}

void cUITextEdit::OnAlphaChange() {
	mTextInput->Alpha( mAlpha );
	mHScrollBar->Alpha( mAlpha );
	mVScrollBar->Alpha( mAlpha );

	cUIComplexControl::OnAlphaChange();
}

void cUITextEdit::FixScroll() {
	eeInt Width		= mSize.Width()		- mPadding.Left - mPadding.Right;
	eeInt Height	= mSize.Height()	- mPadding.Top	- mPadding.Bottom;

	if ( mHScrollBar->Visible() )
		Height -= mHScrollBar->Size().Height();

	eeInt diff;
	eeFloat pos;

	if ( mTextInput->Size().Height() - Height >= 0 ) {
		diff = mTextInput->Size().Height() - Height;

		pos = diff * mVScrollBar->Value();

		mTextInput->Pos( mTextInput->Pos().x, mPadding.Top - pos );
	}

	if ( mTextInput->Size().Width() - Width >= 0 ) {
		diff = mTextInput->Size().Width() - Width;

		pos = diff * mHScrollBar->Value();

		mTextInput->Pos( mPadding.Left - pos, mTextInput->Pos().y );
	}
}

void cUITextEdit::ScrollbarsSet() {
	switch ( mHScrollBarMode ) {
		case UI_SCROLLBAR_ALWAYS_OFF:
		{
			mHScrollBar->Visible( false );
			mHScrollBar->Enabled( false );
			break;
		}
		case UI_SCROLLBAR_ALWAYS_ON:
		{
			mHScrollBar->Visible( true );
			mHScrollBar->Enabled( true );
			break;
		}
		case UI_SCROLLBAR_AUTO:
		{
			if ( mTextInput->GetTextWidth() > mSize.Width() - mPadding.Left - mPadding.Right ) {
				mHScrollBar->Visible( true );
				mHScrollBar->Enabled( true );
			} else {
				mHScrollBar->Visible( false );
				mHScrollBar->Enabled( false );
			}
			break;
		}
	}

	switch ( mVScrollBarMode ) {
		case UI_SCROLLBAR_ALWAYS_OFF:
		{
			mVScrollBar->Visible( false );
			mVScrollBar->Enabled( false );
			break;
		}
		case UI_SCROLLBAR_ALWAYS_ON:
		{
			mVScrollBar->Visible( true );
			mVScrollBar->Enabled( true );
			break;
		}
		case UI_SCROLLBAR_AUTO:
		{
			eeInt extraH = 0;

			if ( mHScrollBar->Visible() )
				extraH = mHScrollBar->Size().Height();

			if ( mTextInput->GetTextHeight() > mSize.Height() - mPadding.Top - mPadding.Bottom - extraH ) {
				mVScrollBar->Visible( true );
				mVScrollBar->Enabled( true );
			} else {
				mVScrollBar->Visible( false );
				mVScrollBar->Enabled( false );
			}
			break;
		}
	}

	if ( !mVScrollBar->Visible() && mHScrollBar->Visible() ) {
		mHScrollBar->Size( mSize.Width(), mHScrollBar->Size().Height() );
	} else {
		mHScrollBar->Size( mSize.Width() - mVScrollBar->Size().Width(), mHScrollBar->Size().Height() );
	}

	if ( UI_SCROLLBAR_AUTO == mHScrollBarMode && mVScrollBar->Visible() && !mHScrollBar->Visible() ) {
		if ( mTextInput->GetTextWidth() > mSize.Width() - mPadding.Left - mPadding.Right - mVScrollBar->Size().Width() ) {
			mHScrollBar->Visible( true );
			mHScrollBar->Enabled( true );
		}
	}

	if ( mFlags & UI_AUTO_SHRINK_TEXT ) {
		mVScrollBar->Visible( true );
		mVScrollBar->Enabled( true );
	}
}

void cUITextEdit::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = MakePadding();
	}
}

void cUITextEdit::OnVScrollValueChange( const cUIEvent * Event ) {
	if ( !mSkipValueChange )
		FixScroll();
}

void cUITextEdit::OnHScrollValueChange( const cUIEvent * Event ) {
	if ( !mSkipValueChange )
		FixScroll();
}

cUITextInput * cUITextEdit::TextInput() const {
	return mTextInput;
}

cUIScrollBar * cUITextEdit::HScrollBar() const {
	return mHScrollBar;
}

cUIScrollBar * cUITextEdit::VScrollBar() const {
	return mVScrollBar;
}

const String& cUITextEdit::Text() const {
	return mTextInput->Text();
}

void cUITextEdit::Text( const String& Txt ) {
	mTextInput->Text( Txt );

	OnInputSizeChange();

	OnSizeChange();
}

void cUITextEdit::OnInputSizeChange( const cUIEvent * Event ) {
	eeInt Width		= mSize.Width()		- mPadding.Left - mPadding.Right;
	eeInt Height	= mSize.Height()	- mPadding.Top	- mPadding.Bottom;

	if ( NULL != Event ) {
		if ( Event->EventType() == cUIEvent::EventOnPressEnter ) {
			mHScrollBar->Value( 0 );
		}
	}

	ScrollbarsSet();

	if ( mHScrollBar->Visible() )
		Height	-= mHScrollBar->Size().Height();

	if ( mVScrollBar->Visible() )
		Width	-= mVScrollBar->Size().Width();

	ShrinkText( Width );

	if ( ( mFlags & UI_AUTO_SHRINK_TEXT ) && mTextInput->GetTextHeight() < Height ) {
		mVScrollBar->Visible( false );
		mVScrollBar->Enabled( false );
	}

	if ( mTextInput->Size().Width() < Width || mTextInput->Size().Height() < Height ) {
		if ( mTextInput->Size().Width() < Width && mTextInput->Size().Height() < Height ) {
			mTextInput->Size( Width, Height );
		} else {
			if ( mTextInput->Size().Width() < Width ) {
				mTextInput->Size( Width, mTextInput->Size().Height() );
			} else {
				mTextInput->Size( mTextInput->Size().Width(), Height );
			}
		}
	}

	if ( mTextInput->GetTextWidth() > Width || mTextInput->GetTextHeight() > Height ) {
		if ( mTextInput->GetTextWidth() > Width && mTextInput->GetTextHeight() > Height ) {
			mTextInput->Size( mTextInput->GetTextWidth(), mTextInput->GetTextHeight() );
		} else {
			if ( mTextInput->GetTextWidth() > Width ) {
				mTextInput->Size( mTextInput->GetTextWidth(), Height );
			} else {
				mTextInput->Size( Width, mTextInput->GetTextHeight() );
			}
		}
	} else {
		mTextInput->Size( Width, Height );
	}

	FixScroll();
	FixScrollToCursor();
}

void cUITextEdit::OnCursorPosChange( const cUIEvent * Event ) {
	FixScrollToCursor();
}

void cUITextEdit::FixScrollToCursor() {
	eeInt Width		= mSize.Width()		- mPadding.Left - mPadding.Right;
	eeInt Height	= mSize.Height()	- mPadding.Top	- mPadding.Bottom;

	if ( mVScrollBar->Visible() )
		Width -= mVScrollBar->Size().Width();

	if ( mHScrollBar->Visible() )
		Height -= mHScrollBar->Size().Height();

	if ( FontHAlignGet( mTextInput->Flags() ) == UI_HALIGN_LEFT ) {
		Uint32 NLPos	= 0;
		Uint32 LineNum = mTextInput->GetInputTextBuffer()->GetCurPosLinePos( NLPos );

		mTextInput->GetTextCache()->Font()->SetText(
			mTextInput->GetInputTextBuffer()->Buffer().substr(
				NLPos, mTextInput->GetInputTextBuffer()->CurPos() - NLPos
			)
		);

		mSkipValueChange = true;

		eeFloat tW	= mTextInput->GetTextCache()->Font()->GetTextWidth();
		eeFloat tH	= (eeFloat)(LineNum + 1) * (eeFloat)mTextInput->GetTextCache()->Font()->GetFontSize();

		if ( tW > Width ) {
			mTextInput->Pos( mPadding.Left + Width - tW, mTextInput->Pos().y );
		} else {
			mTextInput->Pos( mPadding.Left, mTextInput->Pos().y );
		}

		if ( tH > Height ) {
			mTextInput->Pos( mTextInput->Pos().x, mPadding.Top + Height - tH );
		} else {
			mTextInput->Pos( mTextInput->Pos().x, mPadding.Top );
		}

		mHScrollBar->Value( tW / mTextInput->Size().Width() );
		mVScrollBar->Value( tH / mTextInput->Size().Height() );

		mSkipValueChange = false;
	}
}

void cUITextEdit::ShrinkText( const Uint32& Width ) {
	if ( Flags() & UI_AUTO_SHRINK_TEXT ) {
		mTextInput->ShrinkText( Width );
	}
}

void cUITextEdit::Update() {
	cUIControlAnim::Update();

	if ( mTextInput->Enabled() && mTextInput->Visible() && mTextInput->IsMouseOver() && mVScrollBar->Visible() ) {
		Uint32 Flags 			= cUIManager::instance()->GetInput()->ClickTrigger();

		if ( Flags & EE_BUTTONS_WUWD )
			mVScrollBar->Slider()->ManageClick( Flags );
	}
}

void cUITextEdit::AllowEditing( const bool& allow ) {
	mTextInput->AllowEditing( allow );
}

const bool& cUITextEdit::AllowEditing() const {
	return mTextInput->AllowEditing();
}

}}
