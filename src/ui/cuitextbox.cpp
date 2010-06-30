#include "cuitextbox.hpp"

namespace EE { namespace UI {

cUITextBox::cUITextBox( const cUITextBox::CreateParams& Params ) :
	cUIControlAnim( Params ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mAlignOffset( 0.f, 0.f )
{
	mTextCache.Font( Params.Font );
	mTextCache.Color( mFontColor );
	mTextCache.ShadowColor( mFontShadowColor );

	mType |= UI_TYPE_GET(UI_TYPE_TEXTBOX);

	AutoAlign();
}

cUITextBox::~cUITextBox() {
}

void cUITextBox::Draw() {
	if ( mVisible ) {
		cUIControl::Draw();

		eeVector2i Pos = mPos;
		ControlToScreen( Pos );

		mTextCache.Draw( (eeFloat)Pos.x + mAlignOffset.x, (eeFloat)Pos.y + mAlignOffset.y, Flags(), 1.f, 0.f, mBlend );
	}
}

cFont * cUITextBox::Font() const {
	return mTextCache.Font();
}

void cUITextBox::Font( cFont * font ) {
	mTextCache.Font( font );
	AutoShrink();
	AutoSize();
	AutoAlign();
	OnFontChanged();
}

const std::wstring& cUITextBox::Text() {
	return mTextCache.Text();
}

void cUITextBox::Text( const std::string& text ) {
	Text( stringTowstring( text ) );
}

void cUITextBox::Text( const std::wstring& text ) {
	mTextCache.Text( text );
	AutoShrink();
	AutoSize();
	AutoAlign();
	OnTextChanged();
}

const eeColorA& cUITextBox::Color() const {
	return mFontColor;
}

void cUITextBox::Color( const eeColorA& color ) {
	mFontColor = color;
	Alpha( color.A() );
}

const eeColorA& cUITextBox::ShadowColor() const {
	return mFontShadowColor;
}

void cUITextBox::ShadowColor( const eeColorA& color ) {
	mFontShadowColor = color;
	Alpha( color.A() );
}

void cUITextBox::Alpha( const eeFloat& alpha ) {
	cUIControlAnim::Alpha( alpha );
	mFontColor.Alpha = (Uint8)alpha;
	mFontShadowColor.Alpha = (Uint8)alpha;
}

void cUITextBox::AutoShrink() {
	if ( Flags() & UI_AUTO_SHRINK_TEXT )
		mTextCache.Font()->ShrinkText( mTextCache.Text(), mSize.Width() );
}

void cUITextBox::AutoSize() {
	if ( Flags() & UI_AUTO_SIZE ) {
		mSize.Width( (eeInt)mTextCache.GetTextWidth() );
		mSize.Height( (eeInt)mTextCache.GetTextHeight() );
	}
}

void cUITextBox::AutoAlign() {
	switch ( FontHAlignGet( Flags() ) ) {
		case UI_HALIGN_CENTER:
			mAlignOffset.x = ( (eeFloat)mSize.x - (eeFloat)mTextCache.GetTextWidth() ) * 0.5f;
			break;
		case UI_HALIGN_RIGHT:
			mAlignOffset.x = ( (eeFloat)mSize.x - (eeFloat)mTextCache.GetTextWidth() );
			break;
		case UI_HALIGN_LEFT:
			mAlignOffset.x = 0.f;
			break;
	}

	switch ( FontVAlignGet( Flags() ) ) {
		case UI_VALIGN_CENTER:
			mAlignOffset.y = ( (eeFloat)mSize.y - (eeFloat)mTextCache.GetTextHeight() ) * 0.5f;
			break;
		case UI_VALIGN_BOTTOM:
			mAlignOffset.y = ( (eeFloat)mSize.y - (eeFloat)mTextCache.GetTextHeight() );
			break;
		case UI_VALIGN_TOP:
			mAlignOffset.y = 0.f;
			break;
	}
}

void cUITextBox::OnSizeChange() {
	AutoShrink();
	AutoSize();
	AutoAlign();

	cUIControlAnim::OnSizeChange();
}

void cUITextBox::OnTextChanged() {
	SendCommonEvent( cUIEvent::EventOnTextChanged );
}

void cUITextBox::OnFontChanged() {
	SendCommonEvent( cUIEvent::EventOnFontChanged );
}

}}
