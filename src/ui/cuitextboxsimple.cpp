#include "cuitextboxsimple.hpp"
#include "cuimanager.hpp"
#include "cuithememanager.hpp"

namespace EE { namespace UI {

cUITextBoxSimple::cUITextBoxSimple( const cUITextBox::CreateParams& Params ) :
	cUIControl( Params ),
	mFont( NULL ),
	mCachedWidth( 0 ),
	mNumLines( 1 ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mAlignOffset( 0.f, 0.f )
{
	mType |= UI_TYPE_GET(UI_TYPE_TEXTBOX);

	mFont = Params.Font;

	if ( NULL == Params.Font ) {
		if ( NULL != cUIThemeManager::instance()->DefaultFont() )
			mFont = cUIThemeManager::instance()->DefaultFont();
		else
			eePRINT( "cUITextBoxSimple::cUITextBoxSimple : Created a UI TextBox without a defined font.\n" );
	}
	AutoAlign();
}

cUITextBoxSimple::~cUITextBoxSimple() {
}

void cUITextBoxSimple::Draw() {
	if ( mVisible ) {
		cUIControl::Draw();

		if ( mCachedWidth ) {
			if ( mFlags & UI_CLIP_ENABLE )
				cUIManager::instance()->ClipEnable( mScreenPos.x + mPadding.Left, mScreenPos.y + mPadding.Top, mSize.Width() - mPadding.Left - mPadding.Right, mSize.Height() - mPadding.Bottom );

			eeColorA OldColor 		= mFont->Color();
			eeColorA OldShadowColor = mFont->ShadowColor();

			mFont->Color( mFontColor );
			mFont->ShadowColor( mFontShadowColor );

			mFont->Draw( mText, (eeFloat)mScreenPos.x + mAlignOffset.x + (eeFloat)mPadding.Left + 1.f, (eeFloat)mScreenPos.y + mAlignOffset.y + (eeFloat)mPadding.Top, Flags(), 1.f, 0.f, mBlend );

			mFont->Color( OldColor );
			mFont->ShadowColor( OldShadowColor );

			if ( mFlags & UI_CLIP_ENABLE )
				cUIManager::instance()->ClipDisable();
		}
	}
}

cFont * cUITextBoxSimple::Font() const {
	return mFont;
}

void cUITextBoxSimple::Font( cFont * font ) {
	mFont = font;
	AutoShrink();
	AutoSize();
	AutoAlign();
	OnFontChanged();
}

const std::wstring& cUITextBoxSimple::Text() {
	return mText;
}

void cUITextBoxSimple::Text( const std::string& text ) {
	Text( stringTowstring( text ) );
}

void cUITextBoxSimple::Text( const std::wstring& text ) {
	mText = text;

	if ( NULL != mFont && mText.size() ) {
		std::vector<eeFloat> LinesWidth;

		mFont->CacheWidth( mText, LinesWidth, mCachedWidth, mNumLines );
	}else {
		mCachedWidth = 0;
	}

	AutoShrink();
	AutoSize();
	AutoAlign();
	OnTextChanged();
}

const eeColorA& cUITextBoxSimple::Color() const {
	return mFontColor;
}

void cUITextBoxSimple::Color( const eeColorA& color ) {
	mFontColor = color;
	Alpha( color.A() );
}

const eeColorA& cUITextBoxSimple::ShadowColor() const {
	return mFontShadowColor;
}

void cUITextBoxSimple::ShadowColor( const eeColorA& color ) {
	mFontShadowColor = color;
	Alpha( color.A() );
}

void cUITextBoxSimple::Alpha( const eeFloat& alpha ) {
	mFontColor.Alpha = (Uint8)alpha;
	mFontShadowColor.Alpha = (Uint8)alpha;
}

void cUITextBoxSimple::AutoShrink() {
	if ( Flags() & UI_AUTO_SHRINK_TEXT ) {
		mFont->ShrinkText( mText, mSize.Width() );
	}
}

void cUITextBoxSimple::AutoSize() {
	if ( Flags() & UI_AUTO_SIZE ) {
		mSize.Width( (Int32)mCachedWidth );
		mSize.Height( (Int32)mFont->GetFontSize() * mNumLines );
	}
}

void cUITextBoxSimple::AutoAlign() {
	switch ( FontHAlignGet( Flags() ) ) {
		case UI_HALIGN_CENTER:
			mAlignOffset.x = (eeFloat)( (Int32)( mSize.x - mCachedWidth ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mAlignOffset.x = ( (eeFloat)mSize.x - (eeFloat)mCachedWidth );
			break;
		case UI_HALIGN_LEFT:
			mAlignOffset.x = 0.f;
			break;
	}

	switch ( FontVAlignGet( Flags() ) ) {
		case UI_VALIGN_CENTER:
			mAlignOffset.y = (eeFloat)( ( (Int32)( mSize.y - GetTextHeight() ) ) / 2 );
			break;
		case UI_VALIGN_BOTTOM:
			mAlignOffset.y = ( (eeFloat)mSize.y - GetTextHeight() );
			break;
		case UI_VALIGN_TOP:
			mAlignOffset.y = 0.f;
			break;
	}
}

void cUITextBoxSimple::OnSizeChange() {
	AutoShrink();
	AutoSize();
	AutoAlign();

	cUIControl::OnSizeChange();
}

void cUITextBoxSimple::OnTextChanged() {
	SendCommonEvent( cUIEvent::EventOnTextChanged );
}

void cUITextBoxSimple::OnFontChanged() {
	SendCommonEvent( cUIEvent::EventOnFontChanged );
}

void cUITextBoxSimple::Padding( const eeRecti& padding ) {
	mPadding = padding;
}

const eeRecti& cUITextBoxSimple::Padding() const {
	return mPadding;
}

void cUITextBoxSimple::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme );

	if ( NULL == mFont && NULL != Theme->DefaultFont() ) {
		mFont = Theme->DefaultFont();
	}
}

eeFloat cUITextBoxSimple::GetTextWidth() {
	return mCachedWidth;
}

eeFloat cUITextBoxSimple::GetTextHeight() {
	return ( (eeFloat)mFont->GetFontSize() * (eeFloat)mNumLines );
}

const eeInt& cUITextBoxSimple::GetNumLines() const {
	return mNumLines;
}

}}
