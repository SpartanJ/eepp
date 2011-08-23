#include "cuitooltip.hpp"
#include "cuimanager.hpp"
#include "cuicomplexcontrol.hpp"

namespace EE { namespace UI {

cUITooltip::cUITooltip( cUITooltip::CreateParams& Params, cUIControl * TooltipOf ) :
	cUIControlAnim( Params ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mAlignOffset( 0.f, 0.f ),
	mTooltipTime( 0.f ),
	mTooltipOf( TooltipOf )
{
	mTextCache = eeNew( cTextCache, () );
	mTextCache->Font( Params.Font );
	mTextCache->Color( mFontColor );
	mTextCache->ShadowColor( mFontShadowColor );

	if ( NULL == Params.Font ) {
		if ( NULL != cUIThemeManager::instance()->DefaultFont() )
			mTextCache->Font( cUIThemeManager::instance()->DefaultFont() );
		else
			eePRINT( "cUITooltip::cUITextBox : Created a UI TextBox without a defined font.\n" );
	}

	AutoAlign();

	if ( Params.ParentCtrl != cUIManager::instance()->MainControl() )
		Parent( cUIManager::instance()->MainControl() );

	ApplyDefaultTheme();
}

cUITooltip::~cUITooltip() {
	eeSAFE_DELETE( mTextCache );

	if ( NULL != mTooltipOf && mTooltipOf->IsComplex() ) {
		reinterpret_cast<cUIComplexControl*>( mTooltipOf )->TooltipRemove();
	}
}

Uint32 cUITooltip::Type() const {
	return UI_TYPE_TOOLTIP;
}

bool cUITooltip::IsType( const Uint32& type ) const {
	return cUITooltip::Type() == type ? true : cUIControlAnim::IsType( type );
}

void cUITooltip::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "tooltip" );

	AutoPadding();

	if ( NULL == mTextCache->Font() && NULL != Theme->Font() ) {
		mTextCache->Font( Theme->Font() );
	}
}

void cUITooltip::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = MakePadding( true, true, true, true );
	}
}

void cUITooltip::Show() {
	if ( !Visible() || 0 == mAlpha ) {
		ToFront();

		Visible( true );

		if ( cUIThemeManager::instance()->DefaultEffectsEnabled() ) {
			if ( 255.f == mAlpha )
				StartAlphaAnim( 0.f, 255.f, cUIThemeManager::instance()->ControlsFadeInTime() );
			else
				CreateFadeIn( cUIThemeManager::instance()->ControlsFadeInTime() );
		}
	}
}

void cUITooltip::Hide() {
	if ( Visible() ) {
		if ( cUIThemeManager::instance()->DefaultEffectsEnabled() ) {
			DisableFadeOut( cUIThemeManager::instance()->ControlsFadeOutTime() );
		} else {
			Visible( false );
		}
	}
}

void cUITooltip::Draw() {
	if ( mVisible && 0.f != mAlpha ) {
		cUIControlAnim::Draw();

		if ( mTextCache->GetTextWidth() ) {
			mTextCache->Draw( (eeFloat)mScreenPos.x + mAlignOffset.x, (eeFloat)mScreenPos.y + mAlignOffset.y, Flags(), 1.f, 0.f, Blend() );
		}
	}
}

cFont * cUITooltip::Font() const {
	return mTextCache->Font();
}

void cUITooltip::Font( cFont * font ) {
	if ( mTextCache->Font() != font ) {
		mTextCache->Font( font );
		AutoPadding();
		AutoSize();
		AutoAlign();
		OnFontChanged();
	}
}

const String& cUITooltip::Text() {
	return mTextCache->Text();
}

void cUITooltip::Text( const String& text ) {
	mTextCache->Text( text );
	AutoPadding();
	AutoSize();
	AutoAlign();
	OnTextChanged();
}

const eeColorA& cUITooltip::Color() const {
	return mFontColor;
}

void cUITooltip::Color( const eeColorA& color ) {
	mFontColor = color;
	Alpha( color.A() );
}

const eeColorA& cUITooltip::ShadowColor() const {
	return mFontShadowColor;
}

void cUITooltip::ShadowColor( const eeColorA& color ) {
	mFontShadowColor = color;
	Alpha( color.A() );
	mTextCache->ShadowColor( mFontColor );
}

void cUITooltip::Alpha( const eeFloat& alpha ) {
	cUIControlAnim::Alpha( alpha );
	mFontColor.Alpha = (Uint8)alpha;
	mFontShadowColor.Alpha = (Uint8)alpha;

	mTextCache->Color( mFontColor );
}

void cUITooltip::AutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		mSize.Width( (eeInt)mTextCache->GetTextWidth() + mPadding.Left + mPadding.Right );
		mSize.Height( (eeInt)mTextCache->GetTextHeight() + mPadding.Top + mPadding.Bottom );
	}
}

void cUITooltip::AutoAlign() {
	Uint32 Width	= mSize.Width()		- mPadding.Left - mPadding.Right;
	Uint32 Height	= mSize.Height()	- mPadding.Top	- mPadding.Right;

	switch ( FontHAlignGet( Flags() ) ) {
		case UI_HALIGN_CENTER:
			mAlignOffset.x = mPadding.Left + (eeFloat)( (Int32)( Width - mTextCache->GetTextWidth() ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mAlignOffset.x = ( (eeFloat)Width - (eeFloat)mTextCache->GetTextWidth() ) - mPadding.Right;
			break;
		case UI_HALIGN_LEFT:
			mAlignOffset.x = mPadding.Left;
			break;
	}

	switch ( FontVAlignGet( Flags() ) ) {
		case UI_VALIGN_CENTER:
			mAlignOffset.y = mPadding.Top + (eeFloat)( ( (Int32)( Height - mTextCache->GetTextHeight() ) ) / 2 );
			break;
		case UI_VALIGN_BOTTOM:
			mAlignOffset.y = ( (eeFloat)Height - (eeFloat)mTextCache->GetTextHeight() ) - mPadding.Bottom;
			break;
		case UI_VALIGN_TOP:
			mAlignOffset.y = mPadding.Top;
			break;
	}
}

void cUITooltip::OnSizeChange() {
	AutoPadding();
	AutoSize();
	AutoAlign();

	cUIControlAnim::OnSizeChange();

	mTextCache->Cache();
}

void cUITooltip::OnTextChanged() {
	SendCommonEvent( cUIEvent::EventOnTextChanged );
}

void cUITooltip::OnFontChanged() {
	SendCommonEvent( cUIEvent::EventOnFontChanged );
}

void cUITooltip::Padding( const eeRecti& padding ) {
	mPadding = padding;
}

const eeRecti& cUITooltip::Padding() const {
	return mPadding;
}

cTextCache * cUITooltip::GetTextCache() {
	return mTextCache;
}

eeFloat cUITooltip::GetTextWidth() {
	return mTextCache->GetTextWidth();
}

eeFloat cUITooltip::GetTextHeight() {
	return mTextCache->GetTextHeight();
}

const eeInt& cUITooltip::GetNumLines() const {
	return mTextCache->GetNumLines();
}

const eeVector2f& cUITooltip::AlignOffset() const {
	return mAlignOffset;
}

void cUITooltip::TooltipTime( const eeFloat& Time ) {
	mTooltipTime = Time;
}

void cUITooltip::TooltipTimeAdd( const eeFloat& Time ) {
	mTooltipTime += Time;
}

const eeFloat& cUITooltip::TooltipTime() const {
	return mTooltipTime;
}

}}
