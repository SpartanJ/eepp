#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/ui/uicomplexcontrol.hpp>
#include <eepp/graphics/textcache.hpp>

namespace EE { namespace UI {

UITooltip::UITooltip( UITooltip::CreateParams& Params, UIControl * TooltipOf ) :
	UIControlAnim( Params ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mAlignOffset( 0.f, 0.f ),
	mPadding( Params.Padding ),
	mTooltipTime( Time::Zero ),
	mTooltipOf( TooltipOf )
{
	mTextCache = eeNew( TextCache, () );
	mTextCache->font( Params.Font );
	mTextCache->color( mFontColor );
	mTextCache->shadowColor( mFontShadowColor );

	if ( NULL == Params.Font ) {
		if ( NULL != UIThemeManager::instance()->defaultFont() )
			mTextCache->font( UIThemeManager::instance()->defaultFont() );
		else
			eePRINTL( "UITooltip::UITextBox : Created a UI TextBox without a defined font." );
	}

	autoPadding();

	if ( Params.ParentCtrl != UIManager::instance()->mainControl() )
		parent( UIManager::instance()->mainControl() );

	applyDefaultTheme();
}

UITooltip::~UITooltip() {
	eeSAFE_DELETE( mTextCache );

	if ( NULL != mTooltipOf && mTooltipOf->isComplex() ) {
		reinterpret_cast<UIComplexControl*>( mTooltipOf )->tooltipRemove();
	}
}

Uint32 UITooltip::getType() const {
	return UI_TYPE_TOOLTIP;
}

bool UITooltip::isType( const Uint32& type ) const {
	return UITooltip::getType() == type ? true : UIControlAnim::isType( type );
}

void UITooltip::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "tooltip" );

	autoPadding();

	if ( NULL == mTextCache->font() && NULL != Theme->font() ) {
		mTextCache->font( Theme->font() );
	}
}

void UITooltip::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = makePadding( true, true, true, true );
	}
}

void UITooltip::show() {
	if ( !visible() || 0 == mAlpha ) {
		toFront();

		visible( true );

		if ( UIThemeManager::instance()->defaultEffectsEnabled() ) {
			startAlphaAnim( 255.f == mAlpha ? 0.f : mAlpha, 255.f, UIThemeManager::instance()->controlsFadeInTime() );
		}
	}
}

void UITooltip::hide() {
	if ( visible() ) {
		if ( UIThemeManager::instance()->defaultEffectsEnabled() ) {
			disableFadeOut( UIThemeManager::instance()->controlsFadeOutTime() );
		} else {
			visible( false );
		}
	}
}

void UITooltip::draw() {
	if ( mVisible && 0.f != mAlpha ) {
		UIControlAnim::draw();

		if ( mTextCache->getTextWidth() ) {
			mTextCache->flags( flags() );
			mTextCache->draw( (Float)mScreenPos.x + mAlignOffset.x, (Float)mScreenPos.y + mAlignOffset.y, Vector2f::One, 0.f, blend() );
		}
	}
}

Graphics::Font * UITooltip::font() const {
	return mTextCache->font();
}

void UITooltip::font( Graphics::Font * font ) {
	if ( mTextCache->font() != font ) {
		mTextCache->font( font );
		autoPadding();
		autoSize();
		autoAlign();
		onFontChanged();
	}
}

const String& UITooltip::text() {
	return mTextCache->text();
}

void UITooltip::text( const String& text ) {
	mTextCache->text( text );
	autoPadding();
	autoSize();
	autoAlign();
	onTextChanged();
}

const ColorA& UITooltip::color() const {
	return mFontColor;
}

void UITooltip::color( const ColorA& color ) {
	mFontColor = color;
	alpha( color.a() );
}

const ColorA& UITooltip::shadowColor() const {
	return mFontShadowColor;
}

void UITooltip::shadowColor( const ColorA& color ) {
	mFontShadowColor = color;
	alpha( color.a() );
	mTextCache->shadowColor( mFontColor );
}

void UITooltip::alpha( const Float& alpha ) {
	UIControlAnim::alpha( alpha );
	mFontColor.Alpha = (Uint8)alpha;
	mFontShadowColor.Alpha = (Uint8)alpha;

	mTextCache->color( mFontColor );
}

void UITooltip::autoSize() {
	if ( mFlags & UI_AUTO_SIZE ) {
		mSize.setWidth( (int)mTextCache->getTextWidth() + mPadding.Left + mPadding.Right );
		mSize.setHeight( (int)mTextCache->getTextHeight() + mPadding.Top + mPadding.Bottom );
	}
}

void UITooltip::autoAlign() {
	Uint32 Width	= mSize.getWidth()		- mPadding.Left - mPadding.Right;
	Uint32 Height	= mSize.getHeight()	- mPadding.Top	- mPadding.Bottom;

	switch ( FontHAlignGet( flags() ) ) {
		case UI_HALIGN_CENTER:
			mAlignOffset.x = mPadding.Left + (Float)( (Int32)( Width - mTextCache->getTextWidth() ) / 2 );
			break;
		case UI_HALIGN_RIGHT:
			mAlignOffset.x = ( (Float)Width - (Float)mTextCache->getTextWidth() ) - mPadding.Right;
			break;
		case UI_HALIGN_LEFT:
			mAlignOffset.x = mPadding.Left;
			break;
	}

	switch ( FontVAlignGet( flags() ) ) {
		case UI_VALIGN_CENTER:
			mAlignOffset.y = mPadding.Top + (Float)( ( (Int32)( Height - mTextCache->getTextHeight() ) ) / 2 );
			break;
		case UI_VALIGN_BOTTOM:
			mAlignOffset.y = ( (Float)Height - (Float)mTextCache->getTextHeight() ) - mPadding.Bottom;
			break;
		case UI_VALIGN_TOP:
			mAlignOffset.y = mPadding.Top;
			break;
	}
}

void UITooltip::onSizeChange() {
	autoPadding();
	autoSize();
	autoAlign();

	UIControlAnim::onSizeChange();

	mTextCache->cache();
}

void UITooltip::onTextChanged() {
	sendCommonEvent( UIEvent::EventOnTextChanged );
}

void UITooltip::onFontChanged() {
	sendCommonEvent( UIEvent::EventOnFontChanged );
}

void UITooltip::padding( const Recti& padding ) {
	mPadding = padding;
}

const Recti& UITooltip::padding() const {
	return mPadding;
}

TextCache * UITooltip::getTextCache() {
	return mTextCache;
}

Float UITooltip::getTextWidth() {
	return mTextCache->getTextWidth();
}

Float UITooltip::getTextHeight() {
	return mTextCache->getTextHeight();
}

const int& UITooltip::getNumLines() const {
	return mTextCache->getNumLines();
}

const Vector2f& UITooltip::alignOffset() const {
	return mAlignOffset;
}

void UITooltip::tooltipTime( const Time& Time ) {
	mTooltipTime = Time;
}

void UITooltip::tooltipTimeAdd( const Time& Time ) {
	mTooltipTime += Time;
}

const Time& UITooltip::tooltipTime() const {
	return mTooltipTime;
}

}}
