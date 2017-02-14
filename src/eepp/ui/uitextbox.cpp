#include <eepp/ui/uitextbox.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/graphics/textcache.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/primitives.hpp>
#include <eepp/window/clipboard.hpp>

namespace EE { namespace UI {

UITextBox::UITextBox( const UITextBox::CreateParams& Params ) :
	UIComplexControl( Params ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mFontSelectionBackColor( Params.FontSelectionBackColor ),
	mAlignOffset( 0.f, 0.f ),
	mSelCurInit( -1 ),
	mSelCurEnd( -1 )
{
	mTextCache = eeNew( TextCache, () );
	mTextCache->Font( Params.Font );
	mTextCache->Color( mFontColor );
	mTextCache->ShadowColor( mFontShadowColor );

	if ( NULL == Params.Font ) {
		if ( NULL != UIThemeManager::instance()->DefaultFont() )
			mTextCache->Font( UIThemeManager::instance()->DefaultFont() );
		else
			eePRINTL( "UITextBox::UITextBox : Created a UI TextBox without a defined font." );
	}

	AutoAlign();
}

UITextBox::~UITextBox() {
	eeSAFE_DELETE( mTextCache );
}

Uint32 UITextBox::Type() const {
	return UI_TYPE_TEXTBOX;
}

bool UITextBox::IsType( const Uint32& type ) const {
	return UITextBox::Type() == type ? true : UIComplexControl::IsType( type );
}

void UITextBox::Draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UIControlAnim::Draw();

		DrawSelection();

		if ( mTextCache->GetTextWidth() ) {
			if ( mFlags & UI_CLIP_ENABLE ) {
				UIManager::instance()->ClipEnable(
						mScreenPos.x + mPadding.Left,
						mScreenPos.y + mPadding.Top,
						mSize.Width() - mPadding.Left - mPadding.Right,
						mSize.Height() - mPadding.Top - mPadding.Bottom
				);
			}

			mTextCache->Flags( Flags() );
			mTextCache->Draw( (Float)mScreenPos.x + mAlignOffset.x + (Float)mPadding.Left, (Float)mScreenPos.y + mAlignOffset.y + (Float)mPadding.Top, Vector2f::One, 0.f, Blend() );

			if ( mFlags & UI_CLIP_ENABLE ) {
				UIManager::instance()->ClipDisable();
			}
		}
	}
}

Graphics::Font * UITextBox::Font() const {
	return mTextCache->Font();
}

void UITextBox::Font( Graphics::Font * font ) {
	if ( mTextCache->Font() != font ) {
		mTextCache->Font( font );
		AutoShrink();
		AutoSize();
		AutoAlign();
		OnFontChanged();
	}
}

const String& UITextBox::Text() {
	if ( mFlags & UI_AUTO_SHRINK_TEXT )
		return mString;

	return mTextCache->Text();
}

void UITextBox::Text( const String& text ) {
	if ( mFlags & UI_AUTO_SHRINK_TEXT ) {
		mString = text;
		mTextCache->Text( mString );
	} else {
		mTextCache->Text( text );
	}

	AutoShrink();
	AutoSize();
	AutoAlign();
	OnTextChanged();
}

const ColorA& UITextBox::Color() const {
	return mFontColor;
}

void UITextBox::Color( const ColorA& color ) {
	mFontColor = color;
	mTextCache->Color( color );

	Alpha( color.a() );
}

const ColorA& UITextBox::ShadowColor() const {
	return mFontShadowColor;
}

void UITextBox::ShadowColor( const ColorA& color ) {
	mFontShadowColor = color;
	mTextCache->ShadowColor( mFontColor );
}

const ColorA& UITextBox::SelectionBackColor() const {
	return mFontSelectionBackColor;
}

void UITextBox::SelectionBackColor( const ColorA& color ) {
	mFontSelectionBackColor = color;
}

void UITextBox::Alpha( const Float& alpha ) {
	UIControlAnim::Alpha( alpha );
	mFontColor.Alpha = (Uint8)alpha;
	mFontShadowColor.Alpha = (Uint8)alpha;

	mTextCache->Alpha( mFontColor.Alpha );
}

void UITextBox::AutoShrink() {
	if ( mFlags & UI_AUTO_SHRINK_TEXT ) {
		ShrinkText( mSize.Width() );
	}
}

void UITextBox::ShrinkText( const Uint32& MaxWidth ) {
	if ( mFlags & UI_AUTO_SHRINK_TEXT ) {
		mTextCache->Text( mString );
	}

	mTextCache->Font()->ShrinkText( mTextCache->Text(), MaxWidth );
}

void UITextBox::AutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		mSize.Width( (int)mTextCache->GetTextWidth() );
		mSize.Height( (int)mTextCache->GetTextHeight() );
	}
}

void UITextBox::AutoAlign() {
	switch ( FontHAlignGet( Flags() ) ) {
		case UI_HALIGN_CENTER:
			mAlignOffset.x = (Float)( (Int32)( mSize.x - mTextCache->GetTextWidth() ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mAlignOffset.x = ( (Float)mSize.x - (Float)mTextCache->GetTextWidth() );
			break;
		case UI_HALIGN_LEFT:
			mAlignOffset.x = 0.f;
			break;
	}

	switch ( FontVAlignGet( Flags() ) ) {
		case UI_VALIGN_CENTER:
			mAlignOffset.y = (Float)( ( (Int32)( mSize.y - mTextCache->GetTextHeight() ) ) / 2 ) - 1;
			break;
		case UI_VALIGN_BOTTOM:
			mAlignOffset.y = ( (Float)mSize.y - (Float)mTextCache->GetTextHeight() );
			break;
		case UI_VALIGN_TOP:
			mAlignOffset.y = 0.f;
			break;
	}
}

Uint32 UITextBox::OnFocusLoss() {
	SelCurInit( -1 );
	SelCurEnd( -1 );

	return 1;
}

void UITextBox::OnSizeChange() {
	AutoShrink();
	AutoSize();
	AutoAlign();

	UIControlAnim::OnSizeChange();

	mTextCache->Cache();
}

void UITextBox::OnTextChanged() {
	SendCommonEvent( UIEvent::EventOnTextChanged );
}

void UITextBox::OnFontChanged() {
	SendCommonEvent( UIEvent::EventOnFontChanged );
}

void UITextBox::Padding( const Recti& padding ) {
	mPadding = padding;
}

const Recti& UITextBox::Padding() const {
	return mPadding;
}

void UITextBox::SetTheme( UITheme * Theme ) {
	UIControlAnim::SetTheme( Theme );

	if ( NULL == mTextCache->Font() && NULL != Theme->Font() ) {
		mTextCache->Font( Theme->Font() );
	}
}

TextCache * UITextBox::GetTextCache() {
	return mTextCache;
}

Float UITextBox::GetTextWidth() {
	return mTextCache->GetTextWidth();
}

Float UITextBox::GetTextHeight() {
	return mTextCache->GetTextHeight();
}

const int& UITextBox::GetNumLines() const {
	return mTextCache->GetNumLines();
}

const Vector2f& UITextBox::AlignOffset() const {
	return mAlignOffset;
}

Uint32 UITextBox::OnMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( IsTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		Vector2i controlPos( Pos );
		WorldToControl( controlPos );

		Int32 curPos = mTextCache->Font()->FindClosestCursorPosFromPoint( mTextCache->Text(), controlPos );

		if ( -1 != curPos ) {
			Int32 tSelCurInit, tSelCurEnd;

			mTextCache->Font()->SelectSubStringFromCursor( mTextCache->Text(), curPos, tSelCurInit, tSelCurEnd );

			SelCurInit( tSelCurInit );
			SelCurEnd( tSelCurEnd );

			mControlFlags &= ~UI_CTRL_FLAG_SELECTING;
		}
	}

	return UIComplexControl::OnMouseDoubleClick( Pos, Flags );
}

Uint32 UITextBox::OnMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( IsTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		if ( SelCurInit() == SelCurEnd() ) {
			SelCurInit( -1 );
			SelCurEnd( -1 );
		}

		mControlFlags &= ~UI_CTRL_FLAG_SELECTING;
	}

	return 1;
}

Uint32 UITextBox::OnMouseDown( const Vector2i& Pos, const Uint32 Flags ) {
	if ( IsTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		Vector2i controlPos( Pos );
		WorldToControl( controlPos );

		Int32 curPos = mTextCache->Font()->FindClosestCursorPosFromPoint( mTextCache->Text(), controlPos );

		if ( -1 != curPos ) {
			if ( -1 == SelCurInit() || !( mControlFlags & UI_CTRL_FLAG_SELECTING ) ) {
				SelCurInit( curPos );
				SelCurEnd( curPos );
			} else {
				SelCurEnd( curPos );
			}
		}

		mControlFlags |= UI_CTRL_FLAG_SELECTING;
	}

	return UIComplexControl::OnMouseDown( Pos, Flags );
}

void UITextBox::DrawSelection() {
	if ( SelCurInit() != SelCurEnd() ) {
		Int32 init		= eemin( SelCurInit(), SelCurEnd() );
		Int32 end		= eemax( SelCurInit(), SelCurEnd() );

		if ( init < 0 && end > (Int32)mTextCache->Text().size() ) {
			return;
		}

		Int32 lastEnd;
		Vector2i initPos, endPos;

		Primitives P;
		P.SetColor( mFontSelectionBackColor );

		do {
			initPos	= mTextCache->Font()->GetCursorPos( mTextCache->Text(), init );
			lastEnd = mTextCache->Text().find_first_of( '\n', init );

			if ( lastEnd < end && -1 != lastEnd ) {
				endPos	= mTextCache->Font()->GetCursorPos( mTextCache->Text(), lastEnd );
				init	= lastEnd + 1;
			} else {
				endPos	= mTextCache->Font()->GetCursorPos( mTextCache->Text(), end );
				lastEnd = end;
			}

			P.DrawRectangle( Rectf( mScreenPos.x + initPos.x + mAlignOffset.x + mPadding.Left,
									  mScreenPos.y + initPos.y - mTextCache->Font()->GetFontHeight() + mAlignOffset.y + mPadding.Top,
									  mScreenPos.x + endPos.x + mAlignOffset.x + mPadding.Left,
									  mScreenPos.y + endPos.y + mAlignOffset.y + mPadding.Top )
			);
		} while ( end != lastEnd );
	}
}

bool UITextBox::IsTextSelectionEnabled() const {
	return 0 != ( mFlags & UI_TEXT_SELECTION_ENABLED );
}

void UITextBox::SelCurInit( const Int32& init ) {
	mSelCurInit = init;
}

void UITextBox::SelCurEnd( const Int32& end ) {
	mSelCurEnd = end;
}

Int32 UITextBox::SelCurInit() {
	return mSelCurInit;
}

Int32 UITextBox::SelCurEnd() {
	return mSelCurEnd;
}

}}
