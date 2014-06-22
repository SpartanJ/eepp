#include <eepp/ui/cuitooltip.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/ui/cuicomplexcontrol.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

cUITooltip::cUITooltip( cUITooltip::CreateParams& Params, cUIControl * TooltipOf ) :
	cUIControlAnim( Params ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mAlignOffset( 0.f, 0.f ),
	mPadding( Params.Padding ),
	mTooltipTime( Time::Zero ),
	mTooltipOf( TooltipOf )
{
	mTextCache = eeNew( TextCache, () );
	mTextCache->Font( Params.Font );
	mTextCache->Color( mFontColor );
	mTextCache->ShadowColor( mFontShadowColor );

	if ( NULL == Params.Font ) {
		if ( NULL != cUIThemeManager::instance()->DefaultFont() )
			mTextCache->Font( cUIThemeManager::instance()->DefaultFont() );
		else
			eePRINTL( "cUITooltip::cUITextBox : Created a UI TextBox without a defined font." );
	}

	AutoPadding();

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
	cUIControl::SetThemeControl( Theme, "tooltip" );

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
			mTextCache->Flags( Flags() );
			mTextCache->Draw( (Float)mScreenPos.x + mAlignOffset.x, (Float)mScreenPos.y + mAlignOffset.y, Vector2f::One, 0.f, Blend() );
		}
	}
}

Graphics::Font * cUITooltip::Font() const {
	return mTextCache->Font();
}

void cUITooltip::Font( Graphics::Font * font ) {
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

const ColorA& cUITooltip::Color() const {
	return mFontColor;
}

void cUITooltip::Color( const ColorA& color ) {
	mFontColor = color;
	Alpha( color.A() );
}

const ColorA& cUITooltip::ShadowColor() const {
	return mFontShadowColor;
}

void cUITooltip::ShadowColor( const ColorA& color ) {
	mFontShadowColor = color;
	Alpha( color.A() );
	mTextCache->ShadowColor( mFontColor );
}

void cUITooltip::Alpha( const Float& alpha ) {
	cUIControlAnim::Alpha( alpha );
	mFontColor.Alpha = (Uint8)alpha;
	mFontShadowColor.Alpha = (Uint8)alpha;

	mTextCache->Color( mFontColor );
}

void cUITooltip::AutoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		mSize.Width( (int)mTextCache->GetTextWidth() + mPadding.Left + mPadding.Right );
		mSize.Height( (int)mTextCache->GetTextHeight() + mPadding.Top + mPadding.Bottom );
	}
}

void cUITooltip::AutoAlign() {
	Uint32 Width	= mSize.Width()		- mPadding.Left - mPadding.Right;
	Uint32 Height	= mSize.Height()	- mPadding.Top	- mPadding.Bottom;

	switch ( FontHAlignGet( Flags() ) ) {
		case UI_HALIGN_CENTER:
			mAlignOffset.x = mPadding.Left + (Float)( (Int32)( Width - mTextCache->GetTextWidth() ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mAlignOffset.x = ( (Float)Width - (Float)mTextCache->GetTextWidth() ) - mPadding.Right;
			break;
		case UI_HALIGN_LEFT:
			mAlignOffset.x = mPadding.Left;
			break;
	}

	switch ( FontVAlignGet( Flags() ) ) {
		case UI_VALIGN_CENTER:
			mAlignOffset.y = mPadding.Top + (Float)( ( (Int32)( Height - mTextCache->GetTextHeight() ) ) / 2 );
			break;
		case UI_VALIGN_BOTTOM:
			mAlignOffset.y = ( (Float)Height - (Float)mTextCache->GetTextHeight() ) - mPadding.Bottom;
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

void cUITooltip::Padding( const Recti& padding ) {
	mPadding = padding;
}

const Recti& cUITooltip::Padding() const {
	return mPadding;
}

TextCache * cUITooltip::GetTextCache() {
	return mTextCache;
}

Float cUITooltip::GetTextWidth() {
	return mTextCache->GetTextWidth();
}

Float cUITooltip::GetTextHeight() {
	return mTextCache->GetTextHeight();
}

const int& cUITooltip::GetNumLines() const {
	return mTextCache->GetNumLines();
}

const Vector2f& cUITooltip::AlignOffset() const {
	return mAlignOffset;
}

void cUITooltip::TooltipTime( const Time& Time ) {
	mTooltipTime = Time;
}

void cUITooltip::TooltipTimeAdd( const Time& Time ) {
	mTooltipTime += Time;
}

const Time& cUITooltip::TooltipTime() const {
	return mTooltipTime;
}

}}
