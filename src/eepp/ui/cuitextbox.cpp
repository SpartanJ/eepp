#include <eepp/ui/cuitextbox.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/ui/cuithememanager.hpp>
#include <eepp/graphics/ctextcache.hpp>
#include <eepp/graphics/cfont.hpp>
#include <eepp/graphics/cprimitives.hpp>
#include <eepp/window/cclipboard.hpp>

namespace EE { namespace UI {

cUITextBox::cUITextBox( const cUITextBox::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mFontSelectionBackColor( Params.FontSelectionBackColor ),
	mAlignOffset( 0.f, 0.f ),
	mSelCurInit( -1 ),
	mSelCurEnd( -1 )
{
	mTextCache = eeNew( cTextCache, () );
	mTextCache->Font( Params.Font );
	mTextCache->Color( mFontColor );
	mTextCache->ShadowColor( mFontShadowColor );

	if ( NULL == Params.Font ) {
		if ( NULL != cUIThemeManager::instance()->DefaultFont() )
			mTextCache->Font( cUIThemeManager::instance()->DefaultFont() );
		else
			eePRINTL( "cUITextBox::cUITextBox : Created a UI TextBox without a defined font." );
	}

	AutoAlign();
}

cUITextBox::~cUITextBox() {
	eeSAFE_DELETE( mTextCache );
}

Uint32 cUITextBox::Type() const {
	return UI_TYPE_TEXTBOX;
}

bool cUITextBox::IsType( const Uint32& type ) const {
	return cUITextBox::Type() == type ? true : cUIComplexControl::IsType( type );
}

void cUITextBox::Draw() {
	if ( mVisible && 0.f != mAlpha ) {
		cUIControlAnim::Draw();

		DrawSelection();

		if ( mTextCache->GetTextWidth() ) {
			if ( mFlags & UI_CLIP_ENABLE ) {
				cUIManager::instance()->ClipEnable(
						mScreenPos.x + mPadding.Left,
						mScreenPos.y + mPadding.Top,
						mSize.Width() - mPadding.Left - mPadding.Right,
						mSize.Height() - mPadding.Top - mPadding.Bottom
				);
			}

			mTextCache->Flags( Flags() );
			mTextCache->Draw( (Float)mScreenPos.x + mAlignOffset.x + (Float)mPadding.Left, (Float)mScreenPos.y + mAlignOffset.y + (Float)mPadding.Top, Vector2f::One, 0.f, Blend() );

			if ( mFlags & UI_CLIP_ENABLE ) {
				cUIManager::instance()->ClipDisable();
			}
		}
	}
}

cFont * cUITextBox::Font() const {
	return mTextCache->Font();
}

void cUITextBox::Font( cFont * font ) {
	if ( mTextCache->Font() != font ) {
		mTextCache->Font( font );
		AutoShrink();
		AutoSize();
		AutoAlign();
		OnFontChanged();
	}
}

const String& cUITextBox::Text() {
	if ( mFlags & UI_AUTO_SHRINK_TEXT )
		return mString;

	return mTextCache->Text();
}

void cUITextBox::Text( const String& text ) {
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

const ColorA& cUITextBox::Color() const {
	return mFontColor;
}

void cUITextBox::Color( const ColorA& color ) {
	mFontColor = color;
	mTextCache->Color( color );

	Alpha( color.A() );
}

const ColorA& cUITextBox::ShadowColor() const {
	return mFontShadowColor;
}

void cUITextBox::ShadowColor( const ColorA& color ) {
	mFontShadowColor = color;
	mTextCache->ShadowColor( mFontColor );
}

const ColorA& cUITextBox::SelectionBackColor() const {
	return mFontSelectionBackColor;
}

void cUITextBox::SelectionBackColor( const ColorA& color ) {
	mFontSelectionBackColor = color;
}

void cUITextBox::Alpha( const Float& alpha ) {
	cUIControlAnim::Alpha( alpha );
	mFontColor.Alpha = (Uint8)alpha;
	mFontShadowColor.Alpha = (Uint8)alpha;

	mTextCache->Alpha( mFontColor.Alpha );
}

void cUITextBox::AutoShrink() {
	if ( mFlags & UI_AUTO_SHRINK_TEXT ) {
		ShrinkText( mSize.Width() );
	}
}

void cUITextBox::ShrinkText( const Uint32& MaxWidth ) {
	if ( mFlags & UI_AUTO_SHRINK_TEXT ) {
		mTextCache->Text( mString );
	}

	mTextCache->Font()->ShrinkText( mTextCache->Text(), MaxWidth );
}

void cUITextBox::AutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		mSize.Width( (int)mTextCache->GetTextWidth() );
		mSize.Height( (int)mTextCache->GetTextHeight() );
	}
}

void cUITextBox::AutoAlign() {
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

Uint32 cUITextBox::OnFocusLoss() {
	SelCurInit( -1 );
	SelCurEnd( -1 );

	return 1;
}

void cUITextBox::OnSizeChange() {
	AutoShrink();
	AutoSize();
	AutoAlign();

	cUIControlAnim::OnSizeChange();

	mTextCache->Cache();
}

void cUITextBox::OnTextChanged() {
	SendCommonEvent( cUIEvent::EventOnTextChanged );
}

void cUITextBox::OnFontChanged() {
	SendCommonEvent( cUIEvent::EventOnFontChanged );
}

void cUITextBox::Padding( const Recti& padding ) {
	mPadding = padding;
}

const Recti& cUITextBox::Padding() const {
	return mPadding;
}

void cUITextBox::SetTheme( cUITheme * Theme ) {
	cUIControlAnim::SetTheme( Theme );

	if ( NULL == mTextCache->Font() && NULL != Theme->Font() ) {
		mTextCache->Font( Theme->Font() );
	}
}

cTextCache * cUITextBox::GetTextCache() {
	return mTextCache;
}

Float cUITextBox::GetTextWidth() {
	return mTextCache->GetTextWidth();
}

Float cUITextBox::GetTextHeight() {
	return mTextCache->GetTextHeight();
}

const int& cUITextBox::GetNumLines() const {
	return mTextCache->GetNumLines();
}

const Vector2f& cUITextBox::AlignOffset() const {
	return mAlignOffset;
}

Uint32 cUITextBox::OnMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags ) {
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

	return cUIComplexControl::OnMouseDoubleClick( Pos, Flags );
}

Uint32 cUITextBox::OnMouseClick( const Vector2i& Pos, const Uint32 Flags ) {
	if ( IsTextSelectionEnabled() && ( Flags & EE_BUTTON_LMASK ) ) {
		if ( SelCurInit() == SelCurEnd() ) {
			SelCurInit( -1 );
			SelCurEnd( -1 );
		}

		mControlFlags &= ~UI_CTRL_FLAG_SELECTING;
	}

	return 1;
}

Uint32 cUITextBox::OnMouseDown( const Vector2i& Pos, const Uint32 Flags ) {
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

	return cUIComplexControl::OnMouseDown( Pos, Flags );
}

void cUITextBox::DrawSelection() {
	if ( SelCurInit() != SelCurEnd() ) {
		Int32 init		= eemin( SelCurInit(), SelCurEnd() );
		Int32 end		= eemax( SelCurInit(), SelCurEnd() );

		if ( init < 0 && end > (Int32)mTextCache->Text().size() ) {
			return;
		}

		Int32 lastEnd;
		Vector2i initPos, endPos;

		cPrimitives P;
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

bool cUITextBox::IsTextSelectionEnabled() const {
	return 0 != ( mFlags & UI_TEXT_SELECTION_ENABLED );
}

void cUITextBox::SelCurInit( const Int32& init ) {
	mSelCurInit = init;
}

void cUITextBox::SelCurEnd( const Int32& end ) {
	mSelCurEnd = end;
}

Int32 cUITextBox::SelCurInit() {
	return mSelCurInit;
}

Int32 cUITextBox::SelCurEnd() {
	return mSelCurEnd;
}

}}
